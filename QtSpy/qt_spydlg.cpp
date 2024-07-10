#include "qt_spydlg.h"
#include <QSignalSpy>
#include <QMenu>
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
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsScene>
//#include "QtCore/5.14.2/QtCore/private/qobject_p.h"
#include "publicfunction.h"
#include "proxyStyle/ProxyStyle.h"

CSpyIndicatorWnd::CSpyIndicatorWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
	setStyleSheet("background-color: rgba(255, 0, 0, 255);");
	setWindowOpacity(0.3);
	QTimer* timer = new QTimer(this);
	m_nSpanPeriod = 30;
	QObject::connect(timer, &QTimer::timeout, [&]() {
		if (this->m_nSpanPeriod == 0) {
			this->close();
		}
		this->setWindowOpacity((float)m_nSpanPeriod / 30);
		this->repaint();
		m_nSpanPeriod--;
		});
	timer->start(100);
}

CMoveOrScaleWidgetWnd::CMoveOrScaleWidgetWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	this->setWindowTitle("移动&缩放");
	m_pEditMoveStep = new QLineEdit();
	m_pEditScaleStep = new QLineEdit();
	auto layout = new QVBoxLayout();
	auto layout1 = new QHBoxLayout();
	auto btnMoveUp = new QPushButton("向上移动");
	auto btnMoveDown = new QPushButton("向下移动");
	auto btnMoveLeft = new QPushButton("向左移动");
	auto btnMoveRight = new QPushButton("向右移动");
	layout1->addWidget(m_pEditMoveStep);
	layout1->addWidget(btnMoveUp);
	layout1->addWidget(btnMoveDown);
	layout1->addWidget(btnMoveLeft);
	layout1->addWidget(btnMoveRight);
	m_pEditMoveStep->setText("1");
	QObject::connect(m_pEditMoveStep, &QLineEdit::textChanged, [&]() {
		if (m_pEditMoveStep) {
			m_nMoveStep = m_pEditMoveStep->text().toInt();
		}
		});
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
	auto layout2 = new QHBoxLayout();
	auto btnScaleUp = new QPushButton("缩放上边界");
	auto btnScaleDown = new QPushButton("缩放下边界");
	auto btnScaleLeft = new QPushButton("缩放左边界");
	auto btnScaleRight = new QPushButton("缩放右边界");
	layout2->addWidget(m_pEditScaleStep);
	layout2->addWidget(btnScaleUp);
	layout2->addWidget(btnScaleDown);
	layout2->addWidget(btnScaleLeft);
	layout2->addWidget(btnScaleRight);
	m_pEditScaleStep->setText("1");
	QObject::connect(m_pEditScaleStep, &QLineEdit::textChanged, [&]() {
		if (m_pEditScaleStep) {
			m_nScaleStep = m_pEditScaleStep->text().toInt();
		}
		});
	QObject::connect(btnScaleUp, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, -m_nScaleStep, 0, 0);
			m_pTargetWidget->setGeometry(rc);
		}
		});
	QObject::connect(btnScaleDown, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, 0, 0, m_nScaleStep);
			m_pTargetWidget->setGeometry(rc);
		}
		});
	QObject::connect(btnScaleLeft, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(-1 * m_nScaleStep, 0, 0, 0);
			m_pTargetWidget->setGeometry(rc);
		}
		});
	QObject::connect(btnScaleRight, &QPushButton::clicked, [&] {
		if (m_pTargetWidget) {
			QRect rc = m_pTargetWidget->geometry();
			rc.adjust(0, 0, m_nScaleStep, 0);
			m_pTargetWidget->setGeometry(rc);
		}
		});
	layout->addLayout(layout1);
	layout->addLayout(layout2);
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
	setWindowFlags(windowFlags() | Qt::Popup | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint | Qt::CustomizeWindowHint);
	resize(320, 380);
	setLayout(new QVBoxLayout());

	InitTableWidget();
}

void CListInfoWnd::InitTableWidget()
{
	m_pTableWidget = new QTableWidget();
	this->layout()->addWidget(m_pTableWidget);

	QStringList headers;
	headers << _QStr("属性") << _QStr("值");
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
	QString strText = ObjectString(target);

	setWindowTitle(strText);
	setContent();

	ParseSignal(target);
}

void CSignalSpyWnd::ParseSignal(QObject* target)
{
	int row = m_pSignalTable->rowCount();
	//QObjectPrivate* sp = QObjectPrivate::get(target);
	//{
	//	QObjectPrivate::ConnectionDataPointer connections(sp->connections.loadRelaxed());
	//	QObjectPrivate::SignalVector* signalVector = connections->signalVector.loadRelaxed();
	//	for (int i = 0; i < signalVector->count(); ++i)
	//	{
	//		const QObjectPrivate::ConnectionList& list = signalVector->at(i);
	//		QAtomicPointer<QObjectPrivate::Connection> curConnection = list.first;
	//		QObject* obj = curConnection.loadRelaxed()->receiver.loadRelaxed();
	//		QString str = obj->metaObject()->className();
	//		int a = 0;
	//	}
	//}

	QTableWidgetItem* item = new QTableWidgetItem;
	auto fnQuerySignalSlot = [&](const QMetaObject* metaObject, QObject* widget) {
		for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
			QMetaMethod method = metaObject->method(i);
			QString strSignature1 = QString("%1::%2").arg(metaObject->className()).arg(QString(method.methodSignature()));
			if (method.methodType() == QMetaMethod::Signal) {
				QString strSignature = QString("%1::%2").arg(metaObject->className()).arg(QString(method.methodSignature()));

				//pInfo->AddInfo(strSignature, row++, 0);
				//dynamic_cast<QObject*>(widget)->receivers(SIGNAL(clicked(bool)));

			}
			if (method.methodType() == QMetaMethod::Slot) {
				QString strSignature = QString("%1::%2").arg(metaObject->className()).arg(QString(method.methodSignature()));
				//pInfo->AddInfo(strSignature, slotIndex++, 1);
			}
		}
	};
}

