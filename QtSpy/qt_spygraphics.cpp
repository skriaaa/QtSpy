#include "qt_spygraphics.h"
#include <QEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QGraphicsItem>
#include "publicfunction.h"

CViewSpyTree::CViewSpyTree(QWidget* parent /*= nullptr*/):QTreeWidget(parent)
{
	installEventFilter(this);
	setHeaderHidden(true);
}

void CViewSpyTree::setTargetView(QGraphicsView* target)
{
	m_viewTarget = target;
	addNode();
}

void CViewSpyTree::addNode()
{
	QTreeWidgetItem* root = new QTreeWidgetItem();
	addTopLevelItem(root);
	root->setData(0, Qt::UserRole, QVariant::fromValue(m_viewTarget->scene()));
	root->setText(0, "Scene");
	for (auto child : m_viewTarget->scene()->children())
	{
		if (child->inherits("QGraphicsItem"))
		{
			addSubNode(dynamic_cast<QGraphicsItem*>(child), root);
		}
	}
}

void CViewSpyTree::addSubNode(QGraphicsItem* item, QTreeWidgetItem* parentNode)
{
	QTreeWidgetItem* node = new QTreeWidgetItem;
	node->setText(0, To<QObject>(item) ? To<QObject>(item)->objectName() : "graphicsItem");
	node->setData(0, Qt::UserRole, QVariant::fromValue(item));
	parentNode->addChild(node);

	for (auto child : item->childItems())
	{
		addSubNode(child, node);
	}
}

void CViewSpyTree::showContextMenu(QPoint pos)
{
	QMenu contextMenu(this);
	QAction* actionLocate = contextMenu.addAction("目标定位");
	QAction* actionInfo = contextMenu.addAction("基础信息");
	QAction* actionSignal = contextMenu.addAction("信号/槽");
	QAction* actionEventTrace = contextMenu.addAction("事件跟踪");
	QAction* actionStyleEdit = contextMenu.addAction("风格编辑");
	QAction* actionUserDrawParam = contextMenu.addAction("自绘参数");
	QAction* actionWidgetStatus = contextMenu.addAction("组件状态");
	QAction* actionResourceHub = contextMenu.addAction("资源仓库");
	QAction* actionChangeVisible = contextMenu.addAction("显示|隐藏");
	QAction* actionChangeEnable = contextMenu.addAction("启用|禁用");
	QAction* actionMoveOrScale = contextMenu.addAction("移动|缩放");
	QAction* actionSpyParent = contextMenu.addAction("父组件");
	QAction* actionSpyFirstParent = contextMenu.addAction("第一父组件");
}

bool CViewSpyTree::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == this && event->type() == QEvent::ContextMenu) {
		QContextMenuEvent* contextMenuEvent = dynamic_cast<QContextMenuEvent*>(event);
		showContextMenu(contextMenuEvent->globalPos());
		return true;
	}
	if (obj == this && event->type() == QEvent::MouseButtonDblClick) {
		QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
		//IndicatorWidget(mouseEvent->globalPos());
	}
}

void CGraphicsItemManager::setTargetView(QGraphicsView* target)
{
	m_viewTarget = target;
}

QGraphicsItem* CGraphicsItemManager::ItemAt(QPoint pos)
{
	return m_viewTarget->scene()->itemAt(pos, QTransform());
}
