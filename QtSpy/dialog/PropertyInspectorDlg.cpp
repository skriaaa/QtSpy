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
#include <QListView>
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

	// 值列文本化/解析/编辑器统一走 ParamEditor(参数/属性编辑公共设施)
	ParamEditor::ParamType paramTypeOfItem(const PROPERTY_ITEM_INFO& PropertyItemInfo)
	{
		ParamEditor::ParamType type;
		type.nTypeId = PropertyItemInfo.nTypeId;
		type.metaEnum = PropertyItemInfo.metaEnum;
		type.bFlag = PropertyItemInfo.bFlag;
		return type;
	}

	bool isSupportedPropertyType(const QMetaProperty& metaProperty)
	{
		return ParamEditor::isEditable(ParamEditor::fromProperty(metaProperty));
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

	// 属性值 -> 值列文本(枚举/flags/各值类型统一在 ParamEditor::valueToText)
	QString propertyValueText(const PROPERTY_ITEM_INFO& PropertyItemInfo, const QVariant& value)
	{
		return ParamEditor::valueToText(paramTypeOfItem(PropertyItemInfo), value);
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

void CPropertyItemDelegate::setParamTypeResolver(const ParamTypeResolver& fnResolver)
{
	m_fnResolveParamType = fnResolver;
}

QWidget* CPropertyItemDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);
	if (!m_fnResolveParamType)
	{
		return nullptr;
	}
	// 行号 -> 参数类型 -> ParamEditor 统一创建编辑器
	const ParamEditor::ParamType type = m_fnResolveParamType(index.row());
	if (!ParamEditor::isEditable(type))
	{
		return nullptr;
	}
	QWidget* pEditor = ParamEditor::createEditor(type, pParent);
	if (nullptr != pEditor)
	{
		// 按钮型编辑器(复合值/字体)在属性面板双击单元格即弹窗, 免去再点一次按钮;
		// 发送信号弹窗不设此属性, 按钮仍是普通点击弹窗
		pEditor->setProperty("qtspyAutoOpenDialog", true);
	}
	return pEditor;
}

void CPropertyItemDelegate::updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(index);
	// 编辑器高度固定 28, 在单元格内垂直居中:
	// 默认按左上角贴齐, 编辑器比行高时看起来"弹出位置偏移", 居中后上下对称
	QRect rcEditor = option.rect;
	rcEditor.setTop(rcEditor.center().y() - 28 / 2);
	rcEditor.setHeight(28);
	pEditor->setGeometry(rcEditor);
}

void CPropertyItemDelegate::setEditorData(QWidget* pEditor, const QModelIndex& index) const
{
	// 编辑器统一实现 paramText 属性(文本往返), 无该声明属性时走基类
	if (0 <= pEditor->metaObject()->indexOfProperty("paramText"))
	{
		pEditor->setProperty("paramText", index.data(Qt::EditRole).toString());
		return;
	}

	QStyledItemDelegate::setEditorData(pEditor, index);
}

void CPropertyItemDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
	if (0 <= pEditor->metaObject()->indexOfProperty("paramText"))
	{
		pModel->setData(index, pEditor->property("paramText").toString(), Qt::EditRole);
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
	// 并随自动刷新保持最新; 列表头部为 Class/ObjectName/Pointer(原顶部 QLabel)
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
	// 值列编辑器按行取参数类型(m_vectorPropertyItems 里有 metatype/枚举信息)
	CPropertyItemDelegate* pValueDelegate = new CPropertyItemDelegate(m_pPropertyTable);
	pValueDelegate->setParamTypeResolver([this](int nRow) {
		if ((0 > nRow) || (nRow >= m_vectorPropertyItems.size()))
		{
			return ParamEditor::ParamType();
		}
		return paramTypeOfItem(m_vectorPropertyItems.at(nRow));
	});
	m_pPropertyTable->setItemDelegateForColumn(PROPERTY_COLUMN_VALUE, pValueDelegate);
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
		QString strValue = PropertyItemInfo.bReadable ? propertyValueText(PropertyItemInfo, metaProperty.read(pTargetObject)) : "<不可读>";
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
	// 原顶部 QLabel 的 Class/ObjectName/Pointer 并入列表头部(对所有目标类型生效)
	{
		QString strObjectName = pTargetObject->objectName();
		listBaseInfo.append(qMakePair(QString("class"), QString(pTargetObject->metaObject()->className())));
		listBaseInfo.append(qMakePair(QString("objectName"), strObjectName.isEmpty() ? QString("<未命名>") : strObjectName));
		listBaseInfo.append(qMakePair(QString("pointer"), pointerToHex(pTargetObject)));
	}
	if (QWidget* pTargetWidget = qobject_cast<QWidget*>(pTargetObject))
	{
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
	// Class/ObjectName 移入基础信息列表; 状态 tab 只留运行时状态。
	// Thread 行删掉: 目标都来自控件树(QWidget/布局, 必在主线程), 永远显示"当前线程"没有信息量;
	// 跨线程禁编辑的安全护栏(bCurrentThread)不受影响, 仍在属性编辑路径生效

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
	// 顶部 QLabel 已删, Class/ObjectName/Pointer 内容移入基础信息列表(refreshBaseInfo)
	if (m_pTargetObject.isNull())
	{
		return;
	}

	setWindowTitle(QString("QtSpy · 组件信息 - %1").arg(objectString(m_pTargetObject.data())));
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
	// 文本 -> 值的解析统一在 ParamEditor(各类型精确可逆格式, 失败返回无效 QVariant)
	QVariant value = ParamEditor::textToValue(paramTypeOfItem(PropertyItemInfo), pItem->text());
	if (!value.isValid())
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
		strCurrentValue = propertyValueText(PropertyItemInfo, currentValue);
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
