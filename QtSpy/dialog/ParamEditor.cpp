#include "ParamEditor.h"

#include "theme/QtSpyTheme.h"

#include <QByteArray>
#include <QColor>
#include <QColorDialog>
#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QDate>
#include <QDateTime>
#include <QDateEdit>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFontDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QListView>
#include <QMetaProperty>
#include <QPlainTextEdit>
#include <QPoint>
#include <QRect>
#include <QRegularExpression>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTime>
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVector>

namespace
{
	// 枚举值的可读文本: key / "A|B"(flags), 无对应 key 时退数值
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

	// 文本解析出的自然 QVariant 收敛到目标 userType;
	// bLenient: 枚举类形参常未注册 metatype, 收敛失败时保留 int(与旧属性面板行为一致)
	bool coerceTypeId(QVariant& value, int nTypeId, bool bLenient)
	{
		if ((QMetaType::UnknownType == nTypeId) || (nTypeId == value.userType()))
		{
			return true;
		}
		if (value.convert(nTypeId))
		{
			return true;
		}
		return bLenient;
	}

	// 拆 "a, b, c" -> n 份数值(按 sepCount 校验个数; 浮点/整数由调用方选)
	bool splitNumbers(const QString& strText, int nCount, QVector<double>& arrOut)
	{
		const QStringList listParts = strText.split(QLatin1Char(','), Qt::SkipEmptyParts);
		if (listParts.size() != nCount)
		{
			return false;
		}
		for (const QString& strPart : listParts)
		{
			bool bConverted = false;
			const double dValue = strPart.trimmed().toDouble(&bConverted);
			if (!bConverted)
			{
				return false;
			}
			arrOut.append(dValue);
		}
		return true;
	}

	const char* const kDateTimeFormat = "yyyy-MM-dd hh:mm:ss";

	// QSizePolicy::Policy / Qt::CursorShape 都没有 Q_ENUM, 用静态表做 key<->值
	const struct
	{
		QSizePolicy::Policy policy;
		const char* pszKey;
	} kSizePolicyKeys[] = {
		{ QSizePolicy::Fixed, "Fixed" },
		{ QSizePolicy::Minimum, "Minimum" },
		{ QSizePolicy::Maximum, "Maximum" },
		{ QSizePolicy::Preferred, "Preferred" },
		{ QSizePolicy::MinimumExpanding, "MinimumExpanding" },
		{ QSizePolicy::Expanding, "Expanding" },
		{ QSizePolicy::Ignored, "Ignored" },
	};

	const struct
	{
		Qt::CursorShape shape;
		const char* pszKey;
	} kCursorShapeKeys[] = {
		{ Qt::ArrowCursor, "ArrowCursor" },
		{ Qt::UpArrowCursor, "UpArrowCursor" },
		{ Qt::CrossCursor, "CrossCursor" },
		{ Qt::WaitCursor, "WaitCursor" },
		{ Qt::IBeamCursor, "IBeamCursor" },
		{ Qt::SizeVerCursor, "SizeVerCursor" },
		{ Qt::SizeHorCursor, "SizeHorCursor" },
		{ Qt::SizeBDiagCursor, "SizeBDiagCursor" },
		{ Qt::SizeFDiagCursor, "SizeFDiagCursor" },
		{ Qt::SizeAllCursor, "SizeAllCursor" },
		{ Qt::BlankCursor, "BlankCursor" },
		{ Qt::SplitVCursor, "SplitVCursor" },
		{ Qt::SplitHCursor, "SplitHCursor" },
		{ Qt::PointingHandCursor, "PointingHandCursor" },
		{ Qt::ForbiddenCursor, "ForbiddenCursor" },
		{ Qt::WhatsThisCursor, "WhatsThisCursor" },
		{ Qt::BusyCursor, "BusyCursor" },
		{ Qt::OpenHandCursor, "OpenHandCursor" },
		{ Qt::ClosedHandCursor, "ClosedHandCursor" },
		{ Qt::DragCopyCursor, "DragCopyCursor" },
		{ Qt::DragMoveCursor, "DragMoveCursor" },
		{ Qt::DragLinkCursor, "DragLinkCursor" },
	};

	QString sizePolicyText(QSizePolicy::Policy policy)
	{
		for (const auto& item : kSizePolicyKeys)
		{
			if (item.policy == policy)
			{
				return QString::fromLatin1(item.pszKey);
			}
		}
		return QString::number(int(policy));
	}

	QString cursorShapeText(Qt::CursorShape shape)
	{
		for (const auto& item : kCursorShapeKeys)
		{
			if (item.shape == shape)
			{
				return QString::fromLatin1(item.pszKey);
			}
		}
		return QString::number(int(shape));
	}

	// key 或数值文本 -> 枚举值(bKeyValid 标记是否命中)
	QSizePolicy::Policy sizePolicyFromString(const QString& strText, bool& bKeyValid)
	{
		for (const auto& item : kSizePolicyKeys)
		{
			if (strText == QLatin1String(item.pszKey))
			{
				bKeyValid = true;
				return item.policy;
			}
		}
		bool bConverted = false;
		const int nValue = strText.toInt(&bConverted, 0);
		bKeyValid = bConverted && (0 <= nValue);
		return bKeyValid ? QSizePolicy::Policy(nValue) : QSizePolicy::Fixed;
	}

