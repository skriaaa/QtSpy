#include "qt_spydlg.h"
#include <QSignalSpy>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTreeWidget>
#include <QTreeView>
#include <QTableWidget>
#include <QTableView>
#include <QApplication>
#include <QScreen>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QMenuBar>
#include <QHelpEvent>
#include <QToolTip>
#include <QContextMenuEvent>
#include <QSysInfo>
#include <QDebug>
#include <QFontDatabase>
#include <QFontDialog>
#include <QPlainTextEdit>
#include <QListView>
#include <QStringListModel>
#include <QHeaderView>
#include <QStyleFactory>
#include <QPainter>
#include <QDirIterator>
#include <QResource>
#include <QColorDialog>
#include <QMessageBox>
#include <QMetaMethod>
#include <QTime>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include "publicfunction.h"
#include "proxyStyle/ProxyStyle.h"
#include "StyleEditDlg.h"
#include "utils/LogRecorder.h"
#include "SpyMainWindow.h"
#include "ObjectTree.h"
#include "ConnectionInfo.h"

namespace
{
	constexpr int LOG_LIST_MAX_COUNT = 5000;
	constexpr int LOG_LIST_FLUSH_INTERVAL = 50;
	constexpr int LOG_LIST_FLUSH_BATCH = 500;
	constexpr int LOG_LIST_PENDING_MAX_COUNT = LOG_LIST_MAX_COUNT + LOG_LIST_FLUSH_BATCH;

	void selectTreeItem(QTreeWidget* pTree, QTreeWidgetItem* pItem)
	{
		if ((nullptr == pTree) || (nullptr == pItem))
		{
			return;
		}

		for (QTreeWidgetItem* pParentItem = pItem->parent(); nullptr != pParentItem; pParentItem = pParentItem->parent())
		{
			pParentItem->setExpanded(true);
		}
		pItem->setExpanded(true);
		pTree->setCurrentItem(pItem);
		pTree->scrollToItem(pItem);
	}
}

bool CXDialog::event(QEvent* event)
{
	// 目标程序可能在任意时刻改全局 QSS/字体, 触发本窗口 StyleChange/FontChange;
	// 重新 apply 恢复主题。apply() 幂等(QSS 未变不 set)且带重入保护, 不会递归。
	// 字体不走 apply: 由 QSS font 规则经 QStyleSheetStyle 自动维持(含自愈);
	// 若在 apply 里 setFont 会与 QSS 字体机制打架造成 FontChange 无限递归(栈溢出)。
	// CSpyIndicatorWnd 指示框完全自绘(paintEvent, 不绘制 QSS 背景),
	// StyleChange 重挂主题 QSS 对其高亮效果无影响。
	if ((QEvent::StyleChange == event->type()) || (QEvent::FontChange == event->type()))
	{
		QtSpyTheme::apply(this);
	}
	return QDialog::event(event);
}

CMenuBarTooltipFilter::CMenuBarTooltipFilter(QMenuBar* pMenuBar) : QObject(pMenuBar)
{
	if (nullptr != pMenuBar)
	{
		pMenuBar->installEventFilter(this);
	}
}

bool CMenuBarTooltipFilter::eventFilter(QObject* watched, QEvent* event)
{
	if (QEvent::ToolTip == event->type())
	{
		auto pMenuBar = qobject_cast<QMenuBar*>(watched);
		auto pHelpEvent = dynamic_cast<QHelpEvent*>(event);
		if (nullptr != pMenuBar && nullptr != pHelpEvent)
		{
			QAction* pAction = pMenuBar->actionAt(pHelpEvent->pos());
			if (nullptr != pAction && !pAction->toolTip().isEmpty())
			{
				QToolTip::showText(pHelpEvent->globalPos(), pAction->toolTip(), pMenuBar);
				return true;
			}
		}
	}
	return QObject::eventFilter(watched, event);
}

CSpyIndicatorWnd::CSpyIndicatorWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent),
m_Timer(QTimer(this)), m_nSpanPeriod(0)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_TranslucentBackground);
	// WindowStaysOnTopHint: 指示框要压过 QtSpy 各置顶窗口(目标被置顶窗口遮挡时高亮仍可见)
	setWindowFlags(windowFlags() | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowStaysOnTopHint);
	// 高亮全部走 paintEvent 自绘, 不用 QSS:
	// 透明背景下 QSS 背景规则会画出实心底色; 且 StyleChange 重挂主题 QSS
	// 也不影响自绘效果(paintEvent 不绘制 QSS 背景)

	m_Timer.setInterval(100);
	QObject::connect(&m_Timer, &QTimer::timeout, [&]() {
		if (m_nSpanPeriod == 0) {
			m_Timer.stop();
			hide();
		}
		else
		{
			// 渐弱走填充/边框 alpha(见 paintEvent), 不动 windowOpacity —— 曲线与旧版一致
			repaint();
			m_nSpanPeriod-=2;
		}
	});
}

CSpyIndicatorWnd& CSpyIndicatorWnd::instance()
{
	static CSpyIndicatorWnd wnd;
	return wnd;
}

void CSpyIndicatorWnd::showWnd(QRect rcArea, bool bHold)
{
	if (rcArea.isEmpty())
	{
		instance().hide();
		return;
	}

	instance().setGeometry(rcArea);
	instance().show(bHold);
}

