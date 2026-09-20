#include "PropertyInspectorDlg.h"

#include <QAbstractButton>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMetaProperty>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QStyleOption>
#include <QTabWidget>
#include <QTableWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#include "publicfunction.h"
#include "theme/QtSpyTheme.h"

namespace
{
	const int AUTO_REFRESH_INTERVAL = 300;
	const int HIGHLIGHT_REFRESH_COUNT = 4;
	const QColor PROPERTY_CHANGED_COLOR = QtSpyTheme::palette().highlightChange;

	enum PROPERTY_EDITOR_TYPE
	{
		PROPERTY_EDITOR_NONE = 0,
		PROPERTY_EDITOR_BOOL,
		PROPERTY_EDITOR_SIGNED_INTEGER,
		PROPERTY_EDITOR_UNSIGNED_INTEGER,
		PROPERTY_EDITOR_FLOAT,
		PROPERTY_EDITOR_STRING,
		PROPERTY_EDITOR_ENUM,
		PROPERTY_EDITOR_FLAG
	};

	enum PROPERTY_ITEM_ROLE
	{
		PROPERTY_ITEM_ROLE_EDITOR_TYPE = Qt::UserRole + 1,
		PROPERTY_ITEM_ROLE_ENUM_KEYS
	};

	PROPERTY_EDITOR_TYPE propertyEditorType(const PROPERTY_ITEM_INFO& PropertyItemInfo)
	{
		if (!PropertyItemInfo.bEditable)
		{
			return PROPERTY_EDITOR_NONE;
		}
		if (PropertyItemInfo.metaEnum.isValid())
		{
			return PropertyItemInfo.bFlag ? PROPERTY_EDITOR_FLAG : PROPERTY_EDITOR_ENUM;
		}

		switch (PropertyItemInfo.nTypeId)
		{
		case QMetaType::Bool:
			return PROPERTY_EDITOR_BOOL;
		case QMetaType::Int:
		case QMetaType::LongLong:
		case QMetaType::Long:
		case QMetaType::Short:
		case QMetaType::Char:
		case QMetaType::SChar:
			return PROPERTY_EDITOR_SIGNED_INTEGER;
		case QMetaType::UInt:
		case QMetaType::ULongLong:
		case QMetaType::ULong:
		case QMetaType::UShort:
		case QMetaType::UChar:
			return PROPERTY_EDITOR_UNSIGNED_INTEGER;
		case QMetaType::Double:
		case QMetaType::Float:
			return PROPERTY_EDITOR_FLOAT;
		case QMetaType::QString:
			return PROPERTY_EDITOR_STRING;
		default:
			break;
		}

		return PROPERTY_EDITOR_NONE;
	}

	bool isSupportedPropertyType(const QMetaProperty& metaProperty)
	{
		PROPERTY_ITEM_INFO PropertyItemInfo;
		PropertyItemInfo.nTypeId = metaProperty.userType();
		PropertyItemInfo.metaEnum = metaProperty.enumerator();
		PropertyItemInfo.bFlag = metaProperty.isFlagType();
		PropertyItemInfo.bEditable = true;
		return PROPERTY_EDITOR_NONE != propertyEditorType(PropertyItemInfo);
	}

	QString propertyOwnerName(const QMetaObject* pMetaObject, int nPropertyIndex)
	{
		const QMetaObject* pCurrentMetaObject = pMetaObject;
		while (nullptr != pCurrentMetaObject)
		{
			if (nPropertyIndex >= pCurrentMetaObject->propertyOffset())
			{
				return pCurrentMetaObject->className();
			}
			pCurrentMetaObject = pCurrentMetaObject->superClass();
		}

		return "";
	}

	QString propertyPermissionText(const QMetaProperty& metaProperty)
	{
		QStringList listPermissions;
		if (metaProperty.isReadable())
		{
			listPermissions.append("读");
		}
		if (metaProperty.isWritable())
		{
			listPermissions.append("写");
		}
		if (metaProperty.isResettable())
		{
			listPermissions.append("Reset");
		}
		if (metaProperty.hasNotifySignal())
		{
			listPermissions.append("Notify");
		}
		if (listPermissions.isEmpty())
		{
			return "-";
		}

		return listPermissions.join(" / ");
	}

	QString enumValueText(const QMetaEnum& metaEnum, int nValue, bool bFlag)
	{
		QByteArray arrFlagValue;
		const char* pszValue = nullptr;
		if (bFlag)
		{
			arrFlagValue = metaEnum.valueToKeys(nValue);
			pszValue = arrFlagValue.constData();
		}
		else
		{
			pszValue = metaEnum.valueToKey(nValue);
		}
		if ((nullptr != pszValue) && ('\0' != pszValue[0]))
		{
			return QString::fromLatin1(pszValue);
		}

		return QString::number(nValue);
	}