	Qt::CursorShape cursorShapeFromString(const QString& strText, bool& bKeyValid)
	{
		for (const auto& item : kCursorShapeKeys)
		{
			if (strText == QLatin1String(item.pszKey))
			{
				bKeyValid = true;
				return item.shape;
			}
		}
		bool bConverted = false;
		const int nValue = strText.toInt(&bConverted, 0);
		bKeyValid = bConverted && (0 <= nValue) && (Qt::LastCursor >= nValue);
		return bKeyValid ? Qt::CursorShape(nValue) : Qt::ArrowCursor;
	}
}

namespace ParamEditor
{
	ParamType fromProperty(const QMetaProperty& metaProperty)
	{
		ParamType type;
		type.nTypeId = metaProperty.userType();
		type.strTypeName = QString::fromLatin1(metaProperty.typeName() ? metaProperty.typeName() : "");
		type.metaEnum = metaProperty.enumerator();
		type.bFlag = metaProperty.isFlagType();
		return type;
	}

	bool isEditable(const ParamType& type, bool bAllowPointer)
	{
		if (type.bPointer)
		{
			return bAllowPointer;
		}
		if (type.metaEnum.isValid())
		{
			return true;   // 枚举/flags
		}
		switch (type.nTypeId)
		{
		case QMetaType::Bool:
		case QMetaType::Int:
		case QMetaType::UInt:
		case QMetaType::LongLong:
		case QMetaType::ULongLong:
		case QMetaType::Long:
		case QMetaType::ULong:
		case QMetaType::Short:
		case QMetaType::UShort:
		case QMetaType::Char:
		case QMetaType::SChar:
		case QMetaType::UChar:
		case QMetaType::Double:
		case QMetaType::Float:
		case QMetaType::QString:
		case QMetaType::QChar:
		case QMetaType::QByteArray:
		case QMetaType::QColor:
		case QMetaType::QFont:
		case QMetaType::QPoint:
		case QMetaType::QPointF:
		case QMetaType::QSize:
		case QMetaType::QSizeF:
		case QMetaType::QRect:
		case QMetaType::QRectF:
		case QMetaType::QDate:
		case QMetaType::QTime:
		case QMetaType::QDateTime:
		case QMetaType::QKeySequence:
		case QMetaType::QUrl:
		case QMetaType::QStringList:
		case QMetaType::QSizePolicy:
		case QMetaType::QCursor:
			return true;
		default:
			break;
		}
		return false;
	}

	QString valueToText(const ParamType& type, const QVariant& value)
	{
		if (!value.isValid())
		{
			return "<无效>";
		}
		if (type.metaEnum.isValid())
		{
			return enumValueText(type.metaEnum, value.toInt(), type.bFlag);
		}

		switch (type.nTypeId)
		{
		case QMetaType::Bool:
			return value.toBool() ? "true" : "false";
		case QMetaType::QString:
			return value.toString();
		case QMetaType::Double:
		case QMetaType::Float:
			return QString::number(value.toDouble(), 'g', 15);
		case QMetaType::QChar:
			return value.toChar();
		case QMetaType::QByteArray:
			return QString::fromLatin1(value.toByteArray());
		case QMetaType::QColor:
			return value.value<QColor>().name(QColor::HexArgb);
		case QMetaType::QFont:
		{
			// 紧凑格式 "family,pt[,bold][,italic]"(textToValue 按同格式解析)
			const QFont font = value.value<QFont>();
			QString strText = QStringLiteral("%1,%2").arg(font.family()).arg(font.pointSize());
			if (font.bold())
			{
				strText += QStringLiteral(",bold");
			}
			if (font.italic())
			{
				strText += QStringLiteral(",italic");
			}
			return strText;
		}
		case QMetaType::QPoint:
		{
			const QPoint pt = value.toPoint();
			return QStringLiteral("%1, %2").arg(pt.x()).arg(pt.y());
		}
		case QMetaType::QPointF:
		{
			const QPointF pt = value.toPointF();
			return QStringLiteral("%1, %2").arg(pt.x()).arg(pt.y());
		}
		case QMetaType::QSize:
		{
			const QSize sz = value.toSize();
			return QStringLiteral("%1, %2").arg(sz.width()).arg(sz.height());
		}
		case QMetaType::QSizeF:
		{
			const QSizeF sz = value.toSizeF();
			return QStringLiteral("%1, %2").arg(sz.width()).arg(sz.height());
		}
		case QMetaType::QRect:
		{
			// "%1, %2, %3, %4": 必须与 textToValue 的 splitNumbers(4) 逗号格式一致,
			// 用 "x" 连接宽高会破坏文本往返(属性面板编辑器取不到当前值)
			const QRect rc = value.toRect();
			return QStringLiteral("%1, %2, %3, %4").arg(rc.x()).arg(rc.y()).arg(rc.width()).arg(rc.height());
		}
		case QMetaType::QRectF:
		{
			const QRectF rc = value.toRectF();
			return QStringLiteral("%1, %2, %3, %4").arg(rc.x()).arg(rc.y()).arg(rc.width()).arg(rc.height());
		}
		case QMetaType::QDate:
			return value.toDate().toString(QStringLiteral("yyyy-MM-dd"));
		case QMetaType::QTime:
			return value.toTime().toString(QStringLiteral("hh:mm:ss"));
		case QMetaType::QDateTime:
			return value.toDateTime().toString(QLatin1String(kDateTimeFormat));
		case QMetaType::QKeySequence:
			return value.value<QKeySequence>().toString();
		case QMetaType::QUrl:
			return value.toUrl().toString();
		case QMetaType::QStringList:
			return value.toStringList().join(QLatin1Char(','));
		case QMetaType::QSizePolicy:
		{
			// "hPolicy, vPolicy, hStretch, vStretch"(Policy 用 key, 无对应 key 退数值)
			const QSizePolicy sp = value.value<QSizePolicy>();
			return QStringLiteral("%1, %2, %3, %4")
				.arg(sizePolicyText(sp.horizontalPolicy()), sizePolicyText(sp.verticalPolicy()))
				.arg(sp.horizontalStretch())
				.arg(sp.verticalStretch());
		}
		case QMetaType::QCursor:
			// 光标可编辑的只有形状(位图/热点光标退数值)
			return cursorShapeText(value.value<QCursor>().shape());
		default:
			break;
		}

		// 无编辑器的类型仍要能显示: 先 toString, 空则 QDebug 转储
		QString strValue = value.toString();
		if (!strValue.isEmpty())
		{
			return strValue;
		}
		QDebug debugOutput(&strValue);
		debugOutput.noquote().nospace() << value;
		return strValue;
	}