void CSpyIndicatorWnd::show(bool bHold)
{
	if(!bHold)
	{
		// 渐弱曲线与旧版完全一致: 从等效 0.5 起(比驻留略亮, 旧版手感), 15 步 x 100ms 衰减到 0
		m_nSpanPeriod = 30;
		m_Timer.stop();
		m_Timer.start();
	}
	else
	{
		m_nSpanPeriod = 0;   // 驻留: 填充/边框 alpha 102(等效旧版整窗 0.4)
		m_Timer.stop();
	}

	QDialog::show();
	raise();
}

void CSpyIndicatorWnd::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	// 与旧版观感完全一致(旧版 = 整窗透明度 x 白底/青边实色):
	// 驻留: 等效 0.4 -> alpha 102; 渐弱: 等效 0.5 -> 0, alpha = 255 * span / 60
	// 颜色唯一来源 QtSpyTheme
	const int nAlpha = (m_nSpanPeriod > 0) ? (255 * m_nSpanPeriod / 60) : 102;
	QPainter painter(this);
	const SpyPalette& palette = QtSpyTheme::palette();
	QColor clFill = palette.contentBg;      // 白
	clFill.setAlpha(nAlpha);
	painter.fillRect(rect(), clFill);
	QColor clBorder = palette.spyHighlight; // 青
	clBorder.setAlpha(nAlpha);
	QPen pen(clBorder);
	pen.setWidth(1);
	painter.setPen(pen);
	painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

// fixed(min==max)的控件, setGeometry 会被最小/最大尺寸钳制, 缩放完全无效;
// 落几何前把确实构成钳制的边界放开到新尺寸(只放宽不收窄, 原约束不丢):
// 例: fixed 100x100 缩到 90 -> min 变 90、max 仍 100, 缩回 100 无需再动约束
void CMoveOrScaleWidgetWnd::scaleGeometry(const QRect& rc)
{
	QSize szMin = m_pTargetWidget->minimumSize();
	QSize szMax = m_pTargetWidget->maximumSize();

	if (rc.width() < szMin.width())
	{
		m_pTargetWidget->setMinimumWidth(rc.width());
	}
	else if (rc.width() > szMax.width())
	{
		m_pTargetWidget->setMaximumWidth(rc.width());
	}
	if (rc.height() < szMin.height())
	{
		m_pTargetWidget->setMinimumHeight(rc.height());
	}
	else if (rc.height() > szMax.height())
	{
		m_pTargetWidget->setMaximumHeight(rc.height());
	}

	m_pTargetWidget->setGeometry(rc);
}

CMoveOrScaleWidgetWnd::CMoveOrScaleWidgetWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	this->setWindowTitle("QtSpy · 移动&缩放");
	m_pEditMoveStep = new QLineEdit();
	m_pEditScaleStep = new QLineEdit();
	m_pEditMoveStep->setAlignment(Qt::AlignCenter);
	m_pEditScaleStep->setAlignment(Qt::AlignCenter);
	m_pEditMoveStep->setText("1");
	m_pEditScaleStep->setText("1");
	QObject::connect(m_pEditMoveStep, &QLineEdit::textChanged, [&]() {
		if (m_pEditMoveStep) {
			m_nMoveStep = m_pEditMoveStep->text().toInt();
		}
		});
	QObject::connect(m_pEditScaleStep, &QLineEdit::textChanged, [&]() {
		if (m_pEditScaleStep) {
			m_nScaleStep = m_pEditScaleStep->text().toInt();
		}
		});

	// 十字布局: 输入框居中, 上/下/左/右按钮环绕其四周, 统一宽度 46px
	constexpr int CROSS_CTRL_WIDTH = 46;
	auto btnMoveUp = new QPushButton("上");
	auto btnMoveDown = new QPushButton("下");
	auto btnMoveLeft = new QPushButton("左");
	auto btnMoveRight = new QPushButton("右");
	btnMoveUp->setFixedWidth(CROSS_CTRL_WIDTH);
	btnMoveDown->setFixedWidth(CROSS_CTRL_WIDTH);
	btnMoveLeft->setFixedWidth(CROSS_CTRL_WIDTH);
	btnMoveRight->setFixedWidth(CROSS_CTRL_WIDTH);
	QObject::connect(btnMoveUp, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, -m_nMoveStep, 0, -m_nMoveStep);
			m_pTargetWidget->setGeometry(rc);
		}
		if (m_pTargetItem) {
			m_pTargetItem->setY(m_pTargetItem->y() - 1);
		}
		});
	QObject::connect(btnMoveDown, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, m_nMoveStep, 0, m_nMoveStep);
			m_pTargetWidget->setGeometry(rc);
		}
		if (m_pTargetItem) {
			m_pTargetItem->setY(m_pTargetItem->y() + 1);
		}
		});
	QObject::connect(btnMoveLeft, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(-1 * m_nMoveStep, 0, -1 * m_nMoveStep, 0);
			m_pTargetWidget->setGeometry(rc);
		}
		if (m_pTargetItem) {
			m_pTargetItem->setY(m_pTargetItem->x() - 1);
		}
		});
	QObject::connect(btnMoveRight, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(m_nMoveStep, 0, m_nMoveStep, 0);
			m_pTargetWidget->setGeometry(rc);
		}
		if (m_pTargetItem) {
			m_pTargetItem->setY(m_pTargetItem->x() + 1);
		}
		});
	auto gridMove = new QGridLayout();
	gridMove->addWidget(btnMoveUp, 0, 2);
	gridMove->addWidget(btnMoveLeft, 1, 1);
	gridMove->addWidget(m_pEditMoveStep, 1, 2);
	gridMove->addWidget(btnMoveRight, 1, 3);
	gridMove->addWidget(btnMoveDown, 2, 2);
	// 统一 46px; 两侧空列(col0/col4)吸收多余宽度, 十字整体居中且按钮不被拉伸
	m_pEditMoveStep->setFixedWidth(CROSS_CTRL_WIDTH);
	gridMove->setColumnStretch(0, 1);
	gridMove->setColumnStretch(4, 1);
	auto groupMove = new QGroupBox("移动");
	groupMove->setLayout(gridMove);

	auto btnScaleUp = new QPushButton("上");
	auto btnScaleDown = new QPushButton("下");
	auto btnScaleLeft = new QPushButton("左");
	auto btnScaleRight = new QPushButton("右");
	btnScaleUp->setFixedWidth(CROSS_CTRL_WIDTH);
	btnScaleDown->setFixedWidth(CROSS_CTRL_WIDTH);
	btnScaleLeft->setFixedWidth(CROSS_CTRL_WIDTH);
	btnScaleRight->setFixedWidth(CROSS_CTRL_WIDTH);
	QObject::connect(btnScaleUp, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			// 上/左: 对应边界向外(上方/左侧)平移 n 像素, 与下/右语义一致
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, -m_nScaleStep, 0, 0);
			scaleGeometry(rc);
		}
		});
	QObject::connect(btnScaleDown, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, 0, 0, m_nScaleStep);
			scaleGeometry(rc);
		}
		});
	QObject::connect(btnScaleLeft, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(-m_nScaleStep, 0, 0, 0);
			scaleGeometry(rc);
		}
		});
	QObject::connect(btnScaleRight, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, 0, m_nScaleStep, 0);
			scaleGeometry(rc);
		}
		});
	auto gridScale = new QGridLayout();
	gridScale->addWidget(btnScaleUp, 0, 2);
	gridScale->addWidget(btnScaleLeft, 1, 1);
	gridScale->addWidget(m_pEditScaleStep, 1, 2);
	gridScale->addWidget(btnScaleRight, 1, 3);
	gridScale->addWidget(btnScaleDown, 2, 2);
	m_pEditScaleStep->setFixedWidth(CROSS_CTRL_WIDTH);
	gridScale->setColumnStretch(0, 1);
	gridScale->setColumnStretch(4, 1);
	auto groupScale = new QGroupBox("缩放");
	groupScale->setLayout(gridScale);

	// 左侧移动, 右侧缩放
	auto layout = new QHBoxLayout();
	layout->addWidget(groupMove);
	layout->addWidget(groupScale);
	this->setLayout(layout);
}

