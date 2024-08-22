#include "MemoryMonitorDlg.h"
#include <QHBoxLayout>
#include <QListView>
#include <QPushButton>
#include <QTime>
#include <QStringListModel>
#include "utils/ProcessInfo.h"

CMemoryMonitorDlg::CMemoryMonitorDlg(QWidget* parent /*= nullptr*/):CXDialog(parent)
{
	setWindowTitle("内存监控");
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::Popup | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint | Qt::CustomizeWindowHint);
	resize(320, 380);
	initWidgets();
}

void CMemoryMonitorDlg::initWidgets()
{
	setLayout(new QVBoxLayout());

	m_listView = new QListView;
	m_listView->setModel(new QStringListModel());
	connect(m_listView, &QListView::clicked, [=]() {
		m_bTrace = false;
		});

	QPushButton* btn = new QPushButton(u8"开始监控");
	connect(btn, &QPushButton::clicked, [=]() {
		if (m_nTimerID == -1)
		{
			m_nTimerID = startTimer(100);
			btn->setText(u8"停止监控");
		}
		else
		{
			killTimer(m_nTimerID);
			m_nTimerID = -1;
			btn->setText(u8"开始监控");
		}
		});
	QPushButton* btnTrace = new QPushButton(u8"Trace");
	connect(btnTrace, &QPushButton::clicked, [=]() {
		m_bTrace = !m_bTrace;
		});
	
	layout()->addWidget(m_listView);
	layout()->addWidget(btnTrace);
	layout()->addWidget(btn);
}

void CMemoryMonitorDlg::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == m_nTimerID)
	{
		int nMemoryCount = ProcessInfo::queryProcessCurrentMemory();
		int nDiffer = nMemoryCount - m_nMemoryCount;
		m_nMemoryCount = nMemoryCount;
		QStringListModel* model = static_cast<QStringListModel*>(m_listView->model());
		model->insertRow(model->rowCount());

		QString strRowString = QString("%1 | 当前内存占用 %2 (Byte)| 变化值 %3 (Byte)").arg(QTime().currentTime().toString("hh:mm:ss::zzz")).arg(m_nMemoryCount).arg(nDiffer);
		model->setData(model->index(model->rowCount() - 1), strRowString);
		if (m_bTrace)
		{
			m_listView->scrollTo(model->index(model->rowCount() - 1));
		}
	}
}