void CSignalSpyWnd::initWidgets()
{
	resize(800, 600);
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::Popup | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint | Qt::CustomizeWindowHint);
	setLayout(new QVBoxLayout);

	initTableWidget();

	QTabWidget* tab = new QTabWidget;
	layout()->addWidget(tab);
	tab->addTab(m_pSignalTable, "信号");
	tab->addTab(m_pSlotTable, "槽");
}

void CSignalSpyWnd::initTableWidget()
{
	auto fnSetTable = [](QTableWidget*& table, const QStringList& titles) {
		table = new QTableWidget;
		table->setColumnCount(titles.size());
		table->setHorizontalHeaderLabels(titles);
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	};

	fnSetTable(m_pSignalTable, { "信号", "槽", "接收对象" });
	fnSetTable(m_pSlotTable, { "槽", "信号", "接收对象" });
}

void CSignalSpyWnd::initContextMenu()
{
	m_pSignalTable->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_pSignalTable, &QTableWidget::customContextMenuRequested, [&]() {
		QMenu contextMenu(m_pSignalTable);
		QAction* acEmit = contextMenu.addAction("发送信号");
		QAction* acInspectThis = contextMenu.addAction("监控信号");
		QAction* acInspectAll = contextMenu.addAction("监控所有信号");
		QAction* acQueryConnects = contextMenu.addAction("查看所有连接");

		QPoint pt = QCursor().pos();
		QAction* acbk = contextMenu.exec(pt);
		if (acbk == acEmit)
		{
			QTableWidgetItem* item = m_pSignalTable->itemAt(mapFromGlobal(pt));
			assert(item);
			if (item)
			{
				QMetaMethod* method = static_cast<QMetaMethod*>(item->data(Qt::UserRole).value<void*>());
				emit method->methodSignature();
			}
		}
		if (acbk == acInspectThis)
		{
			QTableWidgetItem* item = m_pSignalTable->itemAt(mapFromGlobal(pt));
			if (item)
			{
				QMetaMethod* method = static_cast<QMetaMethod*>(item->data(Qt::UserRole).value<void*>());
				if (m_arrSignal[method] == nullptr)
				{
					CSignalSpy* spy = new CSignalSpy(m_pTargetObject, *method);
					spy->setTraceWnd(traceWnd());
				}
				traceWnd()->ShowOnTop();
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
			traceWnd()->ShowOnTop();
		}
		});
}

void CSignalSpyWnd::addItem(QTableWidget* table, int row, int col, const QMetaObject* metaObject, QMetaMethod* method)
{
	if (method == nullptr)
		return;

	if (table->rowCount() <= row)
	{
		table->insertRow(row);
	}

	QString strSignature = QString("%1::%2").arg(metaObject->className()).arg(QString(method->methodSignature()));
	QTableWidgetItem* item = new QTableWidgetItem;
	item->setText(strSignature);
	item->setData(Qt::UserRole, QVariant::fromValue((void*)(method)));
	table->setItem(row, col, item);
}

void CSignalSpyWnd::setContent()
{
	clearContent();
	auto fnQuerySignalSlot = [&](const QMetaObject* metaObject) {
		int rowSignal = m_pSignalTable->rowCount();
		int rowSlot = m_pSlotTable->rowCount();
		for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
			QMetaMethod&& method = metaObject->method(i);
			if (method.methodType() == QMetaMethod::Signal)
			{
				auto ret = m_arrSignal.insert({ new QMetaMethod(method), nullptr });
				addItem(m_pSignalTable, rowSignal++, 0, metaObject, ret.first->first);
			}

			if (method.methodType() == QMetaMethod::Slot)
			{
				addItem(m_pSlotTable, rowSlot++, 0, metaObject, &method);
			}
		}
	};

	const QMetaObject* metaObject = m_pTargetObject->metaObject();
	while (metaObject)
	{
		fnQuerySignalSlot(metaObject);
		metaObject = metaObject->superClass();
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
	}
	return m_pTraceWnd;
}

CStatusInfoWnd::CStatusInfoWnd() : CListInfoWnd()
{
	setWindowTitle(_QStr("状态信息"));
	UpdateStatusInfo();
}

void CStatusInfoWnd::UpdateStatusInfo()
{
	ClearAll();
	//TLogAction& log = GetCoreInst().RefLogAction();
	//AddInfo(_QStr("当前对象数"), QString::number(log.nCurrentObjectNum));
	//AddInfo(_QStr("最大分配对象数"), QString::number(log.nMaxCreatedObjectNum));
	//AddInfo(_QStr("已析构对象数"), QString::number(log.nMaxCreatedObjectNum - log.nCurrentObjectNum));
}

void CStatusInfoWnd::keyReleaseEvent(QKeyEvent* event)
{
	QKeyEvent* pTypeEvent = dynamic_cast<QKeyEvent*>(event);
	if (pTypeEvent && pTypeEvent->key() == Qt::Key_F5) {
		UpdateStatusInfo();
	}
	CListInfoWnd::keyReleaseEvent(event);
}

CCursorLocateWnd::CCursorLocateWnd() :CXDialog(nullptr)
{
	setStyleSheet(QString("background-color:#00ff00;"));
	setWindowOpacity(0.3);
	setWindowFlag(Qt::FramelessWindowHint);
	setWindowFlag(Qt::WindowStaysOnTopHint);
	setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
	setWindowTitle(QString::fromUtf8("定位鼠标..."));
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

CLogTraceWnd::CLogTraceWnd(QWidget* parent /*= nullptr*/) :CXDialog(parent)
{
	resize(400, 300);
	setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowMinMaxButtonsHint);
	initWidgets();
}

