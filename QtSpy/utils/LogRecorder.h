#pragma once
#include <QString>
#include <QVariantList>
class CLogRecorder
{
public:
	static CLogRecorder& instance();
	static void shutdown();
	void addLog(const char* szLog);
	void addLogVar(const char* szFormat, ...);
	void addLog(QString strFormat, QVariantList arrArgs = {});
};
#define LogRecorder() CLogRecorder::instance()

