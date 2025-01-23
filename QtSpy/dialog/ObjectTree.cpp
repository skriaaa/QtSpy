#include "ObjectTree.h"

#include <QMenu>
#include <QEvent>
#include <QLayout>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QContextMenuEvent>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>

#include "qt_spydlg.h"
#include "StyleEditDlg.h"
#include "publicfunction.h"
#include "proxyStyle/ProxyStyle.h"
#include "qt_spyobject.h"


CWidgetSpyTree::CWidgetSpyTree(QWidget* parent /*= nullptr*/) : QTreeWidget(parent)
{
	installEventFilter(this);
	setHeaderHidden(true);

	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);

	connect(this, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem* pCurrentItem) {
		CSpyIndicatorWnd::showWnd(itemArea(pCurrentItem), false);
	});
}

bool CWidgetSpyTree::setTreeTarget(QGraphicsItem* target)
{
	m_mapWidgetNode.clear();
	clear();

	QTreeWidgetItem* root = new QTreeWidgetItem;
	addTopLevelItem(root);
	AddSubSpyNode(target, root);

	return true;
}

bool CWidgetSpyTree::setTreeTarget(QObject* target)
{
	m_mapWidgetNode.clear();
	clear();
	if(OTo<QWidget>(target))
	{
		QTreeWidgetItem* root = new QTreeWidgetItem;
		addTopLevelItem(root);
		AddSubSpyNode(OTo<QWidget>(target), root);
	}
	return true;
}

bool CWidgetSpyTree::AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode) {
	if (parent && parentNode) {
		m_mapWidgetNode[parent] = parentNode;
		parentNode->setText(0, objectString(parent));
		parentNode->setData(0, Qt::UserRole, QVariant::fromValue(parent));
		QList<QWidget*> children = parent->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
		for (QWidget* child : children) {
			QTreeWidgetItem* treenode = new QTreeWidgetItem();
			parentNode->addChild(treenode);
			AddSubSpyNode(child, treenode);
		}
		if (OTo<QGraphicsView>(parent))
		{
			auto arrItems = OTo<QGraphicsView>(parent)->items();
			for (QGraphicsItem* item : arrItems)
			{
				if (item->parentItem() == nullptr)
				{
					QTreeWidgetItem* treenode = new QTreeWidgetItem();
					parentNode->addChild(treenode);
					AddSubSpyNode(item, treenode);
				}
			}
		}
	}
	return true;
};


bool CWidgetSpyTree::AddSubSpyNode(QGraphicsItem* parent, QTreeWidgetItem* parentNode)
{
	if (parent && parentNode) {
		m_mapWidgetNode[parent] = parentNode;
		parentNode->setText(0, objectString(parent));
		parentNode->setData(0, Qt::UserRole, QVariant::fromValue(parent));
		QList<QGraphicsItem*> children = parent->childItems();
		for (QGraphicsItem* child : children) {
			QTreeWidgetItem* treenode = new QTreeWidgetItem();
			parentNode->addChild(treenode);
			AddSubSpyNode(child, treenode);
		}

		if(To<QGraphicsProxyWidget>(parent))
		{
			QWidget* widget = To<QGraphicsProxyWidget>(parent)->widget();
			QTreeWidgetItem* treenode = new QTreeWidgetItem();
			parentNode->addChild(treenode);
			AddSubSpyNode(widget, treenode);
		}
	}
	return true;
}

bool CWidgetSpyTree::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == this && event->type() == QEvent::ContextMenu) {
		QContextMenuEvent* contextMenuEvent = dynamic_cast<QContextMenuEvent*>(event);
		showContextMenu(contextMenuEvent->globalPos());
		return true;
	}
	if (obj == this && event->type() == QEvent::MouseButtonDblClick) {
		QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
		IndicatorWidget(mouseEvent->globalPos());
	}
	return false;
}

