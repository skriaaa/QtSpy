#include "SpyMainWindow.h"
#include <QtWidgets/QApplication>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeView>
#include <QTableWidget>
#include <QTableView>
#include <QScreen>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QMenuBar>
#include <QKeySequence>
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
#include <QTimer>
#include <fstream>
#include <QSignalSpy>
#include <QThread>
#include <QStackedLayout>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QButtonGroup>
#include <QGraphicsProxyWidget>
#include <dialog/qt_spydlg.h>
#include "dialog/MemoryMonitorDlg.h"
#include "publicfunction.h"
#include "qt_spygraphics.h"
#include "dialog/ObjectTree.h"
#include "qtspy.h"
const char* MAIN_WINDOW = "QtSpy_MainWindow";


CSpyMainWindow::CSpyMainWindow(QWidget* parent) :CXDialog(parent)
{
	initWindow();
	initMenuBar();
	initSpyTree();
	setGeometry(0, 0, 400, 300);
	if (nullptr != parent)
	{
		connect(parent, &QWidget::hide, this, &QWidget::hide);
	}
}

void CSpyMainWindow::initWindow()
{
	QString strTitle = "QtSpy";
	if (nullptr != parentWidget())
	{
		strTitle += QString(" - %1").arg(parentWidget()->metaObject()->className());
	}
	setWindowTitle(strTitle);
	setObjectName(MAIN_WINDOW);
	setLayout(new QVBoxLayout());
	layout()->setMargin(1);
	layout()->setSpacing(0);
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowContextHelpButtonHint);
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_QuitOnClose, false);
}

void CSpyMainWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
	{
		event->ignore();
		return;
	}
	QDialog::keyPressEvent(event);
}

bool CSpyMainWindow::eventFilter(QObject* object, QEvent* event)
{
	if (object == parent())
	{
		if(event->type() == QEvent::Hide)
		{
			hide();
		}
		if(event->type() == QEvent::Show)
		{
			show();
		}
	}

	if (selfEventFilter(object, event))
	{
		return true;
	}

	return CXDialog::eventFilter(object, event);
}

CWidgetSpyTree* CSpyMainWindow::tree()
{
	return m_pTree;
}

void CSpyMainWindow::initSpyTree()
{
	m_pTree = new CWidgetSpyTree;
	layout()->addWidget(m_pTree);

	// 底部信息栏: 捕获控件(树内节点) / 进程控件(进程内全部 QWidget)
	// 颜色唯一来源 QtSpyTheme
	m_pStatusInfo = new QLabel(this);
	m_pStatusInfo->setStyleSheet(QStringLiteral("color: %1; padding: 2px 4px;")
		.arg(QtSpyTheme::palette().textSecondary.name()));
	dynamic_cast<QVBoxLayout*>(layout())->addWidget(m_pStatusInfo);
	updateStatusInfo();

	// 进程控件数不受本窗口控制(目标程序随时增删), 定时刷新兜底;
	// 捕获/刷新/清空路径上另有即时刷新
	m_pStatusTimer = new QTimer(this);
	m_pStatusTimer->setInterval(1000);
	QObject::connect(m_pStatusTimer, &QTimer::timeout, this, &CSpyMainWindow::updateStatusInfo);
	m_pStatusTimer->start();
}

void CSpyMainWindow::updateStatusInfo()
{
	if (nullptr == m_pStatusInfo)
	{
		return;
	}

	m_pStatusInfo->setText(QString("捕获控件：%1个    进程控件：%2个")
		.arg(nullptr != m_pTree ? m_pTree->currentCount() : 0)
		.arg(qApp->allWidgets().count()));
}

void CSpyMainWindow::clearSpyTree()
{
	m_pSpyWidget = nullptr;
	m_pSpyViewItem = nullptr;
	m_pTree->clearContent();
	updateStatusInfo();
}

