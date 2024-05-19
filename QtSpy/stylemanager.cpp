#include "stylemanager.h"
#include <QStyle>

CWidgetProxy::CWidgetProxy()
{
	m_hashSubCtrl = {
		{ESubCtrl::Self,"Self"},
	};
}

QVector<ESubCtrl> CWidgetProxy::subCtrl(QWidget* widget)
{
	QVector<ESubCtrl> arrSubCtrl;
	if (widget->inherits("QScrollBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::add_line,
			ESubCtrl::add_page,
			ESubCtrl::down_button,
			ESubCtrl::handle,
			ESubCtrl::left_arrow,
			ESubCtrl::right_arrow,
			ESubCtrl::up_arrow,
			ESubCtrl::down_arrow,
			ESubCtrl::sub_line,
			ESubCtrl::sub_page
			});
	}
	if (widget->inherits("QTreeView"))
	{
		arrSubCtrl.append({
			ESubCtrl::branch
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk
			});
	}
	if (widget->inherits("QDockWidget"))
	{
		arrSubCtrl.append({
			ESubCtrl::close_button,
			ESubCtrl::float_button,
			ESubCtrl::title
			});
	}
	if (widget->inherits("QTabBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			ESubCtrl::tab,
			ESubCtrl::scroller,
			ESubCtrl::tear
			});
	}
	if (widget->inherits("QTabWidget"))
	{
		arrSubCtrl.append({
			ESubCtrl::left_corner,
			ESubCtrl::right_corner,
			ESubCtrl::pane,
			ESubCtrl::tab_bar
			});
	}
	if (widget->inherits("QAbstractScrollArea"))
	{
		arrSubCtrl.append({
			ESubCtrl::corner,
			});
	}
	if (widget->inherits("QComboBox"))
	{
		arrSubCtrl.append({
			ESubCtrl::down_arrow,
			ESubCtrl::drop_down,
			});
	}
	if (widget->inherits("QHeaderView"))
	{
		arrSubCtrl.append({
			ESubCtrl::down_arrow,
			ESubCtrl::up_arrow,
			ESubCtrl::section
			});
	}
	if (widget->inherits("QSpinBox"))
	{
		arrSubCtrl.append({
			ESubCtrl::down_arrow,
			ESubCtrl::down_button,
			ESubCtrl::up_arrow,
			ESubCtrl::up_button
			});
	}
	if (widget->inherits("QSlider"))
	{
		arrSubCtrl.append({
			ESubCtrl::groove,
			ESubCtrl::handle
			});
	}
	if (widget->inherits("QAbstractItemView"))
	{
		arrSubCtrl.append({
			ESubCtrl::indicator,
			ESubCtrl::icon,
			ESubCtrl::item,
			ESubCtrl::text,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	if (widget->inherits("QProgressBar"))
	{
		arrSubCtrl.append({
			ESubCtrl::chunk,
			});
	}
	return arrSubCtrl;
}
