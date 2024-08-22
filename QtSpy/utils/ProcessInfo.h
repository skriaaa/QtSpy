#pragma once
namespace ProcessInfo
{
	// 获取进程当前内存占用大小（字节）
	int queryProcessCurrentMemory();

	// 获取进程当前CPU占用（百分比）
	int queryProcessCpu();
};

