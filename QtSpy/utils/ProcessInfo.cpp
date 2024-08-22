#include "ProcessInfo.h"
#include "windows.h"
#include "psapi.h"

int ProcessInfo::queryProcessCurrentMemory()
{
	PROCESS_MEMORY_COUNTERS memInfo;
	GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo));
	return memInfo.WorkingSetSize/1024/1024;
}

int ProcessInfo::queryProcessCpu()
{
	return 0;
}
