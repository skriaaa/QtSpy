#include "SpyWndManager.h"
#include "SpyMainWindow.h"
#include "publicfunction.h"
#include "utils/LogRecorder.h"
// qt module
#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QScreen>

bool s_bAutoCreate = false;
extern const char* MAIN_WINDOW;
CSpyWndManager::CSpyWndManager(QObject* parent) : QObject(parent)
{
	qApp->installEventFilter(this);
	m_pMainWnd = new CSpyMainWindow(qApp->desktop());
	m_pMainWnd->setAttribute(Qt::WA_DeleteOnClose, false);
	connect(qApp, &QApplication::lastWindowClosed, this, [this]() {
		if (nullptr != m_pMainWnd)
		{
			m_pMainWnd->close();
		}
	});
	spyWnd()->showCenter();
}

CSpyWndManager::~CSpyWndManager()
{
	CLogRecorder::shutdown();
}

bool CSpyWndManager::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() != QEvent::KeyPress)
	{
		return QObject::eventFilter(watched, event);
	}

	auto pKeyEvent = dynamic_cast<QKeyEvent*>(event);
	if (pKeyEvent->modifiers() == Qt::AltModifier)
	{
		switch (pKeyEvent->key())
		{
			case Qt::Key_Q:
			{
				spyWnd()->showCenter();
				return true;
			}
			case Qt::Key_E:
			{
				spyWnd()->findTarget();
				spyWnd()->raise();
				return true;
			}
			default:
				break;
		}
	}
	
	return QObject::eventFilter(watched, event);
}

CSpyMainWindow* CSpyWndManager::spyWnd()
{ 
	auto pModalWidget = qApp->activeModalWidget();
	if (nullptr != pModalWidget)
	{
		auto pSpyWnd = pModalWidget->findChild<CSpyMainWindow*>(MAIN_WINDOW);
		if (nullptr != pSpyWnd)
		{
			return pSpyWnd;
		}
		return new CSpyMainWindow(pModalWidget);
	}
	return m_pMainWnd;
}
