#pragma once
#include <QTreeWidget>
#include <QGraphicsView>
class CGraphicsItemManager
{
public:
	void setTargetView(QGraphicsView* target);
	QGraphicsItem*  ItemAt(QPoint pos);
private:
	QGraphicsView* m_viewTarget{nullptr};
};
class CViewSpyTree : public QTreeWidget {
public:
	CViewSpyTree(QWidget* parent = nullptr);
public:
	void setTargetView(QGraphicsView* target);
protected:
	void addNode();
	void addSubNode(QGraphicsItem* item, QTreeWidgetItem* parentNode);
	void showContextMenu(QPoint pos);


	virtual bool eventFilter(QObject* obj, QEvent* event);

private:
	QGraphicsView* m_viewTarget{nullptr};
};