bool CSpyMainWindow::selfEventFilter(QObject* watched, QEvent* event)
{
	if (watched != this)
	{
		return false;
	}

	switch (event->type())
	{
		case QEvent::MouseMove:
		{
			if (EScreenMouseAction::SearchWidget != m_eCursorAction &&
				EScreenMouseAction::SpyTarget != m_eCursorAction)
			{
				break;
			}
			
			QPoint ptGlobal = (dynamic_cast<QMouseEvent*>(event))->globalPos();
			if (QWidget* pWidget = widgetAt(ptGlobal))
			{
				CSpyIndicatorWnd::showWnd(ScreenRect(pWidget));
			}
			else if(QGraphicsItem* pItem = graphicsItemAt(ptGlobal))
			{
				CSpyIndicatorWnd::showWnd(ScreenRect(pItem));
			}
			return true;
		}
		case QEvent::MouseButtonRelease:
		{
			EScreenMouseAction eAction = m_eCursorAction;
			m_eCursorAction = EScreenMouseAction::None;
			removeEventFilter(this);
			releaseMouse();
			QApplication::restoreOverrideCursor();
			CSpyIndicatorWnd::instance().hide();
			QMouseEvent* pEvent = dynamic_cast<QMouseEvent*>(event);
			if (pEvent->button() == Qt::RightButton)
			{
				return true;
			}

			QPoint ptMouse = pEvent->globalPos();
			switch (eAction)
			{
				case EScreenMouseAction::SearchWidget:
				{
					locateCursorWidget(ptMouse);
					break;
				}
				case EScreenMouseAction::SpyTarget:
				{
					setTreeTarget(ptMouse);
					break;
				}
				case EScreenMouseAction::CheckColor:
				{
					QColor pixelColor = QGuiApplication::primaryScreen()->grabWindow(0, ptMouse.x(), ptMouse.y(), 1, 1).toImage().pixel(0, 0);
					QColorDialog dialog;
					dialog.setCurrentColor(pixelColor);
					dialog.setOption(QColorDialog::ShowAlphaChannel);
					dialog.exec();
					break;
				}
				default:
					break;
			}

			// 抓取结束, 把主窗口置顶 (覆盖监控/鼠标定位/ALT+E 三种入口)
			if (!isVisible())
			{
				showCenter();           // 复用既有: show + raise + 居中
			}
			else
			{
				if (isMinimized())
				{
					showNormal();
				}
				raise();
				activateWindow();
			}
			return true;
		}
	default:
		break;
	}

	return false;
}

void CSpyMainWindow::locateCursorWidget(QPoint pt)
{
	void* pTarget = widgetAt(pt);
	if (nullptr == pTarget)
	{
		pTarget = graphicsItemAt(pt);
	}

	tree()->setCurrentSpyItem(pTarget);
}

int CSpyMainWindow::currentItemCount()
{
	return tree()->currentCount();
}

void CSpyMainWindow::showCenter()
{ 
	QPoint pt = QCursor::pos();
	if (nullptr != qApp->activeWindow())
	{
		pt = qApp->activeWindow()->geometry().center();
	}

	if (nullptr != parentWidget())
	{
		pt = parentWidget()->geometry().center();
	}

	QScreen* pCurrentScreen = qApp->screenAt(pt);
	if (nullptr == pCurrentScreen)
	{
		return;
	}

	QRect rcScreen = pCurrentScreen->availableGeometry();
	QRect rcWnd = geometry();
	rcWnd.moveCenter(rcScreen.center());
	setGeometry(rcWnd);
	show();
	raise();
}

