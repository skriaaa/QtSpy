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
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowContextHelpButtonHint);
	setAttribute(Qt::WA_DeleteOnClose);
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
}

void CSpyMainWindow::clearSpyTree()
{
	m_pSpyWidget = nullptr;
	m_pSpyViewItem = nullptr;
	m_pTree->clearContent();
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
					if (!isVisible())
					{
						showCenter();
					}
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
	QAction* actionSpyTarget = new QAction("监控");
	QObject::connect(actionSpyTarget, &QAction::triggered, [&]() {
		findTarget();
		});
	QAction* actionReload = new QAction("重载");
	QObject::connect(actionReload, &QAction::triggered, [&]() {
		setTreeTarget(m_pSpyWidget);
		});
	QAction* actionNameSearch = new QAction("名称查找");
	QObject::connect(actionNameSearch, &QAction::triggered, [&]() {
		searchSpyTreeByName();
		});
	QAction* actionCursorSearch = new QAction("鼠标查找");
	QObject::connect(actionCursorSearch, &QAction::triggered, [&]() {
		searchSpyTreeByCursor();
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
	QAction* actionStatusInfo = new QAction("状态信息");
	QObject::connect(actionStatusInfo, &QAction::triggered, [&]() {
		showStatusInfo();
		});
	menuDebug->addAction(actionStatusInfo);

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
	menuBar->addAction(actionReload);
	menuBar->addAction(actionNameSearch);
	menuBar->addAction(actionCursorSearch);
	//menuBar->addAction(actionCursorLocate);
	//menuBar->addMenu(menuSetting);
	//menuBar->addMenu(menuSystem);
	menuBar->addMenu(menuDebug);
	//menuBar->addMenu(menuColor);
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

	pInfo->setWindowTitle("系统信息");
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


bool CSpyMainWindow::showStatusInfo()
{
	CStatusInfoWnd* pInfo = new CStatusInfoWnd(this);
	pInfo->showOnTop();
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


bool CSpyMainWindow::searchSpyTreeByCursor()
{
	m_eCursorAction = EScreenMouseAction::SearchWidget;
	grabMouse();
	installEventFilter(this);
	QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
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