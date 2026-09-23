#pragma once
#include <QString>
#include <QVariantList>
class CLogRecorder
{
public:
	static CLogRecorder& instance();
	static void shutdown();
	// 当前正在写入的日志文件完整路径(与写入线程共用同一路径, 跨天也不会错位)
	static QString logFilePath();
	void addLog(const char* szLog);
	void addLogVar(const char* szFormat, ...);
	void addLog(QString strFormat, QVariantList arrArgs = {});
};
#define LogRecorder() CLogRecorder::instance()