	QString propertyValueText(const QMetaProperty& metaProperty, const QVariant& value)
	{
		if (!value.isValid())
		{
			return "<无效>";
		}
		if (metaProperty.isEnumType())
		{
			return enumValueText(metaProperty.enumerator(), value.toInt(), metaProperty.isFlagType());
		}
		if (QMetaType::Bool == value.userType())
		{
			return value.toBool() ? "true" : "false";
		}
		if (QMetaType::QString == value.userType())
		{
			return value.toString();
		}
		if ((QMetaType::Double == value.userType()) || (QMetaType::Float == value.userType()))
		{
			return QString::number(value.toDouble(), 'g', 15);
		}

		QString strValue = value.toString();
		if (!strValue.isEmpty())
		{
			return strValue;
		}

		QDebug debugOutput(&strValue);
		debugOutput.noquote().nospace() << value;
		return strValue;
	}

	bool convertValueType(QVariant& value, int nTypeId)
	{
		if (nTypeId == value.userType())
		{
			return true;
		}
		return value.convert(nTypeId);
	}

	bool convertPropertyValue(const PROPERTY_ITEM_INFO& PropertyItemInfo, const QString& strText, QVariant& value)
	{
		QString strValue = strText.trimmed();
		if (PropertyItemInfo.metaEnum.isValid())
		{
			QByteArray arrValue = strValue.toLatin1();
			bool bConverted = false;
			int nValue = PropertyItemInfo.bFlag
				? PropertyItemInfo.metaEnum.keysToValue(arrValue.constData(), &bConverted)
				: PropertyItemInfo.metaEnum.keyToValue(arrValue.constData(), &bConverted);
			if (!bConverted)
			{
				nValue = strValue.toInt(&bConverted, 0);
			}
			if (!bConverted)
			{
				return false;
			}

			value = nValue;
			if ((QMetaType::UnknownType != PropertyItemInfo.nTypeId) && !convertValueType(value, PropertyItemInfo.nTypeId))
			{
				value = nValue;
			}
			return true;
		}

		bool bConverted = false;
		switch (PropertyItemInfo.nTypeId)
		{
		case QMetaType::Bool:
			if ((0 == strValue.compare("true", Qt::CaseInsensitive)) || ("1" == strValue))
			{
				value = true;
				return true;
			}
			if ((0 != strValue.compare("false", Qt::CaseInsensitive)) && ("0" != strValue))
			{
				return false;
			}
			value = false;
			return true;
		case QMetaType::Int:
		case QMetaType::LongLong:
		case QMetaType::Long:
		case QMetaType::Short:
		case QMetaType::Char:
		case QMetaType::SChar:
		{
			qlonglong llValue = strValue.toLongLong(&bConverted, 0);
			if (!bConverted)
			{
				return false;
			}
			value = llValue;
			return convertValueType(value, PropertyItemInfo.nTypeId);
		}
		case QMetaType::UInt:
		case QMetaType::ULongLong:
		case QMetaType::ULong:
		case QMetaType::UShort:
		case QMetaType::UChar:
		{
			qulonglong ullValue = strValue.toULongLong(&bConverted, 0);
			if (!bConverted)
			{
				return false;
			}
			value = ullValue;
			return convertValueType(value, PropertyItemInfo.nTypeId);
		}
		case QMetaType::Double:
		case QMetaType::Float:
		{
			double dValue = strValue.toDouble(&bConverted);
			if (!bConverted)
			{
				return false;
			}
			value = dValue;
			return convertValueType(value, PropertyItemInfo.nTypeId);
		}
		case QMetaType::QString:
			value = strText;
			return true;
		default:
			break;
		}

		return false;
	}

	void setTableItemText(QTableWidget* pTable, int nRow, int nColumn, const QString& strText)
	{
		QTableWidgetItem* pItem = pTable->item(nRow, nColumn);
		if (nullptr == pItem)
		{
			pItem = new QTableWidgetItem;
			pItem->setFlags(pItem->flags() & ~Qt::ItemIsEditable);
			pTable->setItem(nRow, nColumn, pItem);
		}
		pItem->setText(strText);
	}
}

CPropertyItemDelegate::CPropertyItemDelegate(QObject* pParent)
	: QStyledItemDelegate(pParent)
{
}

