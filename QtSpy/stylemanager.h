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

public:

	// QHeaderView 
	/// 隐藏分割线
	QString hideHeaderGrid(QWidget* widget);

public:
	static QString queryStyleSheet(QWidget* widget, ESubCtrl subCtrl, EPseudoStates pseudoState, QString value);
	static QString querySubCtrlName(ESubCtrl subCtrl);
	static QString queryPseudoStateName(EPseudoStates pseudoState);
private:
	QVector<StyleSheetProperty> m_arrStyleProperties;
};
Q_DECLARE_METATYPE(CStyleProxy::ESubCtrl);
Q_DECLARE_METATYPE(CStyleProxy::EPseudoStates);
