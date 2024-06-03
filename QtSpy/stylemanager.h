#pragma once
#include <QWidget>
#include <QString>

enum ESubCtrl {
	Self = 0,
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
Q_DECLARE_METATYPE(ESubCtrl)
enum class EPseudoStates
{
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
Q_DECLARE_METATYPE(EPseudoStates)
struct StyleSheetProperty
{
	QString m_strProperty;
};
class CWidgetProxy
{
public:
	explicit CWidgetProxy();
	~CWidgetProxy() = default;
public:
	QVector<ESubCtrl> subCtrl(QWidget* widget);
public:
	static QString queryStyleSheet(QWidget* widget, ESubCtrl subCtrl, EPseudoStates pseudoState, QString value);
	
private:
	QHash<ESubCtrl, QString> m_hashSubCtrl;
	QVector<StyleSheetProperty> m_arrStyleProperties;
};