QWidget* CPropertyItemDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);

	PROPERTY_EDITOR_TYPE eEditorType = static_cast<PROPERTY_EDITOR_TYPE>(index.data(PROPERTY_ITEM_ROLE_EDITOR_TYPE).toInt());
	if (PROPERTY_EDITOR_BOOL == eEditorType)
	{
		QComboBox* pComboBox = new QComboBox(pParent);
		pComboBox->addItem("false");
		pComboBox->addItem("true");
		pComboBox->setProperty("qtspyPropertyEditor", true);
		return pComboBox;
	}
	if (PROPERTY_EDITOR_ENUM == eEditorType)
	{
		QComboBox* pComboBox = new QComboBox(pParent);
		pComboBox->addItems(index.data(PROPERTY_ITEM_ROLE_ENUM_KEYS).toStringList());
		pComboBox->setProperty("qtspyPropertyEditor", true);
		return pComboBox;
	}

	QLineEdit* pLineEdit = new QLineEdit(pParent);
	pLineEdit->setProperty("qtspyPropertyEditor", true);
	if (PROPERTY_EDITOR_SIGNED_INTEGER == eEditorType)
	{
		QRegularExpression expression("-?[0-9]+");
		pLineEdit->setValidator(new QRegularExpressionValidator(expression, pLineEdit));
	}
	else if (PROPERTY_EDITOR_UNSIGNED_INTEGER == eEditorType)
	{
		QRegularExpression expression("[0-9]+");
		pLineEdit->setValidator(new QRegularExpressionValidator(expression, pLineEdit));
	}
	else if (PROPERTY_EDITOR_FLOAT == eEditorType)
	{
		QDoubleValidator* pValidator = new QDoubleValidator(pLineEdit);
		pValidator->setNotation(QDoubleValidator::ScientificNotation);
		pLineEdit->setValidator(pValidator);
	}

	return pLineEdit;
}

void CPropertyItemDelegate::setEditorData(QWidget* pEditor, const QModelIndex& index) const
{
	QString strValue = index.data(Qt::EditRole).toString();
	if (QComboBox* pComboBox = qobject_cast<QComboBox*>(pEditor))
	{
		int nIndex = pComboBox->findText(strValue);
		if (0 > nIndex)
		{
			pComboBox->addItem(strValue);
			nIndex = pComboBox->count() - 1;
		}
		pComboBox->setCurrentIndex(nIndex);
		return;
	}
	if (QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor))
	{
		pLineEdit->setText(strValue);
		pLineEdit->selectAll();
		return;
	}

	QStyledItemDelegate::setEditorData(pEditor, index);
}

void CPropertyItemDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
	if (QComboBox* pComboBox = qobject_cast<QComboBox*>(pEditor))
	{
		pModel->setData(index, pComboBox->currentText(), Qt::EditRole);
		return;
	}
	if (QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor))
	{
		pModel->setData(index, pLineEdit->text(), Qt::EditRole);
		return;
	}

	QStyledItemDelegate::setModelData(pEditor, pModel, index);
}

CPropertyInspectorDlg::CPropertyInspectorDlg(QWidget* pParent)
	: CXDialog(pParent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
	resize(860, 520);
	initWidgets();
}

bool CPropertyInspectorDlg::setTargetObject(QObject* pTargetObject)
{
	if (nullptr == pTargetObject)
	{
		return false;
	}
	if (!m_pTargetObject.isNull())
	{
		disconnect(m_pTargetObject.data(), nullptr, this, nullptr);
	}

	m_pTargetObject = pTargetObject;
	connect(pTargetObject, &QObject::destroyed, this, [this]() {
		handleTargetDestroyed();
	});

	refreshTargetCaption();
	buildPropertyRows();
	if (m_pAutoRefreshCheckBox->isChecked())
	{
		m_pRefreshTimer->start();
	}
	return true;
}

