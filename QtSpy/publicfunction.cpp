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
	auto rb = pWidget->mapToGlobal(QPoint(geo.width() - 1, geo.height() - 1));
	QRect rcTarget(lt, rb);
	return rcTarget;
}

QRect ScreenRect(QGraphicsItem* pItem)
{
	auto geo = pItem->sceneBoundingRect();
	auto lt = pItem->scene()->views().front()->mapToGlobal(QPoint(0, 0));
	auto rb = pItem->scene()->views().front()->mapToGlobal(QPoint(geo.width() - 1, geo.height() - 1));
	QRect rcTarget(lt, rb);
	return rcTarget;
}

QString WidgetClass(QWidget* pWidget)
{
	return pWidget->metaObject()->className();
}

QString WidgetString(QWidget* pWidget)
{
	if (pWidget == nullptr)
		return "";
	QString strText;
	if (dynamic_cast<QDialog*>(pWidget)) {
		auto typeWidget = dynamic_cast<QDialog*>(pWidget);
		strText = typeWidget->windowTitle();
	}
	else if (dynamic_cast<QPushButton*>(pWidget)) {
		auto typeWidget = dynamic_cast<QPushButton*>(pWidget);
		strText = typeWidget->text();
	}
	else if (dynamic_cast<QLineEdit*>(pWidget)) {
		auto typeWidget = dynamic_cast<QLineEdit*>(pWidget);
		strText = typeWidget->text();
	}
	else if (dynamic_cast<QLabel*>(pWidget)) {
		auto typeWidget = dynamic_cast<QLabel*>(pWidget);
		strText = typeWidget->text();
	}
	else if (dynamic_cast<QComboBox*>(pWidget)) {
		auto typeWidget = dynamic_cast<QComboBox*>(pWidget);
		strText = typeWidget->windowTitle();
	}
	else {
		strText = "widget";
	}

	QString strWidgetInfo = QString("%1(%2)").arg(WidgetClass(pWidget)).arg(strText);
	if (!pWidget->isVisible())
	{
		strWidgetInfo += "[hide]";
	}

	return strWidgetInfo;
}

QString GraphicsItemClass(QGraphicsItem* item)
{
	if (To<QObject>(item))
	{
		return To<QObject>(item)->metaObject()->className();
	}

	return "QGraphicsItem";
}

QString GraphicsItemName(QGraphicsItem* item)
{
	if (To<QObject>(item))
	{
		return (To<QObject>(item)->objectName());
	}
	return "";
}

QString GraphicsItemString(QGraphicsItem* item)
{
	return QString("%1(%2)").arg(GraphicsItemClass(item)).arg(GraphicsItemName(item));
}