void CWidgetSpyTree::showContextMenu(const QPoint& pos)
{
	QMenu contextMenu(this);
	QAction* actionSpyParent = contextMenu.addAction("父组件");
	QAction* actionLocate = contextMenu.addAction("目标定位");
	QAction* actionInfo = contextMenu.addAction("基础信息");
	QAction* actionLayout = contextMenu.addAction("布局信息");
	QAction* actionObject = contextMenu.addAction("对象树");
	QAction* actionSignal = contextMenu.addAction("信号/槽");
	QAction* actionEventTrace = contextMenu.addAction("事件跟踪");
	QAction* actionEventTraceAll = contextMenu.addAction("事件跟踪All");
	QAction* actionStyleEdit = contextMenu.addAction("风格编辑");
	QAction* actionUserDrawParam = contextMenu.addAction("自绘控件");
	QAction* actionWidgetStatus = contextMenu.addAction("组件状态");
	QAction* actionChangeVisible = contextMenu.addAction("显示|隐藏");
	QAction* actionChangeEnable = contextMenu.addAction("启用|禁用");
	QAction* actionMoveOrScale = contextMenu.addAction("移动|缩放");
	QAction* actionSpyFirstParent = contextMenu.addAction("第一父组件");
	QAction* selectedAction = contextMenu.exec(pos);
	if (selectedAction == actionLocate) {
		IndicatorWidget(pos);
	}
	else if (selectedAction == actionInfo) {
		ShowWidgetInfo(pos);
	}
	else if (selectedAction == actionLayout) {
		showLayout(pos);
	}
	else if (selectedAction == actionObject) {
		showObjectTree(pos);
	}
	else if (selectedAction == actionSignal) {
		ShowSignalSlot(pos, true);
	}
	else if (selectedAction == actionEventTrace) {
		ShowEventTrace(pos);
	}
	else if(selectedAction == actionEventTraceAll){
		ShowEventTraceAll(pos);
	}
	else if (selectedAction == actionStyleEdit) {
		ShowStyleEdit(pos);
	}
	else if (selectedAction == actionUserDrawParam) {
		SetUserDraw(pos);
	}
	else if (selectedAction == actionWidgetStatus) {
		ShowWidgetStatus(pos);
	}
	else if (selectedAction == actionSpyParent) {
		SpyParentWidget(pos);
	}
	else if (selectedAction == actionSpyFirstParent) {
		SpyFirstParentWidget(pos);
	}
	else if (selectedAction == actionChangeVisible) {
		ChangeWidgetVisible(pos);
	}
	else if (selectedAction == actionChangeEnable) {
		ChangeWidgetEnable(pos);
	}
	else if (selectedAction == actionMoveOrScale) {
		ChangeWidgetPosOrSize(pos);
	}
}

void CWidgetSpyTree::ChangeWidgetVisible(QPoint ptGlobal)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {
		if (QWidget* pTargetWidget = widgetData(clickedItem))
		{
			pTargetWidget->setVisible(!pTargetWidget->isVisible());
			clickedItem->setText(0, objectString(pTargetWidget));
		}
		else if (QGraphicsItem* pTargetItem = graphicsData(clickedItem))
		{
			pTargetItem->setVisible(!pTargetItem->isVisible());
			clickedItem->setText(0, objectString(pTargetItem));
		}
	}
}

void CWidgetSpyTree::ChangeWidgetEnable(QPoint ptGlobal)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {
		if (QWidget* pTargetWidget = widgetData(clickedItem))
		{
			pTargetWidget->setEnabled(!pTargetWidget->isEnabled());
			clickedItem->setText(0, objectString(pTargetWidget));
		}
		else if (QGraphicsItem* pTargetItem = graphicsData(clickedItem))
		{
			pTargetItem->setEnabled(!pTargetItem->isEnabled());
			clickedItem->setText(0, objectString(pTargetItem));
		}
	}
}

void CWidgetSpyTree::ChangeWidgetPosOrSize(QPoint ptGlobal)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {
		if (QWidget* pTargetWidget = widgetData(clickedItem)) {
			CMoveOrScaleWidgetWnd* pWindow = new CMoveOrScaleWidgetWnd(window());
			pWindow->ControlTarget(pTargetWidget);
			pWindow->ShowOnTop();
		}
		else if (QGraphicsItem* pTargetItem = graphicsData(clickedItem)) {
			CMoveOrScaleWidgetWnd* pWindow = new CMoveOrScaleWidgetWnd();
			pWindow->GraphicsTarget(pTargetItem);
			pWindow->ShowOnTop();
		}
	}
}

