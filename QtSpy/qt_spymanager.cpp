#include "qt_spymanager.h"

CQtSpyManager::~CQtSpyManager()
{
	
}

CQtSpyManager& CQtSpyManager::GetInstance()
{
	static CQtSpyManager mgr;
	return mgr;
}

void CQtSpyManager::addSpy(QtSpy* spy, CQtSpyObject* object)
{
	m_mapSpy[spy] = object;
}

void CQtSpyManager::removeSpy(QtSpy* spy)
{
	auto it = m_mapSpy.find(spy);
	if (it != m_mapSpy.end())
	{
		if (it->second)
			delete it->second;
		m_mapSpy.erase(it);
	}
}

CQtSpyObject* CQtSpyManager::spyObject(QtSpy* spy)
{
	return m_mapSpy[spy];
}
