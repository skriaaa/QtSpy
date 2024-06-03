#pragma once
#include "qt_spyobject.h"
#include "qtspy.h"
#include <map>
class CQtSpyManager
{
public:
	CQtSpyManager() = default;
	~CQtSpyManager();
	static CQtSpyManager& GetInstance();
public:
	void addSpy(QtSpy* spy, CQtSpyObject* object);
	void removeSpy(QtSpy* spy);
	CQtSpyObject* spyObject(QtSpy* spy);;
private:
	std::map<QtSpy*, CQtSpyObject*> m_mapSpy;
};