void CWidgetSpyTree::IndicatorWidget(QPoint ptGlobal)
{
	CSpyIndicatorWnd::showWnd(itemArea(itemAt(mapFromGlobal(ptGlobal))),false);
}

void CWidgetSpyTree::ShowSignalSlot(QPoint ptGlobal, bool bRecusive /*= false*/)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {
		CSignalSpyWnd* pSpy = new CSignalSpyWnd(window());
		if (QWidget* pTargetWidget = widgetData(clickedItem))
		{
			pSpy->setTargetObject(pTargetWidget);
		}
		else if(QGraphicsItem* pItem = graphicsData(clickedItem))
		{
			pSpy->setTargetObject(To<QObject>(pItem));
		}
		pSpy->ShowOnTop();
	}
}

bool CWidgetSpyTree::ShowWidgetInfo(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		if (QWidget* pTargetWidget = widgetData(clickedItem)) {
			auto geo = pTargetWidget->geometry();
			auto geoScreen = ScreenRect(pTargetWidget);
			auto client = pTargetWidget->rect();
			int nUniqueId = -1;
			if (dynamic_cast<QWidget*>(pTargetWidget)) {
				nUniqueId = dynamic_cast<QWidget*>(pTargetWidget)->winId();
			}
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle(objectString(pTargetWidget));
			pInfo->AddAttribute(_QStr("class name"), objectClass(pTargetWidget));
			pInfo->AddAttribute(_QStr("object name"), pTargetWidget->objectName());
			pInfo->AddAttribute(_QStr("geometry"), QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			pInfo->AddAttribute(_QStr("screen geometry"), QString("(%1,%2,%3,%4)").arg(geoScreen.left()).arg(geoScreen.top()).arg(geoScreen.right()).arg(geoScreen.bottom()));
			pInfo->AddAttribute(_QStr("geometry size"), QString("(%1,%2)").arg(geoScreen.width()).arg(geoScreen.height()));
			pInfo->AddAttribute(_QStr("client"), QString("(%1,%2,%3,%4)").arg(client.left()).arg(client.top()).arg(client.right()).arg(client.bottom()));
			pInfo->AddAttribute(_QStr("visible"), pTargetWidget->isVisible() ? _QStr("yes") : _QStr("no"));
			pInfo->AddAttribute(_QStr("enabled"), pTargetWidget->isEnabled() ? _QStr("yes") : _QStr("no"));
			pInfo->AddAttribute(_QStr("object unique id"), QString("%1").arg(nUniqueId));
			pInfo->AddAttribute(_QStr("font"), pTargetWidget->font().toString());
			pInfo->AddAttribute(_QStr("stylesheet"), pTargetWidget->styleSheet());
			QMetaEnum metaEnum = QMetaEnum::fromType<QSizePolicy::Policy>();
			pInfo->AddAttribute("maxsize", QString("(%1,%2)").arg(pTargetWidget->maximumWidth()).arg(pTargetWidget->maximumHeight()));
			pInfo->AddAttribute("minsize", QString("(%1,%2)").arg(pTargetWidget->minimumWidth()).arg(pTargetWidget->minimumHeight()));
			pInfo->AddAttribute("sizePolicy", QString("%1 | %2").arg(metaEnum.valueToKey(pTargetWidget->sizePolicy().horizontalPolicy())).arg(metaEnum.valueToKey(pTargetWidget->sizePolicy().verticalPolicy())));
			pInfo->AddAttribute("stretch", QString("(%1,%2)").arg(pTargetWidget->sizePolicy().horizontalStretch()).arg(pTargetWidget->sizePolicy().verticalStretch()));
			pInfo->AddAttribute(_QStr("winid"), QString("%1").arg(pTargetWidget->winId()));
			pInfo->ShowOnTop();
		}
		else if (QGraphicsItem* pItem = graphicsData(clickedItem))
		{
			auto geo = pItem->sceneBoundingRect();
			auto geoScreen = ScreenRect(pItem);
			auto client = pItem->boundingRect();
			int nUniqueId = -1;
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle(objectString(pItem));
			pInfo->AddAttribute(_QStr("class name"), objectClass(To<QObject>(pItem)));
			pInfo->AddAttribute(_QStr("object name"), ::objectName(To<QObject>(pItem)));
			pInfo->AddAttribute(_QStr("geometry"), QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			pInfo->AddAttribute(_QStr("screen geometry"), QString("(%1,%2,%3,%4)").arg(geoScreen.left()).arg(geoScreen.top()).arg(geoScreen.right()).arg(geoScreen.bottom()));
			pInfo->AddAttribute(_QStr("geometry size"), QString("(%1,%2)").arg(geoScreen.width()).arg(geoScreen.height()));
			pInfo->AddAttribute(_QStr("client"), QString("(%1,%2,%3,%4)").arg(client.left()).arg(client.top()).arg(client.right()).arg(client.bottom()));
			pInfo->AddAttribute(_QStr("visible"), pItem->isVisible() ? _QStr("yes") : _QStr("no"));
			pInfo->AddAttribute(_QStr("enabled"), pItem->isEnabled() ? _QStr("yes") : _QStr("no"));
			pInfo->ShowOnTop();
		}
		else if (QSpacerItem* pItem = itemData<QSpacerItem>(clickedItem))
		{
			auto geo = pItem->geometry();
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle("QSpacerItem");
			pInfo->AddAttribute("class name", "QSpacerItem");
			pInfo->AddAttribute("geometry", QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			pInfo->AddAttribute("geometry size", QString("(%1,%2)").arg(geo.width()).arg(geo.height()));
			pInfo->AddAttribute("maxsize", QString("(%1,%2)").arg(pItem->maximumSize().width()).arg(pItem->maximumSize().height()));
			pInfo->AddAttribute("minsize", QString("(%1,%2)").arg(pItem->minimumSize().width()).arg(pItem->minimumSize().width()));
			QMetaEnum metaEnum = QMetaEnum::fromType<QSizePolicy::Policy>();
			pInfo->AddAttribute("sizePolicy", QString("%1 | %2").arg(metaEnum.valueToKey(pItem->sizePolicy().horizontalPolicy())).arg(metaEnum.valueToKey(pItem->sizePolicy().verticalPolicy())));
			pInfo->AddAttribute("stretch", QString("(%1,%2)").arg(pItem->sizePolicy().horizontalStretch()).arg(pItem->sizePolicy().verticalStretch()));
			pInfo->ShowOnTop();
		}
		else if(QLayout* pLayout = itemData<QLayout>(clickedItem))
		{
			auto geo = pLayout->geometry();
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle("QSpacerItem");
			pInfo->AddAttribute("class name", objectString(pLayout));
			pInfo->AddAttribute("object name", pLayout->objectName());
			pInfo->AddAttribute("geometry", QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			pInfo->AddAttribute("geometry size", QString("(%1,%2)").arg(geo.width()).arg(geo.height()));
			pInfo->AddAttribute("sizeHint", QString("(%1,%2)").arg(pLayout->totalSizeHint().width()).arg(pLayout->totalSizeHint().height()));
			pInfo->AddAttribute("maxSize", QString("(%1,%2)").arg(pLayout->totalMaximumSize().width()).arg(pLayout->totalMaximumSize().height()));
			pInfo->AddAttribute("minSize", QString("(%1,%2)").arg(pLayout->totalMinimumSize().width()).arg(pLayout->totalMinimumSize().height()));
			pInfo->AddAttribute("sizeConstrant", QMetaEnum::fromType<QLayout::SizeConstraint>().valueToKey(pLayout->sizeConstraint()));
			pInfo->ShowOnTop();
		}
	}
	return true;
}

bool CWidgetSpyTree::ShowWidgetStatus(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (!clickedItem) {
		return false;
	}

	if (QWidget* pTargetWidget = widgetData(clickedItem)) {
		CListInfoWnd* pInfo = new CListInfoWnd(window());
		pInfo->setWindowTitle(objectString(pTargetWidget));

		QStyleOption option;
		option.initFrom(pTargetWidget);
		QStringList arrState;
		for (int i = 0; i < queryEnumCount<QStyle::StateFlag>(); i++)
		{
			if (option.state.testFlag(queryEnumValue<QStyle::StateFlag>(i)))
			{
				arrState.append(queryEnumName<QStyle::StateFlag>(i));
			}
		}
		pInfo->AddAttribute("QStyle::StateFlag", arrState.join(" | "));

		QStringList arrAttribute;
		for (int i = 0; i < Qt::AA_AttributeCount; i++)
		{
			if (pTargetWidget->testAttribute((Qt::WidgetAttribute)i))
			{
				arrAttribute.append(queryEnumName<Qt::WidgetAttribute>((Qt::WidgetAttribute)i));
			}
		}
		pInfo->AddAttribute("WidgetAttribute", arrAttribute.join(" | "));

		QStringList arrWindowType;
		for (int i = 0; i < queryEnumCount<Qt::WindowType>(); ++i)
		{
			if (pTargetWidget->windowFlags().testFlag(queryEnumValue<Qt::WindowType>(i)))
			{
				arrWindowType.append(queryEnumName<Qt::WindowType>(i));
			}
		}
		pInfo->AddAttribute("WindowType", arrWindowType.join(" | "));

		QStringList arrUserProperty;
		for (int i = 0; i < pTargetWidget->metaObject()->propertyCount(); i++)
		{
			QMetaProperty property = pTargetWidget->metaObject()->property(i);
		}

		auto button = dynamic_cast<QAbstractButton*>(pTargetWidget);
		if (button) {
			pInfo->AddAttribute(_QStr("抽象按钮类"), button->isCheckable() ? _QStr("可勾选") : _QStr("不可勾选"));
			pInfo->AddAttribute(_QStr("抽象按钮类"), button->isChecked() ? _QStr("已勾选") : _QStr("未勾选"));
			pInfo->AddAttribute(_QStr("抽象按钮类"), button->group() ? _QStr("已分组") : _QStr("未分组"));
		}
		pInfo->ShowOnTop();
	}

	return true;
}

bool CWidgetSpyTree::ShowEventTrace(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		QWidget* pTargetWidget = widgetData(clickedItem);
		QObject* pTarget = widgetData(clickedItem) ? widgetData(clickedItem) : To<QObject>(graphicsData(clickedItem));
		CEventTraceWnd* pEventTraceWnd = new CEventTraceWnd(window());
		pEventTraceWnd->setWindowTitle(objectString(pTarget));
		pEventTraceWnd->MonitorWidget(pTarget);
		pEventTraceWnd->ShowOnTop();
	}
	return true;
}

void travelTreeItem(QTreeWidgetItem* pItem, std::function<void(QTreeWidgetItem* pItem)> fnOperation)
{
	if(nullptr == pItem)
	{
		return;
	}
	fnOperation(pItem);
	for (int nIndex = 0; nIndex < pItem->childCount(); nIndex++)
	{
		travelTreeItem(pItem->child(nIndex), fnOperation);
	}
}

bool CWidgetSpyTree::ShowEventTraceAll(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));

	if (clickedItem) {
		QObject* pTarget = widgetData(clickedItem) ? widgetData(clickedItem) : To<QObject>(graphicsData(clickedItem));
		CEventTraceWnd* pEventTraceWnd = new CEventTraceWnd(window());
		pEventTraceWnd->setWindowTitle(objectString(pTarget)+"[All]");
		travelTreeItem(clickedItem, [=](QTreeWidgetItem* pItem) {
			QObject* pTarget = widgetData(pItem) ? widgetData(pItem) : To<QObject>(graphicsData(pItem));
			pEventTraceWnd->MonitorWidget(pTarget);
		});
		pEventTraceWnd->ShowOnTop();

	}
	return true;
}