	QVariant textToValue(const ParamType& type, const QString& strText)
	{
		const QString strValue = strText.trimmed();
		if (type.bPointer)
		{
			bool bConverted = false;
			const qulonglong ullAddress = strValue.toULongLong(&bConverted, 0);
			return bConverted ? QVariant(ullAddress) : QVariant();
		}
		if (type.metaEnum.isValid())
		{
			QByteArray arrValue = strValue.toLatin1();
			bool bConverted = false;
			int nValue = type.bFlag
				? type.metaEnum.keysToValue(arrValue.constData(), &bConverted)
				: type.metaEnum.keyToValue(arrValue.constData(), &bConverted);
			if (!bConverted)
			{
				nValue = strValue.toInt(&bConverted, 0);
			}
			if (!bConverted)
			{
				return QVariant();
			}
			QVariant value = nValue;
			coerceTypeId(value, type.nTypeId, true);
			return value;
		}

		bool bConverted = false;
		switch (type.nTypeId)
		{
		case QMetaType::Bool:
			if ((0 == strValue.compare("true", Qt::CaseInsensitive)) || ("1" == strValue))
			{
				return QVariant(true);
			}
			if ((0 == strValue.compare("false", Qt::CaseInsensitive)) || ("0" == strValue))
			{
				return QVariant(false);
			}
			return QVariant();
		case QMetaType::Int:
		case QMetaType::LongLong:
		case QMetaType::Long:
		case QMetaType::Short:
		case QMetaType::Char:
		case QMetaType::SChar:
		{
			const qlonglong llValue = strValue.toLongLong(&bConverted, 0);
			if (!bConverted)
			{
				return QVariant();
			}
			QVariant value = llValue;
			return coerceTypeId(value, type.nTypeId, false) ? value : QVariant();
		}
		case QMetaType::UInt:
		case QMetaType::ULongLong:
		case QMetaType::ULong:
		case QMetaType::UShort:
		case QMetaType::UChar:
		{
			const qulonglong ullValue = strValue.toULongLong(&bConverted, 0);
			if (!bConverted)
			{
				return QVariant();
			}
			QVariant value = ullValue;
			return coerceTypeId(value, type.nTypeId, false) ? value : QVariant();
		}
		case QMetaType::Double:
		case QMetaType::Float:
		{
			const double dValue = strValue.toDouble(&bConverted);
			if (!bConverted)
			{
				return QVariant();
			}
			QVariant value = dValue;
			return coerceTypeId(value, type.nTypeId, false) ? value : QVariant();
		}
		case QMetaType::QString:
			return QVariant(strText);
		case QMetaType::QChar:
			return strValue.isEmpty() ? QVariant() : QVariant::fromValue(strValue.at(0));
		case QMetaType::QByteArray:
			return QVariant(strValue.toLatin1());
		case QMetaType::QColor:
		{
			// "#RRGGBB" / "#AARRGGBB" / 颜色名
			const QColor color(strValue);
			return color.isValid() ? QVariant::fromValue(color) : QVariant();
		}
		case QMetaType::QFont:
		{
			// 紧凑格式 "family,pt[,bold][,italic]"
			static const QRegularExpression expression(QStringLiteral("^(.+),(-?\\d+)(,bold)?(,italic)?$"));
			const auto match = expression.match(strValue);
			if (!match.hasMatch())
			{
				return QVariant();
			}
			QFont font(match.captured(1).trimmed(), match.captured(2).toInt());
			font.setBold(match.captured(3).contains(QLatin1String("bold")));
			font.setItalic(match.captured(4).contains(QLatin1String("italic")));
			return QVariant::fromValue(font);
		}
		case QMetaType::QPoint:
		case QMetaType::QPointF:
		{
			QVector<double> arrNumbers;
			if (!splitNumbers(strValue, 2, arrNumbers))
			{
				return QVariant();
			}
			if (QMetaType::QPoint == type.nTypeId)
			{
				return QVariant(QPoint(int(arrNumbers[0]), int(arrNumbers[1])));
			}
			return QVariant::fromValue(QPointF(arrNumbers[0], arrNumbers[1]));
		}
		case QMetaType::QSize:
		case QMetaType::QSizeF:
		{
			QVector<double> arrNumbers;
			if (!splitNumbers(strValue, 2, arrNumbers))
			{
				return QVariant();
			}
			if (QMetaType::QSize == type.nTypeId)
			{
				return QVariant(QSize(int(arrNumbers[0]), int(arrNumbers[1])));
			}
			return QVariant::fromValue(QSizeF(arrNumbers[0], arrNumbers[1]));
		}
		case QMetaType::QRect:
		case QMetaType::QRectF:
		{
			QVector<double> arrNumbers;
			if (!splitNumbers(strValue, 4, arrNumbers))
			{
				return QVariant();
			}
			if (QMetaType::QRect == type.nTypeId)
			{
				return QVariant(QRect(int(arrNumbers[0]), int(arrNumbers[1]), int(arrNumbers[2]), int(arrNumbers[3])));
			}
			return QVariant::fromValue(QRectF(arrNumbers[0], arrNumbers[1], arrNumbers[2], arrNumbers[3]));
		}
		case QMetaType::QDate:
		{
			const QDate date = QDate::fromString(strValue, QStringLiteral("yyyy-MM-dd"));
			return date.isValid() ? QVariant(date) : QVariant();
		}
		case QMetaType::QTime:
		{
			const QTime time = QTime::fromString(strValue, QStringLiteral("hh:mm:ss"));
			return time.isValid() ? QVariant(time) : QVariant();
		}
		case QMetaType::QDateTime:
		{
			QDateTime dateTime = QDateTime::fromString(strValue, QLatin1String(kDateTimeFormat));
			if (!dateTime.isValid())
			{
				dateTime = QDateTime::fromString(strValue, Qt::ISODate);
			}
			return dateTime.isValid() ? QVariant(dateTime) : QVariant();
		}
		case QMetaType::QKeySequence:
			return QVariant::fromValue(QKeySequence(strValue));
		case QMetaType::QUrl:
			return strValue.isEmpty() ? QVariant() : QVariant::fromValue(QUrl(strValue));
		case QMetaType::QStringList:
			return QVariant(strValue.split(QLatin1Char(',')));
		case QMetaType::QSizePolicy:
		{
			// "hPolicy, vPolicy[, hStretch, vStretch]"; 缺省 stretch 为 0
			const QStringList listParts = strValue.split(QLatin1Char(','), Qt::SkipEmptyParts);
			if ((2 > listParts.size()) || (4 < listParts.size()))
			{
				return QVariant();
			}
			bool bKeyValid = false;
			const QSizePolicy::Policy hPolicy = sizePolicyFromString(listParts.at(0).trimmed(), bKeyValid);
			if (!bKeyValid)
			{
				return QVariant();
			}
			const QSizePolicy::Policy vPolicy = sizePolicyFromString(listParts.at(1).trimmed(), bKeyValid);
			if (!bKeyValid)
			{
				return QVariant();
			}
			QSizePolicy sp(hPolicy, vPolicy);
			for (int i = 2; i < listParts.size(); ++i)
			{
				bool bConverted = false;
				const int nStretch = listParts.at(i).trimmed().toInt(&bConverted, 0);
				if (!bConverted || (0 > nStretch) || (255 < nStretch))
				{
					return QVariant();
				}
				(2 == i) ? sp.setHorizontalStretch(nStretch) : sp.setVerticalStretch(nStretch);
			}
			return QVariant::fromValue(sp);
		}
		case QMetaType::QCursor:
		{
			bool bKeyValid = false;
			const Qt::CursorShape shape = cursorShapeFromString(strValue, bKeyValid);
			return bKeyValid ? QVariant::fromValue(QCursor(shape)) : QVariant();
		}
		default:
			break;
		}
		return QVariant();
	}

