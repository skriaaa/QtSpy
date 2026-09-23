#include "ObjectTree.h"
#include "theme/QtSpyTheme.h"

#include <QMenu>
#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QLayout>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QDialog>
#include <QMenuBar>
#include <QApplication>
#include <QMouseEvent>
#include <QPointer>
#include <QCursor>
#include <QSize>
#include <QStringList>
#include <QKeySequence>

#include "qt_spydlg.h"
#include "PropertyInspectorDlg.h"
#include "StyleEditDlg.h"
#include "publicfunction.h"
#include "proxyStyle/ProxyStyle.h"
#include "SpyMainWindow.h"
#include "utils/LogRecorder.h"

namespace
{
	const QSize SPY_TREE_DIALOG_DEFAULT_SIZE(400, 300);

	void activateWindow(QWidget* pWidget)
	{
		if (nullptr == pWidget)
		{
			return;
		}

		if (!pWidget->isVisible())
		{
			pWidget->show();
		}
		if (pWidget->isMinimized())
		{
			pWidget->showNormal();
		}
		pWidget->raise();
		pWidget->activateWindow();
	}

	QDialog* createSpyTreeDialog(QWidget* pParentWidget, const QString& strTitle, CWidgetSpyTree* pTree)
	{
		// 用 CXDialog 构造以挂载主题(CXDialog 构造函数内 apply)
		QDialog* pDialog = new CXDialog(pParentWidget);
		pDialog->setAttribute(Qt::WA_DeleteOnClose);
		pDialog->setWindowTitle("QtSpy · " + strTitle);

		pDialog->resize(SPY_TREE_DIALOG_DEFAULT_SIZE);

		QVBoxLayout* pLayout = new QVBoxLayout(pDialog);
		pLayout->setMargin(1);

		QMenuBar* pMenuBar = new QMenuBar(pDialog);
		QAction* pActionFind = pMenuBar->addAction("查找");
		pActionFind->setToolTip("按名称或屏幕拾取定位目标在当前控件树中的位置");
		// Ctrl+F 快捷键(默认 WindowShortcut 上下文: 本弹窗有焦点才触发, 不串进目标程序)
		pActionFind->setShortcut(QKeySequence(QStringLiteral("Ctrl+F")));
		QObject::connect(pActionFind, &QAction::triggered, [pDialog, pTree]() {
			CFindWnd* pFindWnd = new CFindWnd(pTree, pDialog);
			pFindWnd->showOnTop();
		});
		// 挂菜单栏动作浮窗提示(查找)
		new CMenuBarTooltipFilter(pMenuBar);
		pLayout->setMenuBar(pMenuBar);
		pLayout->addWidget(pTree);

		return pDialog;
	}
}

CTreeCursorSearchFilter::CTreeCursorSearchFilter(QWidget* pHostWidget, CWidgetSpyTree* pTree)
	: QObject(pHostWidget)
	, m_pHostWidget(pHostWidget)
	, m_pTree(pTree)
{
}

CTreeCursorSearchFilter::~CTreeCursorSearchFilter()
{
	finish();
}

void CTreeCursorSearchFilter::setPickedCallback(std::function<void()> callback)
{
	m_fnPicked = std::move(callback);
}

void CTreeCursorSearchFilter::start()
{
	if (m_pHostWidget.isNull() || m_pTree.isNull())
	{
		deleteLater();
		return;
	}

	m_bRunning = true;
	m_pHostWidget->grabMouse();
	m_pHostWidget->installEventFilter(this);
	m_pHostWidget->setMouseTracking(true);
	QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
}

bool CTreeCursorSearchFilter::eventFilter(QObject* pWatched, QEvent* pEvent)
{
	if (pWatched != m_pHostWidget.data())
	{
		return QObject::eventFilter(pWatched, pEvent);
	}

	switch (pEvent->type())
	{
	case QEvent::MouseMove:
	{
		QMouseEvent* pMouseEvent = dynamic_cast<QMouseEvent*>(pEvent);
		if ((nullptr != pMouseEvent) && !m_pTree.isNull())
		{
			CSpyIndicatorWnd::showWnd(m_pTree->itemAreaAt(pMouseEvent->globalPos()));
		}
		return true;
	}
	case QEvent::MouseButtonRelease:
	{
		QMouseEvent* pMouseEvent = dynamic_cast<QMouseEvent*>(pEvent);
		Qt::MouseButton eButton = (nullptr == pMouseEvent) ? Qt::NoButton : pMouseEvent->button();
		QPoint ptGlobal = (nullptr == pMouseEvent) ? QPoint() : pMouseEvent->globalPos();
		finish();
		if ((Qt::RightButton != eButton) && !m_pTree.isNull())
		{
			m_pTree->setCurrentSpyItemAt(ptGlobal);
		}
		activateWindow(m_pHostWidget.data());
		deleteLater();
		// 回调放最后: 拾取成功后通知宿主(如查找窗口"拾取完成即关闭")。
		// WA_DeleteOnClose 走 deleteLater, 此处回调即便销毁本对象也是延迟到事件循环, 安全。
		if ((Qt::RightButton != eButton) && m_fnPicked)
		{
			m_fnPicked();
		}
		return true;
	}
	default:
		break;
	}

	return QObject::eventFilter(pWatched, pEvent);
}