void CPropertyInspectorDlg::initWidgets()
{
	QVBoxLayout* pMainLayout = new QVBoxLayout(this);

	m_pTargetLabel = new QLabel(this);
	m_pTargetLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	pMainLayout->addWidget(m_pTargetLabel);

	QHBoxLayout* pToolLayout = new QHBoxLayout;
	m_pSearchEdit = new QLineEdit(this);
	m_pSearchEdit->setClearButtonEnabled(true);
	m_pSearchEdit->setPlaceholderText("按属性名称查找");
	pToolLayout->addWidget(m_pSearchEdit, 1);

	QPushButton* pRefreshButton = new QPushButton("刷新", this);
	pToolLayout->addWidget(pRefreshButton);

	m_pAutoRefreshCheckBox = new QCheckBox("自动刷新", this);
	m_pAutoRefreshCheckBox->setChecked(true);
	pToolLayout->addWidget(m_pAutoRefreshCheckBox);
	pMainLayout->addLayout(pToolLayout);

	QTabWidget* pTabWidget = new QTabWidget(this);
	pMainLayout->addWidget(pTabWidget, 1);
	// tab 顺序: 基础信息 -> 状态 -> 属性(默认停在基础信息)

	// 基础信息 tab: 原独立"基础信息"窗口(geometry/size/sizePolicy/font 等)并入,
	// 并随自动刷新保持最新; class/objectName 已在"状态"tab, 不重复
	m_pBaseInfoTable = new QTableWidget(this);
	m_pBaseInfoTable->setColumnCount(2);
	m_pBaseInfoTable->setHorizontalHeaderLabels({ "项目", "值" });
	m_pBaseInfoTable->setAlternatingRowColors(true);
	m_pBaseInfoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pBaseInfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pBaseInfoTable->verticalHeader()->setVisible(false);
	m_pBaseInfoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_pBaseInfoTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	pTabWidget->addTab(m_pBaseInfoTable, "基础信息");

	m_pStatusTable = new QTableWidget(this);
	m_pStatusTable->setColumnCount(2);
	m_pStatusTable->setHorizontalHeaderLabels({ "状态", "值" });
	m_pStatusTable->setAlternatingRowColors(true);
	m_pStatusTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pStatusTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pStatusTable->verticalHeader()->setVisible(false);
	m_pStatusTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_pStatusTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	pTabWidget->addTab(m_pStatusTable, "状态");

	m_pPropertyTable = new QTableWidget(this);
	m_pPropertyTable->setColumnCount(PROPERTY_COLUMN_COUNT);
	m_pPropertyTable->setHorizontalHeaderLabels({ "属性", "类型", "值", "权限", "定义类" });
	m_pPropertyTable->setAlternatingRowColors(true);
	m_pPropertyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pPropertyTable->setSelectionMode(QAbstractItemView::SingleSelection);
	QAbstractItemView::EditTriggers editTriggers = QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed;
	editTriggers |= QAbstractItemView::SelectedClicked;
	m_pPropertyTable->setEditTriggers(editTriggers);
	m_pPropertyTable->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pPropertyTable->setItemDelegateForColumn(PROPERTY_COLUMN_VALUE, new CPropertyItemDelegate(m_pPropertyTable));
	m_pPropertyTable->verticalHeader()->setVisible(false);
	m_pPropertyTable->horizontalHeader()->setSectionResizeMode(PROPERTY_COLUMN_NAME, QHeaderView::ResizeToContents);
	m_pPropertyTable->horizontalHeader()->setSectionResizeMode(PROPERTY_COLUMN_TYPE, QHeaderView::ResizeToContents);
	m_pPropertyTable->horizontalHeader()->setSectionResizeMode(PROPERTY_COLUMN_VALUE, QHeaderView::Stretch);
	m_pPropertyTable->horizontalHeader()->setSectionResizeMode(PROPERTY_COLUMN_PERMISSION, QHeaderView::ResizeToContents);
	m_pPropertyTable->horizontalHeader()->setSectionResizeMode(PROPERTY_COLUMN_OWNER, QHeaderView::ResizeToContents);
	pTabWidget->addTab(m_pPropertyTable, "属性");

	m_pRefreshTimer = new QTimer(this);
	m_pRefreshTimer->setInterval(AUTO_REFRESH_INTERVAL);
	connect(m_pRefreshTimer, &QTimer::timeout, this, [this]() {
		refreshProperties(true);
	});
	connect(pRefreshButton, &QPushButton::clicked, this, [this]() {
		refreshProperties(true);
	});
	connect(m_pAutoRefreshCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) {
		if (bChecked && !m_pTargetObject.isNull())
		{
			m_pRefreshTimer->start();
		}
		else
		{
			m_pRefreshTimer->stop();
		}
	});
	connect(m_pSearchEdit, &QLineEdit::textChanged, this, [this](const QString& strKeyword) {
		filterProperties(strKeyword);
	});
	connect(m_pPropertyTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* pItem) {
		applyPropertyEdit(pItem);
	});
	connect(m_pPropertyTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& ptPosition) {
		showPropertyContextMenu(ptPosition);
	});
}

