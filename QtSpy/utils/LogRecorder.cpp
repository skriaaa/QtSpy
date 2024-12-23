#include "LogRecorder.h"
#include "stdarg.h"
#include <atomic>
#include <QFile>
#include <QTime>
#include <QQueue>
#include <QThread>
#include <QMutex>
#include <QTextStream>
#include <QCoreApplication>
#include "publicfunction.h"

class CLogThread:public QThread
{
public:
	CLogThread()
	{
		m_bRun.store(true);
		QThread::start();
	}
	~CLogThread()
	{
		m_bRun.store(false);
		while (!isFinished());	// 等待结束
	}
public:
	virtual void run() override
	{
		QFile file(QCoreApplication::applicationDirPath() + "/" + QDate::currentDate().toString("yyyyMMdd") + ".log");
		if (false == file.open(QIODevice::Append))
		{
			return;
		}

		do
		{
			QString strLog = popLog();
			QTextStream stream(&file);
			do
			{
				if (strLog.isEmpty())
				{
					break;
				}
				stream << strLog.toLocal8Bit();
				strLog = popLog();
			} while (true);

			if(!m_bRun.load())
			{
				break;
			}

			QThread::msleep(20);
		} while (true);
		file.close();
	}
	void addLog(QString strLog)
	{
		QMutexLocker lock(&m_mutex);
		m_queueLog.append(strLog);
	}
	QString popLog()
	{
		if(m_queueLog.empty())
		{
			return "";
		}
		QString strData = m_queueLog.back();
		m_queueLog.pop_back();
		return strData;
	}
private:
	QMutex m_mutex;
	std::atomic<bool> m_bRun;
	QQueue<QString> m_queueLog;
};
CLogRecorder& CLogRecorder::instance()
{
	static CLogRecorder recorder;
	return recorder;
}

void CLogRecorder::addLog(const char* szLog)
{
	QString strTime = QTime::currentTime().toString("hh:mm:ss:zzz");
	QString strLog = strTime + " | " + szLog + "\n";

	static CLogThread thread;
	thread.addLog(strLog);
}

void CLogRecorder::addLogVar(const char* szFormat,...)
{
	va_list arrParam;
	va_start(arrParam, szFormat);
	char szBuff[1024] = { 0 };
	snprintf(szBuff, sizeof(szBuff) - 1, szFormat, arrParam);
	va_end(arrParam);
	addLog(szBuff);
}

void CLogRecorder::addLog(QString strFormat, QVariantList arrArgs)
{
	for (int nIndex = 0; nIndex < arrArgs.size(); nIndex++)
	{
		strFormat = strFormat.arg(arrArgs[nIndex].toString());
	}
	addLog(strFormat.toStdString().c_str());
}