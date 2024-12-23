#include "publicfunction.h"
// c++ or lib
#include <typeinfo>

// qt
#include <QPoint>
#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QLayout>
#include <QLabel>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QApplication>

QPoint convertGlobalPointToWidget(QPoint ptGlobal, QWidget* pTargetWidget)
{
	if (nullptr == pTargetWidget)
	{
		return ptGlobal;
	}

	QWidget* pWidget = pTargetWidget;
	for (; nullptr != pWidget; pWidget = pWidget->parentWidget())
	{
		QGraphicsProxyWidget* pProxyWidget = pWidget->graphicsProxyWidget();
		if (nullptr == pProxyWidget)
		{
			continue;
		}

		QGraphicsScene* pScene = pProxyWidget->scene();
		if (nullptr == pScene)
		{
			continue;
		}

		QList<QGraphicsView*> arrViews = pScene->views();
		if (arrViews.empty() || nullptr == arrViews.front())
		{
			continue;
		}

		QPoint ptScene = arrViews.front()->mapToScene(arrViews.front()->mapFromGlobal(ptGlobal)).toPoint();
		return pTargetWidget->mapFrom(pWidget, pProxyWidget->mapFromScene(ptScene).toPoint());
	}

	return pTargetWidget->mapFromGlobal(ptGlobal);
}

QPoint convertWidgetPointToGlobal(QPoint ptGlobal, QPoint ptWidget, QObject* pTarget)
{
	QWidget* pTargetWidget = dynamic_cast<QWidget*>(pTarget);
	if (nullptr == pTargetWidget)
	{
		return ptGlobal;
	}

	QWidget* pWidget = pTargetWidget;
	for (; nullptr != pWidget; pWidget = pWidget->parentWidget())
	{
		QGraphicsProxyWidget* pProxyWidget = pWidget->graphicsProxyWidget();
		if (nullptr == pProxyWidget)
		{
			continue;
		}

		QGraphicsScene* pScene = pProxyWidget->scene();
		if (nullptr == pScene)
		{
			continue;
		}

		QList<QGraphicsView*> arrViews = pScene->views();
		if (arrViews.empty() || nullptr == arrViews.front())
		{
			continue;
		}

		QPoint ptScene = pProxyWidget->mapToScene(pTargetWidget->mapTo(pWidget, ptWidget)).toPoint();
		return arrViews.front()->mapToGlobal(arrViews.front()->mapFromScene(ptScene));
	}

	return pTargetWidget->mapToGlobal(ptWidget);
}

QPoint MapToGlobal(QWidget* pWidget, QPoint pt)
{
	return convertWidgetPointToGlobal(pt, pt, pWidget);
}

QRect MapToGlobal(QWidget* pWidget, QRect rc)
{
	return QRect(MapToGlobal(pWidget, rc.topLeft()), MapToGlobal(pWidget, rc.bottomRight()));
}

QPoint MapFromGlobal(QWidget* pWidget, QPoint pt)
{
	return convertGlobalPointToWidget(pt, pWidget);
}

QRect MapFromGlobal(QWidget* pWidget, QRect rc)
{
	return QRect(MapFromGlobal(pWidget, rc.topLeft()), MapFromGlobal(pWidget, rc.bottomRight()));
}

QRect ScreenRect(QWidget* pWidget)
{
	if(nullptr == pWidget)
	{
		return QRect();
	}
	return MapToGlobal(pWidget, pWidget->rect());
}

QRect ScreenRect(QGraphicsItem* pItem)
{
	if (nullptr == pItem)
	{
		return QRect();
	}

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
		strText += " | " + object->objectName();
	}
	else if (OTo<QGraphicsItem>(object))
	{
		//strText = "graphicsItem";
		strText = QString("%1=0x%2").arg("graphicsItem").arg((quintptr)OTo<QGraphicsItem>(object),
				QT_POINTER_SIZE * 2, 16, QChar('0'));
	}
	else if(OTo<QLayout>(object))
	{
		strText += " | " + object->objectName();
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

QWidget* widgetAt(QPoint pt)
{
	auto pGraphicsViewItem = graphicsItemAt(pt);
	if(nullptr == pGraphicsViewItem)
	{
		return QApplication::widgetAt(pt);
	}

	if (To<QGraphicsProxyWidget>(pGraphicsViewItem))
	{
		auto pWidget = To<QGraphicsProxyWidget>(pGraphicsViewItem)->widget();
		if (!pWidget->children().empty())
		{
			auto pFind = pWidget->childAt(MapFromGlobal(pWidget, pt));
			if (nullptr != pFind)
			{
				pWidget = pFind;
			}
		}

		return pWidget;
	}

	return nullptr;
}

QGraphicsItem* graphicsItemAt(QPoint pt)
{
	auto pWidget = QApplication::widgetAt(pt);
	if (nullptr == pWidget)
	{
		return nullptr;
	}

	auto pView = OTo<QGraphicsView>(pWidget->parent());
	if(nullptr == pView)
	{
		return nullptr;
	}

	if(nullptr == pView->scene())
	{
		return nullptr;
	}

	return pView->scene()->itemAt(pView->mapToScene(pView->mapFromGlobal(pt)), QTransform());
}