void CPropertyInspectorDlg::buildPropertyRows()
{
	if (m_pTargetObject.isNull())
	{
		return;
	}

	QSignalBlocker signalBlocker(m_pPropertyTable);
	m_bUpdating = true;
	m_pPropertyTable->clearContents();
	m_pPropertyTable->setRowCount(0);
	m_vectorPropertyItems.clear();
	m_mapPropertyValues.clear();
	m_mapHighlightTicks.clear();

	QObject* pTargetObject = m_pTargetObject.data();
	const QMetaObject* pMetaObject = pTargetObject->metaObject();
	bool bCurrentThread = pTargetObject->thread() == QThread::currentThread();
	int nPropertyCount = pMetaObject->propertyCount();
	m_pPropertyTable->setRowCount(nPropertyCount);

	for (int nPropertyIndex = 0; nPropertyIndex < nPropertyCount; ++nPropertyIndex)
	{
		QMetaProperty metaProperty = pMetaObject->property(nPropertyIndex);
		PROPERTY_ITEM_INFO PropertyItemInfo;
		PropertyItemInfo.nPropertyIndex = nPropertyIndex;
		PropertyItemInfo.nTypeId = metaProperty.userType();
		PropertyItemInfo.strPropertyName = QString::fromLatin1(metaProperty.name());
		PropertyItemInfo.metaEnum = metaProperty.enumerator();
		PropertyItemInfo.bReadable = metaProperty.isReadable();
		PropertyItemInfo.bWritable = metaProperty.isWritable();
		PropertyItemInfo.bFlag = metaProperty.isFlagType();
		PropertyItemInfo.bEditable = bCurrentThread && PropertyItemInfo.bWritable && isSupportedPropertyType(metaProperty);
		m_vectorPropertyItems.append(PropertyItemInfo);

		QTableWidgetItem* pNameItem = new QTableWidgetItem(PropertyItemInfo.strPropertyName);
		pNameItem->setFlags(pNameItem->flags() & ~Qt::ItemIsEditable);
		m_pPropertyTable->setItem(nPropertyIndex, PROPERTY_COLUMN_NAME, pNameItem);

		QString strTypeName = nullptr == metaProperty.typeName() ? "" : QString::fromLatin1(metaProperty.typeName());
		QTableWidgetItem* pTypeItem = new QTableWidgetItem(strTypeName);
		pTypeItem->setFlags(pTypeItem->flags() & ~Qt::ItemIsEditable);
		m_pPropertyTable->setItem(nPropertyIndex, PROPERTY_COLUMN_TYPE, pTypeItem);

		QTableWidgetItem* pValueItem = new QTableWidgetItem;
		if (!PropertyItemInfo.bEditable)
		{
			pValueItem->setFlags(pValueItem->flags() & ~Qt::ItemIsEditable);
			if (!bCurrentThread && PropertyItemInfo.bWritable)
			{
				pValueItem->setToolTip("目标对象位于其他线程，已禁止写入。");
			}
			else if (PropertyItemInfo.bWritable && !isSupportedPropertyType(metaProperty))
			{
				pValueItem->setToolTip("当前类型暂不支持编辑。");
			}
		}
		PROPERTY_EDITOR_TYPE eEditorType = propertyEditorType(PropertyItemInfo);
		pValueItem->setData(PROPERTY_ITEM_ROLE_EDITOR_TYPE, static_cast<int>(eEditorType));
		if (PROPERTY_EDITOR_ENUM == eEditorType)
		{
			QStringList listEnumKeys;
			for (int nKeyIndex = 0; nKeyIndex < PropertyItemInfo.metaEnum.keyCount(); ++nKeyIndex)
			{
				listEnumKeys.append(QString::fromLatin1(PropertyItemInfo.metaEnum.key(nKeyIndex)));
			}
			pValueItem->setData(PROPERTY_ITEM_ROLE_ENUM_KEYS, listEnumKeys);
		}
		m_pPropertyTable->setItem(nPropertyIndex, PROPERTY_COLUMN_VALUE, pValueItem);

		QTableWidgetItem* pPermissionItem = new QTableWidgetItem(propertyPermissionText(metaProperty));
		pPermissionItem->setFlags(pPermissionItem->flags() & ~Qt::ItemIsEditable);
		m_pPropertyTable->setItem(nPropertyIndex, PROPERTY_COLUMN_PERMISSION, pPermissionItem);

		QTableWidgetItem* pOwnerItem = new QTableWidgetItem(propertyOwnerName(pMetaObject, nPropertyIndex));
		pOwnerItem->setFlags(pOwnerItem->flags() & ~Qt::ItemIsEditable);
		m_pPropertyTable->setItem(nPropertyIndex, PROPERTY_COLUMN_OWNER, pOwnerItem);
	}

	m_bUpdating = false;
	refreshProperties(false);
	filterProperties(m_pSearchEdit->text());
}

void CPropertyInspectorDlg::refreshProperties(bool bHighlightChanges)
{
	if (m_pTargetObject.isNull())
	{
		handleTargetDestroyed();
		return;
	}

	QObject* pTargetObject = m_pTargetObject.data();
	if (pTargetObject->metaObject()->propertyCount() != m_vectorPropertyItems.size())
	{
		buildPropertyRows();
		return;
	}

	QSignalBlocker signalBlocker(m_pPropertyTable);
	m_bUpdating = true;
	for (int nRow = 0; nRow < m_vectorPropertyItems.size(); ++nRow)
	{
		if (isEditingPropertyRow(nRow))
		{
			continue;
		}

		PROPERTY_ITEM_INFO PropertyItemInfo = m_vectorPropertyItems.at(nRow);
		QMetaProperty metaProperty = pTargetObject->metaObject()->property(PropertyItemInfo.nPropertyIndex);
		QString strValue = PropertyItemInfo.bReadable ? propertyValueText(metaProperty, metaProperty.read(pTargetObject)) : "<不可读>";
		bool bChanged = bHighlightChanges && m_mapPropertyValues.contains(nRow) && (m_mapPropertyValues.value(nRow) != strValue);
		if (bChanged)
		{
			m_mapHighlightTicks[nRow] = HIGHLIGHT_REFRESH_COUNT;
		}
		else if (m_mapHighlightTicks.contains(nRow))
		{
			int nRemainCount = m_mapHighlightTicks.value(nRow) - 1;
			if (0 >= nRemainCount)
			{
				m_mapHighlightTicks.remove(nRow);
			}
			else
			{
				m_mapHighlightTicks[nRow] = nRemainCount;
			}
		}

		QTableWidgetItem* pValueItem = m_pPropertyTable->item(nRow, PROPERTY_COLUMN_VALUE);
		pValueItem->setText(strValue);
		pValueItem->setBackground(m_mapHighlightTicks.contains(nRow) ? QBrush(PROPERTY_CHANGED_COLOR) : QBrush());
		m_mapPropertyValues[nRow] = strValue;
	}
	m_bUpdating = false;
	refreshBaseInfo();
	refreshStatus();
}

