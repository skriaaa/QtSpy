#pragma once
#include <QWidget>
#include <QString>
struct StyleSheetProperty
{
	QString m_strProperty;
};

class CStyleProxy : public QObject
{
	Q_OBJECT
public:
	enum class ESubCtrl {
		self = 0,
		add_line,
		add_page,
		branch,
		chunk,
		close_button,
		corner,
		down_arrow,
		down_button,
		drop_down,
		float_button,
		groove,
		indicator,
		handle,
		icon,
		item,
		left_arrow,
		left_corner,
		menu_arrow,
		menu_button,
		menu_indicator,
		right_arrow,
		pane,
		right_corner,
		scroller,
		section,
		separator,
		sub_line,
		sub_page,
		tab,
		tab_bar,
		tear,
		tearoff,
		text,
		title,
		up_arrow,
		up_button,
		num // number of ESubCtrl
	};
	Q_ENUM(ESubCtrl);

	enum class EPseudoStates
	{
		none = -1,
		active = 0,
		adjoins_item,
		alternate,
		bottom,
		checked,
		closable,
		closed,
		defaultItem,
		disabled,
		editable,
		edit_focus,
		enabled,
		exclusive,
		first,
		flat,
		floatable,
		focus,
		has_children,
		has_siblings,
		horizontal,
		hover,
		indeterminate,
		last,
		left,
		maximized,
		middle,
		minimized,
		movable,
		no_frame,
		non_exclusive,
		off,
		on,
		only_one,
		open,
		next_selected,
		pressed,
		previous_selected,
		read_only,
		right,
		selected,
		top,
		unchecked,
		vertical,
		window,
		num // number of PseudoStates
	};

	Q_ENUM(EPseudoStates);

	enum EStateFlag {
		State_None = 0x00000000,
		State_Enabled = 0x00000001,
		State_Raised = 0x00000002,
		State_Sunken = 0x00000004,
		State_Off = 0x00000008,
		State_NoChange = 0x00000010,
		State_On = 0x00000020,
		State_DownArrow = 0x00000040,
		State_Horizontal = 0x00000080,
		State_HasFocus = 0x00000100,
		State_Top = 0x00000200,
		State_Bottom = 0x00000400,
		State_FocusAtBorder = 0x00000800,
		State_AutoRaise = 0x00001000,
		State_MouseOver = 0x00002000,
		State_UpArrow = 0x00004000,
		State_Selected = 0x00008000,
		State_Active = 0x00010000,
		State_Window = 0x00020000,
		State_Open = 0x00040000,
		State_Children = 0x00080000,
		State_Item = 0x00100000,
		State_Sibling = 0x00200000,
		State_Editing = 0x00400000,
		State_KeyboardFocusChange = 0x00800000,
#ifdef QT_KEYPAD_NAVIGATION
		State_HasEditFocus = 0x01000000,
#endif
		State_ReadOnly = 0x02000000,
		State_Small = 0x04000000,
		State_Mini = 0x08000000
	};
	Q_ENUM(EStateFlag);

	// 子控件位置
	enum ESubcontrolOrigin
	{
		none = 0,
		margin,
		border,
		padding,
		content,
	};
	Q_ENUM(ESubcontrolOrigin);

	typedef std::pair<QString, QString> PropertyPair;
	typedef QVector<PropertyPair> PropertyArray;
public:
	explicit CStyleProxy() = default;
public:
	QVector<ESubCtrl> subCtrl(QWidget* widget);

public:
	//<< 通用接口 >>
	QString makeQssKey(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, PropertyArray arrProperty);
	QString makeQssKey(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, PropertyPair pairProperty);

	QString className(QWidget* widget);

#pragma region

	// 交替背景色
	// 适用于  @QAbstractItemView
	QString setAlternateBackGroundColor(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, QColor color);

	// 背景色
	QString setBackgroundColor(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, QColor color);

#pragma endregion

	// 不显示焦点虚线
	QString hideOutLine(QWidget* widget);

	// 边界
	QString setMargin(QWidget* widget, ESubCtrl subCtrl, QMargins margin);
	QString setPadding(QWidget* widget, ESubCtrl subCtrl, QMargins padding);

	// 下划线 (!!!注意 QLabel不支持 :hover 的状态，因此搭配使用不好使)
	QString setUnderLine(QWidget* widget, ESubCtrl subCtrl, EPseudoStates ePseudoStates);

	// 删除线 (!!!注意 QLabel不支持 :hover 的状态，因此搭配使用不好使)
	QString setDeleteLine(QWidget* widget, ESubCtrl subCtrl, EPseudoStates ePseudoStates);

	// QGroupBox
	QString setTitleStyle(QWidget* widget, EPseudoStates ePseudoStates, ESubcontrolOrigin origin, QMargins margin, QMargins padding);
public:

	// QHeaderView 
	/// 隐藏分割线
	QString hideHeaderGrid(QWidget* widget);

public:
	static QString queryStyleSheet(QWidget* widget, ESubCtrl subCtrl, EPseudoStates pseudoState, QString value);
	static QString querySubCtrlName(ESubCtrl subCtrl);
	static QString queryPseudoStateName(EPseudoStates pseudoState);
	static QString marginToString(QMargins margin);
private:
	QVector<StyleSheetProperty> m_arrStyleProperties;
};

Q_DECLARE_METATYPE(CStyleProxy::ESubCtrl);
Q_DECLARE_METATYPE(CStyleProxy::EPseudoStates);