bool CLogTraceWnd::AddInfo(QString strInfo)
{
	strInfo = QString("%1 | %2 | %3").arg(m_nCount++, 4, 10, QLatin1Char('0')).arg(QTime::currentTime().toString("hh:mm:ss::zzz")).arg(strInfo);
	do
	{
		if (!m_strHas.isEmpty()) {
			if (!strInfo.contains(m_strHas, Qt::CaseInsensitive)) {
				break;
			}
		}
		if (!m_strNo.isEmpty()) {
			if (strInfo.contains(m_strNo, Qt::CaseInsensitive)) {
				break;
			}
		}
		m_listModel.insertRow(m_listModel.rowCount());
		m_listModel.setData(m_listModel.index(m_listModel.rowCount() - 1), strInfo);
		if (m_bTrace)
		{
			m_listView->scrollTo(m_listModel.index(m_listModel.rowCount() - 1, 0));
		}
	} while (0);
	return true;
}

void CLogTraceWnd::initWidgets()
{
	auto mainLayout = new QVBoxLayout();
	this->setLayout(mainLayout);
	m_listView = new QListView();
	m_listView->setModel(&m_listModel);
	connect(m_listView, &QListView::clicked, [&]() {m_bTrace = false; });
	mainLayout->addWidget(m_listView);

	auto control_1 = new QHBoxLayout();
	{
		auto btnClear = new QPushButton("clear");
		QObject::connect(btnClear, &QPushButton::clicked, [&]() {
			m_listModel.removeRows(0, m_listModel.rowCount());
			this->m_nCount = 0;
			});
		control_1->addWidget(btnClear);
		auto btnTrace = new QPushButton("trace");
		QObject::connect(btnTrace, &QPushButton::clicked, [&]() { m_bTrace = true; });
		control_1->addWidget(btnTrace);
	}


	auto control_2 = new QHBoxLayout();
	auto editFilterHas = new QLineEdit();
	QObject::connect(editFilterHas, &QLineEdit::textChanged, [&](const QString& str) {
		this->m_strHas = str;
		});
	control_2->addWidget(new QLabel("has:"));
	control_2->addWidget(editFilterHas);

	auto control_3 = new QHBoxLayout();
	auto editFilterNo = new QLineEdit();
	QObject::connect(editFilterNo, &QLineEdit::textChanged, [&](const QString& str) {
		this->m_strNo = str;
		});
	control_3->addWidget(new QLabel("no:"));
	control_3->addWidget(editFilterNo);

	mainLayout->addLayout(control_1);
	mainLayout->addLayout(control_2);
	mainLayout->addLayout(control_3);
}

CEventTraceWnd::CEventTraceWnd(QWidget* parent /*= nullptr*/) : CLogTraceWnd(parent)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	m_pMonitorObject = nullptr;
}

CEventTraceWnd::~CEventTraceWnd()
{
	if (m_pGraphicsSpy)
	{
		delete m_pGraphicsSpy;
	}
}

bool CEventTraceWnd::MonitorWidget(QObject* object)
{
	if (object) {
		if (OTo<QGraphicsItem>(object))
		{
			m_pGraphicsSpy = new CGraphicsItemSpy(OTo<QGraphicsItem>(object), this);
		}
		else
		{
			m_pMonitorObject = object;
			m_pMonitorObject->installEventFilter(this);
			QObject::connect(this, &QDialog::close, [&]() {
				if (m_pMonitorObject) {
					m_pMonitorObject->removeEventFilter(this);
				}
				});
		}
		return true;
	}
	return false;
}

bool CEventTraceWnd::AddInfo(QEvent* event)
{
	CLogTraceWnd::AddInfo(EventInfo(event));
	return true;
}

bool CEventTraceWnd::eventFilter(QObject* pObject, QEvent* event)
{
	if (pObject == m_pMonitorObject) {
		AddInfo(event);
	}
	return QDialog::eventFilter(pObject, event);
}

bool CGraphicsItemSpy::sceneEventFilter(QGraphicsItem* watched, QEvent* event)
{
	if (watched == m_pTarget) {
		m_pWnd->AddInfo(event);
	}
	return QGraphicsItem::sceneEventFilter(watched, event);
}