CListInfoWnd::CListInfoWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent)
{
	initWidgets();
}

bool CListInfoWnd::AddAttribute(QString strName, QString value)
{
	if (m_pTableWidget) {
		int row = m_pTableWidget->rowCount();
		m_pTableWidget->insertRow(row);
		AddInfo(strName, row, 0);
		AddInfo(value, row, 1);
		return true;
	}
	return false;
}

bool CListInfoWnd::AddInfo(QString strText, int row, int col)
{
	if (m_pTableWidget)
	{
		int count = m_pTableWidget->rowCount();
		if (m_pTableWidget->rowCount() <= row)
		{
			m_pTableWidget->insertRow(row);
		}
		auto item = new QTableWidgetItem(strText);
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_pTableWidget->setItem(row, col, item);

		return true;
	}

	return false;
}

void CListInfoWnd::ClearAll()
{
	if (m_pTableWidget) {
		m_pTableWidget->clear();
		m_pTableWidget->clearContents();
		m_pTableWidget->setRowCount(0);
	}
}

void CListInfoWnd::initWidgets()
{
	setAttribute(Qt::WA_DeleteOnClose);
	resize(320, 380);
	setLayout(new QVBoxLayout());

	InitTableWidget();
}

void CListInfoWnd::InitTableWidget()
{
	m_pTableWidget = new QTableWidget();
	this->layout()->addWidget(m_pTableWidget);

	QStringList headers;
	headers << "属性" << "值";
	m_pTableWidget->setColumnCount(headers.size());
	m_pTableWidget->setHorizontalHeaderLabels(headers);
	m_pTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

CSignalSpyWnd::CSignalSpyWnd(QWidget* parent /*= nullptr*/) :CXDialog(parent)
{
	initWidgets();
	initContextMenu();
}

CSignalSpyWnd::~CSignalSpyWnd()
{
	clearContent();
}

void CSignalSpyWnd::setTargetObject(QObject* target)
{
	m_pTargetObject = target;
	QString strText = objectString(target);

	// 窗口定位为"连接"窗口(信号/槽/连接三个 tab), 名称与首个高频 tab 区分
	setWindowTitle("QtSpy · 连接 " + strText);
	// 监控日志窗可能已随上次目标创建, 同步刷新其标题
	if (m_pTraceWnd)
	{
		m_pTraceWnd->setWindowTitle("QtSpy · 信号 " + strText);
	}
	setContent();

	ParseSignal(target);
}

void CSignalSpyWnd::ParseSignal(QObject* target)
{

}

void CSignalSpyWnd::initWidgets()
{
	resize(800, 600);
	setAttribute(Qt::WA_DeleteOnClose);
	// 正常对话框标题栏(此前 Qt::Popup+CustomizeWindowHint 是窄标题栏的来源);
	// 置顶由 showOnTop 的 WindowStaysOnTopHint 负责
	setLayout(new QVBoxLayout);

	initTableWidget();

	QTabWidget* tab = new QTabWidget;
	layout()->addWidget(tab);
	// tab 顺序: 信号 -> 槽 -> 连接 (按使用频率)
	tab->addTab(m_pSignalTable, "信号");
	tab->addTab(m_pSlotTable, "槽");
	tab->addTab(m_pConnectionTable, "连接");
}

void CSignalSpyWnd::initTableWidget()
{
	auto fnSetTable = [](QTableWidget*& table, const QStringList& titles) {
		table = new QTableWidget;
		table->setColumnCount(titles.size());
		table->setHorizontalHeaderLabels(titles);
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	};

	fnSetTable(m_pSignalTable, { "signal" });
	fnSetTable(m_pSlotTable, { "slot" });
	fnSetTable(m_pConnectionTable, { "sender", "signal", "receiver", "slot", "type" });
	m_pConnectionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
}

void CSignalSpyWnd::initContextMenu()
{
	m_pSignalTable->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_pSignalTable, &QTableWidget::customContextMenuRequested, [&]() {
		QMenu contextMenu(m_pSignalTable);
		QAction* acEmit = contextMenu.addAction("发送信号");
		QAction* acInspectThis = contextMenu.addAction("监控信号");
		QAction* acInspectAll = contextMenu.addAction("监控所有信号");

		QPoint pt = QCursor().pos();
		QAction* acbk = contextMenu.exec(pt);
		if (acbk == acEmit)
		{
			QTableWidgetItem* item = m_pSignalTable->itemAt(m_pSignalTable->viewport()->mapFromGlobal(pt));
			if (item)
			{
				QMetaMethod* method = static_cast<QMetaMethod*>(item->data(Qt::UserRole).value<void*>());
				method->invoke(m_pTargetObject,Qt::DirectConnection);
			}
		}
		if (acbk == acInspectThis)
		{
			QTableWidgetItem* item = m_pSignalTable->itemAt(m_pSignalTable->viewport()->mapFromGlobal(pt));
			if (item)
			{
				QMetaMethod* method = static_cast<QMetaMethod*>(item->data(Qt::UserRole).value<void*>());
				if (m_arrSignal[method] == nullptr)
				{
					CSignalSpy* spy = new CSignalSpy(m_pTargetObject, *method);
					spy->setTraceWnd(traceWnd());
				}
				traceWnd()->showOnTop();
			}
		}
		if (acbk == acInspectAll)
		{
			for (auto item : m_arrSignal)
			{
				if (item.second == nullptr)
				{
					item.second = new CSignalSpy(m_pTargetObject, *item.first);
					item.second->setTraceWnd(traceWnd());
				}
			}
			traceWnd()->showOnTop();
		}
	});
}

