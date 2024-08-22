#pragma once
#include <QTime>
#include <QHash>
class CTimeKeeper
{
public:
	CTimeKeeper() = default;
	~CTimeKeeper() = default;
public:
	void startMonitor();
	void addTimePoint(QString explaination);
	int queryTimePoint(QString explaination);
	int queryTimePoint(QString explaination1, QString explaination2);
	QString queryTimePointString(QString explaination1, QString explaination2);
	QString queryTimePointString(QString explaination);
	QString queryTimePointString();
private:
	QTime m_tStart;
	QHash<QString, int> m_hashTimePoint;
	QStringList m_listTimePoint;
};