	QWidget* createEditor(const ParamType& type, QWidget* pParent)
	{
		if (!isEditable(type, true))
		{
			return nullptr;
		}
		if (type.bPointer)
		{
			return new CParamLineEdit(type, pParent);   // 十六进制地址文本
		}
		if (QMetaType::Bool == type.nTypeId)
		{
			return new CParamComboBox(type, true, pParent);
		}
		if (type.metaEnum.isValid() && !type.bFlag)
		{
			return new CParamComboBox(type, false, pParent);
		}
		if (QMetaType::QColor == type.nTypeId)
		{
			return new CParamColorEditor(pParent);
		}
		if (QMetaType::QCursor == type.nTypeId)
		{
			return new CParamCursorCombo(pParent);
		}
		switch (type.nTypeId)
		{
		// 复合值(多字段): 按钮弹窗编辑, 逗号文本不直观
		case QMetaType::QPoint:
		case QMetaType::QPointF:
		case QMetaType::QSize:
		case QMetaType::QSizeF:
		case QMetaType::QRect:
		case QMetaType::QRectF:
		case QMetaType::QSizePolicy:
		case QMetaType::QDateTime:
		case QMetaType::QStringList:
			return new CParamStructButton(type, pParent);
		default:
			break;
		}
		if (QMetaType::QFont == type.nTypeId)
		{
			return new CParamFontButton(pParent);
		}
		return new CParamLineEdit(type, pParent);
	}

