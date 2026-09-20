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
#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QApplication>
#include <QStringList>

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

QRect ScreenRect(QWidget* pWidget, QRect rc)
{
	if (nullptr == pWidget)
	{
		return QRect();
	}
	return MapToGlobal(pWidget, rc.isEmpty() ? pWidget->rect() : rc);
}

QRect ScreenRect(QGraphicsItem* pItem)
{
	if (nullptr == pItem)
	{
		return QRect();
	}

	auto geo = pItem->sceneBoundingRect();
	auto view = pItem->scene()->views().front();
	auto lt = view->mapToGlobal(view->mapFromScene(QPoint(geo.left(), geo.top())));
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

QString pointerToHex(const void* pointer) {
	return "0x" + QString::number((uintptr_t)pointer, 16);
}

QString objectString(QObject* object)
{
	if (object == nullptr)
		return "";

	QString strText;
	if (OTo<QWidget>(object))
	{
		if (auto pWidget = OTo<QDialog>(object)) 
		{
			strText = pWidget->windowTitle();
		}
		else if (auto pWidget = OTo<QAbstractButton>(object))
		{
			strText = pWidget->text();
		}
		else if (auto pWidget = OTo<QLineEdit>(object))
		{
			strText = pWidget->text();
		}
		else if (auto pWidget = OTo<QLabel>(object))
		{
			strText = pWidget->text();
		}
		else if (auto pWidget = OTo<QComboBox>(object)) 
		{
			strText = pWidget->currentText();
		}
		else if (auto pWidget = OTo<QCheckBox>(object))
		{
			strText = pWidget->text();
		}
	}

	/* 树节点文本组装: 括号仅在有内容(文字/objectName 任一)时展示;
	   '|' 仅在两侧都有值时展示; 对象地址不再展示 */
	QStringList arrParts;
	if (!strText.isEmpty())
	{
		arrParts.append(strText);
	}
	const QString strObjectName = object->objectName();
	if (!strObjectName.isEmpty())
	{
		arrParts.append(strObjectName);
	}
	QString strItemInfo = objectClass(object);
	if (!arrParts.isEmpty())
	{
		strItemInfo += QString("(%1)").arg(arrParts.join(" | "));
	}
	if (OTo<QWidget>(object) && !OTo<QWidget>(object)->isVisible())
	{
		strItemInfo += "[hide]";
	}

	if (OTo<QWidget>(object) && !OTo<QWidget>(object)->isEnabled())
	{
		strItemInfo += "[disabled]";
	}

	return strItemInfo;
}

QString objectString(QGraphicsItem* pItem)
{
	if (nullptr == pItem)
	{
		return "";
	}

	// 非 QObject 图元取不到类名, 用 graphicsItem 占位; 地址不展示
	QString strClass = objectClass(To<QObject>(pItem));
	if (strClass.isEmpty())
	{
		strClass = "graphicsItem";
	}
	QString strItemInfo = strClass;
	const QString strObjectName = objectName(To<QObject>(pItem));
	if (!strObjectName.isEmpty())
	{
		strItemInfo += QString("(%1)").arg(strObjectName);
	}
	if(!pItem->isVisible())
	{
		strItemInfo += "[hide]";
	}
	if(!pItem->isEnabled())
	{
		strItemInfo += "[disabled]";
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