bool CWidgetSpyTree::SetUserDraw(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		QWidget* pTargetWidget = widgetData(clickedItem);
		pTargetWidget->setStyle(new CCommonProxyStyle(pTargetWidget->style()));
	}
	return true;
}

bool CWidgetSpyTree::ShowStyleEdit(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		QWidget* pTargetWidget = widgetData(clickedItem);
		CStyleEditWnd* pEditStyleWnd = new CStyleEditWnd(window());
		pEditStyleWnd->setWindowTitle(objectString(pTargetWidget));
		pEditStyleWnd->EditWidgetStyle(pTargetWidget);
		pEditStyleWnd->ShowOnTop();
	}
	return true;
}

bool CWidgetSpyTree::SpyParentWidget(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		if (widgetData(clickedItem)) {
			if(dynamic_cast<CSpyMainWindow*>(parent()))
			{
				dynamic_cast<CSpyMainWindow*>(parent())->spyObject()->setTreeTarget(OTo<QWidget>(widgetData(clickedItem)->parent()));
			}
			else
			{
				setTreeTarget(OTo<QWidget>(widgetData(clickedItem)->parent()));
			}
		}
		else if (auto item = graphicsData(clickedItem)) {
			if (item->parentItem())
			{
				dynamic_cast<CSpyMainWindow*>(parent())->spyObject()->setTreeTarget(item->parentItem());
			}
			else
			{
				dynamic_cast<CSpyMainWindow*>(parent())->spyObject()->setTreeTarget(item->scene()->views().front());
			}
		}
	}
	return true;
}