QString CEventTraceWnd::EventInfo(QEvent* event)
{
	if (!event) {
		return _QStr("event [NULL]");
	}
	auto type = event->type();
	if (type == QEvent::None) {
		return _QStr("event [None]");
	}
	if (type == QEvent::Timer) {
		return _QStr("event [Timer]");
	}
	if (type == QEvent::MouseButtonPress) {
		return _QStr("event [MouseButtonPress]");
	}
	if (type == QEvent::MouseButtonRelease) {
		return _QStr("event [MouseButtonRelease]");
	}
	if (type == QEvent::MouseButtonDblClick) {
		return _QStr("event [MouseButtonDblClick]");
	}
	if (type == QEvent::MouseMove) {
		return _QStr("event [MouseMove]");
	}
	if (type == QEvent::KeyPress) {
		return _QStr("event [KeyPress]");
	}
	if (type == QEvent::KeyRelease) {
		return _QStr("event [KeyRelease]");
	}
	if (type == QEvent::FocusIn) {
		return _QStr("event [FocusIn]");
	}
	if (type == QEvent::FocusOut) {
		return _QStr("event [FocusOut]");
	}
	if (type == QEvent::FocusAboutToChange) {
		return _QStr("event [FocusAboutToChange]");
	}
	if (type == QEvent::Enter) {
		return _QStr("event [Enter]");
	}
	if (type == QEvent::Leave) {
		return _QStr("event [Leave]");
	}
	if (type == QEvent::Paint) {
		return _QStr("event [Paint]");
	}
	if (type == QEvent::Move) {
		return _QStr("event [Move]");
	}
	if (type == QEvent::Resize) {
		return _QStr("event [Resize]");
	}
	if (type == QEvent::Create) {
		return _QStr("event [Create]");
	}
	if (type == QEvent::Destroy) {
		return _QStr("event [Destroy]");
	}
	if (type == QEvent::Show) {
		return _QStr("event [Show]");
	}
	if (type == QEvent::Hide) {
		return _QStr("event [Hide]");
	}
	if (type == QEvent::Close) {
		return _QStr("event [Close]");
	}
	if (type == QEvent::Quit) {
		return _QStr("event [Quit]");
	}
	if (type == QEvent::ParentChange) {
		return _QStr("event [ParentChange]");
	}
	if (type == QEvent::ParentAboutToChange) {
		return _QStr("event [ParentAboutToChange]");
	}
	if (type == QEvent::ThreadChange) {
		return _QStr("event [ThreadChange]");
	}
	if (type == QEvent::WindowActivate) {
		return _QStr("event [WindowActivate]");
	}
	if (type == QEvent::WindowDeactivate) {
		return _QStr("event [WindowDeactivate]");
	}
	if (type == QEvent::ShowToParent) {
		return _QStr("event [ShowToParent]");
	}
	if (type == QEvent::HideToParent) {
		return _QStr("event [HideToParent]");
	}
	if (type == QEvent::Wheel) {
		return _QStr("event [Wheel]");
	}
	if (type == QEvent::WindowTitleChange) {
		return _QStr("event [WindowTitleChange]");
	}
	if (type == QEvent::WindowIconChange) {
		return _QStr("event [WindowIconChange]");
	}
	if (type == QEvent::ApplicationWindowIconChange) {
		return _QStr("event [ApplicationWindowIconChange]");
	}
	if (type == QEvent::ApplicationFontChange) {
		return _QStr("event [ApplicationFontChange]");
	}
	if (type == QEvent::ApplicationLayoutDirectionChange) {
		return _QStr("event [ApplicationLayoutDirectionChange]");
	}
	if (type == QEvent::ApplicationPaletteChange) {
		return _QStr("event [ApplicationPaletteChange]");
	}
	if (type == QEvent::PaletteChange) {
		return _QStr("event [PaletteChange]");
	}
	if (type == QEvent::Clipboard) {
		return _QStr("event [Clipboard]");
	}
	if (type == QEvent::Speech) {
		return _QStr("event [Speech]");
	}
	if (type == QEvent::MetaCall) {
		return _QStr("event [MetaCall]");
	}
	if (type == QEvent::SockAct) {
		return _QStr("event [SockAct]");
	}
	if (type == QEvent::WinEventAct) {
		return _QStr("event [WinEventAct]");
	}
	if (type == QEvent::DeferredDelete) {
		return _QStr("event [DeferredDelete]");
	}
	if (type == QEvent::DragEnter) {
		return _QStr("event [DragEnter]");
	}
	if (type == QEvent::DragMove) {
		return _QStr("event [DragMove]");
	}
	if (type == QEvent::DragLeave) {
		return _QStr("event [DragLeave]");
	}
	if (type == QEvent::Drop) {
		return _QStr("event [Drop]");
	}
	if (type == QEvent::DragResponse) {
		return _QStr("event [DragResponse]");
	}
	if (type == QEvent::ChildAdded) {
		return _QStr("event [ChildAdded]");
	}
	if (type == QEvent::ChildPolished) {
		return _QStr("event [ChildPolished]");
	}
	if (type == QEvent::ChildRemoved) {
		return _QStr("event [ChildRemoved]");
	}
	if (type == QEvent::ShowWindowRequest) {
		return _QStr("event [ShowWindowRequest]");
	}
	if (type == QEvent::PolishRequest) {
		return _QStr("event [PolishRequest]");
	}
	if (type == QEvent::Polish) {
		return _QStr("event [Polish]");
	}
	if (type == QEvent::LayoutRequest) {
		return _QStr("event [LayoutRequest]");
	}
	if (type == QEvent::UpdateRequest) {
		return _QStr("event [UpdateRequest]");
	}
	if (type == QEvent::UpdateLater) {
		return _QStr("event [UpdateLater]");
	}
	if (type == QEvent::EmbeddingControl) {
		return _QStr("event [EmbeddingControl]");
	}
	if (type == QEvent::ActivateControl) {
		return _QStr("event [ActivateControl]");
	}
	if (type == QEvent::DeactivateControl) {
		return _QStr("event [DeactivateControl]");
	}
	if (type == QEvent::ContextMenu) {
		return _QStr("event [ContextMenu]");
	}
	if (type == QEvent::InputMethod) {
		return _QStr("event [InputMethod]");
	}
	if (type == QEvent::TabletMove) {
		return _QStr("event [TabletMove]");
	}
	if (type == QEvent::LocaleChange) {
		return _QStr("event [LocaleChange]");
	}
	if (type == QEvent::LanguageChange) {
		return _QStr("event [LanguageChange]");
	}
	if (type == QEvent::LayoutDirectionChange) {
		return _QStr("event [LayoutDirectionChange]");
	}
	if (type == QEvent::Style) {
		return _QStr("event [Style]");
	}
	if (type == QEvent::TabletPress) {
		return _QStr("event [TabletPress]");
	}
	if (type == QEvent::TabletRelease) {
		return _QStr("event [TabletRelease]");
	}
	if (type == QEvent::OkRequest) {
		return _QStr("event [TabletRelease]");
	}
	if (type == QEvent::HelpRequest) {
		return _QStr("event [HelpRequest]");
	}
	if (type == QEvent::IconDrag) {
		return _QStr("event [IconDrag]");
	}
	if (type == QEvent::FontChange) {
		return _QStr("event [FontChange]");
	}
	if (type == QEvent::EnabledChange) {
		return _QStr("event [EnabledChange]");
	}
	if (type == QEvent::ActivationChange) {
		return _QStr("event [ActivationChange]");
	}
	if (type == QEvent::StyleChange) {
		return _QStr("event [StyleChange]");
	}
	if (type == QEvent::IconTextChange) {
		return _QStr("event [IconTextChange]");
	}
	if (type == QEvent::ModifiedChange) {
		return _QStr("event [ModifiedChange]");
	}
	if (type == QEvent::MouseTrackingChange) {
		return _QStr("event [MouseTrackingChange]");
	}
	if (type == QEvent::WindowBlocked) {
		return _QStr("event [WindowBlocked]");
	}
	if (type == QEvent::WindowUnblocked) {
		return _QStr("event [WindowUnblocked]");
	}
	if (type == QEvent::WindowStateChange) {
		return _QStr("event [WindowStateChange]");
	}
	if (type == QEvent::ReadOnlyChange) {
		return _QStr("event [ReadOnlyChange]");
	}
	if (type == QEvent::ToolTip) {
		return _QStr("event [ToolTip]");
	}
	if (type == QEvent::WhatsThis) {
		return _QStr("event [WhatsThis]");
	}
	if (type == QEvent::StatusTip) {
		return _QStr("event [StatusTip]");
	}
	if (type == QEvent::ActionChanged) {
		return _QStr("event [ActionChanged]");
	}
	if (type == QEvent::ActionAdded) {
		return _QStr("event [ActionAdded]");
	}
	if (type == QEvent::ActionRemoved) {
		return _QStr("event [ActionRemoved]");
	}
	if (type == QEvent::FileOpen) {
		return _QStr("event [FileOpen]");
	}
	if (type == QEvent::Shortcut) {
		return _QStr("event [Shortcut]");
	}
	if (type == QEvent::ShortcutOverride) {
		return _QStr("event [ShortcutOverride]");
	}
	if (type == QEvent::WhatsThisClicked) {
		return _QStr("event [WhatsThisClicked]");
	}
	if (type == QEvent::ToolBarChange) {
		return _QStr("event [ToolBarChange]");
	}
	if (type == QEvent::ApplicationActivate) {
		return _QStr("event [ApplicationActivate]");
	}
	if (type == QEvent::QueryWhatsThis) {
		return _QStr("event [QueryWhatsThis]");
	}
	if (type == QEvent::EnterWhatsThisMode) {
		return _QStr("event [EnterWhatsThisMode]");
	}
	if (type == QEvent::LeaveWhatsThisMode) {
		return _QStr("event [LeaveWhatsThisMode]");
	}
	if (type == QEvent::ZOrderChange) {
		return _QStr("event [ZOrderChange]");
	}
	if (type == QEvent::HoverEnter) {
		return _QStr("event [HoverEnter]");
	}
	if (type == QEvent::HoverLeave) {
		return _QStr("event [HoverLeave]");
	}
	if (type == QEvent::HoverMove) {
		return _QStr("event [HoverMove]");
	}
	if (type == QEvent::AcceptDropsChange) {
		return _QStr("event [AcceptDropsChange]");
	}
	if (type == QEvent::ZeroTimerEvent) {
		return _QStr("event [ZeroTimerEvent]");
	}
	if (type == QEvent::GraphicsSceneMouseMove) {
		return _QStr("event [GraphicsSceneMouseMove]");
	}
	if (type == QEvent::GraphicsSceneMousePress) {
		return _QStr("event [GraphicsSceneMousePress]");
	}
	if (type == QEvent::GraphicsSceneMouseRelease) {
		return _QStr("event [GraphicsSceneMouseRelease]");
	}
	if (type == QEvent::GraphicsSceneMouseDoubleClick) {
		return _QStr("event [GraphicsSceneMouseDoubleClick]");
	}
	if (type == QEvent::GraphicsSceneContextMenu) {
		return _QStr("event [GraphicsSceneContextMenu]");
	}
	if (type == QEvent::GraphicsSceneHoverEnter) {
		return _QStr("event [GraphicsSceneHoverEnter]");
	}
	if (type == QEvent::GraphicsSceneHoverMove) {
		return _QStr("event [GraphicsSceneHoverMove]");
	}
	if (type == QEvent::GraphicsSceneHoverLeave) {
		return _QStr("event [GraphicsSceneHoverLeave]");
	}
	if (type == QEvent::GraphicsSceneHelp) {
		return _QStr("event [GraphicsSceneHelp]");
	}
	if (type == QEvent::GraphicsSceneDragEnter) {
		return _QStr("event [GraphicsSceneDragEnter]");
	}
	if (type == QEvent::GraphicsSceneDragMove) {
		return _QStr("event [GraphicsSceneDragMove]");
	}
	if (type == QEvent::GraphicsSceneDragLeave) {
		return _QStr("event [GraphicsSceneDragLeave]");
	}
	if (type == QEvent::GraphicsSceneDrop) {
		return _QStr("event [GraphicsSceneDrop]");
	}
	if (type == QEvent::GraphicsSceneWheel) {
		return _QStr("event [GraphicsSceneWheel]");
	}
	//if (type == QEvent::GraphicsSceneLeave) {
	//	return _QStr("event [GraphicsSceneLeave]");
	//}
	if (type == QEvent::KeyboardLayoutChange) {
		return _QStr("event [KeyboardLayoutChange]");
	}
	if (type == QEvent::DynamicPropertyChange) {
		return _QStr("event [DynamicPropertyChange]");
	}
	if (type == QEvent::TabletEnterProximity) {
		return _QStr("event [TabletEnterProximity]");
	}
	if (type == QEvent::TabletLeaveProximity) {
		return _QStr("event [TabletLeaveProximity]");
	}
	if (type == QEvent::NonClientAreaMouseMove) {
		return _QStr("event [NonClientAreaMouseMove]");
	}
	if (type == QEvent::NonClientAreaMouseButtonPress) {
		return _QStr("event [NonClientAreaMouseButtonPress]");
	}
	if (type == QEvent::NonClientAreaMouseButtonRelease) {
		return _QStr("event [NonClientAreaMouseButtonRelease]");
	}
	if (type == QEvent::NonClientAreaMouseButtonDblClick) {
		return _QStr("event [NonClientAreaMouseButtonDblClick]");
	}
	if (type == QEvent::MacSizeChange) {
		return _QStr("event [MacSizeChange]");
	}
	if (type == QEvent::ContentsRectChange) {
		return _QStr("event [ContentsRectChange]");
	}
	if (type == QEvent::MacGLWindowChange) {
		return _QStr("event [MacGLWindowChange]");
	}
	if (type == QEvent::FutureCallOut) {
		return _QStr("event [FutureCallOut]");
	}
	if (type == QEvent::GraphicsSceneResize) {
		return _QStr("event [GraphicsSceneResize]");
	}
	if (type == QEvent::GraphicsSceneMove) {
		return _QStr("event [GraphicsSceneMove]");
	}
	if (type == QEvent::CursorChange) {
		return _QStr("event [CursorChange]");
	}
	if (type == QEvent::ToolTipChange) {
		return _QStr("event [ToolTipChange]");
	}
	if (type == QEvent::NetworkReplyUpdated) {
		return _QStr("event [NetworkReplyUpdated]");
	}
	if (type == QEvent::GrabMouse) {
		return _QStr("event [GrabMouse]");
	}
	if (type == QEvent::UngrabMouse) {
		return _QStr("event [UngrabMouse]");
	}
	if (type == QEvent::GrabKeyboard) {
		return _QStr("event [GrabKeyboard]");
	}
	if (type == QEvent::UngrabKeyboard) {
		return _QStr("event [UngrabKeyboard]");
	}
	if (type == QEvent::StateMachineSignal) {
		return _QStr("event [StateMachineSignal]");
	}
	if (type == QEvent::StateMachineWrapped) {
		return _QStr("event [StateMachineWrapped]");
	}
	if (type == QEvent::TouchBegin) {
		return _QStr("event [TouchBegin]");
	}
	if (type == QEvent::TouchUpdate) {
		return _QStr("event [TouchUpdate]");
	}
	if (type == QEvent::TouchEnd) {
		return _QStr("event [TouchEnd]");
	}
	if (type == QEvent::NativeGesture) {
		return _QStr("event [NativeGesture]");
	}
	if (type == QEvent::RequestSoftwareInputPanel) {
		return _QStr("event [RequestSoftwareInputPanel]");
	}
	if (type == QEvent::CloseSoftwareInputPanel) {
		return _QStr("event [CloseSoftwareInputPanel]");
	}
	if (type == QEvent::WinIdChange) {
		return _QStr("event [WinIdChange]");
	}
	if (type == QEvent::Gesture) {
		return _QStr("event [Gesture]");
	}
	if (type == QEvent::GestureOverride) {
		return _QStr("event [GestureOverride]");
	}
	if (type == QEvent::ScrollPrepare) {
		return _QStr("event [ScrollPrepare]");
	}
	if (type == QEvent::Scroll) {
		return _QStr("event [Scroll]");
	}
	if (type == QEvent::Expose) {
		return _QStr("event [Expose]");
	}
	if (type == QEvent::OrientationChange) {
		return _QStr("event [OrientationChange]");
	}
	if (type == QEvent::InputMethodQuery) {
		return _QStr("event [InputMethodQuery]");
	}
	if (type == QEvent::TouchCancel) {
		return _QStr("event [TouchCancel]");
	}
	if (type == QEvent::ThemeChange) {
		return _QStr("event [ThemeChange]");
	}
	if (type == QEvent::SockClose) {
		return _QStr("event [SockClose]");
	}
	if (type == QEvent::PlatformPanel) {
		return _QStr("event [PlatformPanel]");
	}
	if (type == QEvent::StyleAnimationUpdate) {
		return _QStr("event [StyleAnimationUpdate]");
	}
	if (type == QEvent::ApplicationStateChange) {
		return _QStr("event [ApplicationStateChange]");
	}
	if (type == QEvent::WindowChangeInternal) {
		return _QStr("event [WindowChangeInternal]");
	}
	if (type == QEvent::ScreenChangeInternal) {
		return _QStr("event [ScreenChangeInternal]");
	}
	if (type == QEvent::PlatformSurface) {
		return _QStr("event [PlatformSurface]");
	}
	if (type == QEvent::Pointer) {
		return _QStr("event [Pointer]");
	}
	if (type == QEvent::TabletTrackingChange) {
		return _QStr("event [TabletTrackingChange]");
	}
	//if (type == QEvent::WindowAboutToChangeInternal) {
	//	return _QStr("event [WindowAboutToChangeInternal]");
	//}
	if (type >= QEvent::User && type < QEvent::MaxUser) {
		return _QStr("event [user define]");
	}
	return _QStr("event [unknow]");
}

