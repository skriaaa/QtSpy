#pragma once
#include <QTreeWidget>
#include <QGraphicsItem>
class CEventTraceWnd;
class CWidgetSpyTree : public QTreeWidget {
public:
	CWidgetSpyTree(QWidget* parent = nullptr);

	bool setTreeTarget(QGraphicsItem* item);
	virtual bool setTreeTarget(QObject* target);
	bool AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode);
	bool AddSubSpyNode(QGraphicsItem* parent, QTreeWidgetItem* parentNode);

	bool eventFilter(QObject* obj, QEvent* event) override;

	void showContextMenu(const QPoint& pos);

	void ChangeWidgetVisible(QPoint ptGlobal);

	void ChangeWidgetEnable(QPoint ptGlobal);

	void ChangeWidgetPosOrSize(QPoint ptGlobal);

	void IndicatorWidget(QPoint ptGlobal);

	void ShowSignalSlot(QPoint ptGlobal, bool bRecusive = false);

	bool ShowWidgetInfo(const QPoint& pos);

	bool ShowWidgetStatus(const QPoint& pos);

	bool ShowWidgetResourceHub(const QPoint& pos);

	bool ShowEventTrace(const QPoint& pos);

	bool ShowEventTraceAll(const QPoint& pos);

	bool SetUserDraw(const QPoint& pos);

	bool ShowStyleEdit(const QPoint& pos);

	bool SpyParentWidget(const QPoint& pos);

	bool SpyFirstParentWidget(const QPoint& pos);

	void showLayout(const QPoint& pos);

	void showObjectTree(const QPoint& pos);

	QRect itemArea(QTreeWidgetItem* pItem);

	template<class T> T* itemData(QTreeWidgetItem* item);
	QGraphicsItem* graphicsData(QTreeWidgetItem* item);
	QWidget* widgetData(QTreeWidgetItem* item);
	QHash<QWidget*, QTreeWidgetItem*> m_mapWidgetNode;
};


class CLayoutTree : public CWidgetSpyTree
{
public:
	CLayoutTree(QWidget* parent = nullptr);
	virtual bool setTreeTarget(QObject* target) override;
	bool AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode);
	bool AddSubSpyNode(QLayout* parent, QTreeWidgetItem* parentNode);
};

class CObjectTree : public CWidgetSpyTree
{
public:
	CObjectTree(QWidget* parent = nullptr);
	virtual bool setTreeTarget(QObject* target) override;
	bool AddSubSpyNode(QObject* parent, QTreeWidgetItem* parentNode);
};