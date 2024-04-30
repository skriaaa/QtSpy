#include "qtspy.h"
#include "qt_spyobject.h"
QtSpy::QtSpy()
{
	StartSpy();
}
QtSpy::~QtSpy()
{
	StopSpy();
}

void QtSpy::StartSpy()
{
	CQtSpyObject::GetInstance().StartSpy(nullptr);
}

void QtSpy::StopSpy()
{
	CQtSpyObject::GetInstance().ShutdownSpy();
}
