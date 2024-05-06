#include "publicfunction.h"
#include <QPoint>
#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
QPoint MapToGlobal(QWidget* pWidget, QPoint pt)
{
	return pWidget->mapToGlobal(pt);
}

QRect MapToGlobal(QWidget* pWidget, QRect rc)
{
	return QRect(pWidget->mapToGlobal(rc.topLeft()), pWidget->mapToGlobal(rc.bottomRight()));
}

QPoint MapFromGlobal(QWidget* pWidget, QPoint pt)
{
	return pWidget->mapFromGlobal(pt);
}

QRect MapFromGlobal(QWidget* pWidget, QRect rc)
{
	return QRect(pWidget->mapFromGlobal(rc.topLeft()), pWidget->mapFromGlobal(rc.bottomRight()));
}

QRect ScreenRect(QWidget* pWidget)
{
	auto geo = pWidget->geometry();
	auto lt = pWidget->mapToGlobal(QPoint(0, 0));
	return QRect(lt, QSize(geo.width(), geo.height()));
}

QRect ScreenRect(QGraphicsItem* pItem)
{
	auto geo = pItem->sceneBoundingRect();
	auto lt = pItem->scene()->views().front()->mapToGlobal(QPoint(geo.left(), geo.top()));
	return QRect(lt, QSize(geo.width(), geo.height()));
}

QString objectClass(QObject* object)
{
	if (object)
	{
		return object->metaObject()->className();
	}
	return QString();
}
QString objectName(QObject* object)
{
	if (object)
	{
		return object->objectName();
	}
	return QString();
}

QString ObjectString(QObject* object)
{
	if (object == nullptr)
		return "";

	QString strText;
	if (OTo<QWidget>(object))
	{
		if (dynamic_cast<QDialog*>(object)) {
			auto typeWidget = dynamic_cast<QDialog*>(object);
			strText = typeWidget->windowTitle();
		}
		else if (dynamic_cast<QPushButton*>(object)) {
			auto typeWidget = dynamic_cast<QPushButton*>(object);
			strText = typeWidget->text();
		}
		else if (dynamic_cast<QLineEdit*>(object)) {
			auto typeWidget = dynamic_cast<QLineEdit*>(object);
			strText = typeWidget->text();
		}
		else if (dynamic_cast<QLabel*>(object)) {
			auto typeWidget = dynamic_cast<QLabel*>(object);
			strText = typeWidget->text();
		}
		else if (dynamic_cast<QComboBox*>(object)) {
			auto typeWidget = dynamic_cast<QComboBox*>(object);
			strText = typeWidget->windowTitle();
		}
		else {
			strText = "widget";
		}
	}
	else if (OTo<QGraphicsItem>(object))
	{
		strText = "graphicsItem";
	}

	QString strItemInfo = QString("%1(%2)").arg(objectClass(object)).arg(strText);
	if (OTo<QWidget>(object) && !OTo<QWidget>(object)->isVisible()
		|| OTo<QGraphicsItem>(object) && !OTo<QGraphicsItem>(object)->isVisible())
	{
		strItemInfo += "[hide]";
	}

	return strItemInfo;
}
