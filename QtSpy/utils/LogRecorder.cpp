#include "LogRecorder.h"
#include "stdarg.h"
#include <QFile>
#include <QTime>
#include <QCoreApplication>

CLogRecorder& CLogRecorder::instance()
{
	static CLogRecorder recorder;
	return recorder;
}

void CLogRecorder::addLog(const char* szLog)
{
	QString strTime = QTime::currentTime().toString("hh:mm:ss:zzz");
	QString strLog = strTime + " | " + szLog + "\n";

	QFile file(QCoreApplication::applicationDirPath()+ "/" + QDate::currentDate().toString("yyyyMMdd") + ".log");
	if (false == file.open(QIODevice::Append))
	{
		return;
	}
	file.write(strLog.toUtf8());
	file.close();
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