CStyleEditWnd::CStyleEditWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent)
{
	m_pTargetWidget = nullptr;
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowMinMaxButtonsHint);
	auto mainLayout = new QVBoxLayout();
	this->setLayout(mainLayout);
	auto edit = new QPlainTextEdit();
	mainLayout->addWidget(edit);
	auto btn = new QPushButton("modify style");
	mainLayout->addWidget(btn);
	QObject::connect(btn, &QPushButton::clicked, [&]() {
		if (m_pTargetWidget) {
			auto editStyle = dynamic_cast<QPlainTextEdit*>(layout()->itemAt(0)->widget());
			QString strStyleSheet = editStyle->toPlainText();
			m_pTargetWidget->setStyleSheet(strStyleSheet);
		}
		});
}

bool CStyleEditWnd::EditWidgetStyle(QWidget* pWidget)
{
	if (pWidget) {
		m_pTargetWidget = pWidget;
		auto edit = dynamic_cast<QPlainTextEdit*>(layout()->itemAt(0)->widget());
		if (edit) {
			edit->setPlainText(m_pTargetWidget->styleSheet());
		}
		return true;
	}
	return false;
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

CWidgetSpyTree::CWidgetSpyTree(QWidget* parent /*= nullptr*/) : QTreeWidget(parent)
{
	installEventFilter(this);
	setHeaderHidden(true);

	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
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
	QAction* actionLocate = contextMenu.addAction("目标定位");
	QAction* actionInfo = contextMenu.addAction("基础信息");
	QAction* actionSignal = contextMenu.addAction("信号/槽");
	QAction* actionEventTrace = contextMenu.addAction("事件跟踪");
	QAction* actionStyleEdit = contextMenu.addAction("风格编辑");
	QAction* actionUserDrawParam = contextMenu.addAction("自绘控件");
	QAction* actionWidgetStatus = contextMenu.addAction("组件状态");
	QAction* actionResourceHub = contextMenu.addAction("资源仓库");
	QAction* actionChangeVisible = contextMenu.addAction("显示|隐藏");
	QAction* actionChangeEnable = contextMenu.addAction("启用|禁用");
	QAction* actionMoveOrScale = contextMenu.addAction("移动|缩放");
	QAction* actionSpyParent = contextMenu.addAction("父组件");
	QAction* actionSpyFirstParent = contextMenu.addAction("第一父组件");
	QAction* selectedAction = contextMenu.exec(pos);
	if (selectedAction == actionLocate) {
		IndicatorWidget(pos);
	}
	else if (selectedAction == actionInfo) {
		ShowWidgetInfo(pos);
	}
	else if (selectedAction == actionSignal) {
		ShowSignalSlot(pos, true);
	}
	else if (selectedAction == actionEventTrace) {
		ShowEventTrace(pos);
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
	else if (selectedAction == actionResourceHub) {
		ShowWidgetResourceHub(pos);
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
		QWidget* pTargetWidget;
		QGraphicsItem* pTargetItem;
		if (pTargetWidget = widgetData(clickedItem)) {
			if (pTargetWidget->isVisible()) {
				pTargetWidget->setVisible(false);
			}
			else {
				pTargetWidget->setVisible(true);
			}
		}
		else if (pTargetItem = graphicsData(clickedItem))
		{
			if (pTargetItem->isVisible())
			{
				pTargetItem->hide();
			}
			else
			{
				pTargetItem->setVisible(true);
			}
		}
	}
}

void CWidgetSpyTree::ChangeWidgetEnable(QPoint ptGlobal)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {
		QWidget* pTargetWidget;
		QGraphicsItem* pTargetItem;
		if (pTargetWidget = widgetData(clickedItem)) {
			if (pTargetWidget->isEnabled()) {
				pTargetWidget->setEnabled(false);
			}
			else {
				pTargetWidget->setEnabled(true);
			}
		}
		else if (pTargetItem = graphicsData(clickedItem))
		{
			if (pTargetItem->isEnabled())
			{
				pTargetItem->setEnabled(false);
			}
			else
			{
				pTargetItem->setEnabled(true);
			}
		}
	}
}

void CWidgetSpyTree::ChangeWidgetPosOrSize(QPoint ptGlobal)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {	
		if (QWidget* pTargetWidget = widgetData(clickedItem)) {
			CMoveOrScaleWidgetWnd* window = new CMoveOrScaleWidgetWnd();
			window->ControlTarget(pTargetWidget);
			window->ShowOnTop();
		}
		else if (QGraphicsItem* pTargetItem = graphicsData(clickedItem)) {
			CMoveOrScaleWidgetWnd* window = new CMoveOrScaleWidgetWnd();
			window->GraphicsTarget(pTargetItem);
			window->ShowOnTop();
		}
	}
}