void CSignalSpyWnd::addMethodRow(QTableWidget* table, QMetaMethod* method)
{
	if (method == nullptr || nullptr == method->enclosingMetaObject())
		return;

	int nRow = table->rowCount();
	table->insertRow(nRow);
	QString strSignature = QString("%1::%2").arg(method->enclosingMetaObject()->className()).arg(QString(method->methodSignature()));
	QTableWidgetItem* item = new QTableWidgetItem;
	item->setText(strSignature);
	item->setData(Qt::UserRole, QVariant::fromValue((void*)(method)));
	table->setItem(nRow, 0, item);
}

void CSignalSpyWnd::addConnectionRow(ConnectionInfo* pInfo)
{
	int nRow = m_pConnectionTable->rowCount();
	m_pConnectionTable->insertRow(nRow);
	m_pConnectionTable->setItem(nRow, 0, new QTableWidgetItem(objectClass(pInfo->pSender) + ("(0x" + QString::number((uintptr_t)pInfo->pSender, 16) + ")")));
	m_pConnectionTable->setItem(nRow, 1, new QTableWidgetItem(pInfo->strSignal));
	m_pConnectionTable->setItem(nRow, 2, new QTableWidgetItem(objectClass(pInfo->pReceiver) + ("(0x" + QString::number((uintptr_t)pInfo->pReceiver, 16) + ")")));
	m_pConnectionTable->setItem(nRow, 3, new QTableWidgetItem(pInfo->strSlot));
	m_pConnectionTable->setItem(nRow, 4, new QTableWidgetItem(pInfo->strConnectType));
}

void CSignalSpyWnd::setContent()
{
	clearContent();
	CConnectionAanlyzer analyzer(m_pTargetObject);
	for (auto method : analyzer.objectSignals())
	{
		auto ret = m_arrSignal.insert({ new QMetaMethod(method), nullptr });
		addMethodRow(m_pSignalTable, ret.first->first);
	}

	for (auto method : analyzer.objectSlots())
	{
		addMethodRow(m_pSlotTable, &method);
	}

	for (auto connect : analyzer.outBoundConnections())
	{
		addConnectionRow(&connect);
	}

	for (auto connect : analyzer.inBoundConnections())
	{
		addConnectionRow(&connect);
	}
}

void CSignalSpyWnd::clearContent()
{
	for (auto pair : m_arrSignal)
	{
		if (pair.first)
		{
			delete pair.first;
		}
		if (pair.second)
		{
			delete pair.second;
		}
	}
	m_arrSignal.clear();
	m_pSignalTable->setRowCount(0);
	m_pSlotTable->setRowCount(0);
}

