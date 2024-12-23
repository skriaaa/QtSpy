#include "qt_spyobject.h"
#include <QtWidgets/QApplication>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QTreeWidget>
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
CSpyMainWindow::CSpyMainWindow(QWidget* parent):QDialog(parent)
{
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(0, 0, 0, 0);

	setWindowFlags(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
	setAttribute(Qt::WA_DeleteOnClose);
#ifdef Q_OS_MAC
	setNativeMenuBar(false);
#endif
}

CQtSpyObject* CSpyMainWindow::spyObject()
{
	return m_pSpyObject;
}

void CSpyMainWindow::initWindow(CQtSpyObject* object)
{
	m_pSpyObject = object;
	setWindowTitle(_QStr("qt spy"));
	setObjectName(MAIN_WINDOW);
	initSpyTree();

	setGeometry(
		QStyle::alignedRect(
			Qt::LeftToRight,
			Qt::AlignCenter,
			QSize(400, 300),
			qApp->primaryScreen()->availableGeometry()
		)
	);
}

void CSpyMainWindow::setMenuBar(QMenuBar* menuBar)
{
	layout()->setMenuBar(menuBar);
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


CWidgetSpyTree* CSpyMainWindow::tree()
{
	return m_pTree;
}

void CSpyMainWindow::initSpyTree()
{
	m_pTree = new CWidgetSpyTree;
	layout()->addWidget(m_pTree);
	QTreeWidgetItem* rootNode = new QTreeWidgetItem();
	m_pTree->addTopLevelItem(rootNode);
}

void CSpyMainWindow::clearSpyTree()
{
	m_pTree->clear();
}

bool CQtSpyObject::eventFilter(QObject* watched, QEvent* event)
{
	if(watched->objectName().compare(QString(MAIN_WINDOW), Qt::CaseInsensitive) != 0)
	{
		return QObject::eventFilter(watched, event);
	}

	switch(event->type())
	{
		case QEvent::MouseMove:
		{
			if(EScreenMouseAction::SearchWidget == m_eCursorAction ||
				EScreenMouseAction::SpyTarget == m_eCursorAction)
			{
				CSpyIndicatorWnd::showWnd(ScreenRect(widgetAt((dynamic_cast<QMouseEvent*>(event))->globalPos())));
				return true;
			}
			break;
		}
		case QEvent::MouseButtonRelease:
		{
			EScreenMouseAction eAction = m_eCursorAction;
			m_eCursorAction = EScreenMouseAction::None;
			CSpyMainWindow* mainWindow = dynamic_cast<CSpyMainWindow*>(watched);
			mainWindow->removeEventFilter(this);
			mainWindow->releaseMouse();
			QApplication::restoreOverrideCursor();
			CSpyIndicatorWnd::instance().hide();
			QMouseEvent* pEvent = dynamic_cast<QMouseEvent*>(event);
			if(pEvent->button() == Qt::RightButton)
			{
				break;
			}

			QPoint ptMouse = pEvent->globalPos();
			switch (eAction)
			{
				case EScreenMouseAction::SearchWidget:
				{
					QWidget* pTarget = widgetAt(ptMouse);
					if (m_mapWidgetNode.contains(pTarget))
					{
						m_pMainWindow->tree()->setCurrentItem(m_mapWidgetNode[pTarget]);
						m_mapWidgetNode[pTarget]->setExpanded(true);
					}
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
		}
	default:
		break;
	}

	return QObject::eventFilter(watched, event);
}

CQtSpyObject::CQtSpyObject(QWidget* parent):QObject(nullptr)
{
	setParent(parent);
	m_pSpyWidget = nullptr;
	m_pMainWindow = new CSpyMainWindow();
	initToolWindow();
}

CQtSpyObject::~CQtSpyObject()
{
	m_pMainWindow->close();
}

bool CQtSpyObject::initToolWindow()
{
	QMenuBar* menuBar = new QMenuBar();
	QAction* actionSpyTarget = new QAction(_QStr("监控"));
	QObject::connect(actionSpyTarget, &QAction::triggered, [&]() {
		SpyTargetTree();
		});
	QAction* actionReload = new QAction(_QStr("重载"));
	QObject::connect(actionReload, &QAction::triggered, [&]() {
		setTreeTarget(m_pSpyWidget);
		});
	QAction* actionNameSearch = new QAction(_QStr("名称查找"));
	QObject::connect(actionNameSearch, &QAction::triggered, [&]() {
		SearchSpyTreeByName();
		});
	QAction* actionCursorSearch = new QAction(_QStr("鼠标查找"));
	QObject::connect(actionCursorSearch, &QAction::triggered, [&]() {
		SearchSpyTreeByCursor();
		});
	QAction* actionCursorLocate = new QAction(_QStr("鼠标定位"));
	QObject::connect(actionCursorLocate, &QAction::triggered, [&]() {
		ShowCursorLocate();
		});
	QMenu* menuSystem = new QMenu(_QStr("系统"));
	QAction* actionSysInfo = new QAction(_QStr("系统信息"));
	QObject::connect(actionSysInfo, &QAction::triggered, [&]() {
		ShowSystemInfo();
		});
	QAction* actionSysFont = new QAction(_QStr("系统字体"));
	QObject::connect(actionSysFont, &QAction::triggered, [&]() {
		ShowSystemFont();
		});
	menuSystem->addAction(actionSysInfo);
	menuSystem->addAction(actionSysFont);


	QMenu* menuDebug = new QMenu(_QStr("调试"));
	QAction* actionStatusInfo = new QAction(_QStr("状态信息"));
	QObject::connect(actionStatusInfo, &QAction::triggered, [&]() {
		ShowStatusInfo();
		});
	menuDebug->addAction(actionStatusInfo);

	QAction* actionMem = new QAction("内存监控");
	QObject::connect(actionMem, &QAction::triggered, [=]() {
		showMemoryMonitor();
		});
	menuDebug->addAction(actionMem);


	QMenu* menuResource = new QMenu(_QStr("资源"));
	QAction* menuResourceManage = menuResource->addAction("管理");
	QObject::connect(menuResourceManage, &QAction::triggered, []() {
		//CResourceManageWnd dlg;
		//dlg.exec();
		});
	QAction* menuLookup = menuResource->addAction("查看");
	QObject::connect(menuLookup, &QAction::triggered, []() {
		//CResourceCheckWnd dlg;
		//dlg.exec();
		});

	QMenu* menuColor = new QMenu(_QStr("颜色"));
	QAction* menuScreenColor = menuColor->addAction("取色");
	QObject::connect(menuScreenColor, &QAction::triggered, [&]() {
		CheckColor();
		});
	QAction* menuAdjustColor = menuColor->addAction("调色");
	QObject::connect(menuAdjustColor, &QAction::triggered, []() {
		QColorDialog dlg;
		dlg.setOption(QColorDialog::ShowAlphaChannel);
		dlg.exec();
		});

	QMenu* menuSetting = new QMenu(_QStr("设置"));
	QMenu* menuUIStyle = menuSetting->addMenu("界面风格");
	for (QString strStyleName : QStyleFactory::keys())
	{
		QAction* actionStyle = menuUIStyle->addAction(strStyleName);
		QObject::connect(actionStyle, &QAction::triggered, [strStyleName]() {
			QApplication::setStyle(QStyleFactory::create(strStyleName));
			});
	}
	// 暂时屏蔽 skria
	//QMenu* menuSimulateDPI = menuSetting->addMenu("模拟DPI");
	//QObject::connect(menuSimulateDPI->addAction("重置系统值"), &QAction::triggered, []() {
	//	QScreen* screen = QGuiApplication::primaryScreen();
	//	if (screen) {
	//		GetCoreInst().uilSetDPI(screen->logicalDotsPerInch());
	//	}
	//	});
	//QObject::connect(menuSimulateDPI->addAction("96 100%"), &QAction::triggered, []() {
	//	GetCoreInst().uilSetDPI(96);
	//	});
	//QObject::connect(menuSimulateDPI->addAction("120 125%"), &QAction::triggered, []() {
	//	GetCoreInst().uilSetDPI(120);
	//	});
	//QObject::connect(menuSimulateDPI->addAction("144 150%"), &QAction::triggered, []() {
	//	GetCoreInst().uilSetDPI(144);
	//	});
	//QObject::connect(menuSimulateDPI->addAction("168 175%"), &QAction::triggered, []() {
	//	GetCoreInst().uilSetDPI(168);
	//	});
	//QObject::connect(menuSimulateDPI->addAction("192 200%"), &QAction::triggered, []() {
	//	GetCoreInst().uilSetDPI(192);
	//	});
	menuBar->addAction(actionSpyTarget);
	menuBar->addAction(actionReload);
	menuBar->addAction(actionNameSearch);
	menuBar->addAction(actionCursorSearch);
	menuBar->addAction(actionCursorLocate);
	menuBar->addMenu(menuSystem);
	menuBar->addMenu(menuDebug);
	menuBar->addMenu(menuResource);
	menuBar->addMenu(menuColor);
	menuBar->addMenu(menuSetting);
	m_pMainWindow->setMenuBar(menuBar);


	m_pMainWindow->initWindow(this);
	m_pMainWindow->raise();
	m_pMainWindow->show();
	return true;
}

bool CQtSpyObject::setTreeTarget(QPoint pt)
{
	QWidget* pTarget = QApplication::widgetAt(pt);
	if(pTarget && OTo<QGraphicsView>(pTarget->parent()))
	{
		setTreeTarget(OTo<QGraphicsView>(pTarget->parent()));
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

bool CQtSpyObject::setTreeTarget(QGraphicsItem* target)
{
	m_pSpyViewItem = target;
	m_pSpyWidget = nullptr;

	m_pMainWindow->tree()->setTreeTarget(target);
	m_mapWidgetNode = m_pMainWindow->tree()->m_mapWidgetNode;
	return true;
}

bool CQtSpyObject::setTreeTarget(QWidget* target)
{
	m_pSpyWidget = target;
	m_pSpyViewItem = nullptr;

	m_pMainWindow->tree()->setTreeTarget(target);
	m_mapWidgetNode = m_pMainWindow->tree()->m_mapWidgetNode;
	return true;
}

bool CQtSpyObject::ShowSystemInfo()
{
	CListInfoWnd* pInfo = new CListInfoWnd();
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

	pInfo->setWindowTitle(_QStr("系统信息"));
	pInfo->AddAttribute(_QStr("程序构建时CPU架构"), QSysInfo::buildCpuArchitecture());
	pInfo->AddAttribute(_QStr("程序运行时CPU架构"), QSysInfo::currentCpuArchitecture());
	pInfo->AddAttribute(_QStr("程序构建时ABI规范"), QSysInfo::buildAbi());
	pInfo->AddAttribute(_QStr("操作系统内核类型"), QSysInfo::kernelType());
	pInfo->AddAttribute(_QStr("操作系统内核版本"), QSysInfo::kernelVersion());
	pInfo->AddAttribute(_QStr("操作系统产品类型"), QSysInfo::productType());
	pInfo->AddAttribute(_QStr("操作系统产品版本"), QSysInfo::productVersion());
	pInfo->AddAttribute(_QStr("操作系统产品名称"), QSysInfo::prettyProductName());
	pInfo->AddAttribute(_QStr("计算机名称"), QSysInfo::machineHostName());
	pInfo->AddAttribute(_QStr("屏幕数量"), QString("%1").arg(QGuiApplication::screens().count()));
	//auto rcVirtualScreen = GetUICore().uilVirtualScreenRect();
	//pInfo->AddInfo(_QStr("虚拟屏幕"), QString("(%1,%2,%3,%4)").arg(rcVirtualScreen.left).arg(rcVirtualScreen.top).arg(rcVirtualScreen.right).arg(rcVirtualScreen.bottom));
	pInfo->AddAttribute(_QStr("窗口系统"), QGuiApplication::platformName());
	pInfo->AddAttribute(_QStr("可用界面风格"), strAvailStyleName);
	pInfo->AddAttribute(_QStr("当前界面风格"), strAppStyleName);
	pInfo->AddAttribute(_QStr("DPI"), strSystemDPI);
	pInfo->exec();
	return true;
}


bool CQtSpyObject::ShowStatusInfo()
{
	CStatusInfoWnd* pInfo = new CStatusInfoWnd();
	pInfo->ShowOnTop();
	return true;
}

bool CQtSpyObject::showMemoryMonitor()
{
	CMemoryMonitorDlg* pMemMonitorDlg = new CMemoryMonitorDlg();
	pMemMonitorDlg->ShowOnTop();
	return true;
}

bool CQtSpyObject::ShowSystemFont()
{
	QFontDialog fontDialog;
	fontDialog.exec();
	return true;
}


bool CQtSpyObject::SearchSpyTreeByName()
{
	QDialog &dlg = *(new QDialog());
	dlg.setAttribute(Qt::WA_DeleteOnClose);
	dlg.setWindowTitle(_QStr("查找"));
	dlg.setLayout(new QVBoxLayout());
	auto layout = dynamic_cast<QVBoxLayout*>(dlg.layout());
	auto layout1 = new QHBoxLayout();
	QPushButton* pBtnYes, * pBtnNext, * pBtnPrev;
	QLineEdit* pEdit;
	layout1->addWidget(new QLabel("对象名称"));
	layout1->addWidget(pEdit = new QLineEdit());
	layout1->addWidget(pBtnYes = new QPushButton("查找全部"));
	layout1->addWidget(pBtnNext = new QPushButton("下一个"));
	layout1->addWidget(pBtnPrev = new QPushButton("上一个"));
	QList<QWidget*> arrTargetItem;
	int nCurrentIndex = 0;
	QObject::connect(pBtnYes, &QPushButton::clicked, [&]() {
		arrTargetItem.clear();
		QString strName = pEdit->text();
		if (m_pSpyWidget) {
			for (auto pWidget : m_mapWidgetNode.keys()) {
				if (pWidget->objectName().contains(strName, Qt::CaseInsensitive)
					|| QString(pWidget->metaObject()->className()).contains(strName, Qt::CaseInsensitive))
				{
					arrTargetItem.append(pWidget);
				}
			}
			if (!arrTargetItem.empty())
			{
				m_mapWidgetNode[arrTargetItem.front()]->setExpanded(true);
				m_pMainWindow->tree()->setCurrentItem(m_mapWidgetNode[arrTargetItem.front()]);
			}
		}
		});
	QObject::connect(pBtnNext, &QPushButton::clicked, [&]() {
		if(arrTargetItem.size() - 1 > nCurrentIndex)
		{
			nCurrentIndex++;
			QWidget* pWidget = *(arrTargetItem.begin() + nCurrentIndex);
			m_mapWidgetNode[pWidget]->setExpanded(true);
			m_pMainWindow->tree()->setCurrentItem(m_mapWidgetNode[pWidget]);
		}
		});
	QObject::connect(pBtnPrev, &QPushButton::clicked, [&]() {
		if (nCurrentIndex > 0)
		{
			nCurrentIndex--;
			QWidget* pWidget = *(arrTargetItem.begin() + nCurrentIndex);
			m_mapWidgetNode[pWidget]->setExpanded(true);
			m_pMainWindow->tree()->setCurrentItem(m_mapWidgetNode[pWidget]);
		}
	});
	layout->addLayout(layout1);
	dlg.show();
	return true;
}


bool CQtSpyObject::SearchSpyTreeByCursor()
{
	m_eCursorAction = EScreenMouseAction::SearchWidget;
	m_pMainWindow->grabMouse();
	m_pMainWindow->installEventFilter(this);
	QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
	return true;
}


bool CQtSpyObject::ShowCursorLocate()
{
	CCursorLocateWnd dlg;
	dlg.exec();
	return true;
}


bool CQtSpyObject::SpyTargetTree()
{
	m_eCursorAction = EScreenMouseAction::SpyTarget;
	m_pMainWindow->grabMouse();
	m_pMainWindow->installEventFilter(this);
	m_pMainWindow->setMouseTracking(true);
	QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
	return true;
}

bool CQtSpyObject::CheckColor()
{
	m_eCursorAction = EScreenMouseAction::CheckColor;
	m_pMainWindow->grabMouse();
	m_pMainWindow->installEventFilter(this);
	QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
	return true;
}