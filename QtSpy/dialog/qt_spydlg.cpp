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
#include "StyleEditDlg.h"
#include "utils/LogRecorder.h"
#include "qt_spyobject.h"
#include "ObjectTree.h"

CSpyIndicatorWnd::CSpyIndicatorWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent),
m_Timer(QTimer(this))
{
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setWindowFlags(windowFlags() | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
	setStyleSheet("border: 1px solid rgba(0, 255, 225, 255); background:rgba(255, 255, 255, 255);");

	m_Timer.setInterval(100);
	QObject::connect(&m_Timer, &QTimer::timeout, [&]() {
		if (m_nSpanPeriod == 0) {
			m_Timer.stop();
			hide();
		}
		else
		{
			setWindowOpacity((float)m_nSpanPeriod / 30);
			repaint();
			m_nSpanPeriod--;
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
	setWindowOpacity(0.4);
	if(!bHold)
	{
		m_nSpanPeriod = 30;
		m_Timer.stop();
		m_Timer.start();
	}
	else
	{
		m_Timer.stop();
	}

	QDialog::show();
	raise();
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
	QString strText = objectString(target);

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

CStatusInfoWnd::CStatusInfoWnd(QWidget* parent) : CListInfoWnd(parent)
{
	setWindowTitle(_QStr("状态信息"));
	UpdateStatusInfo();
}

void CStatusInfoWnd::UpdateStatusInfo()
{
	ClearAll();
	AddAttribute("当前控件数", QString::number(qApp->allWidgets().size()));
	AddAttribute("页面控件数", QString::number(dynamic_cast<CSpyMainWindow*>(parentWidget())->spyObject()->currentItemCount()));
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

CLogTraceWnd::CLogTraceWnd(QWidget* parent /*= nullptr*/) :CXDialog(parent)
{
	resize(400, 300);
	setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowMinMaxButtonsHint);
	initWidgets();
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
	m_listModel.insertRow(m_listModel.rowCount());
	m_listModel.setData(m_listModel.index(m_listModel.rowCount() - 1), strInfo);
	if (m_bTrace)
	{
		m_listView->scrollTo(m_listModel.index(m_listModel.rowCount() - 1, 0));
	}

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

	mainLayout->addLayout(control_1);
	mainLayout->addLayout(control_2);
	mainLayout->addLayout(control_3);
}

CEventTraceWnd::CEventTraceWnd(QWidget* parent /*= nullptr*/) : CLogTraceWnd(parent)
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
	pLayout->addWidget(filter);
	pLayout->addSpacerItem(new QSpacerItem(1 , 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
	pLayout->addWidget(btnStop);
	dynamic_cast<QVBoxLayout*>(layout())->addLayout(pLayout);
	connect(filter, &QCheckBox::stateChanged, [this](int state) {
		m_bFilterEvent = state != Qt::Unchecked;
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
		return _QStr("event [NULL]") + strTarget;
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
		return _QStr("event [user define]") + strTarget;
	}
	return _QStr("event [unknow]") + strTarget;
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

CFindWnd::CFindWnd(CQtSpyObject* pSpyObject, QWidget* parent /*= nullptr*/):CXDialog(parent)
{
	m_pSpyObject = pSpyObject;
	initWidget();
}

CFindWnd::~CFindWnd()
{

}

void CFindWnd::initWidget()
{
	QPushButton* pBtnYes = new QPushButton("查找全部");
	QPushButton* pBtnNext = new QPushButton("下一个");
	QPushButton* pBtnPrev = new QPushButton("上一个");
	QLineEdit* pEdit = new QLineEdit();

	QObject::connect(pBtnYes, &QPushButton::clicked, [this, pEdit]() {
		m_arrTargetItem = m_pSpyObject->m_pMainWindow->tree()->findItems(pEdit->text(), Qt::MatchFlag::MatchContains | Qt::MatchRecursive);
		if (!m_arrTargetItem.empty())
		{
			m_arrTargetItem.front()->setExpanded(true);
			m_pSpyObject->m_pMainWindow->tree()->setCurrentItem(m_arrTargetItem.front());
		}
	});
	QObject::connect(pBtnNext, &QPushButton::clicked, [&]() {
		if (m_arrTargetItem.size() - 1 > m_nCurrentIndex)
		{
			m_nCurrentIndex++;
			m_arrTargetItem[m_nCurrentIndex]->setExpanded(true);
			m_pSpyObject->m_pMainWindow->tree()->setCurrentItem(m_arrTargetItem[m_nCurrentIndex]);
		}
	});
	QObject::connect(pBtnPrev, &QPushButton::clicked, [&]() {
		if (m_nCurrentIndex > 0)
		{
			m_nCurrentIndex--;
			m_arrTargetItem[m_nCurrentIndex]->setExpanded(true);
			m_pSpyObject->m_pMainWindow->tree()->setCurrentItem(m_arrTargetItem[m_nCurrentIndex]);
		}
	});

	auto pLayout = new QHBoxLayout();
	pLayout->addWidget(new QLabel("对象名称"));
	pLayout->addWidget(pEdit);
	pLayout->addWidget(pBtnYes);
	pLayout->addWidget(pBtnNext);
	pLayout->addWidget(pBtnPrev);

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(_QStr("查找"));
	setLayout(new QVBoxLayout());
	dynamic_cast<QVBoxLayout*>(layout())->addLayout(pLayout);
}