void CPropertyInspectorDlg::refreshBaseInfo()
{
	if (m_pTargetObject.isNull())
	{
		return;
	}

	QObject* pTargetObject = m_pTargetObject.data();
	QList<QPair<QString, QString>> listBaseInfo;
	auto strRect = [](const QRect& rc) {
		return QString("(%1,%2,%3,%4)").arg(rc.left()).arg(rc.top()).arg(rc.right()).arg(rc.bottom());
	};
	auto strSize = [](const QSize& sz) {
		return QString("(%1,%2)").arg(sz.width()).arg(sz.height());
	};
	if (QWidget* pTargetWidget = qobject_cast<QWidget*>(pTargetObject))
	{
		// 内容与原独立"基础信息"窗口一致(去掉 class/objectName, 在"状态"tab)
		listBaseInfo.append(qMakePair(QString("geometry"), strRect(pTargetWidget->geometry())));
		listBaseInfo.append(qMakePair(QString("screen geometry"), strRect(ScreenRect(pTargetWidget))));
		listBaseInfo.append(qMakePair(QString("size"), strSize(pTargetWidget->size())));
		listBaseInfo.append(qMakePair(QString("maxsize"), strSize(QSize(pTargetWidget->maximumWidth(), pTargetWidget->maximumHeight()))));
		listBaseInfo.append(qMakePair(QString("minsize"), strSize(QSize(pTargetWidget->minimumWidth(), pTargetWidget->minimumHeight()))));
		listBaseInfo.append(qMakePair(QString("sizeHint"), strSize(pTargetWidget->sizeHint())));
		listBaseInfo.append(qMakePair(QString("sizePolicy"),
			QString("%1 | %2")
			.arg(queryEnumName<QSizePolicy::Policy>(pTargetWidget->sizePolicy().horizontalPolicy()))
			.arg(queryEnumName<QSizePolicy::Policy>(pTargetWidget->sizePolicy().verticalPolicy()))));
		listBaseInfo.append(qMakePair(QString("stretch"),
			QString("(%1,%2)").arg(pTargetWidget->sizePolicy().horizontalStretch()).arg(pTargetWidget->sizePolicy().verticalStretch())));
		listBaseInfo.append(qMakePair(QString("stylesheet"), pTargetWidget->styleSheet()));
		listBaseInfo.append(qMakePair(QString("font"), pTargetWidget->font().toString()));
		listBaseInfo.append(qMakePair(QString("winid"), QString("%1").arg(pTargetWidget->winId())));
	}
	else if (QLayout* pTargetLayout = qobject_cast<QLayout*>(pTargetObject))
	{
		listBaseInfo.append(qMakePair(QString("geometry"), strRect(pTargetLayout->geometry())));
		listBaseInfo.append(qMakePair(QString("size"), strSize(pTargetLayout->geometry().size())));
		listBaseInfo.append(qMakePair(QString("sizeHint"), strSize(pTargetLayout->totalSizeHint())));
		listBaseInfo.append(qMakePair(QString("maxSize"), strSize(pTargetLayout->totalMaximumSize())));
		listBaseInfo.append(qMakePair(QString("minSize"), strSize(pTargetLayout->totalMinimumSize())));
		listBaseInfo.append(qMakePair(QString("margins"),
			QString("(%1,%2,%3,%4)")
			.arg(pTargetLayout->contentsMargins().left()).arg(pTargetLayout->contentsMargins().top())
			.arg(pTargetLayout->contentsMargins().right()).arg(pTargetLayout->contentsMargins().bottom())));
		listBaseInfo.append(qMakePair(QString("sizeConstrant"), QMetaEnum::fromType<QLayout::SizeConstraint>().valueToKey(pTargetLayout->sizeConstraint())));
		listBaseInfo.append(qMakePair(QString("spacing"), QString::number(pTargetLayout->spacing())));
	}

	QSignalBlocker signalBlocker(m_pBaseInfoTable);
	m_pBaseInfoTable->setRowCount(listBaseInfo.size());
	for (int nRow = 0; nRow < listBaseInfo.size(); ++nRow)
	{
		setTableItemText(m_pBaseInfoTable, nRow, 0, listBaseInfo.at(nRow).first);
		setTableItemText(m_pBaseInfoTable, nRow, 1, listBaseInfo.at(nRow).second);
	}
}