void CWidgetSpyTree::IndicatorWidget(QPoint ptGlobal)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {
		QWidget* pTargetWidget = widgetData(clickedItem);
		QGraphicsItem* pTargetItem = graphicsData(clickedItem);
		QRect rcArea;
		if (pTargetWidget)
		{
			rcArea = ScreenRect(pTargetWidget);
		}
		if (pTargetItem)
		{
			rcArea = ScreenRect(pTargetItem);
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
		CSpyIndicatorWnd* pIndicator = new CSpyIndicatorWnd();
		pIndicator->setGeometry(rcArea);
		pIndicator->show();
		pIndicator->raise();
	}
}

void CWidgetSpyTree::ShowSignalSlot(QPoint ptGlobal, bool bRecusive /*= false*/)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(ptGlobal));
	if (clickedItem) {
		QWidget* pTargetWidget = widgetData(clickedItem);
		QGraphicsItem* pItem = graphicsData(clickedItem);

		CSignalSpyWnd* pSpy = new CSignalSpyWnd();
		if (pTargetWidget)
		{
			pSpy->setTargetObject(pTargetWidget);
		}
		else
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
			CListInfoWnd* pInfo = new CListInfoWnd();
			pInfo->setWindowTitle(ObjectString(pTargetWidget));
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
			pInfo->AddAttribute(_QStr("winid"), QString("%1").arg(pTargetWidget->winId()));
			pInfo->ShowOnTop();
		}
		else if (QGraphicsItem* pItem = graphicsData(clickedItem))
		{
			auto geo = pItem->sceneBoundingRect();
			auto geoScreen = ScreenRect(pItem);
			auto client = pItem->boundingRect();
			int nUniqueId = -1;
			CListInfoWnd* pInfo = new CListInfoWnd();
			pInfo->setWindowTitle(ObjectString(To<QObject>(pItem)));
			pInfo->AddAttribute(_QStr("class name"), objectClass(To<QObject>(pItem)));
			pInfo->AddAttribute(_QStr("object name"), To<QObject>(pItem)->objectName());
			pInfo->AddAttribute(_QStr("geometry"), QString("(%1,%2,%3,%4)").arg(geo.left()).arg(geo.top()).arg(geo.right()).arg(geo.bottom()));
			pInfo->AddAttribute(_QStr("screen geometry"), QString("(%1,%2,%3,%4)").arg(geoScreen.left()).arg(geoScreen.top()).arg(geoScreen.right()).arg(geoScreen.bottom()));
			pInfo->AddAttribute(_QStr("geometry size"), QString("(%1,%2)").arg(geoScreen.width()).arg(geoScreen.height()));
			pInfo->AddAttribute(_QStr("client"), QString("(%1,%2,%3,%4)").arg(client.left()).arg(client.top()).arg(client.right()).arg(client.bottom()));
			pInfo->AddAttribute(_QStr("visible"), pItem->isVisible() ? _QStr("yes") : _QStr("no"));
			pInfo->AddAttribute(_QStr("enabled"), pItem->isEnabled() ? _QStr("yes") : _QStr("no"));
			pInfo->ShowOnTop();
		}
	}
	return true;
}

