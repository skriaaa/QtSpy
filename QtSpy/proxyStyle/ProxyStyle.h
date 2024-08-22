#pragma once
#include "stylemanager.h"
#include <QProxyStyle>
class CCommonProxyStyle : public QProxyStyle
{
	Q_OBJECT
public:
	CCommonProxyStyle(QStyle* style = nullptr);
	CCommonProxyStyle(const QString& key);
	~CCommonProxyStyle();
public:

	void setBackgroundColor(QColor color);
	void setBackgroundImage(QString imageUrl);
	void setBorderImage(QString imageUrl, QMargins margin);

	/*
	The image that is drawn in the contents rectangle of a subcontrol.
	The image property accepts a list of Urls or an svg.  The actual image that is drawn is determined using the same algorithm as QIcon (i.e) the image is never scaled up but always scaled down if necessary.
	If a svg is specified, the image is scaled to the size of the contents rectangle.
	Setting the image property on sub controls implicitly sets the width and height of the sub-control (unless the image in a SVG).
	In Qt 4.3 and later, the alignment of the image within the rectangle can be specified using image-position.
	This property is for subcontrols only--we don't support it for other elements.
	*/
	void setImage(QString imageUrl, Qt::Alignment align = Qt::Alignment(0));
public:
	void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
	void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
	void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = nullptr) const override;
	void drawItemText(QPainter* painter, const QRect& rect, int flags, const QPalette& pal, bool enabled,
		const QString& text, QPalette::ColorRole textRole = QPalette::NoRole) const override;
	virtual void drawItemPixmap(QPainter* painter, const QRect& rect, int alignment, const QPixmap& pixmap) const override;

	QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;

	QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override;
	QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const override;
	QRect itemTextRect(const QFontMetrics& fm, const QRect& r, int flags, bool enabled, const QString& text) const override;
	QRect itemPixmapRect(const QRect& r, int flags, const QPixmap& pixmap) const override;

	SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option, const QPoint& pos, const QWidget* widget = nullptr) const override;
	int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override;
	int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
	int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
		Qt::Orientation orientation, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;

	QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
	QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption* opt, const QWidget* widget = nullptr) const override;
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* opt) const override;
	QPalette standardPalette() const override;

	void polish(QWidget* widget) override;
	void polish(QPalette& pal) override;
	void polish(QApplication* app) override;

	void unpolish(QWidget* widget) override;
	void unpolish(QApplication* app) override;

protected:
	bool event(QEvent* e) override;
};

