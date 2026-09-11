#include "LogRecorder.h"
#include "stdarg.h"
#include "stdio.h"
#include <atomic>
#include <QFile>
#include <QtGlobal>
#include <QTime>
#include <QQueue>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTextStream>
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFileInfo>
extern QString s_strDllPath;
extern QString s_strLogRootPath;
namespace
{
	QString logRootPath();
	void appendLogText(const QString& strText);
}

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
		stop();
	}
	void stop()
	{
		m_bRun.store(false);
		m_waitCondition.wakeOne();
		if (QThread::currentThread() != this)
		{
			wait();
		}
	}
public:
	virtual void run() override
	{
		QDir logDir(logRootPath());
		logDir.mkpath("log");
		QString strLogPath = logDir.absoluteFilePath("log") + "/";
		strLogPath += QCoreApplication::applicationName() + "_" + QString::number(QCoreApplication::applicationPid()) + "_";
		strLogPath += QDate::currentDate().toString("yyyyMMdd") + ".log";

		QFile file(strLogPath);
		if (false == file.open(QIODevice::Append | QIODevice::Text))
		{
			return;
		}

		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		while (true)
		{
			QQueue<QString> queueLogs = takeLogs();
			bool bHasLog = !queueLogs.empty();
			while (!queueLogs.empty())
			{
				QString strLog = queueLogs.front();
				queueLogs.pop_front();
				stream << strLog;
			}
			if (bHasLog)
			{
				stream.flush();
			}
			if (!m_bRun.load() && queueLogs.empty() && !hasLog())
			{
				break;
			}

			QMutexLocker lock(&m_mutex);
			if (m_queueLog.empty() && m_bRun.load())
			{
				m_waitCondition.wait(&m_mutex, 20);
			}
		}
		stream.flush();
	}
	void addLog(QString strLog)
	{
		QMutexLocker lock(&m_mutex);
		if (LOG_QUEUE_MAX <= m_queueLog.size())
		{
			++m_nDroppedCount;
			return;
		}
		m_queueLog.append(strLog);
		m_waitCondition.wakeOne();
	}
	QQueue<QString> takeLogs()
	{
		QQueue<QString> queueLogs;
		QMutexLocker lock(&m_mutex);
		if (0 < m_nDroppedCount)
		{
			m_queueLog.prepend(QString("[QtSpy] dropped %1 log lines because log queue is full\n").arg(m_nDroppedCount));
			m_nDroppedCount = 0;
		}
		queueLogs.swap(m_queueLog);
		return queueLogs;
	}
	bool hasLog()
	{
		QMutexLocker lock(&m_mutex);
		return !m_queueLog.empty();
	}
private:
	QMutex m_mutex;
	QWaitCondition m_waitCondition;
	std::atomic<bool> m_bRun;
	QQueue<QString> m_queueLog;
	int m_nDroppedCount = 0;
	static const int LOG_QUEUE_MAX = 10000;
};

namespace
{
	Q_GLOBAL_STATIC(CLogThread, g_logThread)
	std::atomic<bool> g_bLogShutdown(false);
	std::atomic<bool> g_bConnectQuit(false);

	QString logRootPath()
	{
		if (!s_strLogRootPath.isEmpty())
		{
			QDir dir(s_strLogRootPath);
			do
			{
				if (QFileInfo::exists(dir.absoluteFilePath("QtInjector.exe")))
				{
					return dir.absolutePath();
				}
			} while (dir.cdUp());
		}
		if (!s_strDllPath.isEmpty())
		{
			return s_strDllPath;
		}
		return QCoreApplication::applicationDirPath();
	}

	CLogThread* logThread()
	{
		if (g_bLogShutdown.load())
		{
			return nullptr;
		}
		if (g_logThread.isDestroyed())
		{
			return nullptr;
		}
		CLogThread* pThread = g_logThread();
		if ((nullptr != QCoreApplication::instance()) && !g_bConnectQuit.exchange(true))
		{
			QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, []() {
				CLogRecorder::shutdown();
			});
		}
		return pThread;
	}

	void appendLogText(const QString& strText)
	{
		QString strTime = QTime::currentTime().toString("hh:mm:ss:zzz");
		QString strLog = strTime + " | " + strText + "\n";
		CLogThread* pThread = logThread();
		if (nullptr != pThread)
		{
			pThread->addLog(strLog);
		}
	}
}

CLogRecorder& CLogRecorder::instance()
{
	static CLogRecorder recorder;
	return recorder;
}

void CLogRecorder::shutdown()
{
	g_bLogShutdown.store(true);
	if (g_logThread.exists() && !g_logThread.isDestroyed())
	{
		g_logThread->stop();
	}
}

void CLogRecorder::addLog(const char* szLog)
{
	if (nullptr == szLog)
	{
		return;
	}
	appendLogText(QString::fromUtf8(szLog));
}

void CLogRecorder::addLogVar(const char* szFormat,...)
{
	va_list arrParam;
	va_start(arrParam, szFormat);
	char szBuff[1024] = { 0 };
	vsnprintf(szBuff, sizeof(szBuff) - 1, szFormat, arrParam);
	va_end(arrParam);
	appendLogText(QString::fromUtf8(szBuff));
}

void CLogRecorder::addLog(QString strFormat, QVariantList arrArgs)
{
	for (int nIndex = 0; nIndex < arrArgs.size(); nIndex++)
	{
		strFormat = strFormat.arg(arrArgs[nIndex].toString());
	}
	appendLogText(strFormat);
}
