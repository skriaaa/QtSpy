#pragma once
#include <QTreeWidget>
#include <QGraphicsView>
class CViewSpyTree : public QTreeWidget {
public:
	CViewSpyTree(QWidget* parent = nullptr);
public:
	void setTargetView(QGraphicsView* target);
protected:
	bool eventFilter();
private:
	QGraphicsView* m_viewTarget;
};