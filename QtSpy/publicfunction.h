#pragma once

class QPoint;
class QWidget;
class QDialog;
class QRect;
class QString;
class QGraphicsItem;
class QObject;
QPoint MapToGlobal(QWidget* pWidget, QPoint pt);

QPoint MapFromGlobal(QWidget* pWidget, QPoint pt);

QRect MapToGlobal(QWidget* pWidget, QRect rc);

QRect MapFromGlobal(QWidget* pWidget, QRect rc);

QRect ScreenRect(QWidget* pWidget);
QRect ScreenRect(QGraphicsItem* pItem);

QString WidgetClass(QWidget* pWidget);

QString WidgetString(QWidget* pWidget);

QString GraphicsItemClass(QGraphicsItem* item);

QString GraphicsItemName(QGraphicsItem* item);

QString GraphicsItemString(QGraphicsItem* item);

template< class T>
T* To(QGraphicsItem* p) {
	return dynamic_cast<T*>(p);
}

template< class T>
T* OTo(QObject* p) {
	return dynamic_cast<T*>(p);
}

