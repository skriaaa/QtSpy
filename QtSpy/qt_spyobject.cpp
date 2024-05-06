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
#include <qt_spydlg.h>
#include "publicfunction.h"
#include <QThread>
#include <QStackedLayout>
#include <QGraphicsView>
#include <QGraphicsItem>
#include "qt_spygraphics.h"

CSpyMainWindow::CSpyMainWindow()
{
	setLayout(new QVBoxLayout());
	if (layout()) {
		layout()->setContentsMargins(0, 0, 0, 0);
	}
	//setWindowFlag(Qt::WindowStaysOnTopHint);
	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
}

void CSpyMainWindow::initWindow()
{
	setWindowTitle(_QStr("qt spy"));
	setObjectName(_QStr("mainwindow"));
	setWindowFlag(Qt::WindowTitleHint);
	setWindowFlag(Qt::WindowSystemMenuHint);
	setWindowFlag(Qt::WindowMinimizeButtonHint);
	setWindowFlag(Qt::WindowMaximizeButtonHint);

	initSpyTree();

	setGeometry(
		QStyle::alignedRect(
			Qt::LeftToRight,
			Qt::AlignCenter,
			size(),
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
	if (event->type() == Qt::Key_Escape)
	{
		return;
	}
	QDialog::keyPressEvent(event);
}


QTreeWidget* CSpyMainWindow::tree()
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

CQtSpyObject& CQtSpyObject::GetInstance()
{
	static CQtSpyObject spy;
	return spy;
}


bool CQtSpyObject::eventFilter(QObject* watched, QEvent* event)
{
	if (watched->objectName().compare(QString("mainwindow"), Qt::CaseInsensitive) == 0) {
		if (m_eCursorAction == EScreenMouseAction::SearchWidget) {
			if (event->type() == QEvent::MouseButtonRelease) {
				CSpyMainWindow* mainWindow = dynamic_cast<CSpyMainWindow*>(watched);
				QMouseEvent* typeEvent = dynamic_cast<QMouseEvent*>(event);
				QPoint ptMouse = typeEvent->globalPos();
				qDebug() << "query widget at point:" << ptMouse;
				QWidget* pTarget = QApplication::widgetAt(ptMouse);
				mainWindow->removeEventFilter(this);
				mainWindow->releaseMouse();
				QApplication::restoreOverrideCursor();
				m_eCursorAction = EScreenMouseAction::None;
				if (pTarget) {
					qDebug() << pTarget;
					if (m_pSpyWidget) {
						for (auto it : m_mapWidgetNode) {
							if (it.first == pTarget) {
								m_pMainWindow->tree()->setCurrentItem(it.second);
							}
						}
					}
				}
				return true;
			}
		}
		if (m_eCursorAction == EScreenMouseAction::SpyTarget) {
			if (event->type() == QEvent::MouseButtonRelease) {
				CSpyMainWindow* mainWindow = dynamic_cast<CSpyMainWindow*>(watched);
				QMouseEvent* typeEvent = dynamic_cast<QMouseEvent*>(event);
				QPoint ptMouse = typeEvent->globalPos();
				qDebug() << "query widget at point:" << ptMouse;
				mainWindow->removeEventFilter(this);
				mainWindow->releaseMouse();
				QApplication::restoreOverrideCursor();
				m_eCursorAction = EScreenMouseAction::None;
				setTreeTarget(ptMouse);
				return true;
			}
		}
		if (m_eCursorAction == EScreenMouseAction::CheckColor) {
			if (event->type() == QEvent::MouseButtonRelease) {
				CSpyMainWindow* mainWindow = dynamic_cast<CSpyMainWindow*>(watched);
				QMouseEvent* typeEvent = dynamic_cast<QMouseEvent*>(event);
				QPoint ptMouse = typeEvent->globalPos();
				QScreen* screen = QGuiApplication::primaryScreen();
				QColor pixelColor = screen->grabWindow(0, ptMouse.x(), ptMouse.y(), 1, 1).toImage().pixel(0, 0);
				mainWindow->removeEventFilter(this);
				mainWindow->releaseMouse();
				QApplication::restoreOverrideCursor();
				m_eCursorAction = EScreenMouseAction::None;
				if (pixelColor.isValid()) {
					QColorDialog dialog;
					dialog.setCurrentColor(pixelColor);
					dialog.setOption(QColorDialog::ShowAlphaChannel);
					dialog.exec();
				}
				return true;
			}
		}
	}
	return QObject::eventFilter(watched, event);
}

CQtSpyObject::CQtSpyObject()
{
	m_pSpyWidget = nullptr;
	m_pMainWindow = new CSpyMainWindow();
}

bool CQtSpyObject::StartSpy(QWidget* object)
{
	initToolWindow();
	return true;
}


bool CQtSpyObject::ShutdownSpy()
{
	if (m_pMainWindow) {
		m_pMainWindow->close();
		delete m_pMainWindow;
		m_pMainWindow = nullptr;
	}
	return true;
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
	QMenu* menuSimulateDPI = menuSetting->addMenu("模拟DPI");
	// 暂时屏蔽 skria
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


	m_pMainWindow->initWindow();
	m_pMainWindow->raise();
	m_pMainWindow->show();
	return true;
}

bool CQtSpyObject::setTreeTarget(QPoint pt)
{
	ClearSpyTree();

	QWidget* target = QApplication::widgetAt(pt);

	if (OTo<QGraphicsView>(target))
	{
		QGraphicsItem* item = OTo<QGraphicsView>(target)->scene()->itemAt(pt, QTransform());
		setTreeTarget(item);
	}
	else
	{
		setTreeTarget(target);
	}
	return true;
}

bool CQtSpyObject::setTreeTarget(QGraphicsItem* target)
{
	ClearSpyTree();

	m_pSpyViewItem = target;
	m_pSpyWidget = nullptr;

	QTreeWidgetItem* root = new QTreeWidgetItem;
	m_pMainWindow->tree()->addTopLevelItem(root);
	AddSubSpyNode(target, root);
	return true;
}

bool CQtSpyObject::setTreeTarget(QWidget* target)
{
	ClearSpyTree();

	m_pSpyWidget = target;
	m_pSpyViewItem = nullptr;

	QTreeWidgetItem* root = new QTreeWidgetItem;
	m_pMainWindow->tree()->addTopLevelItem(root);
	AddSubSpyNode(target, root);
	return true;
}

bool CQtSpyObject::AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode) {
	if (parent && parentNode) {
		m_mapWidgetNode.insert(std::make_pair(parent, parentNode));
		parentNode->setText(0, ObjectString(parent));
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


bool CQtSpyObject::AddSubSpyNode(QGraphicsItem* parent, QTreeWidgetItem* parentNode)
{
	if (parent && parentNode) {
		parentNode->setText(0, ObjectString(To<QObject>(parent)));
		parentNode->setData(0, Qt::UserRole, QVariant::fromValue(parent));
		QList<QGraphicsItem*> children = parent->childItems();
		for (QGraphicsItem* child : children) {
			QTreeWidgetItem* treenode = new QTreeWidgetItem();
			parentNode->addChild(treenode);
			AddSubSpyNode(child, treenode);
		}
	}
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

bool CQtSpyObject::ShowSystemFont()
{
	QFontDialog fontDialog;
	fontDialog.exec();
	return true;
}


bool CQtSpyObject::SearchSpyTreeByName()
{
	QDialog dlg;
	dlg.setWindowTitle(_QStr("查找"));
	dlg.setLayout(new QVBoxLayout());
	auto layout = dynamic_cast<QVBoxLayout*>(dlg.layout());
	auto layout1 = new QHBoxLayout();
	auto layout2 = new QHBoxLayout();
	auto layout3 = new QHBoxLayout();
	layout1->addWidget(new QLabel("对象名称"));
	layout1->addWidget(new QLineEdit());
	layout1->addWidget(new QPushButton("确定"));
	QObject::connect(dynamic_cast<QPushButton*>(layout1->itemAt(2)->widget()), &QPushButton::clicked, [&]() {
		QString name = dynamic_cast<QLineEdit*>(layout1->itemAt(1)->widget())->text();
		if (m_pSpyWidget) {
			for (auto it : m_mapWidgetNode) {
				if (it.first->objectName().compare(name) == 0) {
					m_pMainWindow->tree()->setCurrentItem(it.second);
				}
			}
		}
		dlg.close();
		});
	layout->addLayout(layout1);
	layout->addLayout(layout2);
	layout->addLayout(layout3);
	dlg.exec();
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


bool CQtSpyObject::ClearSpyTree()
{
	m_pMainWindow->tree()->clear();
	m_mapWidgetNode.clear();
	return true;
}
