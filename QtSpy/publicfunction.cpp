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
		//strText = "graphicsItem";
		strText = QString("%1=0x%2").arg("graphicsItem").arg((quintptr)OTo<QGraphicsItem>(object),
				QT_POINTER_SIZE * 2, 16, QChar('0'));
	}

	QString strItemInfo = QString("%1(%2)").arg(objectClass(object)).arg(strText);
	if (OTo<QWidget>(object) && !OTo<QWidget>(object)->isVisible()
		|| OTo<QGraphicsItem>(object) && !OTo<QGraphicsItem>(object)->isVisible())
	{
		strItemInfo += "[hide]";
	}

	return strItemInfo;
}

class CQssAnalyze
{
	struct QssInfo {
		QString strName;
		QString strInfo;
	};
public:
	QString QueryTargetQss(QWidget* widget)
	{
		QString strStyle;
		do
		{
			strStyle += widget->styleSheet();
			widget = OTo<QWidget>(widget->parent());
		} while (widget);
		
		Analyze(strStyle);
		QString strRet;
		for (auto info : m_arrInfo)
		{
			if (info.strName.indexOf(widget->metaObject()->className(), 0, Qt::CaseInsensitive) != -1 
				|| info.strName.isEmpty())
			{
				strRet += info.strName + info.strInfo + "\n";
			}
		}
		return strRet;
	}
private:
	void Analyze(QString strStyleSheet, int offset = 0)
	{
		QssInfo info;
		QString strSplitLeft = "{";
		QString strSplitRight = "}";
		int left = strStyleSheet.indexOf(strSplitLeft, offset);
		if (left == -1)
			return;
		info.strName = strStyleSheet.mid(offset, left - offset);	

		int right = strStyleSheet.indexOf(strSplitRight, left);
		if (right == -1)
			return;
		info.strInfo = strStyleSheet.mid(left, right - left + 1);
		
		m_arrInfo.push_back(info);

		Analyze(strStyleSheet, right + 1);
	}

	QVector<QssInfo> m_arrInfo;
};

QString styleSheet(QWidget* widget)
{
	CQssAnalyze analyze;
	return analyze.QueryTargetQss(widget);
}
