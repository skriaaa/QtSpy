#include "stylemanager.h"
#include <QStyle>
#include <QMetaEnum>

CStyleProxy::CStyleProxy()
{

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

QString CStyleProxy::makeQssKey(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, PropertyArray arrProperty)
{
	QString strClass = className(widget);
	QString strSubCtrl = querySubCtrlName(eSubCtrl);
	if (false == strSubCtrl.isEmpty())
	{
		strSubCtrl = "::" + strSubCtrl;
	}
	QString strPseudoStates = queryPseudoStateName(ePseudoStates);
	if (false == strPseudoStates.isEmpty())
	{
		strPseudoStates = ":" + strPseudoStates;
	}
	QStringList arrStrProperty;
	for (PropertyPair& pair : arrProperty)
	{
		arrStrProperty.push_back(QString("%1 : %2;").arg(pair.first).arg(pair.second));
	}
	QString strStyle = QString("%1%2%3{%3}").arg(strClass).arg(strSubCtrl).arg(strPseudoStates).arg(arrStrProperty.join('\n'));
}

QString CStyleProxy::makeQssKey(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, PropertyPair pairProperty)
{
	return makeQssKey(widget, eSubCtrl, ePseudoStates, { pairProperty });
}

QString CStyleProxy::className(QWidget* widget) 
{
	return widget->metaObject()->className();
}

QString CStyleProxy::setAlternateBackGroundColor(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, QColor color)
{
	return makeQssKey(widget, eSubCtrl, ePseudoStates,
		{ "alternate-background-color" , color.name() });
}

QString CStyleProxy::setBackgroundColor(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates, QColor color)
{
	return makeQssKey(widget, eSubCtrl, ePseudoStates,
		{ "background-color" , color.name() });
}

QString CStyleProxy::hideOutLine(QWidget* widget)
{
	QString strQss = makeQssKey(widget, ESubCtrl::self, EPseudoStates::none, PropertyPair("outline", "none"));

	// 如果是table控件，需要设置子项颜色才能生效
	if (widget->inherits("QTableView"))
	{
		//strQss += "\n" + makeQssKey(widget, ESubCtrl::item, EPseudoStates::none, PropertyPair("background-color", "white"));
	}
	return strQss;
}

QString CStyleProxy::setMargin(QWidget* widget, ESubCtrl eSubCtrl, QMargins margin)
{
	return makeQssKey(widget, eSubCtrl, EPseudoStates::none,
		{ { "margin", QString("%1 %2 %3 %4").arg(margin.top()).arg(margin.right()).arg(margin.bottom()).arg(margin.left()) });
}

QString CStyleProxy::setPadding(QWidget* widget, ESubCtrl eSubCtrl, QMargins padding)
{
	return makeQssKey(widget, eSubCtrl, EPseudoStates::none,
		{ "padding", QString("%1 %2 %3 %4").arg(padding.top()).arg(padding.right()).arg(padding.bottom()).arg(padding.left()) });
}

QString CStyleProxy::setUnderLine(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates)
{
	return makeQssKey(widget, eSubCtrl, ePseudoStates, PropertyPair { "text-decoration", "underline" });
}

QString CStyleProxy::setDeleteLine(QWidget* widget, ESubCtrl eSubCtrl, EPseudoStates ePseudoStates)
{
	return makeQssKey(widget, eSubCtrl, ePseudoStates, PropertyPair{ "text-decoration", "line-through" });
}

QString CStyleProxy::hideHeaderGrid(QWidget* widget)
{
	QString strClass = className(widget);
	QString strSubCtrl = querySubCtrlName(ESubCtrl::section);
	QString strContent = "border:none;"; // background - color:white; 会把背景色搞成灰色，需要搞一下
	QString strStyle = QString("%1::%2{%3}").arg(strClass).arg(strSubCtrl).arg(strContent);
	return strStyle;
}

QString CStyleProxy::queryStyleSheet(QWidget* widget, ESubCtrl subCtrl, EPseudoStates pseudoState, QString value)
{
	return "";
}

QString CStyleProxy::querySubCtrlName(ESubCtrl subCtrl)
{
	if (subCtrl == ESubCtrl::self)
	{
		return "";
	}
	QString strSubCtrl = QMetaEnum::fromType<ESubCtrl>().valueToKey((int)subCtrl);
	strSubCtrl.replace('_', '-');
	return strSubCtrl;
}

QString CStyleProxy::queryPseudoStateName(EPseudoStates pseudoState)
{
	if (pseudoState == EPseudoStates::none)
	{
		return "";
	}
	QString strPseudoState = QMetaEnum::fromType<EPseudoStates>().valueToKey((int)pseudoState);
	strPseudoState.replace('_', '-');
	return strPseudoState;
}