	void setEditorValue(QWidget* pEditor, const QVariant& value)
	{
		pEditor->setProperty("paramValue", value);
	}

	QVariant editorValue(QWidget* pEditor)
	{
		return pEditor->property("paramValue");
	}

	// ---- CParamLineEdit ----

	CParamLineEdit::CParamLineEdit(const ParamType& type, QWidget* pParent)
		: QLineEdit(pParent), m_type(type)
	{
		setProperty("qtspyPropertyEditor", true);
	}

	QVariant CParamLineEdit::paramValue() const
	{
		return textToValue(m_type, text());
	}

	void CParamLineEdit::setParamValue(const QVariant& value)
	{
		setText(valueToText(m_type, value));
	}

	// ---- CParamComboBox ----

	CParamComboBox::CParamComboBox(const ParamType& type, bool bBoolMode, QWidget* pParent)
		: QComboBox(pParent), m_bBoolMode(bBoolMode)
	{
		// QSS 的 QComboBox QAbstractItemView::item 规则(行高 28)在默认内部 view 上不生效,
		// 需显式 setView 换成 QListView; 编辑器高度与输入框对齐
		setView(new QListView);
		setFixedHeight(28);
		setProperty("qtspyPropertyEditor", true);
		if (bBoolMode)
		{
			addItem("false", 0);
			addItem("true", 1);
		}
		else
		{
			for (int nKeyIndex = 0; nKeyIndex < type.metaEnum.keyCount(); ++nKeyIndex)
			{
				addItem(QString::fromLatin1(type.metaEnum.key(nKeyIndex)), type.metaEnum.value(nKeyIndex));
			}
		}
	}

	QVariant CParamComboBox::paramValue() const
	{
		const int nValue = currentData().toInt();
		return m_bBoolMode ? QVariant(0 != nValue) : QVariant(nValue);
	}

	void CParamComboBox::setParamValue(const QVariant& value)
	{
		const int nValue = value.toInt();
		int nIndex = findData(nValue);
		if (0 > nIndex)
		{
			nIndex = m_bBoolMode ? (0 != nValue ? 1 : 0) : 0;
		}
		setCurrentIndex(nIndex);
	}

	// ---- CParamColorEditor ----

	CParamColorEditor::CParamColorEditor(QWidget* pParent)
		: QWidget(pParent)
	{
		setProperty("qtspyPropertyEditor", true);
		QHBoxLayout* pLayout = new QHBoxLayout(this);
		pLayout->setContentsMargins(0, 0, 0, 0);
		pLayout->setSpacing(4);
		m_pTextEdit = new QLineEdit;
		m_pTextEdit->setFixedHeight(28);
		pLayout->addWidget(m_pTextEdit);
		QToolButton* pPickButton = new QToolButton;
		pPickButton->setText("...");
		pPickButton->setFixedHeight(28);
		pLayout->addWidget(pPickButton);
		connect(pPickButton, &QToolButton::clicked, this, [this]() {
			// 选色后同步写文本(paramText 为唯一事实来源, 失败保持原值)
			const QColor color = QColorDialog::getColor(QColor(paramText()), this, "选择颜色");
			if (color.isValid())
			{
				setParamText(color.name(QColor::HexArgb));
			}
		});
	}

	QString CParamColorEditor::paramText() const
	{
		return m_pTextEdit->text();
	}

	void CParamColorEditor::setParamText(const QString& strText)
	{
		m_pTextEdit->setText(strText);
	}

	QVariant CParamColorEditor::paramValue() const
	{
		const QColor color(m_pTextEdit->text().trimmed());
		return color.isValid() ? QVariant::fromValue(color) : QVariant();
	}

	void CParamColorEditor::setParamValue(const QVariant& value)
	{
		m_pTextEdit->setText(value.value<QColor>().name(QColor::HexArgb));
	}

	// ---- CParamCursorCombo ----

