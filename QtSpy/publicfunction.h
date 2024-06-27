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

QString objectClass(QObject* object);

QString objectName(QObject* object);

QString ObjectString(QObject* pWidget);

QString styleSheet(QWidget* widget);

template< class T>
T* To(QGraphicsItem* p) {
	return dynamic_cast<T*>(p);
}

template< class T>
T* OTo(QObject* p) {
	return dynamic_cast<T*>(p);
}