bool CWidgetSpyTree::SpyFirstParentWidget(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		if (QWidget* pTargetWidget = widgetData(clickedItem)) {
			QObject* pParent = pTargetWidget->parent();
			if (pParent) {
				while (pParent->parent()) {
					pParent = pParent->parent();
				}
				dynamic_cast<CSpyMainWindow*>(parent())->spyObject()->setTreeTarget(dynamic_cast<QWidget*>(pParent));
			}
		}
		else if (QGraphicsItem* item = graphicsData(clickedItem))
		{
			dynamic_cast<CSpyMainWindow*>(parent())->spyObject()->setTreeTarget(item->scene()->views().front());
		}
	}
	return true;
}

void CWidgetSpyTree::showLayout(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (QWidget* pTargetWidget = widgetData(clickedItem)) {
		QDialog* dlg = new QDialog(window());
		dlg->setLayout(new QHBoxLayout());
		CLayoutTree* tree = new CLayoutTree();
		tree->setTreeTarget(pTargetWidget);
		dlg->layout()->addWidget(tree);
		dlg->show();
	}

	if(QLayout* layout = itemData<QLayout>(clickedItem))
	{
		QDialog* dlg = new QDialog(window());
		CLayoutTree* tree = new CLayoutTree();
		tree->setTreeTarget(layout);
		dlg->layout()->addWidget(tree);
		dlg->show();
	}
}

