#include "qtspy.h"
#include "SpyMainWindow.h"
#include <QApplication>
#include "SpyWndManager.h"
#ifdef QT_SPY_LIB
#define QT_SPY_API Q_DECL_EXPORT
#else
#define QT_SPY_API Q_DECL_IMPORT
#endif
QString s_strDllPath;
#ifdef Q_OS_WIN
#include <windows.h>
#include <QFileInfo>
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		wchar_t szPath[MAX_PATH];
		if (GetModuleFileNameW(hModule, szPath, MAX_PATH) > 0) {
			QFileInfo fileInfo(QString::fromWCharArray(szPath));
			s_strDllPath = fileInfo.absolutePath(); // 获取所在目录
		}

		QMetaObject::invokeMethod(QCoreApplication::instance(), QtSpy::initSpy, Qt::QueuedConnection);
		return TRUE;
	}
	return FALSE;
}
#endif

void QtSpy::initSpy()
{
	CSpyWndManager* pWndManager = new CSpyWndManager(qApp);
	Q_UNUSED(pWndManager);
}

void QtSpy::initSpyWithParent(void* parent)
{
	CSpyMainWindow* pSpyWnd = new CSpyMainWindow(static_cast<QWidget*>(parent));
	pSpyWnd->showOnTop();
}


extern "C" QT_SPY_API void initQtSpy()
{
	QtSpy::initSpy();
}

extern "C" QT_SPY_API void initQtSpyWithParent(void* parent)
{
	QtSpy::initSpyWithParent(parent);
}
