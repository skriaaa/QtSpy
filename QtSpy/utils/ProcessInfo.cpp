#include "ProcessInfo.h"
#ifdef _WINDOWS
#include "windows.h"
#include "psapi.h"
#endif

int ProcessInfo::queryProcessCurrentMemory()
{
#ifdef _WINDOWS
	PROCESS_MEMORY_COUNTERS memInfo;
	GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo));
	return memInfo.WorkingSetSize/1024/1024;
#endif
}

int ProcessInfo::queryProcessCpu()
{
	return 0;
}