void CSpyMainWindow::initMenuBar()
{
#ifdef Q_OS_MAC
	setNativeMenuBar(false);
#endif

	QMenuBar* menuBar = new QMenuBar();
	// 快捷键标注进菜单文字: QMenuBar 顶层动作不显示 QAction 快捷键(仅下拉 QMenu 会显示), 必须写入文本。
	// Alt+E/Alt+Q 由 CSpyWndManager 的 qApp 级过滤器全局接管(不依赖 QtSpy 窗口焦点), 菜单仅标注不重复挂;
	// F5/Ctrl+F 为新增, 走 QAction::setShortcut(默认 WindowShortcut: QtSpy 主窗有焦点才触发, 不串进目标程序)
	QAction* actionSpyTarget = new QAction("捕捉 (Alt+E)");
	QObject::connect(actionSpyTarget, &QAction::triggered, [&]() {
		findTarget();
		});
	QAction* actionReload = new QAction("刷新 (F5)");
	actionReload->setShortcut(QKeySequence(Qt::Key_F5));
	QObject::connect(actionReload, &QAction::triggered, [&]() {
		setTreeTarget(m_pSpyWidget);
		});
	QAction* actionFind = new QAction("查找 (Ctrl+F)");
	actionFind->setShortcut(QKeySequence(QStringLiteral("Ctrl+F")));
	actionFind->setToolTip("按名称或屏幕拾取定位目标在当前控件树中的位置");
	QObject::connect(actionFind, &QAction::triggered, [&]() {
		searchSpyTreeByName();
		});
	QAction* actionCursorLocate = new QAction("屏幕坐标");
	QObject::connect(actionCursorLocate, &QAction::triggered, [&]() {
		showCursorLocate();
		});
	QMenu* menuSystem = new QMenu("系统", this);
	QAction* actionSysInfo = new QAction("系统信息");
	QObject::connect(actionSysInfo, &QAction::triggered, [&]() {
		showSystemInfo();
		});
	QAction* actionSysFont = new QAction("系统字体");
	QObject::connect(actionSysFont, &QAction::triggered, [&]() {
		showSystemFont();
		});
	menuSystem->addAction(actionSysInfo);
	menuSystem->addAction(actionSysFont);


	QMenu* menuDebug = new QMenu("调试", this);
	QAction* actionMem = new QAction("内存监控");
	QObject::connect(actionMem, &QAction::triggered, [=]() {
		showMemoryMonitor();
		});
	menuDebug->addAction(actionMem);

	//QMenu* menuColor = new QMenu("颜色", this);
	//QAction* menuScreenColor = menuColor->addAction("取色");
	//QObject::connect(menuScreenColor, &QAction::triggered, [&]() {
	//	CheckColor();
	//	});
	//QAction* menuAdjustColor = menuColor->addAction("调色");
	//QObject::connect(menuAdjustColor, &QAction::triggered, []() {
	//	QColorDialog dlg;
	//	dlg.setOption(QColorDialog::ShowAlphaChannel);
	//	dlg.exec();
	//	});

	//QMenu* menuSetting = new QMenu("设置", this);
	//QMenu* menuUIStyle = menuSetting->addMenu("界面风格");
	//for (QString strStyleName : QStyleFactory::keys())
	//{
	//	QAction* actionStyle = menuUIStyle->addAction(strStyleName);
	//	QObject::connect(actionStyle, &QAction::triggered, [strStyleName]() {
	//		QApplication::setStyle(QStyleFactory::create(strStyleName));
	//		});
	//}

	menuBar->addAction(actionSpyTarget);
	menuBar->addAction(actionFind);
	menuBar->addAction(actionReload);
	//menuBar->addAction(actionCursorLocate);
	//menuBar->addMenu(menuSetting);
	//menuBar->addMenu(menuSystem);
	menuBar->addMenu(menuDebug);
	//menuBar->addMenu(menuColor);
	// 挂菜单栏动作浮窗提示(查找)
	new CMenuBarTooltipFilter(menuBar);
	layout()->setMenuBar(menuBar);
}

bool CSpyMainWindow::setTreeTarget(QPoint pt)
{
	QWidget* pTarget = QApplication::widgetAt(pt);
	if(pTarget && OTo<QGraphicsView>(pTarget->parent()))
	{
		setTreeTarget(OTo<QGraphicsView>(pTarget->parent()));
		// QGraphicsView scene中的控件 自动定位到树中的位置
		locateCursorWidget(pt);
		return true;
	}

	QWidget* target = widgetAt(pt);
	if(target)
	{
		setTreeTarget(target);
		return true;
	}

	QGraphicsItem* pItem = graphicsItemAt(pt);
	if (pItem)
	{
		setTreeTarget(pItem);
		return true;
	}

	return false;
}

bool CSpyMainWindow::setTreeTarget(QGraphicsItem* target)
{
	m_pSpyViewItem = target;
	m_pSpyWidget = nullptr;
	tree()->setTreeTarget(target);
	updateStatusInfo();

	if (nullptr != dynamic_cast<QObject*>(target))
	{
		connect(dynamic_cast<QObject*>(target), &QObject::destroyed, this, &CSpyMainWindow::clearSpyTree);
	}
	return true;
}