void CPropertyInspectorDlg::refreshStatus()
{
	if (m_pTargetObject.isNull())
	{
		return;
	}

	QObject* pTargetObject = m_pTargetObject.data();
	QList<QPair<QString, QString>> listStatus;
	listStatus.append(qMakePair(QString("Class"), QString(pTargetObject->metaObject()->className())));
	listStatus.append(qMakePair(QString("ObjectName"), pTargetObject->objectName()));
	listStatus.append(qMakePair(QString("Thread"), pTargetObject->thread() == QThread::currentThread() ? QString("当前线程") : QString("其他线程")));

	if (QWidget* pTargetWidget = qobject_cast<QWidget*>(pTargetObject))
	{
		QStyleOption option;
		option.initFrom(pTargetWidget);
		QStringList listState;
		for (int nIndex = 0; nIndex < queryEnumCount<QStyle::StateFlag>(); ++nIndex)
		{
			if (option.state.testFlag(queryEnumValue<QStyle::StateFlag>(nIndex)))
			{
				listState.append(queryEnumName<QStyle::StateFlag>(nIndex));
			}
		}
		listStatus.append(qMakePair(QString("QStyle::StateFlag"), listState.join(" | ")));

		QStringList listAttributes;
		for (int nIndex = 0; nIndex < Qt::WA_AttributeCount; ++nIndex)
		{
			Qt::WidgetAttribute eAttribute = static_cast<Qt::WidgetAttribute>(nIndex);
			if (pTargetWidget->testAttribute(eAttribute))
			{
				listAttributes.append(queryEnumName<Qt::WidgetAttribute>(eAttribute));
			}
		}
		listStatus.append(qMakePair(QString("WidgetAttribute"), listAttributes.join(" | ")));

		QStringList listWindowTypes;
		for (int nIndex = 0; nIndex < queryEnumCount<Qt::WindowType>(); ++nIndex)
		{
			Qt::WindowType eWindowType = queryEnumValue<Qt::WindowType>(nIndex);
			if (pTargetWidget->windowFlags().testFlag(eWindowType))
			{
				listWindowTypes.append(queryEnumName<Qt::WindowType>(nIndex));
			}
		}
		listStatus.append(qMakePair(QString("WindowType"), listWindowTypes.join(" | ")));

		if (nullptr != pTargetWidget->windowHandle())
		{
			QStringList listWindowFlags;
			for (int nIndex = 0; nIndex < queryEnumCount<Qt::WindowType>(); ++nIndex)
			{
				Qt::WindowType eWindowType = queryEnumValue<Qt::WindowType>(nIndex);
				if (pTargetWidget->windowHandle()->flags().testFlag(eWindowType))
				{
					listWindowFlags.append(queryEnumName<Qt::WindowType>(nIndex));
				}
			}
			listStatus.append(qMakePair(QString("WindowFlag"), listWindowFlags.join(" | ")));
		}

		// 状态 tab 只保留通用状态(QStyle/Attribute/WindowFlags),
		// 控件类专属状态(如 Button 的 checked 等)不再罗列, 属性 tab 已能看对应属性
	}

	QSignalBlocker signalBlocker(m_pStatusTable);
	m_pStatusTable->setRowCount(listStatus.size());
	for (int nRow = 0; nRow < listStatus.size(); ++nRow)
	{
		setTableItemText(m_pStatusTable, nRow, 0, listStatus.at(nRow).first);
		setTableItemText(m_pStatusTable, nRow, 1, listStatus.at(nRow).second);
	}
}

void CPropertyInspectorDlg::refreshTargetCaption()
{
	if (m_pTargetObject.isNull())
	{
		return;
	}

	QObject* pTargetObject = m_pTargetObject.data();
	QString strObjectName = pTargetObject->objectName();
	QString strTarget = QString("Class : %1 | ObjectName : %2 | Pointer : %3")
		.arg(pTargetObject->metaObject()->className())
		.arg(strObjectName.isEmpty() ? "<未命名>" : strObjectName)
		.arg(pointerToHex(pTargetObject));
	m_pTargetLabel->setText(strTarget);
	setWindowTitle(QString("QtSpy · 组件信息 - %1").arg(objectString(pTargetObject)));

	bool bCurrentThread = pTargetObject->thread() == QThread::currentThread();
	m_pTargetLabel->setToolTip(bCurrentThread ? "目标对象位于当前线程，可编辑受支持的可写属性。" : "目标对象位于其他线程，仅允许查看属性。");
}

void CPropertyInspectorDlg::filterProperties(const QString& strKeyword)
{
	QString strFilter = strKeyword.trimmed();
	for (int nRow = 0; nRow < m_pPropertyTable->rowCount(); ++nRow)
	{
		QTableWidgetItem* pNameItem = m_pPropertyTable->item(nRow, PROPERTY_COLUMN_NAME);
		bool bMatched = strFilter.isEmpty() || ((nullptr != pNameItem) && pNameItem->text().contains(strFilter, Qt::CaseInsensitive));
		m_pPropertyTable->setRowHidden(nRow, !bMatched);
	}
}

