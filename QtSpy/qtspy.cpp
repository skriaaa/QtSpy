#include "qtspy.h"
#include "qt_spyobject.h"
#include <QApplication>
#include "publicfunction.h"
bool s_bAutoCreate = false;

#ifdef QT_SPY_LIB
#define QT_SPY_API __declspec(dllexport)
#else
#define QT_SPY_API __declspec(dllimport)
#endif


void QtSpy::initSpy()
{
	static CQtSpyObject object;
	Q_UNUSED(object);

	// 模态对话框自动弹出一个QtSpy子窗口
	static QTimer* pTimer = nullptr;
	if (nullptr == pTimer)
	{
		pTimer = new QTimer;
		pTimer->setInterval(1000);
		pTimer->callOnTimeout([]() {
			if (!s_bAutoCreate)
			{
				return;
			}

			QWidget* pWidget = qApp->activeModalWidget();
			if (nullptr == pWidget)
			{
				return;
			}

			if (OTo<CXDialog>(pWidget))
			{
				return;
			}

			if (nullptr != pWidget->findChild<CQtSpyObject*>("CQtSpyObject"))
			{
				return;
			}

			initSpyWithParent(pWidget);
		});
		pTimer->start();
	}
}

void QtSpy::initSpyWithParent(void* parent)
{
	CQtSpyObject* pObject = new CQtSpyObject(static_cast<QWidget*>(parent));
	Q_UNUSED(pObject);
}


extern "C" QT_SPY_API void initQtSpy()
{
	QtSpy::initSpy();
}

extern "C" QT_SPY_API void initQtSpyWithParent(void* parent)
{
	QtSpy::initSpyWithParent(parent);
}