void CWidgetSpyTree::showObjectTree(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (QObject* pTarget = itemData<QObject>(clickedItem)) {
		QDialog* dlg = new QDialog(window());
		dlg->setLayout(new QHBoxLayout());
		CObjectTree* tree = new CObjectTree();
		tree->setTreeTarget(pTarget);
		dlg->layout()->addWidget(tree);
		dlg->show();
	}
}

QRect CWidgetSpyTree::itemArea(QTreeWidgetItem* pItem)
{
	QRect rcArea;
	if (nullptr == pItem)
	{
		return rcArea;
	}

	if (QWidget* pTargetWidget = widgetData(pItem))
	{
		rcArea = ScreenRect(pTargetWidget);
	}
	else if (QGraphicsItem* pTargetItem = graphicsData(pItem))
	{
		rcArea = ScreenRect(pTargetItem);
	}
	else if (QLayout* pLayout = itemData<QLayout>(pItem))
	{
		rcArea = pLayout->geometry();
		rcArea.moveCenter(pLayout->parentWidget()->mapToGlobal(rcArea.center()));
	}
	else if (QSpacerItem* pSpacerItem = itemData<QSpacerItem>(pItem))
	{
		rcArea = pSpacerItem->geometry();
	}

	if (rcArea.isEmpty())
	{
		if (rcArea.width() == 0)
		{
			rcArea.setWidth(50);
		}
		if (rcArea.height() == 0)
		{
			rcArea.setHeight(50);
		}
	}

	return rcArea;
}