void CPropertyInspectorDlg::applyPropertyEdit(QTableWidgetItem* pItem)
{
	if (m_bUpdating || (nullptr == pItem) || (PROPERTY_COLUMN_VALUE != pItem->column()) || m_pTargetObject.isNull())
	{
		return;
	}

	int nRow = pItem->row();
	if ((0 > nRow) || (nRow >= m_vectorPropertyItems.size()))
	{
		return;
	}

	PROPERTY_ITEM_INFO PropertyItemInfo = m_vectorPropertyItems.at(nRow);
	QObject* pTargetObject = m_pTargetObject.data();
	if (!PropertyItemInfo.bEditable || (pTargetObject->thread() != QThread::currentThread()))
	{
		refreshProperties(false);
		return;
	}

	QMetaProperty metaProperty = pTargetObject->metaObject()->property(PropertyItemInfo.nPropertyIndex);
	QVariant value;
	if (!convertPropertyValue(PropertyItemInfo, pItem->text(), value))
	{
		QMessageBox::warning(this, "属性写入失败", QString("“%1”不是有效的 %2 值。").arg(pItem->text()).arg(metaProperty.typeName()));
		refreshProperties(false);
		return;
	}
	if (!metaProperty.write(pTargetObject, value))
	{
		QMessageBox::warning(this, "属性写入失败", QString("属性 %1 拒绝写入该值。").arg(PropertyItemInfo.strPropertyName));
		refreshProperties(false);
		return;
	}
	if (m_pTargetObject.isNull())
	{
		handleTargetDestroyed();
		return;
	}

	QString strCurrentValue = pItem->text();
	if (PropertyItemInfo.bReadable)
	{
		QVariant currentValue = metaProperty.read(pTargetObject);
		strCurrentValue = propertyValueText(metaProperty, currentValue);
	}
	m_bUpdating = true;
	pItem->setText(strCurrentValue);
	pItem->setBackground(QBrush(PROPERTY_CHANGED_COLOR));
	m_bUpdating = false;
	m_mapPropertyValues[nRow] = strCurrentValue;
	m_mapHighlightTicks[nRow] = HIGHLIGHT_REFRESH_COUNT;

	if (QWidget* pTargetWidget = qobject_cast<QWidget*>(pTargetObject))
	{
		pTargetWidget->update();
	}
	if ("objectName" == PropertyItemInfo.strPropertyName)
	{
		refreshTargetCaption();
	}
	refreshStatus();
}

void CPropertyInspectorDlg::showPropertyContextMenu(const QPoint& ptPosition)
{
	QTableWidgetItem* pItem = m_pPropertyTable->itemAt(ptPosition);
	if (nullptr == pItem)
	{
		return;
	}

	int nRow = pItem->row();
	QTableWidgetItem* pNameItem = m_pPropertyTable->item(nRow, PROPERTY_COLUMN_NAME);
	QTableWidgetItem* pValueItem = m_pPropertyTable->item(nRow, PROPERTY_COLUMN_VALUE);
	if ((nullptr == pNameItem) || (nullptr == pValueItem))
	{
		return;
	}

	QMenu menu(this);
	QAction* pCopyValueAction = menu.addAction("复制值");
	QAction* pCopyRowAction = menu.addAction("复制属性和值");
	QAction* pSelectedAction = menu.exec(m_pPropertyTable->viewport()->mapToGlobal(ptPosition));
	if (pCopyValueAction == pSelectedAction)
	{
		QApplication::clipboard()->setText(pValueItem->text());
	}
	else if (pCopyRowAction == pSelectedAction)
	{
		QApplication::clipboard()->setText(QString("%1 = %2").arg(pNameItem->text()).arg(pValueItem->text()));
	}
}

void CPropertyInspectorDlg::handleTargetDestroyed()
{
	m_pTargetObject = nullptr;
	m_pRefreshTimer->stop();
	m_pPropertyTable->setEnabled(false);
	m_pBaseInfoTable->setEnabled(false);
	m_pStatusTable->setEnabled(false);
	m_pTargetLabel->setText("目标对象已销毁");
	setWindowTitle("QtSpy · 组件信息 - 目标对象已销毁");
}

bool CPropertyInspectorDlg::isEditingPropertyRow(int nRow) const
{
	if ((nRow != m_pPropertyTable->currentRow()) || (PROPERTY_COLUMN_VALUE != m_pPropertyTable->currentColumn()))
	{
		return false;
	}

	QList<QWidget*> listEditorWidgets = m_pPropertyTable->viewport()->findChildren<QWidget*>();
	for (QWidget* pEditorWidget : listEditorWidgets)
	{
		if (pEditorWidget->isVisible() && pEditorWidget->property("qtspyPropertyEditor").toBool())
		{
			return true;
		}
	}
	return false;
}