CLogTraceWnd* CSignalSpyWnd::traceWnd()
{
	if (m_pTraceWnd == nullptr)
	{
		m_pTraceWnd = new CLogTraceWnd();
		// 与 setTargetObject 内的同步逻辑配合: 懒创建时机也带目标标题
		m_pTraceWnd->setWindowTitle("QtSpy · 信号 " + objectString(m_pTargetObject));
	}
	return m_pTraceWnd;
}

CStatusInfoWnd::CStatusInfoWnd(QWidget* parent) : CListInfoWnd(parent)
{
	setWindowTitle("QtSpy · 状态信息");
	UpdateStatusInfo();
}

void CStatusInfoWnd::UpdateStatusInfo()
{
	ClearAll();
	AddAttribute("当前控件数", QString::number(qApp->allWidgets().size()));
	AddAttribute("页面控件数", QString::number(dynamic_cast<CSpyMainWindow*>(parentWidget())->currentItemCount()));
}

void CStatusInfoWnd::keyReleaseEvent(QKeyEvent* event)
{
	QKeyEvent* pTypeEvent = dynamic_cast<QKeyEvent*>(event);
	if (pTypeEvent && pTypeEvent->key() == Qt::Key_F5) {
		UpdateStatusInfo();
	}
	CListInfoWnd::keyReleaseEvent(event);
}

CCursorLocateWnd::CCursorLocateWnd(QWidget* parent) :CXDialog(parent)
{
	setMouseTracking(true);
	setStyleSheet(QString("background-color:#00ff00;"));
	setWindowOpacity(0.3);
	setWindowFlag(Qt::FramelessWindowHint);
	setWindowFlag(Qt::WindowStaysOnTopHint);
	setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
	setWindowTitle(QString::fromUtf8("QtSpy · 定位鼠标..."));
	raise();
}

void CCursorLocateWnd::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.fillRect(rect(), QColor(0, 255, 0, 0));
	QPen pen(QColor(255, 255, 0));
	painter.setPen(pen);
	QPoint ptGlobal = QCursor::pos();
	QPoint ptLocale = mapFromGlobal(ptGlobal);
	painter.drawLine(QPoint(ptLocale.x(), 0), QPoint(ptLocale.x(), rect().height()));
	painter.drawLine(QPoint(0, ptLocale.y()), QPoint(rect().width(), ptLocale.y()));
	QRect rcTips(ptLocale, rect().topRight());
	QFont font("宋体", 32);
	painter.setFont(font);
	QPen pen2(QColor(255, 255, 255));
	painter.setPen(pen2);
	painter.drawText(rect().center(), QString("global mouse : (%1,%2)").arg(ptGlobal.x()).arg(ptGlobal.y()));
}

void CCursorLocateWnd::mousePressEvent(QMouseEvent* event)
{
	if(event->button() == Qt::RightButton)
	{
		close();
	}
}

void CCursorLocateWnd::mouseMoveEvent(QMouseEvent* event)
{
	update();
	QDialog::mouseMoveEvent(event);
}

CLogTraceWnd::CLogTraceWnd(QWidget* parent /*= nullptr*/, bool bShowBreakCheck /*= true*/) :CXDialog(parent)
{
	setWindowTitle("QtSpy · 日志");
	resize(400, 300);
	setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
	initWidgets();
	if (bShowBreakCheck && nullptr != m_pControlLayout)
	{
		m_pControlLayout->addWidget(createBreakCheck());
	}
}

bool CLogTraceWnd::AddInfo(QString strInfo)
{
	bool bHas = m_arrStrHas.isEmpty();
	for (auto strHas : m_arrStrHas)
	{
		if (strInfo.contains(strHas, Qt::CaseInsensitive)) 
		{
			bHas = true;
			break;
		}
	}

	if (!bHas)
	{
		return false;
	}

	for (auto strNo : m_arrStrNo)
	{
		if (strInfo.contains(strNo, Qt::CaseInsensitive)) 
		{
			return false;
		}
	}

	strInfo = QString("%1 | %2 | %3").arg(m_nCount++, 4, 10, QLatin1Char('0')).arg(QTime::currentTime().toString("hh:mm:ss::zzz")).arg(strInfo);
	CLogRecorder::instance().addLog(strInfo);
	if (m_bBreakOnTrace)
	{
		Q_ASSERT_X(false, "QtSpy trace break", qPrintable(strInfo));
	}
	if(m_bOnlyLog)
	{
		return true;
	}

	appendPendingLog(strInfo, m_nLogGeneration);

	return true;
}

