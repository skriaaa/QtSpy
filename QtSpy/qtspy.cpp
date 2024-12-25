#include "qtspy.h"
#include "qt_spyobject.h"
#include <QApplication>
#include "publicfunction.h"
bool s_bAutoCreate = false;
void QtSpy::initSpy()
{
	static CQtSpyObject object;
	Q_UNUSED(object);

	// 模态对话框自动弹出一个QtSpy子窗口
	static QTimer* pTimer = nullptr;
	if(nullptr == pTimer)
	{
		pTimer = new QTimer;
		pTimer->setInterval(1000);
		pTimer->callOnTimeout([]() {
			if(!s_bAutoCreate)
			{
				return;
			}

			QWidget* pWidget = qApp->activeModalWidget();
			if( nullptr == pWidget )
			{
				return;
			}

			if(OTo<CXDialog>(pWidget))
			{
				return;
			}

			if (nullptr != pWidget->findChild<CQtSpyObject*>("CQtSpyObject"))
			{
				return;
			}

			initSpy(pWidget);
		});
		pTimer->start();
	}
}

void QtSpy::initSpy(void* parent)
{
	CQtSpyObject* pObject = new CQtSpyObject(static_cast<QWidget*>(parent));
	Q_UNUSED(pObject);
}