bool CWidgetSpyTree::ShowWidgetStatus(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		QWidget* pTargetWidget = widgetData(clickedItem);
		if (pTargetWidget) {
			CListInfoWnd* pInfo = new CListInfoWnd();
			pInfo->setWindowTitle(ObjectString(pTargetWidget));
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

			auto button = dynamic_cast<QAbstractButton*>(pTargetWidget);
			if (button) {
				pInfo->AddAttribute(_QStr("抽象按钮类"), button->isCheckable() ? _QStr("可勾选") : _QStr("不可勾选"));
				pInfo->AddAttribute(_QStr("抽象按钮类"), button->isChecked() ? _QStr("已勾选") : _QStr("未勾选"));
				pInfo->AddAttribute(_QStr("抽象按钮类"), button->group() ? _QStr("已分组") : _QStr("未分组"));
			}
			pInfo->ShowOnTop();
		}
	}
	return true;
}

bool CWidgetSpyTree::ShowWidgetResourceHub(const QPoint& pos)
{
	//QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	//if (clickedItem) {
	//	QWidget* pTargetWidget = clickedItem->data(0, Qt::UserRole).value<QWidget*>();
	//	if (dynamic_cast<CProxyObject*>(pTargetWidget)) {
	//		CProxyObject* pObject = dynamic_cast<CProxyObject*>(pTargetWidget);
	//		CListInfoWnd* pInfo = new CListInfoWnd();
	//		pInfo->setWindowTitle(WidgetString(pTargetWidget));
	//		if (pObject) {
	//			auto hub = pObject->GetResourceHub();
	//			for (int nIndex = 0; nIndex < hub.CountResource(); ++nIndex)
	//			{
	//				pInfo->AddInfo(_QStr("自身资源仓库"), hub.GetResourceDescribe(nIndex));
	//			}
	//		}
	//		auto hubParent = pObject->GetDomainResourceHub();
	//		for (int nIndex = 0; nIndex < hubParent.CountResource(); ++nIndex)
	//		{
	//			pInfo->AddInfo(_QStr("领域资源仓库"), hubParent.GetResourceDescribe(nIndex));
	//		}
	//		auto hubGlobal = GetCoreInst().GetResourceHub();
	//		for (int nIndex = 0; nIndex < hubGlobal.CountResource(); ++nIndex)
	//		{
	//			pInfo->AddInfo(_QStr("全局资源仓库"), hubGlobal.GetResourceDescribe(nIndex));
	//		}
	//		pInfo->ShowOnTop();
	//	}
	//	else {
	//		QMessageBox::information(nullptr, _QStr("资源仓库"), _QStr("无法查看"));
	//	}
	//}
	return true;
}

