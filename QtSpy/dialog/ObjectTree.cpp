#include "ObjectTree.h"

#include <QMenu>
#include <QEvent>
#include <QLayout>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QContextMenuEvent>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QWindow>

#include "qt_spydlg.h"
#include "StyleEditDlg.h"
#include "publicfunction.h"
#include "proxyStyle/ProxyStyle.h"
#include "SpyMainWindow.h"
#include "utils/LogRecorder.h"

inline void CTreeWidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyledItemDelegate::paint(painter, option, index);
	QStyleOptionViewItem opt(option);
	initStyleOption(&opt, index);
	painter->setPen((opt.text.contains("[hide]") || opt.text.contains("[disabled]")) ? Qt::gray : Qt::black);
	painter->drawText(opt.rect,  opt.displayAlignment, opt.text);
}

CWidgetSpyTree::CWidgetSpyTree(QWidget* parent /*= nullptr*/) : QTreeWidget(parent)
{
	setItemDelegate(new CTreeWidgetDelegate(this));
	installEventFilter(this);
	setHeaderHidden(true);
	
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);

	connect(this, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem* pCurrentItem, QTreeWidgetItem* pPrevItem) {
		if (nullptr == pPrevItem || nullptr == pCurrentItem)
		{
			return;
		}
		CSpyIndicatorWnd::showWnd(itemArea(pCurrentItem), false);
	});
}

bool CWidgetSpyTree::setTreeTarget(QGraphicsItem* target)
{
	if (nullptr == target)
	{
		return false;
	}

	clearContent();
	CTreeWidgetItem* root = new CTreeWidgetItem;
	addTopLevelItem(root);
	AddSubSpyNode(target, root);
	return true;
}

bool CWidgetSpyTree::setTreeTarget(QObject* target)
{
	if (nullptr == target)
	{
		return false;
	}

	clearContent();
	if (OTo<QWidget>(target))
	{
		CTreeWidgetItem* root = new CTreeWidgetItem;
		addTopLevelItem(root);
		AddSubSpyNode(OTo<QWidget>(target), root);
	}
	return true;
}

