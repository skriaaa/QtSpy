#pragma once
#include <QTreeWidget>
#include <QGraphicsItem>
#include <QStyledItemDelegate>
#include <QMenu>

// 自定义委托，根据全局行号改变文字颜色
class CTreeWidgetDelegate : public QStyledItemDelegate
{
public:
	explicit CTreeWidgetDelegate(QObject* parent = nullptr)
		: QStyledItemDelegate(parent) {
	}
	
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

class CTreeWidgetItem :  public QObject, public QTreeWidgetItem
{
	Q_OBJECT
public:
	CTreeWidgetItem(QTreeWidget* parent = nullptr)
		: QTreeWidgetItem(parent), QObject(parent) {
	}
};


class CEventTraceWnd;
class CWidgetSpyTree : public QTreeWidget {
	Q_OBJECT
public:
	enum ESpyTreeMenuAction
	{
		spyParent = 0,
		baseInfo,
		property,
		styleEdit,
		locate,
		layoutTree,
		objectTree,
		signalSlot,
		event,
		eventAll,
		customDraw,
		visible,
		enable,
		move,
		firstParent
	};
	Q_ENUM(ESpyTreeMenuAction);
public:
	CWidgetSpyTree(QWidget* parent = nullptr);
public:
	bool setTreeTarget(QGraphicsItem* item);
	virtual bool setTreeTarget(QObject* target);
	bool AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode);
	bool AddSubSpyNode(QGraphicsItem* parent, QTreeWidgetItem* parentNode);
	void clearContent();
	bool setCurrentSpyItem(void* pTarget);
	int currentCount();
	bool eventFilter(QObject* obj, QEvent* event) override;
protected:
	void changeWidgetVisible(QTreeWidgetItem* pItem);
	void changeWidgetEnable(QTreeWidgetItem* pItem);
	void changeWidgetPosOrSize(QTreeWidgetItem* pItem);
	void indicatorWidget(QTreeWidgetItem* pItem);
	void showSignalSlot(QTreeWidgetItem* pItem, bool bRecusive = false);
	bool showWidgetInfo(QTreeWidgetItem* pItem);
	bool showWidgetStatus(QTreeWidgetItem* pItem);
	bool showEventTrace(QTreeWidgetItem* pItem);
	bool showEventTraceAll(QTreeWidgetItem* pItem);
	bool setUserDraw(QTreeWidgetItem* pItem);
	bool showStyleEdit(QTreeWidgetItem* pItem);
	bool spyParentWidget(QTreeWidgetItem* pItem);
	bool spyFirstParentWidget(QTreeWidgetItem* pItem);
	void showLayout(QTreeWidgetItem* pItem);
	void showObjectTree(QTreeWidgetItem* pItem);

	QRect itemArea(QTreeWidgetItem* pItem);
	template<class T> T* itemData(QTreeWidgetItem* item);
	QGraphicsItem* graphicsData(QTreeWidgetItem* item);
	QWidget* widgetData(QTreeWidgetItem* item);
	void removeTargetNode();

	void showContextMenu(const QPoint& pos);
	QAction* addAction(QMenu& menu, ESpyTreeMenuAction eAction);
	void onMenuClicked(QAction* pAction, QTreeWidgetItem* pItem);
protected:
	QHash<void*, QPointer<CTreeWidgetItem>> m_mapWidgetNode;
};


class CLayoutTree : public CWidgetSpyTree
{
public:
	CLayoutTree(QWidget* parent = nullptr);
	virtual bool setTreeTarget(QObject* target) override;
	bool AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode);
	bool AddSubSpyNode(QLayout* parent, QTreeWidgetItem* parentNode, QWidget* pWidget = nullptr);
};

class CObjectTree : public CWidgetSpyTree
{
public:
	CObjectTree(QWidget* parent = nullptr);
	virtual bool setTreeTarget(QObject* target) override;
	bool AddSubSpyNode(QObject* parent, QTreeWidgetItem* parentNode);
};