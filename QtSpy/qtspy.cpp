#include "qtspy.h"
#include "qt_spyobject.h"
#include "qt_spymanager.h"
QtSpy::QtSpy(void* parent)
{
	StartSpy(parent);
}
QtSpy::~QtSpy()
{
	StopSpy();
}
static CQtSpyObject* obj = nullptr;
void QtSpy::StartSpy(void* parent)
{
	CQtSpyObject* obj = new CQtSpyObject(static_cast<QWidget*>(parent));
	CQtSpyManager::GetInstance().addSpy(this, new CQtSpyObject);
	obj->StartSpy();
}

void QtSpy::StopSpy()
{
	//CQtSpyObject* obj = CQtSpyManager::GetInstance().spyObject(this);
	if (obj)
	{
		obj->ShutdownSpy();
	}
	//CQtSpyManager::GetInstance().removeSpy(this);
}