void CTreeCursorSearchFilter::finish()
{
	if (!m_bRunning)
	{
		return;
	}

	if (!m_pHostWidget.isNull())
	{
		m_pHostWidget->removeEventFilter(this);
		m_pHostWidget->releaseMouse();
	}
	QApplication::restoreOverrideCursor();
	CSpyIndicatorWnd::instance().hide();
	m_bRunning = false;
}

inline void CTreeWidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyledItemDelegate::paint(painter, option, index);
	QStyleOptionViewItem opt(option);
	initStyleOption(&opt, index);
	// [hide]/[disabled] 节点用次级色, 其余主色 —— 颜色唯一来源 QtSpyTheme
	const SpyPalette& palette = QtSpyTheme::palette();
	painter->setPen((opt.text.contains("[hide]") || opt.text.contains("[disabled]"))
		? QPen(palette.textSecondary) : QPen(palette.textPrimary));
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
		Q_UNUSED(pPrevItem);
		if (nullptr == pCurrentItem)
		{
			return;
		}
		CSpyIndicatorWnd::showWnd(itemArea(pCurrentItem), false);
	});
}
CWidgetSpyTree::~CWidgetSpyTree()
{

}

void CWidgetSpyTree::paintEvent(QPaintEvent* event)
{
	QTreeWidget::paintEvent(event);
	if (topLevelItemCount() > 0)
	{
		return;
	}

	// 空树提示: 快捷键引导 —— 颜色唯一来源 QtSpyTheme
	// 逐行绘制, 两行按行首左对齐, 整块水平居中
	const QStringList arrHint = {
		QStringLiteral("1、Alt + Q : 展示窗口并居中"),
		QStringLiteral("2、Alt + E : 进入捕捉状态"),
	};
	QPainter painter(viewport());
	painter.setPen(QtSpyTheme::palette().textSecondary);
	const QFontMetrics fm = painter.fontMetrics();
	int nMaxWidth = 0;
	for (const QString& strLine : arrHint)
	{
		nMaxWidth = qMax(nMaxWidth, fm.horizontalAdvance(strLine));
	}
	QRect rc = viewport()->rect();
	rc.setBottom(rc.bottom() - rc.height() / 6);
	const int nX = rc.center().x() - nMaxWidth / 2;
	int nY = rc.center().y() - arrHint.size() * fm.lineSpacing() / 2;
	for (const QString& strLine : arrHint)
	{
		painter.drawText(QPoint(nX, nY + fm.ascent()), strLine);
		nY += fm.lineSpacing();
	}
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
		// 设定目标/刷新后自动展开第一级, 直接看到目标的子节点;
		// GraphicsView 内组件走 setTreeTarget(QGraphicsItem*) 重载, 保持不展开
		root->setExpanded(true);
	}
	return true;
}

