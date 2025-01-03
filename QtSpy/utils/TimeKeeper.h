#pragma once
#include <QTime>
#include <QHash>
#include <QElapsedTimer>
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
	QElapsedTimer m_Timer;
	QHash<QString, qint64> m_hashTimePoint;
	QStringList m_listTimePoint;
};