template<class T>
T* CWidgetSpyTree::itemData(QTreeWidgetItem* item)
{
	 return item->data(0, Qt::UserRole).value<T*>();
}

QGraphicsItem* CWidgetSpyTree::graphicsData(QTreeWidgetItem* item)
{
	return itemData<QGraphicsItem>(item);
}

QWidget* CWidgetSpyTree::widgetData(QTreeWidgetItem* item)
{
	return itemData<QWidget>(item);
}

CLayoutTree::CLayoutTree(QWidget* parent /*= nullptr*/):CWidgetSpyTree(parent)
{

}

bool CLayoutTree::setTreeTarget(QObject* target)
{
	QTreeWidgetItem* root = new QTreeWidgetItem;
	addTopLevelItem(root);
	if(OTo<QWidget>(target))
	{
		AddSubSpyNode(OTo<QWidget>(target), root);
	}
	else if (OTo<QLayout>(target))
	{
		AddSubSpyNode(OTo<QLayout>(target), root);
	}	
	return true;
}

bool CLayoutTree::AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode)
{
	parentNode->setText(0, objectString(parent));
	parentNode->setData(0, Qt::UserRole, QVariant::fromValue(parent));

	QLayout* layout = parent->layout();
	if( nullptr == layout)
	{
		return false;
	}

	QTreeWidgetItem* treeNode = new QTreeWidgetItem();
	parentNode->addChild(treeNode);
	AddSubSpyNode(layout, treeNode);
	return true;
}

Q_DECLARE_METATYPE(QSpacerItem*);
bool CLayoutTree::AddSubSpyNode(QLayout* layout, QTreeWidgetItem* parentNode)
{
	parentNode->setText(0, objectString(layout));
	parentNode->setData(0, Qt::UserRole, QVariant::fromValue(layout));

	int nCount = layout->count();
	for (int nIndex = 0; nIndex < nCount; nIndex++)
	{
		QLayoutItem* pItem = layout->itemAt(nIndex);
		QTreeWidgetItem* treeNode = new QTreeWidgetItem();
		parentNode->addChild(treeNode);
		if (pItem->widget())
		{
			treeNode->setText(0, pItem->widget()->metaObject()->className());
			AddSubSpyNode(pItem->widget(), treeNode);
		}
		if (pItem->layout())
		{
			treeNode->setText(0, pItem->layout()->metaObject()->className());
			AddSubSpyNode(pItem->layout(), treeNode);
		}
		if (pItem->spacerItem())
		{
			treeNode->setText(0, "QSpacerItem");
			treeNode->setData(0, Qt::UserRole, QVariant::fromValue(pItem->spacerItem()));
		}
	}
	return true;
}

CObjectTree::CObjectTree(QWidget* parent /*= nullptr*/) :CWidgetSpyTree(parent)
{

}

bool CObjectTree::setTreeTarget(QObject* target)
{
	QTreeWidgetItem* root = new QTreeWidgetItem;
	addTopLevelItem(root);
	AddSubSpyNode(target, root);
	return true;
}

bool CObjectTree::AddSubSpyNode(QObject* parent, QTreeWidgetItem* parentNode)
{
	parentNode->setText(0, objectString(parent));
	parentNode->setData(0, Qt::UserRole, QVariant::fromValue(parent));

	for(auto child : parent->children())
	{
		QTreeWidgetItem* treeNode = new QTreeWidgetItem();
		parentNode->addChild(treeNode);
		AddSubSpyNode(child, treeNode);
	}
	return true;
}