bool CWidgetSpyTree::AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode) {
	if (parent && parentNode) {
		if (parent != this && !isAncestorOf(parent))
		{
			connect(parent, &QObject::destroyed, this, &CWidgetSpyTree::removeTargetNode);
		}

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
		// 布局节点: 属性检查(含基础信息分组) + 启用/禁用; 定位走双击, 父组件对布局无意义
		QList<ESpyTreeMenuAction> arrAction = {
			ESpyTreeMenuAction::property,
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

	/* 菜单合并(15 项 -> 10 项): 高频直达 + 低频收子菜单
	   - 基础信息并入组件信息窗口(基础信息分组); 事件跟踪All 并入事件跟踪窗口(包含子组件开关)
	   - 目标定位删掉(双击节点即高亮); 顶层父组件单独放菜单最下面 */
	QMenu contextMenu(this);
	addAction(contextMenu, ESpyTreeMenuAction::spyParent);
	addAction(contextMenu, ESpyTreeMenuAction::property);
	addAction(contextMenu, ESpyTreeMenuAction::signalSlot);
	addAction(contextMenu, ESpyTreeMenuAction::event);
	QMenu* menuStyle = contextMenu.addMenu("样式");
	addAction(*menuStyle, ESpyTreeMenuAction::styleEdit);
	addAction(*menuStyle, ESpyTreeMenuAction::customDraw);
	QMenu* menuSubView = contextMenu.addMenu("子视图");
	addAction(*menuSubView, ESpyTreeMenuAction::layoutTree);
	addAction(*menuSubView, ESpyTreeMenuAction::objectTree);

	QAction* pActionVisible = addAction(contextMenu, ESpyTreeMenuAction::visible);
	if (nullptr != pActionVisible)
	{
		if (nullptr != widgetData(pItem))
		{
			pActionVisible->setText(widgetData(pItem)->isVisible() ? "隐藏" : "显示");
		}
		else if (nullptr != graphicsData(pItem))
		{
			pActionVisible->setText(graphicsData(pItem)->isVisible() ? "隐藏" : "显示");
		}
	}

	QAction* pActionEnable = addAction(contextMenu, ESpyTreeMenuAction::enable);
	if (nullptr != pActionEnable)
	{
		if (nullptr != widgetData(pItem))
		{
			pActionEnable->setText(widgetData(pItem)->isEnabled() ? "禁用" : "启用");
		}
		else if (nullptr != graphicsData(pItem))
		{
			pActionEnable->setText(graphicsData(pItem)->isEnabled() ? "禁用" : "启用");
		}
	}

	addAction(contextMenu, ESpyTreeMenuAction::move);
	// 顶层父组件: 低频跳转, 单独放最下面
	addAction(contextMenu, ESpyTreeMenuAction::firstParent);

	onMenuClicked(contextMenu.exec(pos), pItem);
}

QAction* CWidgetSpyTree::addAction(QMenu& menu, ESpyTreeMenuAction eAction)
{
	static QMap<ESpyTreeMenuAction, QString> s_MapAction = {
		{ ESpyTreeMenuAction::spyParent , "父组件"},
		{ ESpyTreeMenuAction::property, "组件信息" },
		{ ESpyTreeMenuAction::styleEdit, "风格编辑" },
		{ ESpyTreeMenuAction::layoutTree, "布局树" },
		{ ESpyTreeMenuAction::objectTree, "对象树" },
		{ ESpyTreeMenuAction::signalSlot, "信号/槽" },
		{ ESpyTreeMenuAction::event, "事件跟踪" },
		{ ESpyTreeMenuAction::customDraw, "绘图代理" },
		{ ESpyTreeMenuAction::visible, "显示" },
		{ ESpyTreeMenuAction::enable, "启用" },
		{ ESpyTreeMenuAction::move, "移动|缩放" },
		{ ESpyTreeMenuAction::firstParent, "顶层父组件" }
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
			pInfo->setWindowTitle("QtSpy · " + objectString(pTargetWidget));
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
			pInfo->setWindowTitle("QtSpy · " + objectString(pItem));
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
			pInfo->setWindowTitle("QtSpy · QSpacerItem");
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
			pInfo->setWindowTitle("QtSpy · QLayout");
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
			pInfo->AddAttribute("margins", QString("(%1,%2,%3,%4)").arg(pLayout->contentsMargins().left()).arg(pLayout->contentsMargins().top()).arg(pLayout->contentsMargins().right()).arg(pLayout->contentsMargins().bottom()));
			pInfo->AddAttribute("sizeConstrant", QMetaEnum::fromType<QLayout::SizeConstraint>().valueToKey(pLayout->sizeConstraint()));
			pInfo->AddAttribute("spacing", QString::number(pLayout->spacing()));
			pInfo->showOnTop();
		}
	}
	return true;
}

bool CWidgetSpyTree::showWidgetStatus(QTreeWidgetItem* pItem)
{
	if (nullptr == pItem)
	{
		return false;
	}

	QObject* pTargetObject = itemData<QObject>(pItem);
	if (nullptr == pTargetObject)
	{
		pTargetObject = To<QObject>(graphicsData(pItem));
	}
	if (nullptr == pTargetObject)
	{
		// 纯 QGraphicsItem/QSpacerItem 不是 QObject, 没有属性表, 退回基础信息窗口
		return showWidgetInfo(pItem);
	}

	CPropertyInspectorDlg* pInspectorDlg = new CPropertyInspectorDlg(window());
	if (!pInspectorDlg->setTargetObject(pTargetObject))
	{
		delete pInspectorDlg;
		return false;
	}
	pInspectorDlg->showOnTop();
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

bool CWidgetSpyTree::showEventTrace(QTreeWidgetItem* pItem)
{
	if (pItem) {
		QObject* pTarget = widgetData(pItem) ? widgetData(pItem) : To<QObject>(graphicsData(pItem));
		CEventTraceWnd* pEventTraceWnd = new CEventTraceWnd(window());
		pEventTraceWnd->setWindowTitle("QtSpy · 事件 " + objectString(pTarget));
		// 两份目标集交给事件跟踪窗口: 自身 / 子树全部(树节点序),
		// 窗内"包含子组件"开关在两者间切换(原"事件跟踪All"并入)
		QList<QObject*> arrSelf, arrAll;
		if (nullptr != pTarget)
		{
			arrSelf.append(pTarget);
		}
		travelTreeItem(pItem, [&arrAll, this](QTreeWidgetItem* pNode) {
			QObject* pObj = widgetData(pNode) ? widgetData(pNode) : To<QObject>(graphicsData(pNode));
			if (nullptr != pObj)
			{
				arrAll.append(pObj);
			}
		});
		pEventTraceWnd->setTargets(arrSelf, arrAll);
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
		pEditStyleWnd->setWindowTitle("QtSpy · " + objectString(pTargetWidget));
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
		CLayoutTree* pTree = new CLayoutTree();
		pTree->setTreeTarget(pTargetWidget);
		QDialog* pDialog = createSpyTreeDialog(window(), "布局树", pTree);
		pDialog->show();
	}

	if (QLayout* layout = itemData<QLayout>(pItem))
	{
		CLayoutTree* pTree = new CLayoutTree();
		pTree->setTreeTarget(layout);
		QDialog* pDialog = createSpyTreeDialog(window(), "布局树", pTree);
		pDialog->show();
	}
}

void CWidgetSpyTree::showObjectTree(QTreeWidgetItem* pItem)
{
	if (QObject* pTarget = itemData<QObject>(pItem)) {
		CObjectTree* pTree = new CObjectTree();
		pTree->setTreeTarget(pTarget);
		QDialog* pDialog = createSpyTreeDialog(window(), "对象树", pTree);
		pDialog->show();
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
		selectSpyItem(pTreeNode);
		return true;
	}
	
	return false;
}

bool CWidgetSpyTree::setCurrentSpyItemAt(const QPoint& ptGlobal)
{
	QTreeWidgetItem* pItem = spyItemAt(ptGlobal);
	if (nullptr == pItem)
	{
		return false;
	}

	selectSpyItem(pItem);
	return true;
}

QRect CWidgetSpyTree::itemAreaAt(const QPoint& ptGlobal)
{
	QTreeWidgetItem* pItem = spyItemAt(ptGlobal);
	if (nullptr == pItem)
	{
		return QRect();
	}

	return itemArea(pItem);
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

QTreeWidgetItem* CWidgetSpyTree::spyItemAt(const QPoint& ptGlobal)
{
	QTreeWidgetItem* pMatchedItem = nullptr;
	qint64 nMatchedArea = 0;

	auto fnCheckItem = [this, &ptGlobal, &pMatchedItem, &nMatchedArea](QTreeWidgetItem* pItem) {
		QRect rcItemArea = itemArea(pItem);
		if (!rcItemArea.isValid() || rcItemArea.isEmpty() || !rcItemArea.contains(ptGlobal))
		{
			return;
		}

		qint64 nArea = static_cast<qint64>(rcItemArea.width()) * rcItemArea.height();
		if ((nullptr == pMatchedItem) || (nArea <= nMatchedArea))
		{
			pMatchedItem = pItem;
			nMatchedArea = nArea;
		}
	};

	for (int nIndex = 0; nIndex < topLevelItemCount(); ++nIndex)
	{
		travelTreeItem(topLevelItem(nIndex), fnCheckItem);
	}

	return pMatchedItem;
}

void CWidgetSpyTree::selectSpyItem(QTreeWidgetItem* pItem)
{
	if (nullptr == pItem)
	{
		return;
	}

	for (QTreeWidgetItem* pParentItem = pItem->parent(); nullptr != pParentItem; pParentItem = pParentItem->parent())
	{
		pParentItem->setExpanded(true);
	}
	pItem->setExpanded(true);
	setCurrentItem(pItem);
	scrollToItem(pItem);
	CSpyIndicatorWnd::showWnd(itemArea(pItem), false);
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
	// 设定目标后自动展开第一级
	root->setExpanded(true);
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
	// 设定目标后自动展开第一级
	root->setExpanded(true);
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