bool CWidgetSpyTree::ShowEventTrace(const QPoint& pos)
{
	QTreeWidgetItem* clickedItem = itemAt(mapFromGlobal(pos));
	if (clickedItem) {
		QWidget* pTargetWidget = widgetData(clickedItem);
		QObject* pTarget = widgetData(clickedItem) ? widgetData(clickedItem) : To<QObject>(graphicsData(clickedItem));
		CEventTraceWnd* pEventTraceWnd = new CEventTraceWnd();
		pEventTraceWnd->setWindowTitle(ObjectString(pTarget));
		pEventTraceWnd->MonitorWidget(pTarget);
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
		CStyleEditWnd* pEditStyleWnd = new CStyleEditWnd();
		pEditStyleWnd->setWindowTitle(ObjectString(pTargetWidget));
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
			dynamic_cast<CSpyMainWindow*>(parent())->spyObject()->setTreeTarget(OTo<QWidget>(widgetData(clickedItem)->parent()));
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

QGraphicsItem* CWidgetSpyTree::graphicsData(QTreeWidgetItem* item)
{
	return item->data(0, Qt::UserRole).value<QGraphicsItem*>();
}

QWidget* CWidgetSpyTree::widgetData(QTreeWidgetItem* item)
{
	return item->data(0, Qt::UserRole).value<QWidget*>();
}
