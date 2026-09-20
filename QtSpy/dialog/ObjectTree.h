#pragma once
#include <QTreeWidget>
#include <QGraphicsItem>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QPointer>
#include <functional>

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
		property,
		styleEdit,
		layoutTree,
		objectTree,
		signalSlot,
		event,
		customDraw,
		visible,
		enable,
		move,
		firstParent
	};
	Q_ENUM(ESpyTreeMenuAction);
public:
	CWidgetSpyTree(QWidget* parent = nullptr);
	~CWidgetSpyTree() override;
public:
	bool setTreeTarget(QGraphicsItem* item);
	virtual bool setTreeTarget(QObject* target);
	bool AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode);
	bool AddSubSpyNode(QGraphicsItem* parent, QTreeWidgetItem* parentNode);
	void clearContent();
	bool setCurrentSpyItem(void* pTarget);
	bool setCurrentSpyItemAt(const QPoint& ptGlobal);
	QRect itemAreaAt(const QPoint& ptGlobal);
	int currentCount();
	bool eventFilter(QObject* obj, QEvent* event) override;
protected:
	virtual void paintEvent(QPaintEvent* event) override;
	void changeWidgetVisible(QTreeWidgetItem* pItem);
	void changeWidgetEnable(QTreeWidgetItem* pItem);
	void changeWidgetPosOrSize(QTreeWidgetItem* pItem);
	void indicatorWidget(QTreeWidgetItem* pItem);
	void showSignalSlot(QTreeWidgetItem* pItem, bool bRecusive = false);
	bool showWidgetInfo(QTreeWidgetItem* pItem);
	bool showWidgetStatus(QTreeWidgetItem* pItem);
	bool showEventTrace(QTreeWidgetItem* pItem);
	bool setUserDraw(QTreeWidgetItem* pItem);
	bool showStyleEdit(QTreeWidgetItem* pItem);
	bool spyParentWidget(QTreeWidgetItem* pItem);
	bool spyFirstParentWidget(QTreeWidgetItem* pItem);
	void showLayout(QTreeWidgetItem* pItem);
	void showObjectTree(QTreeWidgetItem* pItem);

	QRect itemArea(QTreeWidgetItem* pItem);
	QTreeWidgetItem* spyItemAt(const QPoint& ptGlobal);
	void selectSpyItem(QTreeWidgetItem* pItem);
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

// 屏幕拾取定位: 十字光标在屏幕上点选控件/图元, 选中其在树中的节点(右键取消)。
// 原为 ObjectTree.cpp 内部实现, 查找窗口合并"名称/鼠标定位"后移到此处共用。
class CTreeCursorSearchFilter : public QObject
{
public:
	CTreeCursorSearchFilter(QWidget* pHostWidget, CWidgetSpyTree* pTree);
	~CTreeCursorSearchFilter() override;

	void start();
	// 拾取成功(左键点中)后的回调, 查找窗口用它实现"拾取完成即关闭"
	void setPickedCallback(std::function<void()> callback);
protected:
	bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
private:
	void finish();
private:
	QPointer<QWidget> m_pHostWidget;
	QPointer<CWidgetSpyTree> m_pTree;
	std::function<void()> m_fnPicked;
	bool m_bRunning = false;
};

class CObjectTree : public CWidgetSpyTree
{
public:
	CObjectTree(QWidget* parent = nullptr);
	virtual bool setTreeTarget(QObject* target) override;
	bool AddSubSpyNode(QObject* parent, QTreeWidgetItem* parentNode);
};