	CParamCursorCombo::CParamCursorCombo(QWidget* pParent)
		: QComboBox(pParent)
	{
		// 与 CParamComboBox 同因: QSS item 规则需要显式 QListView
		setView(new QListView);
		setFixedHeight(28);
		setProperty("qtspyPropertyEditor", true);
		for (const auto& item : kCursorShapeKeys)
		{
			addItem(QString::fromLatin1(item.pszKey), int(item.shape));
		}
	}

	QString CParamCursorCombo::paramText() const
	{
		return currentText();
	}

	void CParamCursorCombo::setParamText(const QString& strText)
	{
		// 属性面板的文本可能是 key 名, 也可能是数值(无 key 命中的形状)
		bool bKeyValid = false;
		const Qt::CursorShape shape = cursorShapeFromString(strText.trimmed(), bKeyValid);
		const int nIndex = bKeyValid ? findData(int(shape)) : 0;
		setCurrentIndex((0 > nIndex) ? 0 : nIndex);
	}

	QVariant CParamCursorCombo::paramValue() const
	{
		return QVariant::fromValue(QCursor(Qt::CursorShape(currentData().toInt())));
	}

	void CParamCursorCombo::setParamValue(const QVariant& value)
	{
		const int nIndex = findData(int(value.value<QCursor>().shape()));
		setCurrentIndex((0 > nIndex) ? 0 : nIndex);
	}

	// ---- CParamStructButton ----

	CParamStructButton::CParamStructButton(const ParamType& type, QWidget* pParent)
		: QPushButton(pParent), m_type(type)
	{
		setProperty("qtspyPropertyEditor", true);
		setFixedHeight(28);
		connect(this, &QPushButton::clicked, this, [this]() { openDialog(); });
	}

	QString CParamStructButton::paramText() const
	{
		// 值有效给精确格式; 解析失败回退原文(提交按原文写回, 不伪造数据)
		return m_value.isValid() ? valueToText(m_type, m_value) : m_strRawText;
	}

	void CParamStructButton::setParamText(const QString& strText)
	{
		const QVariant value = textToValue(m_type, strText);
		if (value.isValid())
		{
			setParamValue(value);
		}
		else
		{
			// 原文解析失败: 保留原文显示与提交
			m_value = QVariant();
			m_strRawText = strText;
			setText(strText);
		}
	}

	QVariant CParamStructButton::paramValue() const
	{
		return m_value;
	}

	void CParamStructButton::setParamValue(const QVariant& value)
	{
		m_value = value;
		if (value.isValid())
		{
			m_strRawText.clear();
		}
		setText(paramText());
	}

	void CParamStructButton::showEvent(QShowEvent* event)
	{
		QPushButton::showEvent(event);
		// 属性面板的自动弹窗: 编辑器显示(setEditorData 已完成, setFocus 尚未执行)后
		// 推迟到事件循环弹窗, 免去"双击出按钮再点一次"的二次交互
		if (!property("qtspyAutoOpenDialog").toBool() || m_bAutoOpenDone)
		{
			return;
		}
		m_bAutoOpenDone = true;
		QTimer::singleShot(0, this, [this]() {
			openDialog();
			// 弹窗结束即收尾: 让视图的 delegate 过滤器提交并收掉本编辑器。
			// 焦点此时是否已归还编辑器取决于窗口激活切换的时序(竞争), 两条路都覆盖:
			// 焦点已在编辑器上 -> clearFocus 产生真实 FocusOut;
			// 否则 -> 合成 FocusOut(delegate 只看"应用焦点不在编辑器上"即提交并收)
			if (QApplication::focusWidget() == this)
			{
				clearFocus();
			}
			else
			{
				QFocusEvent focusOutEvent(QEvent::FocusOut);
				QCoreApplication::sendEvent(this, &focusOutEvent);
			}
		});
	}

