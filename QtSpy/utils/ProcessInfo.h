#pragma once
#include <QtCore/QtGlobal>
#include <QString>

// 进程级资源指标采集(Windows 实现)
namespace ProcessInfo
{
	// 多项内存/资源指标聚合, 全部用 64/32 位无符号, 不再截成 int MB
	struct ProcessMemoryInfo
	{
		quint64 privateBytes    = 0;  // PrivateUsage — 主指标(已提交私有页, 泄漏面)
		quint64 workingSet      = 0;  // WorkingSetSize — 物理驻留, 会随系统 trim 波动
		quint64 peakWorkingSet  = 0;  // PeakWorkingSetSize
		quint64 handleCount     = 0;  // GetProcessHandleCount
		quint32 gdiObjects      = 0;  // GetGuiResources(GR_GDIOBJECTS)
		quint32 userObjects     = 0;  // GetGuiResources(GR_USEROBJECTS)
		quint32 threadCount     = 0;  // 进程线程数
	};

	// 聚合多项指标
	ProcessMemoryInfo queryProcessMemoryInfo();

	// 兼容旧接口: 返回 PrivateUsage(字节)
	quint64 queryProcessCurrentMemory();

	// CPU 占用百分比(基于两次调用间隔的 GetProcessTimes 差值, 100% = 满一个核)
	int queryProcessCpu();

	// 自适应格式化字节: B/KB/MB/GB/TB, 保留 2 位小数
	QString formatBytes(quint64 bytes);
};