bool CWidgetSpyTree::AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode) {
	if (parent && parentNode) {
		connect(parent, &QObject::destroyed, this, &CWidgetSpyTree::removeTargetNode);

		m_mapWidgetNode[parent] = dynamic_cast<CTreeWidgetItem*>(parentNode);
		parentNode->setText(0, objectString(parent));
		parentNode->setData(0, Qt::UserRole, QVariant::fromValue(parent));
		QList<QWidget*> children = parent->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
		for (QWidget* child : children) {
			CTreeWidgetItem* treenode = new CTreeWidgetItem();
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
					CTreeWidgetItem* treenode = new CTreeWidgetItem();
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
		m_mapWidgetNode[parent] = dynamic_cast<CTreeWidgetItem*>(parentNode);
		parentNode->setText(0, objectString(parent));
		parentNode->setData(0, Qt::UserRole, QVariant::fromValue(parent));
		if (dynamic_cast<QObject*>(parent))
		{
			connect(dynamic_cast<QObject*>(parent), &QObject::destroyed, this, &CWidgetSpyTree::removeTargetNode);
		}

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
		indicatorWidget(itemAt(mouseEvent->pos()));
	}
	return false;
}

void CWidgetSpyTree::showContextMenu(const QPoint& pos)
{
	auto pItem = itemAt(mapFromGlobal(pos));
	if (nullptr == pItem)
	{
		return;
	}

	if (itemData<QLayout>(pItem))
	{
		QMenu menuLayout(this);
		QList<ESpyTreeMenuAction> arrAction = {
			ESpyTreeMenuAction::spyParent,
			ESpyTreeMenuAction::locate,
			ESpyTreeMenuAction::baseInfo,
			ESpyTreeMenuAction::enable
		};
		for (auto eAction : arrAction)
		{
			auto pAction = addAction(menuLayout, eAction);
			if (nullptr == pAction)
			{
				continue;
			}

			if (ESpyTreeMenuAction::enable == eAction)
			{
				pAction->setText(itemData<QLayout>(pItem)->isEnabled() ? "禁用" : "启用");
			}
		}
		onMenuClicked(menuLayout.exec(pos), pItem);
		return;
	}

	QMenu contextMenu(this);
	for (int nIndex = 0; nIndex < queryEnumCount<ESpyTreeMenuAction>(); nIndex++)
	{
		auto eAction = queryEnumValue<ESpyTreeMenuAction>(nIndex);
		auto pAction = addAction(contextMenu, eAction);
		if (nullptr == pAction)
		{
			continue;
		}

		if (ESpyTreeMenuAction::visible == eAction)
		{
			if (nullptr != widgetData(pItem))
			{
				pAction->setText(widgetData(pItem)->isVisible() ? "隐藏" : "显示");
			}
			else if (nullptr != graphicsData(pItem))
			{
				pAction->setText(graphicsData(pItem)->isVisible() ? "隐藏" : "显示");
			}
		}

		if (ESpyTreeMenuAction::enable == eAction)
		{
			if (nullptr != widgetData(pItem))
			{
				pAction->setText(widgetData(pItem)->isEnabled() ? "禁用" : "启用");
			}
			else if (nullptr != graphicsData(pItem))
			{
				pAction->setText(graphicsData(pItem)->isEnabled() ? "禁用" : "启用");
			}
		}
	}
	
	onMenuClicked(contextMenu.exec(pos), pItem);
}

QAction* CWidgetSpyTree::addAction(QMenu& menu, ESpyTreeMenuAction eAction)
{
	static QMap<ESpyTreeMenuAction, QString> s_MapAction = {
		{ ESpyTreeMenuAction::spyParent , "父组件"},
		{ ESpyTreeMenuAction::baseInfo, "基础信息" },
		{ ESpyTreeMenuAction::property, "属性信息" },
		{ ESpyTreeMenuAction::styleEdit, "风格编辑" },
		{ ESpyTreeMenuAction::locate, "目标定位"},
		{ ESpyTreeMenuAction::layoutTree, "布局树" },
		{ ESpyTreeMenuAction::objectTree, "对象树" },
		{ ESpyTreeMenuAction::signalSlot, "信号/槽" },
		{ ESpyTreeMenuAction::event, "事件跟踪" },
		{ ESpyTreeMenuAction::eventAll, "事件跟踪All" },
		{ ESpyTreeMenuAction::customDraw, "绘图代理" },
		{ ESpyTreeMenuAction::visible, "显示" },
		{ ESpyTreeMenuAction::enable, "启用" },
		{ ESpyTreeMenuAction::move, "移动|缩放" },
		{ ESpyTreeMenuAction::firstParent, "第一父组件" }
	};
	auto pAction = menu.addAction(s_MapAction[eAction]);
	pAction->setProperty("action", eAction);
	return pAction;
}

void CWidgetSpyTree::changeWidgetVisible(QTreeWidgetItem* pItem)
{
	if (pItem) {
		if (QWidget* pTargetWidget = widgetData(pItem))
		{
			pTargetWidget->setVisible(!pTargetWidget->isVisible());
			pItem->setText(0, objectString(pTargetWidget));
		}
		else if (QGraphicsItem* pTargetItem = graphicsData(pItem))
		{
			pTargetItem->setVisible(!pTargetItem->isVisible());
			pItem->setText(0, objectString(pTargetItem));
		}
	}
}

void CWidgetSpyTree::changeWidgetEnable(QTreeWidgetItem* pItem)
{
	if (pItem) {
		if (QWidget* pTargetWidget = widgetData(pItem))
		{
			pTargetWidget->setEnabled(!pTargetWidget->isEnabled());
			pItem->setText(0, objectString(pTargetWidget));
		}
		else if (QGraphicsItem* pTargetItem = graphicsData(pItem))
		{
			pTargetItem->setEnabled(!pTargetItem->isEnabled());
			pItem->setText(0, objectString(pTargetItem));
		}
		else if (QLayout* pLayout = itemData<QLayout>(pItem))
		{
			pLayout->setEnabled(!pLayout->isEnabled());
			pItem->setText(0, objectString(pLayout));
		}
	}
}

void CWidgetSpyTree::changeWidgetPosOrSize(QTreeWidgetItem* pItem)
{
	if (pItem) {
		if (QWidget* pTargetWidget = widgetData(pItem)) {
			CMoveOrScaleWidgetWnd* pWindow = new CMoveOrScaleWidgetWnd(window());
			pWindow->ControlTarget(pTargetWidget);
			pWindow->showOnTop();
		}
		else if (QGraphicsItem* pTargetItem = graphicsData(pItem)) {
			CMoveOrScaleWidgetWnd* pWindow = new CMoveOrScaleWidgetWnd();
			pWindow->GraphicsTarget(pTargetItem);
			pWindow->showOnTop();
		}
	}
}

void CWidgetSpyTree::indicatorWidget(QTreeWidgetItem* pItem)
{
	if (nullptr == pItem)
	{
		return;
	}
	CSpyIndicatorWnd::showWnd(itemArea(pItem),false);
}

void CWidgetSpyTree::showSignalSlot(QTreeWidgetItem* pItem, bool bRecusive /*= false*/)
{
	if (pItem) {
		CSignalSpyWnd* pSpy = new CSignalSpyWnd(window());
		if (QWidget* pTargetWidget = widgetData(pItem))
		{
			pSpy->setTargetObject(pTargetWidget);
		}
		else if (QGraphicsItem* pGrapItem = graphicsData(pItem))
		{
			pSpy->setTargetObject(To<QObject>(pGrapItem));
		}
		pSpy->showOnTop();
	}
}

bool CWidgetSpyTree::showWidgetInfo(QTreeWidgetItem* pTreeItem)
{
	if (pTreeItem) {
		if (QWidget* pTargetWidget = widgetData(pTreeItem)) {
			auto geo = pTargetWidget->geometry();
			auto geoScreen = ScreenRect(pTargetWidget);
			int nUniqueId = -1;
			if (dynamic_cast<QWidget*>(pTargetWidget)) {
				nUniqueId = dynamic_cast<QWidget*>(pTargetWidget)->winId();
			}
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle(objectString(pTargetWidget));
			pInfo->AddAttribute("class name", objectClass(pTargetWidget));
			pInfo->AddAttribute("object name", pTargetWidget->objectName());
			pInfo->AddAttribute("geometry", QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			pInfo->AddAttribute("screen geometry", QString("(%1,%2,%3,%4)").arg(geoScreen.left()).arg(geoScreen.top()).arg(geoScreen.right()).arg(geoScreen.bottom()));
			pInfo->AddAttribute("size", QString("(%1,%2)").arg(pTargetWidget->width()).arg(pTargetWidget->height()));
			pInfo->AddAttribute("maxsize", QString("(%1,%2)").arg(pTargetWidget->maximumWidth()).arg(pTargetWidget->maximumHeight()));
			pInfo->AddAttribute("minsize", QString("(%1,%2)").arg(pTargetWidget->minimumWidth()).arg(pTargetWidget->minimumHeight()));
			pInfo->AddAttribute("sizeHint", QString("%1,%2").arg(pTargetWidget->sizeHint().width()).arg(pTargetWidget->sizeHint().height()));
			pInfo->AddAttribute("sizePolicy", QString("%1 | %2").arg(queryEnumName<QSizePolicy::Policy>(pTargetWidget->sizePolicy().horizontalPolicy())).arg(queryEnumName<QSizePolicy::Policy>(pTargetWidget->sizePolicy().verticalPolicy())));
			pInfo->AddAttribute("stretch", QString("(%1,%2)").arg(pTargetWidget->sizePolicy().horizontalStretch()).arg(pTargetWidget->sizePolicy().verticalStretch()));
			pInfo->AddAttribute("stylesheet", pTargetWidget->styleSheet());
			pInfo->AddAttribute("font", pTargetWidget->font().toString());
			pInfo->AddAttribute("winid", QString("%1").arg(pTargetWidget->winId()));
			pInfo->showOnTop();
		}
		else if (QGraphicsItem* pItem = graphicsData(pTreeItem))
		{
			auto geo = pItem->sceneBoundingRect();
			auto geoScreen = ScreenRect(pItem);
			auto client = pItem->boundingRect();
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle(objectString(pItem));
			pInfo->AddAttribute("class name", objectClass(To<QObject>(pItem)));
			pInfo->AddAttribute("object name", ::objectName(To<QObject>(pItem)));
			pInfo->AddAttribute("boundingRect", QString("(%1,%2,%3,%4)").arg(client.left()).arg(client.top()).arg(client.right()).arg(client.bottom()));
			pInfo->AddAttribute("sceneBoundingRect", QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			pInfo->AddAttribute("screen geometry", QString("(%1,%2,%3,%4)").arg(geoScreen.left()).arg(geoScreen.top()).arg(geoScreen.right()).arg(geoScreen.bottom()));
			pInfo->AddAttribute("screen geometry size", QString("(%1,%2)").arg(geoScreen.width()).arg(geoScreen.height()));
			pInfo->showOnTop();
		}
		else if (QSpacerItem* pItem = itemData<QSpacerItem>(pTreeItem))
		{
			auto geo = pItem->geometry();
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle("QSpacerItem");
			pInfo->AddAttribute("class name", "QSpacerItem");
			pInfo->AddAttribute("geometry", QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			QWidget* pParentWidget = pTreeItem->data(0, Qt::UserRole + 1).value<QWidget*>();
			if (pParentWidget)
			{
				auto geoScreen = ScreenRect(pParentWidget, geo);
				pInfo->AddAttribute("screen geometry", QString("(%1,%2,%3,%4)").arg(geoScreen.left()).arg(geoScreen.top()).arg(geoScreen.right()).arg(geoScreen.bottom()));
			}
			pInfo->AddAttribute("size", QString("(%1,%2)").arg(geo.width()).arg(geo.height()));
			pInfo->AddAttribute("maxsize", QString("(%1,%2)").arg(pItem->maximumSize().width()).arg(pItem->maximumSize().height()));
			pInfo->AddAttribute("minsize", QString("(%1,%2)").arg(pItem->minimumSize().width()).arg(pItem->minimumSize().width()));
			QMetaEnum metaEnum = QMetaEnum::fromType<QSizePolicy::Policy>();
			pInfo->AddAttribute("sizePolicy", QString("%1 | %2").arg(metaEnum.valueToKey(pItem->sizePolicy().horizontalPolicy())).arg(metaEnum.valueToKey(pItem->sizePolicy().verticalPolicy())));
			pInfo->AddAttribute("stretch", QString("(%1,%2)").arg(pItem->sizePolicy().horizontalStretch()).arg(pItem->sizePolicy().verticalStretch()));
			pInfo->showOnTop();
		}
		else if (QLayout* pLayout = itemData<QLayout>(pTreeItem))
		{
			auto geo = pLayout->geometry();
			CListInfoWnd* pInfo = new CListInfoWnd(window());
			pInfo->setWindowTitle("QLayout");
			pInfo->AddAttribute("class name", objectString(pLayout));
			pInfo->AddAttribute("object name", pLayout->objectName());
			pInfo->AddAttribute("geometry", QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			QWidget* pParentWidget = pTreeItem->data(0, Qt::UserRole + 1).value<QWidget*>();
			if(pParentWidget)
			{
				auto geoScreen = ScreenRect(pParentWidget, geo);
				pInfo->AddAttribute("screen geometry", QString("(%1,%2,%3,%4)").arg(geoScreen.left()).arg(geoScreen.top()).arg(geoScreen.right()).arg(geoScreen.bottom()));
			}
			pInfo->AddAttribute("size", QString("(%1,%2)").arg(geo.width()).arg(geo.height()));
			pInfo->AddAttribute("sizeHint", QString("(%1,%2)").arg(pLayout->totalSizeHint().width()).arg(pLayout->totalSizeHint().height()));
			pInfo->AddAttribute("maxSize", QString("(%1,%2)").arg(pLayout->totalMaximumSize().width()).arg(pLayout->totalMaximumSize().height()));
			pInfo->AddAttribute("minSize", QString("(%1,%2)").arg(pLayout->totalMinimumSize().width()).arg(pLayout->totalMinimumSize().height()));
			pInfo->AddAttribute("sizeConstrant", QMetaEnum::fromType<QLayout::SizeConstraint>().valueToKey(pLayout->sizeConstraint()));
			pInfo->AddAttribute("spacing", QString::number(pLayout->spacing()));
			pInfo->showOnTop();
		}
	}
	return true;
}

bool CWidgetSpyTree::showWidgetStatus(QTreeWidgetItem* pItem)
{
	if (QWidget* pTargetWidget = widgetData(pItem)) {
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

		if (pTargetWidget->windowHandle())
		{
			QStringList arrWindowFlags;
			for (int i = 0; i < queryEnumCount<Qt::WindowType>(); ++i)
			{
				if (pTargetWidget->windowHandle()->flags().testFlag(queryEnumValue<Qt::WindowType>(i)))
				{
					arrWindowFlags.append(queryEnumName<Qt::WindowType>(i));
				}
			}
			pInfo->AddAttribute("WindowFlag", arrWindowFlags.join(" | "));
		}

		QStringList arrUserProperty;
		for (int i = 0; i < pTargetWidget->metaObject()->propertyCount(); i++)
		{
			QMetaProperty property = pTargetWidget->metaObject()->property(i);
			arrUserProperty.append(QString("%1 : %2").arg(property.name(), property.read(pTargetWidget).toString()));
		}
		if (!arrUserProperty.empty())
		{
			pInfo->AddAttribute("MetaProperty", arrUserProperty.join(" , "));
		}

		auto button = dynamic_cast<QAbstractButton*>(pTargetWidget);
		if (button) {
			pInfo->AddAttribute("抽象按钮类", button->isCheckable() ? "可勾选" : "不可勾选");
			pInfo->AddAttribute("抽象按钮类", button->isChecked() ? "已勾选" : "未勾选");
			pInfo->AddAttribute("抽象按钮类", button->group() ? "已分组" : "未分组");
		}
		pInfo->showOnTop();
	}

	return true;
}

bool CWidgetSpyTree::showEventTrace(QTreeWidgetItem* pItem)
{
	if (pItem) {
		QObject* pTarget = widgetData(pItem) ? widgetData(pItem) : To<QObject>(graphicsData(pItem));
		CEventTraceWnd* pEventTraceWnd = new CEventTraceWnd(window());
		pEventTraceWnd->setWindowTitle(objectString(pTarget));
		pEventTraceWnd->MonitorWidget(pTarget);
		pEventTraceWnd->showOnTop();
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

bool CWidgetSpyTree::showEventTraceAll(QTreeWidgetItem* pItem)
{
	if (pItem) {
		QObject* pTarget = widgetData(pItem) ? widgetData(pItem) : To<QObject>(graphicsData(pItem));
		CEventTraceWnd* pEventTraceWnd = new CEventTraceWnd(window());
		pEventTraceWnd->setWindowTitle(objectString(pTarget)+"[All]");
		travelTreeItem(pItem, [=](QTreeWidgetItem* pItem) {
			QObject* pTarget = widgetData(pItem) ? widgetData(pItem) : To<QObject>(graphicsData(pItem));
			pEventTraceWnd->MonitorWidget(pTarget);
		});
		pEventTraceWnd->showOnTop();
	}
	return true;
}

bool CWidgetSpyTree::setUserDraw(QTreeWidgetItem* pItem)
{
	if (pItem) {
		QWidget* pTargetWidget = widgetData(pItem);
		pTargetWidget->setStyle(new CCommonProxyStyle(pTargetWidget->style()));
	}
	return true;
}

bool CWidgetSpyTree::showStyleEdit(QTreeWidgetItem* pItem)
{
	if (pItem) {
		QWidget* pTargetWidget = widgetData(pItem);
		CStyleEditWnd* pEditStyleWnd = new CStyleEditWnd(window());
		pEditStyleWnd->setWindowTitle(objectString(pTargetWidget));
		pEditStyleWnd->EditWidgetStyle(pTargetWidget);
		pEditStyleWnd->showOnTop();
	}
	return true;
}

bool CWidgetSpyTree::spyParentWidget(QTreeWidgetItem* pItem)
{
	if (nullptr == pItem) 
	{
		return false;
	}
	
	if (widgetData(pItem)) 
	{
		if(dynamic_cast<CSpyMainWindow*>(parent()))
		{
			dynamic_cast<CSpyMainWindow*>(parent())->setTreeTarget(OTo<QWidget>(widgetData(pItem)->parent()));
		}
		else
		{
			setTreeTarget(OTo<QWidget>(widgetData(pItem)->parent()));
		}
	}
	else if (auto item = graphicsData(pItem))
	{
		if (item->parentItem())
		{
			dynamic_cast<CSpyMainWindow*>(parent())->setTreeTarget(item->parentItem());
		}
		else
		{
			dynamic_cast<CSpyMainWindow*>(parent())->setTreeTarget(item->scene()->views().front());
		}
	}
	
	return true;
}

bool CWidgetSpyTree::spyFirstParentWidget(QTreeWidgetItem* pItem)
{
	if (nullptr == pItem)
	{
		return false;
	}

	if (QWidget* pTargetWidget = widgetData(pItem)) 
	{
		QObject* pParent = pTargetWidget->parent();
		if (pParent) {
			while (pParent->parent()) {
				pParent = pParent->parent();
			}
			dynamic_cast<CSpyMainWindow*>(parent())->setTreeTarget(dynamic_cast<QWidget*>(pParent));
		}
	}
	else if (QGraphicsItem* item = graphicsData(pItem))
	{
		dynamic_cast<CSpyMainWindow*>(parent())->setTreeTarget(item->scene()->views().front());
	}
	
	return true;
}

void CWidgetSpyTree::showLayout(QTreeWidgetItem* pItem)
{
	if (QWidget* pTargetWidget = widgetData(pItem)) {
		QDialog* dlg = new QDialog(window());
		dlg->setWindowTitle("布局信息");
		dlg->setLayout(new QHBoxLayout());
		CLayoutTree* tree = new CLayoutTree();
		tree->setTreeTarget(pTargetWidget);
		dlg->layout()->addWidget(tree);
		dlg->show();
	}

	if (QLayout* layout = itemData<QLayout>(pItem))
	{
		QDialog* dlg = new QDialog(window());
		dlg->setLayout(new QHBoxLayout());
		dlg->setWindowTitle("布局信息");
		CLayoutTree* tree = new CLayoutTree();
		tree->setTreeTarget(layout);
		dlg->layout()->addWidget(tree);
		dlg->show();
	}
}

void CWidgetSpyTree::showObjectTree(QTreeWidgetItem* pItem)
{
	if (QObject* pTarget = itemData<QObject>(pItem)) {
		QDialog* dlg = new QDialog(window());
		dlg->setLayout(new QHBoxLayout());
		CObjectTree* tree = new CObjectTree();
		tree->setTreeTarget(pTarget);
		dlg->layout()->addWidget(tree);
		dlg->show();
	}
}

void CWidgetSpyTree::clearContent()
{
	m_mapWidgetNode.clear();
	clear();
}

bool CWidgetSpyTree::setCurrentSpyItem(void* pTarget)
{
	if(!m_mapWidgetNode.contains(pTarget))
	{
		return false;
	}

	auto pNode = m_mapWidgetNode[pTarget];
	if (pNode.isNull())
	{
		return false;
	}

	auto pTreeNode = dynamic_cast<QTreeWidgetItem*>(pNode.data());
	if (nullptr != pTreeNode)
	{
		setCurrentItem(pTreeNode);
		return true;
	}
	
	return false;
}

int CWidgetSpyTree::currentCount()
{
	return m_mapWidgetNode.size();
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
		QWidget* pParentWidget = pItem->data(0, Qt::UserRole + 1).value<QWidget*>();
		if (pParentWidget)
		{
			rcArea = ScreenRect(pParentWidget, rcArea);
		}
	}
	else if (QSpacerItem* pSpacerItem = itemData<QSpacerItem>(pItem))
	{
		rcArea = pSpacerItem->geometry();
		QWidget* pParentWidget = pItem->data(0, Qt::UserRole + 1).value<QWidget*>();
		if (pParentWidget)
		{
			rcArea = ScreenRect(pParentWidget, rcArea);
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

void CWidgetSpyTree::removeTargetNode()
{
	auto pObject = sender();
	if (nullptr == pObject || !m_mapWidgetNode.contains(pObject))
	{
		return;
	}
	
	LogRecorder().addLog("[CWidgetSpyTree]Item destroyed, Remove Node: " + objectString(pObject));

	auto pNode = m_mapWidgetNode[pObject];
	m_mapWidgetNode.remove(pObject);
	if (pNode.isNull())
	{
		return;
	}
	auto pTreeNode = dynamic_cast<QTreeWidgetItem*>(pNode.data());
	if (nullptr != pTreeNode->parent())
	{
		pTreeNode->parent()->removeChild(pTreeNode);
	}
}

void CWidgetSpyTree::onMenuClicked(QAction* pAction, QTreeWidgetItem* pItem)
{
	if (nullptr == pAction || nullptr == pItem)
	{
		return;
	}

	ESpyTreeMenuAction eAction = pAction->property("action").value<ESpyTreeMenuAction>();
	switch (eAction)
	{
	case ESpyTreeMenuAction::spyParent:
	{
		spyParentWidget(pItem);
		break;
	}
	case ESpyTreeMenuAction::locate:
	{
		indicatorWidget(pItem);
		break;
	}
	case ESpyTreeMenuAction::baseInfo:
	{
		showWidgetInfo(pItem);
		break;
	}
	case ESpyTreeMenuAction::layoutTree:
	{
		showLayout(pItem);
		break;
	}
	case ESpyTreeMenuAction::property:
	{
		showWidgetStatus(pItem);
		break;
	}
	case ESpyTreeMenuAction::objectTree:
	{
		showObjectTree(pItem);
		break;
	}
	case ESpyTreeMenuAction::signalSlot:
	{
		showSignalSlot(pItem);
		break;
	}
	case ESpyTreeMenuAction::event:
	{
		showEventTrace(pItem);
		break;
	}
	case ESpyTreeMenuAction::eventAll:
	{
		showEventTraceAll(pItem);
		break;
	}
	case ESpyTreeMenuAction::styleEdit:
	{
		showStyleEdit(pItem);
		break;
	}
	case ESpyTreeMenuAction::customDraw:
	{
		setUserDraw(pItem);
		break;
	}
	case ESpyTreeMenuAction::visible:
	{
		changeWidgetVisible(pItem);
		break;
	}
	case ESpyTreeMenuAction::enable:
	{
		changeWidgetEnable(pItem);
		break;
	}
	case ESpyTreeMenuAction::move:
	{
		changeWidgetPosOrSize(pItem);
		break;
	}
	case ESpyTreeMenuAction::firstParent:
	{
		spyFirstParentWidget(pItem);
		break;
	}
	default:
		break;
	}
}

CLayoutTree::CLayoutTree(QWidget* parent /*= nullptr*/):CWidgetSpyTree(parent)
{

}

bool CLayoutTree::setTreeTarget(QObject* target)
{
	if (nullptr == target)
	{
		return false;
	}

	clearContent();
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
	if (nullptr == layout)
	{
		return false;
	}

	QTreeWidgetItem* treeNode = new QTreeWidgetItem();
	parentNode->addChild(treeNode);
	AddSubSpyNode(layout, treeNode, parent);
	return true;
}

Q_DECLARE_METATYPE(QSpacerItem*);
bool CLayoutTree::AddSubSpyNode(QLayout* layout, QTreeWidgetItem* parentNode, QWidget* pWidget)
{
	parentNode->setText(0, objectString(layout));
	parentNode->setData(0, Qt::UserRole, QVariant::fromValue(layout));
	parentNode->setData(0, Qt::UserRole + 1, QVariant::fromValue(pWidget));

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
			AddSubSpyNode(pItem->layout(), treeNode, pWidget);
		}
		if (pItem->spacerItem())
		{
			treeNode->setText(0, "QSpacerItem");
			treeNode->setData(0, Qt::UserRole, QVariant::fromValue(pItem->spacerItem()));
			treeNode->setData(0, Qt::UserRole + 1, QVariant::fromValue(pWidget));
		}
	}
	return true;
}

CObjectTree::CObjectTree(QWidget* parent /*= nullptr*/) :CWidgetSpyTree(parent)
{

}

bool CObjectTree::setTreeTarget(QObject* target)
{
	if (nullptr == target)
	{
		return false;
	}

	clearContent();
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