void CLogTraceWnd::initWidgets()
{
	auto mainLayout = new QVBoxLayout();
	this->setLayout(mainLayout);
	m_listView = new QListView();
	m_listView->setModel(&m_listModel);
	connect(m_listView, &QListView::clicked, [&]() {m_bTrace = false; });
	m_timerFlushLog.setInterval(LOG_LIST_FLUSH_INTERVAL);
	QObject::connect(&m_timerFlushLog, &QTimer::timeout, [this]() {
		flushPendingLogs();
	});
	mainLayout->addWidget(m_listView);

	// 列表下方控制行: onlyLog 开关 + 子类追加的控件(触发中断等)
	// 原 showList/onlyLog 切换按钮删掉, 改为勾选框: 勾选后日志仅落文件不进列表
	auto control_1 = new QHBoxLayout();
	m_pControlLayout = control_1;
	{
		auto checkOnlyLog = new QCheckBox("onlyLog");
		checkOnlyLog->setToolTip("勾选后日志仅写入文件, 不再进列表");
		QObject::connect(checkOnlyLog, &QCheckBox::toggled, [this](bool bChecked) {
			m_bOnlyLog = bChecked;
			});
		control_1->addWidget(checkOnlyLog);
	}


	auto control_2 = new QHBoxLayout();
	auto editFilterHas = new QLineEdit();
	QObject::connect(editFilterHas, &QLineEdit::textChanged, [&](const QString& str) {
		if (str.isEmpty())
		{
			m_arrStrHas.clear();
		}
		else
		{
			m_arrStrHas = str.split("|");
		}
		});
	control_2->addWidget(new QLabel("has:"));
	control_2->addWidget(editFilterHas);

	auto control_3 = new QHBoxLayout();
	auto editFilterNo = new QLineEdit();
	QObject::connect(editFilterNo, &QLineEdit::textChanged, [&](const QString& str) {
		if(str.isEmpty())
		{
			m_arrStrNo.clear();
		}
		else
		{
			m_arrStrNo = str.split("|");
		}
		});
	control_3->addWidget(new QLabel("no:"));
	control_3->addWidget(editFilterNo);

	// 底部按钮行: clear / trace 固定 80x30
	auto control_4 = new QHBoxLayout();
	{
		auto btnClear = new QPushButton("clear");
		btnClear->setFixedSize(80, 30);
		QObject::connect(btnClear, &QPushButton::clicked, [&]() {
			++m_nLogGeneration;
			m_listPendingLog.clear();
			m_timerFlushLog.stop();
			m_listModel.removeRows(0, m_listModel.rowCount());
			this->m_nCount = 0;
			});
		control_4->addWidget(btnClear);
		auto btnTrace = new QPushButton("trace");
		btnTrace->setFixedSize(80, 30);
		QObject::connect(btnTrace, &QPushButton::clicked, [&]() { m_bTrace = true; });
		control_4->addWidget(btnTrace);
	}

	mainLayout->addLayout(control_1);
	mainLayout->addLayout(control_2);
	mainLayout->addLayout(control_3);
	mainLayout->addLayout(control_4);
}

void CLogTraceWnd::appendPendingLog(QString strInfo, int nGeneration)
{
	if (m_nLogGeneration != nGeneration)
	{
		return;
	}

	m_listPendingLog.append(strInfo);
	int nOverflowCount = m_listPendingLog.size() - LOG_LIST_PENDING_MAX_COUNT;
	if (0 < nOverflowCount)
	{
		int nRemoveCount = qMax(LOG_LIST_FLUSH_BATCH, nOverflowCount);
		m_listPendingLog.erase(m_listPendingLog.begin(), m_listPendingLog.begin() + nRemoveCount);
	}

	if (!m_timerFlushLog.isActive())
	{
		m_timerFlushLog.start();
	}
}

void CLogTraceWnd::flushPendingLogs()
{
	if (m_listPendingLog.isEmpty())
	{
		m_timerFlushLog.stop();
		return;
	}

	int nFlushCount = qMin(LOG_LIST_FLUSH_BATCH, m_listPendingLog.size());
	int nOverflowCount = (m_listModel.rowCount() + nFlushCount) - LOG_LIST_MAX_COUNT;
	if (0 < nOverflowCount)
	{
		m_listModel.removeRows(0, qMin(nOverflowCount, m_listModel.rowCount()));
	}

	int nStartRow = m_listModel.rowCount();
	m_listModel.insertRows(nStartRow, nFlushCount);
	for (int nIndex = 0; nIndex < nFlushCount; ++nIndex)
	{
		m_listModel.setData(m_listModel.index(nStartRow + nIndex), m_listPendingLog.takeFirst());
	}

	if (m_bTrace)
	{
		m_listView->scrollTo(m_listModel.index(m_listModel.rowCount() - 1, 0));
	}
}

QCheckBox* CLogTraceWnd::createBreakCheck()
{
	auto checkBreak = new QCheckBox("触发中断");
	QObject::connect(checkBreak, &QCheckBox::stateChanged, [this](int state) {
		m_bBreakOnTrace = state != Qt::Unchecked;
	});
	return checkBreak;
}

CEventTraceWnd::CEventTraceWnd(QWidget* parent /*= nullptr*/) : CLogTraceWnd(parent, false)
{
	initWidget();
	setAttribute(Qt::WA_DeleteOnClose, true);
	QObject::connect(this, &QDialog::close, [=]() {
		for(auto pTarget : m_arrMonitorObject)
		{
			pTarget->removeEventFilter(this);
		}
	});
}

CEventTraceWnd::~CEventTraceWnd()
{
	for(auto pItem : m_hashGraphicsSpy)
	{
		delete pItem;
	}
}

bool CEventTraceWnd::MonitorWidget(QObject* object)
{
	if (object) {
		if (OTo<QGraphicsItem>(object))
		{
			QGraphicsScene* pScene = OTo<QGraphicsItem>(object)->scene();
			if(!m_hashGraphicsSpy.contains(pScene))
			{
				m_hashGraphicsSpy[pScene] = new CGraphicsItemSpy(pScene, this);
			}
	
			m_hashGraphicsSpy[pScene]->addTargetItem(OTo<QGraphicsItem>(object));
		}
		else
		{
			m_arrMonitorObject.insert(object);
			object->installEventFilter(this);
		}
		return true;
	}
	return false;
}