	void CParamStructButton::openDialog()
	{
		QDialog dialog(this);
		dialog.setWindowTitle(QStringLiteral("QtSpy · 编辑 %1")
			.arg(QString::fromLatin1(QMetaType::typeName(m_type.nTypeId))));
		QVBoxLayout* pLayout = new QVBoxLayout(&dialog);
		QFormLayout* pForm = new QFormLayout;
		pForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
		pLayout->addLayout(pForm);

		// 通用控件工厂(高度对齐 QSS 的 28 行高; 下拉需显式 QListView)
		auto fnIntSpinBox = []() {
			QSpinBox* pSpinBox = new QSpinBox;
			pSpinBox->setRange(-2147483647, 2147483647);
			pSpinBox->setFixedHeight(28);
			return pSpinBox;
		};
		auto fnDoubleSpinBox = []() {
			QDoubleSpinBox* pSpinBox = new QDoubleSpinBox;
			pSpinBox->setRange(-1000000000.0, 1000000000.0);
			pSpinBox->setDecimals(6);
			pSpinBox->setFixedHeight(28);
			return pSpinBox;
		};
		auto fnComboBox = []() {
			QComboBox* pComboBox = new QComboBox;
			pComboBox->setView(new QListView);
			pComboBox->setFixedHeight(28);
			return pComboBox;
		};
		// 捕获 this: MSVC 对 lambda 内基类静态 connect 报 C4573(误报), 显式捕获可消除
		auto fnAddButtons = [this, &dialog, pLayout]() {
			QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			pButtonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
			pButtonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
			QObject::connect(pButtonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
			QObject::connect(pButtonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
			pLayout->addWidget(pButtonBox);
		};

		switch (m_type.nTypeId)
		{
		case QMetaType::QPoint:
		{
			const QPoint ptInitial = m_value.toPoint();
			QSpinBox* pX = fnIntSpinBox();
			pX->setValue(ptInitial.x());
			QSpinBox* pY = fnIntSpinBox();
			pY->setValue(ptInitial.y());
			pForm->addRow(QStringLiteral("x"), pX);
			pForm->addRow(QStringLiteral("y"), pY);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				setParamValue(QVariant(QPoint(pX->value(), pY->value())));
			}
			return;
		}
		case QMetaType::QPointF:
		{
			const QPointF ptInitial = m_value.toPointF();
			QDoubleSpinBox* pX = fnDoubleSpinBox();
			pX->setValue(ptInitial.x());
			QDoubleSpinBox* pY = fnDoubleSpinBox();
			pY->setValue(ptInitial.y());
			pForm->addRow(QStringLiteral("x"), pX);
			pForm->addRow(QStringLiteral("y"), pY);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				setParamValue(QVariant::fromValue(QPointF(pX->value(), pY->value())));
			}
			return;
		}
		case QMetaType::QSize:
		{
			const QSize szInitial = m_value.toSize();
			QSpinBox* pWidth = fnIntSpinBox();
			pWidth->setValue(szInitial.width());
			QSpinBox* pHeight = fnIntSpinBox();
			pHeight->setValue(szInitial.height());
			pForm->addRow(QStringLiteral("宽"), pWidth);
			pForm->addRow(QStringLiteral("高"), pHeight);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				setParamValue(QVariant(QSize(pWidth->value(), pHeight->value())));
			}
			return;
		}
		case QMetaType::QSizeF:
		{
			const QSizeF szInitial = m_value.toSizeF();
			QDoubleSpinBox* pWidth = fnDoubleSpinBox();
			pWidth->setValue(szInitial.width());
			QDoubleSpinBox* pHeight = fnDoubleSpinBox();
			pHeight->setValue(szInitial.height());
			pForm->addRow(QStringLiteral("宽"), pWidth);
			pForm->addRow(QStringLiteral("高"), pHeight);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				setParamValue(QVariant::fromValue(QSizeF(pWidth->value(), pHeight->value())));
			}
			return;
		}
		case QMetaType::QRect:
		{
			const QRect rcInitial = m_value.toRect();
			QSpinBox* pX = fnIntSpinBox();
			pX->setValue(rcInitial.x());
			QSpinBox* pY = fnIntSpinBox();
			pY->setValue(rcInitial.y());
			QSpinBox* pWidth = fnIntSpinBox();
			pWidth->setValue(rcInitial.width());
			QSpinBox* pHeight = fnIntSpinBox();
			pHeight->setValue(rcInitial.height());
			pForm->addRow(QStringLiteral("x"), pX);
			pForm->addRow(QStringLiteral("y"), pY);
			pForm->addRow(QStringLiteral("宽"), pWidth);
			pForm->addRow(QStringLiteral("高"), pHeight);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				setParamValue(QVariant(QRect(pX->value(), pY->value(), pWidth->value(), pHeight->value())));
			}
			return;
		}
		case QMetaType::QRectF:
		{
			const QRectF rcInitial = m_value.toRectF();
			QDoubleSpinBox* pX = fnDoubleSpinBox();
			pX->setValue(rcInitial.x());
			QDoubleSpinBox* pY = fnDoubleSpinBox();
			pY->setValue(rcInitial.y());
			QDoubleSpinBox* pWidth = fnDoubleSpinBox();
			pWidth->setValue(rcInitial.width());
			QDoubleSpinBox* pHeight = fnDoubleSpinBox();
			pHeight->setValue(rcInitial.height());
			pForm->addRow(QStringLiteral("x"), pX);
			pForm->addRow(QStringLiteral("y"), pY);
			pForm->addRow(QStringLiteral("宽"), pWidth);
			pForm->addRow(QStringLiteral("高"), pHeight);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				setParamValue(QVariant::fromValue(QRectF(pX->value(), pY->value(), pWidth->value(), pHeight->value())));
			}
			return;
		}
		case QMetaType::QSizePolicy:
		{
			const QSizePolicy spInitial = m_value.value<QSizePolicy>();
			auto fnPolicyCombo = [&spInitial, &fnComboBox](bool bHorizontal) {
				QComboBox* pComboBox = fnComboBox();
				for (const auto& item : kSizePolicyKeys)
				{
					pComboBox->addItem(QString::fromLatin1(item.pszKey), int(item.policy));
				}
				const QSizePolicy::Policy policy = bHorizontal
					? spInitial.horizontalPolicy() : spInitial.verticalPolicy();
				int nIndex = pComboBox->findData(int(policy));
				if (0 > nIndex)
				{
					nIndex = pComboBox->findData(int(QSizePolicy::Preferred));   // 兜底 Preferred(表第 4 项)
				}
				pComboBox->setCurrentIndex((0 > nIndex) ? 0 : nIndex);
				return pComboBox;
			};
			QComboBox* pHorizontal = fnPolicyCombo(true);
			QComboBox* pVertical = fnPolicyCombo(false);
			auto fnStretchSpinBox = [&spInitial](bool bHorizontal) {
				QSpinBox* pSpinBox = new QSpinBox;
				pSpinBox->setRange(0, 255);
				pSpinBox->setFixedHeight(28);
				pSpinBox->setValue(bHorizontal ? spInitial.horizontalStretch() : spInitial.verticalStretch());
				return pSpinBox;
			};
			QSpinBox* pHorizontalStretch = fnStretchSpinBox(true);
			QSpinBox* pVerticalStretch = fnStretchSpinBox(false);
			pForm->addRow(QStringLiteral("水平策略"), pHorizontal);
			pForm->addRow(QStringLiteral("垂直策略"), pVertical);
			pForm->addRow(QStringLiteral("水平伸展"), pHorizontalStretch);
			pForm->addRow(QStringLiteral("垂直伸展"), pVerticalStretch);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				QSizePolicy sp(QSizePolicy::Policy(pHorizontal->currentData().toInt()),
					QSizePolicy::Policy(pVertical->currentData().toInt()));
				sp.setHorizontalStretch(pHorizontalStretch->value());
				sp.setVerticalStretch(pVerticalStretch->value());
				setParamValue(QVariant::fromValue(sp));
			}
			return;
		}
		case QMetaType::QDateTime:
		{
			QDateTime dateTimeInitial = m_value.toDateTime();
			if (!dateTimeInitial.isValid())
			{
				dateTimeInitial = QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0));
			}
			QDateEdit* pDate = new QDateEdit;
			pDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
			pDate->setDate(dateTimeInitial.date());
			pDate->setFixedHeight(28);
			QTimeEdit* pTime = new QTimeEdit;
			pTime->setDisplayFormat(QStringLiteral("hh:mm:ss"));
			pTime->setTime(dateTimeInitial.time());
			pTime->setFixedHeight(28);
			pForm->addRow(QStringLiteral("日期"), pDate);
			pForm->addRow(QStringLiteral("时间"), pTime);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				setParamValue(QVariant(QDateTime(pDate->date(), pTime->time())));
			}
			return;
		}
		case QMetaType::QStringList:
		{
			QPlainTextEdit* pListEdit = new QPlainTextEdit;
			pListEdit->setPlainText(m_value.toStringList().join(QLatin1Char('\n')));
			pForm->addRow(QStringLiteral("值(每行一个)"), pListEdit);
			fnAddButtons();
			if (QDialog::Accepted == dialog.exec())
			{
				QStringList listValues = pListEdit->toPlainText().split(QLatin1Char('\n'));
				while (!listValues.isEmpty() && listValues.last().isEmpty())
				{
					listValues.removeLast();
				}
				setParamValue(QVariant(listValues));
			}
			return;
		}
		default:
			break;
		}
	}

	// ---- CParamFontButton ----

	CParamFontButton::CParamFontButton(QWidget* pParent)
		: QPushButton(pParent)
	{
		setProperty("qtspyPropertyEditor", true);
		setFixedHeight(28);
		connect(this, &QPushButton::clicked, this, [this]() { openDialog(); });
	}

	void CParamFontButton::openDialog()
	{
		bool bOk = false;
		const QFont font = QFontDialog::getFont(&bOk, m_font, this, "选择字体");
		if (bOk)
		{
			setParamValue(QVariant::fromValue(font));
		}
	}

	void CParamFontButton::showEvent(QShowEvent* event)
	{
		QPushButton::showEvent(event);
		// 属性面板的自动弹窗: 与 CParamStructButton 同机制
		if (!property("qtspyAutoOpenDialog").toBool() || m_bAutoOpenDone)
		{
			return;
		}
		m_bAutoOpenDone = true;
		QTimer::singleShot(0, this, [this]() {
			openDialog();
			if (QApplication::focusWidget() == this)
			{
				clearFocus();
			}
			else
			{
				QFocusEvent focusOutEvent(QEvent::FocusOut);
				QCoreApplication::sendEvent(this, &focusOutEvent);
			}
		});
	}

	QString CParamFontButton::paramText() const
	{
		const ParamType fontType{ QMetaType::QFont, QString(), QMetaEnum(), false, false };
		return valueToText(fontType, QVariant::fromValue(m_font));
	}

	void CParamFontButton::setParamText(const QString& strText)
	{
		const ParamType fontType{ QMetaType::QFont, QString(), QMetaEnum(), false, false };
		const QVariant value = textToValue(fontType, strText);
		if (value.isValid())
		{
			setParamValue(value);
		}
	}

	QVariant CParamFontButton::paramValue() const
	{
		return QVariant::fromValue(m_font);
	}

	void CParamFontButton::setParamValue(const QVariant& value)
	{
		m_font = value.value<QFont>();
		setText(paramText());
	}
}
