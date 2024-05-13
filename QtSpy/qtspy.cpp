#include "qtspy.h"
#include "qt_spyobject.h"
#include "qt_spymanager.h"
QtSpy::QtSpy()
{
	StartSpy();
}
QtSpy::~QtSpy()
{
	StopSpy();
}

void QtSpy::StartSpy(void* parent)
{
	CQtSpyObject* obj = new CQtSpyObject;
	CQtSpyManager::GetInstance().addSpy(this, new CQtSpyObject);
	obj->StartSpy(static_cast<QWidget*>(parent));
}

void QtSpy::StopSpy()
{
	CQtSpyObject* obj = CQtSpyManager::GetInstance().spyObject(this);
	if (obj)
	{
		obj->ShutdownSpy();
	}
	CQtSpyManager::GetInstance().removeSpy(this);
}