void CEventTraceWnd::setRunning(bool bRun)
{
	m_bRunning = bRun;
}

void CEventTraceWnd::setTargets(const QList<QObject*>& arrSelf, const QList<QObject*>& arrSubTree)
{
	m_arrTargetSelf = arrSelf;
	m_arrTargetAll = arrSubTree.isEmpty() ? arrSelf : arrSubTree;
	// 默认只监控目标自身, "包含子组件"勾选后切换到子树全集
	applyTargets(m_arrTargetSelf);
}

void CEventTraceWnd::applyTargets(const QList<QObject*>& arrTargets)
{
	resetMonitors();
	for (QObject* pTarget : arrTargets)
	{
		MonitorWidget(pTarget);
	}
}

void CEventTraceWnd::resetMonitors()
{
	for (QObject* pTarget : m_arrMonitorObject)
	{
		pTarget->removeEventFilter(this);
	}
	m_arrMonitorObject.clear();
	// CGraphicsItemSpy 析构会 removeSceneEventFilter 并从 scene 移除
	qDeleteAll(m_hashGraphicsSpy);
	m_hashGraphicsSpy.clear();
}

template <typename T>
bool CEventTraceWnd::AddInfo(T* pTarget, QEvent* event)
{
	return CLogTraceWnd::AddInfo(EventInfo(pTarget, event));
}