bool CSpyMainWindow::setTreeTarget(QWidget* target)
{
	m_pSpyWidget = target;
	m_pSpyViewItem = nullptr;
	tree()->setTreeTarget(target);
	updateStatusInfo();

	connect(target, &QObject::destroyed, this, &CSpyMainWindow::clearSpyTree);
	return true;
}

bool CSpyMainWindow::showSystemInfo()
{
	CListInfoWnd* pInfo = new CListInfoWnd(this);
	QString strSystemDPI = "unknow";
	QScreen* screen = QGuiApplication::primaryScreen();
	if (screen) {
		strSystemDPI = QString::number(screen->logicalDotsPerInch());
	}
	QString strAppStyleName = "unknow";
	if (QApplication::style()) {
		strAppStyleName = QApplication::style()->objectName();
	}
	QString strAvailStyleName = "";
	QStringList styleNames = QStyleFactory::keys();
	foreach(QString styleName, styleNames)
	{
		if (strAvailStyleName.isEmpty()) {
			strAvailStyleName = styleName;
		}
		else {
			strAvailStyleName = strAvailStyleName + "," + styleName;
		}
	}

	pInfo->setWindowTitle("QtSpy · 系统信息");
	pInfo->AddAttribute("程序构建时CPU架构", QSysInfo::buildCpuArchitecture());
	pInfo->AddAttribute("程序运行时CPU架构", QSysInfo::currentCpuArchitecture());
	pInfo->AddAttribute("程序构建时ABI规范", QSysInfo::buildAbi());
	pInfo->AddAttribute("操作系统内核类型", QSysInfo::kernelType());
	pInfo->AddAttribute("操作系统内核版本", QSysInfo::kernelVersion());
	pInfo->AddAttribute("操作系统产品类型", QSysInfo::productType());
	pInfo->AddAttribute("操作系统产品版本", QSysInfo::productVersion());
	pInfo->AddAttribute("操作系统产品名称", QSysInfo::prettyProductName());
	pInfo->AddAttribute("计算机名称", QSysInfo::machineHostName());
	pInfo->AddAttribute("屏幕数量", QString("%1").arg(QGuiApplication::screens().count()));
	//auto rcVirtualScreen = GetUICore().uilVirtualScreenRect();
	//pInfo->AddInfo("虚拟屏幕", QString("(%1,%2,%3,%4)").arg(rcVirtualScreen.left).arg(rcVirtualScreen.top).arg(rcVirtualScreen.right).arg(rcVirtualScreen.bottom));
	pInfo->AddAttribute("窗口系统", QGuiApplication::platformName());
	pInfo->AddAttribute("可用界面风格", strAvailStyleName);
	pInfo->AddAttribute("当前界面风格", strAppStyleName);
	pInfo->AddAttribute("DPI", strSystemDPI);
	pInfo->exec();
	return true;
}


bool CSpyMainWindow::showMemoryMonitor()
{
	CMemoryMonitorDlg* pMemMonitorDlg = new CMemoryMonitorDlg(this);
	pMemMonitorDlg->showOnTop();
	return true;
}

bool CSpyMainWindow::showSystemFont()
{
	QFontDialog fontDialog;
	fontDialog.exec();
	return true;
}


bool CSpyMainWindow::searchSpyTreeByName()
{
	CFindWnd* pFindWnd = new CFindWnd(this);
	pFindWnd->show();
	return true;
}


bool CSpyMainWindow::showCursorLocate()
{
	CCursorLocateWnd dlg;
	dlg.exec();
	return true;
}


bool CSpyMainWindow::findTarget()
{
	m_eCursorAction = EScreenMouseAction::SpyTarget;
	grabMouse();
	installEventFilter(this);
	setMouseTracking(true);
	QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
	return true;
}

bool CSpyMainWindow::checkColor()
{
	m_eCursorAction = EScreenMouseAction::CheckColor;
	grabMouse();
	installEventFilter(this);
	QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
	return true;
}
