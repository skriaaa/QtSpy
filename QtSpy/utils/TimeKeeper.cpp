#include "TimeKeeper.h"

void CTimeKeeper::startMonitor()
{
	m_Timer.start();
}

void CTimeKeeper::addTimePoint(QString explaination)
{
	m_hashTimePoint[explaination] = m_Timer.nsecsElapsed();
	m_listTimePoint.append(explaination);
}

int CTimeKeeper::queryTimePoint(QString explaination)
{
	return m_hashTimePoint[explaination];
}

int CTimeKeeper::queryTimePoint(QString explaination1, QString explaination2)
{
	return m_hashTimePoint[explaination2] - m_hashTimePoint[explaination1];
}

QString CTimeKeeper::queryTimePointString(QString explaination)
{
	return QString("[%1] %2").arg(explaination).arg(m_hashTimePoint[explaination]);
}

QString CTimeKeeper::queryTimePointString(QString explaination1, QString explaination2)
{
	return QString("[%1] %2").arg(explaination2).arg(queryTimePoint(explaination1, explaination2));
}

QString CTimeKeeper::queryTimePointString()
{
	if (m_listTimePoint.isEmpty())
		return "";

	QStringList listRet;
	auto it = m_listTimePoint.begin();
	listRet.append(queryTimePointString(*it));
	while (it != m_listTimePoint.end()-1)
	{
		listRet.append(queryTimePointString(*it, *(it + 1)));
		it++;
	}
	return "[TimeKeeper] " + listRet.join(" ");
}
