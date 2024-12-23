#include "qtspy.h"
#include "qt_spyobject.h"
void QtSpy::initSpy()
{
	static CQtSpyObject object;
	Q_UNUSED(object);
}

void QtSpy::initSpy(void* parent)
{
	CQtSpyObject* pObject = new CQtSpyObject(static_cast<QWidget*>(parent));
	Q_UNUSED(pObject);
}