void CEventTraceWnd::initWidget()
{
	QHBoxLayout* pLayout = new QHBoxLayout;
	QPushButton* btnStop = new  QPushButton(m_bRunning ? "runing..." : "stoped");
	QCheckBox* filter = new QCheckBox("屏蔽事件");
	QCheckBox* pCheckSub = new QCheckBox("包含子组件");
	pLayout->addWidget(filter);
	pLayout->addWidget(pCheckSub);
	pLayout->addWidget(createBreakCheck());
	pLayout->addSpacerItem(new QSpacerItem(1 , 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
	pLayout->addWidget(btnStop);
	// 插到列表正下方(索引 1): 布局末尾已被 clear/trace 底部按钮行占据
	dynamic_cast<QVBoxLayout*>(layout())->insertLayout(1, pLayout);
	connect(filter, &QCheckBox::stateChanged, [this](int state) {
		m_bFilterEvent = state != Qt::Unchecked;
		});
	// 勾选切换监控范围: 目标自身 <-> 子树全部(原"事件跟踪All")
	connect(pCheckSub, &QCheckBox::toggled, [this](bool bChecked) {
		applyTargets(bChecked ? m_arrTargetAll : m_arrTargetSelf);
	});
	connect(btnStop, &QPushButton::clicked, [=]() {
		m_bRunning = !m_bRunning;
		btnStop->setText(m_bRunning ? "runing..." : "stoped");
	});
}

bool CEventTraceWnd::eventFilter(QObject* pObject, QEvent* event)
{
	if ( m_arrMonitorObject.contains(pObject)) {
		if (m_bRunning && AddInfo(pObject, event) && m_bFilterEvent)
		{
			return true;
		}
	}
	return QDialog::eventFilter(pObject, event);
}

bool CGraphicsItemSpy::sceneEventFilter(QGraphicsItem* watched, QEvent* event)
{
	if (m_arrMonitorItems.contains(watched)) {
		m_pWnd->AddInfo(watched, event);
	}
	return QGraphicsItem::sceneEventFilter(watched, event);
}

template<typename T>
QString CEventTraceWnd::EventInfo(T* pTarget, QEvent* event)
{
	QString strTarget = " --- ";
	strTarget += objectString(pTarget);
	if (!event) {
		return "event [NULL]" + strTarget;
	}

	QString strEventType = queryEnumName<QEvent::Type>(event->type());
	if(!strEventType.isEmpty())
	{
		QString strInfo = QString("event [%1]%2").arg(strEventType).arg(strTarget);
		switch (event->type())
		{
		case QEvent::CursorChange:
		{
			if (QWidget* pWidget = dynamic_cast<QWidget*>(pTarget))
			{
				strInfo += QString("--- ( %1 )").arg(queryEnumName<Qt::CursorShape>(pWidget->cursor().shape()));
			}

			if (QGraphicsItem* pGraphicsItem = dynamic_cast<QGraphicsItem*>(pTarget))
			{
				strInfo += QString("--- ( %1 )").arg(queryEnumName<Qt::CursorShape>(pGraphicsItem->cursor().shape()));
			}
			break;
		}
		case QEvent::MouseButtonRelease:
		case QEvent::MouseButtonPress:
		case QEvent::MouseMove:
		{
			QMouseEvent* pMouseEvent = dynamic_cast<QMouseEvent*>(event);
			QString strPos = QString("--- Pos(%1,%2) GlobalPos(%3,%4)-(%5,%6)").arg(pMouseEvent->x()).arg(pMouseEvent->y()).arg(pMouseEvent->globalX()).arg(pMouseEvent->globalY()).arg(QCursor::pos().x()).arg(QCursor::pos().y());
			strInfo += strPos;
			break;
		}
		default:
			break;
		}

		return strInfo;
	}

	if (event->type() >= QEvent::User && event->type() < QEvent::MaxUser) {
		return "event [user define]" + strTarget;
	}
	return "event [unknow]" + strTarget;
}

CSignalSpyWnd::CSignalSpy::CSignalSpy(const QObject* obj, const QMetaMethod& signal) :QSignalSpy(obj, signal)
{

}

int CSignalSpyWnd::CSignalSpy::qt_metacall(QMetaObject::Call call, int methodId, void** a)
{
	m_TraceWnd->AddInfo(QString("signal %1").arg(QString(signal())));
	return QSignalSpy::qt_metacall(call, methodId, a);
}

void CSignalSpyWnd::CSignalSpy::setTraceWnd(CLogTraceWnd* wnd)
{
	m_TraceWnd = wnd;
}

CFindWnd::CFindWnd(CSpyMainWindow* parent /*= nullptr*/)
	: CFindWnd((nullptr == parent) ? nullptr : parent->tree(), parent)
{
}

CFindWnd::CFindWnd(QTreeWidget* pTree, QWidget* parent /*= nullptr*/)
	: CXDialog(parent)
	, m_pTargetTree(pTree)
{
	initWidget();
}

CFindWnd::~CFindWnd()
{

}

void CFindWnd::initWidget()
{
	QPushButton* pBtnYes = new QPushButton("查找");
	QPushButton* pBtnNext = new QPushButton("下一个");
	QPushButton* pBtnPrev = new QPushButton("上一个");
	QPushButton* pBtnPick = new QPushButton();
	// 纯图标拾取按钮: 只保留 32x32 一份(槽位自适应缩放)
	{
		pBtnPick->setIcon(QIcon(QStringLiteral(":/icons/resource/catch.png")));
		pBtnPick->setToolTip("屏幕拾取");
	}
	m_pEdit = new QLineEdit();

	// 查找全部: 记录本次结果的关键字, 供"下一个/上一个"判断输入是否已变化
	auto fnSearchAll = [this]() {
		if (m_pTargetTree.isNull())
		{
			return;
		}

		m_strKeyword = m_pEdit->text();
		m_nCurrentIndex = 0;
		m_arrTargetItem = m_pTargetTree->findItems(m_strKeyword, Qt::MatchFlag::MatchContains | Qt::MatchRecursive);
		if (!m_arrTargetItem.empty())
		{
			selectTreeItem(m_pTargetTree.data(), m_arrTargetItem.front());
		}
	};
	// 在结果序列内跳转; 输入框内容与当前结果关键字不一致时, 直接重新查找
	auto fnJump = [this, fnSearchAll](int nStep) {
		if (m_pEdit->text() != m_strKeyword)
		{
			fnSearchAll();
			return;
		}
		if (m_arrTargetItem.empty())
		{
			return;
		}

		int nIndex = qBound(0, m_nCurrentIndex + nStep, (int)m_arrTargetItem.size() - 1);
		if (nIndex != m_nCurrentIndex)
		{
			m_nCurrentIndex = nIndex;
			selectTreeItem(m_pTargetTree.data(), m_arrTargetItem[m_nCurrentIndex]);
		}
	};

	QObject::connect(pBtnYes, &QPushButton::clicked, fnSearchAll);
	// 输入框内回车直接查找全部
	QObject::connect(m_pEdit, &QLineEdit::returnPressed, fnSearchAll);
	QObject::connect(pBtnNext, &QPushButton::clicked, [fnJump]() { fnJump(1); });
	QObject::connect(pBtnPrev, &QPushButton::clicked, [fnJump]() { fnJump(-1); });

	// 屏幕拾取: 十字光标点选屏幕上的控件, 定位其在树中的节点(右键取消), 拾取成功即关窗
	CWidgetSpyTree* pSpyTree = qobject_cast<CWidgetSpyTree*>(m_pTargetTree.data());
	if (nullptr == pSpyTree)
	{
		pBtnPick->setEnabled(false);
	}
	else
	{
		QObject::connect(pBtnPick, &QPushButton::clicked, [this, pSpyTree]() {
			CTreeCursorSearchFilter* pFilter = new CTreeCursorSearchFilter(this, pSpyTree);
			pFilter->setPickedCallback([this]() {
				// 查找窗自身是拾取宿主, 关窗前先把树所在窗口带回前台
				if (!m_pTargetTree.isNull() && (nullptr != m_pTargetTree->window()))
				{
					m_pTargetTree->window()->raise();
					m_pTargetTree->window()->activateWindow();
				}
				close();
			});
			pFilter->start();
		});
	}

	auto pLayout = new QHBoxLayout();
	pLayout->addWidget(new QLabel("文字查找"));
	pLayout->addWidget(m_pEdit);
	pLayout->addWidget(pBtnYes);
	pLayout->addWidget(pBtnNext);
	pLayout->addWidget(pBtnPrev);

	QLabel* pLabelHint = new QLabel("点击屏幕上的控件定位其在树中的位置, 右键取消");
	pLabelHint->setStyleSheet(QStringLiteral("color: %1;").arg(QtSpyTheme::palette().textSecondary.name()));
	auto pLayoutPick = new QHBoxLayout();
	pLayoutPick->addWidget(new QLabel("屏幕拾取"));
	pLayoutPick->addWidget(pBtnPick);
	pLayoutPick->addSpacing(8);
	pLayoutPick->addWidget(pLabelHint);
	pLayoutPick->addStretch();

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle("QtSpy · 查找");
	setLayout(new QVBoxLayout());
	auto pMainLayout = dynamic_cast<QVBoxLayout*>(layout());
	pMainLayout->addLayout(pLayout);
	pMainLayout->addLayout(pLayoutPick);
}
