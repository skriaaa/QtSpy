#include "stylemanager.h"
#include <QStyle>
#include <QMetaEnum>

CStyleProxy::CStyleProxy()
{
	m_hashSubCtrl = {
		{ESubCtrl::Self,"Self"},
	};
}

QVector<CStyleProxy::ESubCtrl> CStyleProxy::subCtrl(QWidget* widget)
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
	return arrSubCtrl;
}

QString CStyleProxy::hideHeaderGrid(QWidget* widget)
{
	QString strClass = widget->metaObject()->className();
	QString strSubCtrl = querySubCtrlName(ESubCtrl::section);
	QString strContent = "border:none;"; // background - color:white; 会把背景色搞成灰色，需要搞一下
	QString strStyle = QString("%1:%2{%3}").arg(strClass).arg(strSubCtrl).arg(strContent);
	return strStyle;
}

QString CStyleProxy::queryStyleSheet(QWidget* widget, ESubCtrl subCtrl, EPseudoStates pseudoState, QString value)
{
	return "";
}

QString CStyleProxy::querySubCtrlName(ESubCtrl subCtrl)
{
	if (subCtrl == ESubCtrl::Self)
	{
		return "";
	}

	QMetaEnum metaEnum = QMetaEnum::fromType<ESubCtrl>();
	return metaEnum.valueToKey(subCtrl);
}
