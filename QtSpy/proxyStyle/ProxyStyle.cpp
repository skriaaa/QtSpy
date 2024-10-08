#include "ProxyStyle.h"
#include <QStyleOption>
CCommonProxyStyle::CCommonProxyStyle(QStyle* style /*= nullptr*/)
{

}

CCommonProxyStyle::CCommonProxyStyle(const QString& key)
{

}

CCommonProxyStyle::~CCommonProxyStyle()
{

}

void CCommonProxyStyle::setBackgroundColor(QColor color)
{
}

void CCommonProxyStyle::setBackgroundImage(QString imageUrl)
{
}

void CCommonProxyStyle::setBorderImage(QString imageUrl, QMargins margin)
{
}

void CCommonProxyStyle::setImage(QString imageUrl, Qt::Alignment align /*= Qt::Alignment(0)*/)
{
}

void CCommonProxyStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget /*= nullptr*/) const
{
	QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void CCommonProxyStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget /*= nullptr*/) const
{
	if(option->state.testFlag(QStyle::State_MouseOver))
	{
		int a = 0;
	}
	QProxyStyle::drawControl(element, option, painter, widget);
}

void CCommonProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget /*= nullptr*/) const
{
	QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void CCommonProxyStyle::drawItemText(QPainter* painter, const QRect& rect, int flags, const QPalette& pal, bool enabled, const QString& text, QPalette::ColorRole textRole /*= QPalette::NoRole*/) const
{
	QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
}

void CCommonProxyStyle::drawItemPixmap(QPainter* painter, const QRect& rect, int alignment, const QPixmap& pixmap) const
{
	QProxyStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}

QSize CCommonProxyStyle::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const
{
	return QProxyStyle::sizeFromContents(type, option, size, widget);
}

QRect CCommonProxyStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
	return QProxyStyle::subElementRect(element, option, widget);
}

QRect CCommonProxyStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const
{
	return QProxyStyle::subControlRect(cc, opt, sc, widget);
}

QRect CCommonProxyStyle::itemTextRect(const QFontMetrics& fm, const QRect& r, int flags, bool enabled, const QString& text) const
{
	return QProxyStyle::itemTextRect(fm, r, flags, enabled, text);
}

QRect CCommonProxyStyle::itemPixmapRect(const QRect& r, int flags, const QPixmap& pixmap) const
{
	return QProxyStyle::itemPixmapRect(r, flags, pixmap);
}

QStyle::SubControl CCommonProxyStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option, const QPoint& pos, const QWidget* widget /*= nullptr*/) const
{
	return QProxyStyle::hitTestComplexControl(control, option, pos, widget);
}

int CCommonProxyStyle::styleHint(StyleHint hint, const QStyleOption* option /*= nullptr*/, const QWidget* widget /*= nullptr*/, QStyleHintReturn* returnData /*= nullptr*/) const
{
	return QProxyStyle::styleHint(hint, option, widget, returnData);
}

int CCommonProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option /*= nullptr*/, const QWidget* widget /*= nullptr*/) const
{
	return QProxyStyle::pixelMetric(metric, option, widget);
}

int CCommonProxyStyle::layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2, Qt::Orientation orientation, const QStyleOption* option /*= nullptr*/, const QWidget* widget /*= nullptr*/) const
{
	return QProxyStyle::layoutSpacing(control1, control2, orientation, option, widget);
}

QIcon CCommonProxyStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption* option /*= nullptr*/, const QWidget* widget /*= nullptr*/) const
{
	return QProxyStyle::standardIcon(standardIcon, option, widget);
}

QPixmap CCommonProxyStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption* opt, const QWidget* widget /*= nullptr*/) const
{
	return QProxyStyle::standardPixmap(standardPixmap, opt, widget);
}

QPixmap CCommonProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* opt) const
{
	return QProxyStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

QPalette CCommonProxyStyle::standardPalette() const
{
	return QProxyStyle::standardPalette();
}

void CCommonProxyStyle::polish(QWidget* widget)
{
	QProxyStyle::polish(widget);
}

void CCommonProxyStyle::polish(QPalette& pal)
{
	QProxyStyle::polish(pal);
}

void CCommonProxyStyle::polish(QApplication* app)
{
	QProxyStyle::polish(app);
}

void CCommonProxyStyle::unpolish(QWidget* widget)
{
	QProxyStyle::unpolish(widget);
}

void CCommonProxyStyle::unpolish(QApplication* app)
{
	QProxyStyle::unpolish(app);
}

bool CCommonProxyStyle::event(QEvent* e)
{
	return QProxyStyle::event(e);
}