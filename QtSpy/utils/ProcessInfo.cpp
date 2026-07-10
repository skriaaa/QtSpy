#include "ProcessInfo.h"
#ifdef Q_OS_WIN
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#endif

namespace {
#ifdef Q_OS_WIN
	quint32 queryThreadCount()
	{
		quint32 count = 0;
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnap == INVALID_HANDLE_VALUE)
			return 0;
		DWORD pid = GetCurrentProcessId();
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(hSnap, &te))
		{
			do {
				if (te.th32OwnerProcessID == pid)
					++count;
			} while (Thread32Next(hSnap, &te));
		}
		CloseHandle(hSnap);
		return count;
	}

	quint64 fileTimeToU64(const FILETIME& ft)
	{
		ULARGE_INTEGER ul;
		ul.LowPart = ft.dwLowDateTime;
		ul.HighPart = ft.dwHighDateTime;
		return ul.QuadPart;  // 100ns 单位
	}

	// CPU% 计算的状态(进程内单例)
	struct CpuState {
		quint64 prevCpuTime = 0;   // 上次累计 kernel+user (100ns)
		quint64 prevTickMs  = 0;   // 上次采样的 GetTickCount64
	};
	CpuState g_cpuState;
#endif
}

namespace ProcessInfo {
#ifdef Q_OS_WIN
	ProcessMemoryInfo queryProcessMemoryInfo()
	{
		ProcessMemoryInfo info;
		PROCESS_MEMORY_COUNTERS_EX mc;
		mc.cb = sizeof(mc);
		if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&mc), sizeof(mc)))
		{
			info.workingSet     = mc.WorkingSetSize;
			info.peakWorkingSet = mc.PeakWorkingSetSize;
			info.privateBytes   = mc.PrivateUsage;
		}
		DWORD handles = 0;
		if (GetProcessHandleCount(GetCurrentProcess(), &handles))
			info.handleCount = handles;
		info.gdiObjects  = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
		info.userObjects = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
		info.threadCount = queryThreadCount();
		return info;
	}

	quint64 queryProcessCurrentMemory()
	{
		return queryProcessMemoryInfo().privateBytes;
	}

	int queryProcessCpu()
	{
		FILETIME ftCreate, ftExit, ftKernel, ftUser;
		if (!GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser))
			return 0;
		quint64 curCpu = fileTimeToU64(ftKernel) + fileTimeToU64(ftUser);
		quint64 curTick = GetTickCount64();
		int cpu = 0;
		if (g_cpuState.prevCpuTime && curTick > g_cpuState.prevTickMs)
		{
			quint64 dtCpu = curCpu - g_cpuState.prevCpuTime;        // 100ns
			quint64 dtWall = (curTick - g_cpuState.prevTickMs) * 10000ULL; // ms -> 100ns
			if (dtWall > 0)
				cpu = static_cast<int>((dtCpu * 100ULL) / dtWall);   // 100% = 满一个核
		}
		g_cpuState.prevCpuTime = curCpu;
		g_cpuState.prevTickMs = curTick;
		return cpu;
	}
#else
	ProcessMemoryInfo queryProcessMemoryInfo() { return ProcessMemoryInfo(); }
	quint64 queryProcessCurrentMemory() { return 0; }
	int queryProcessCpu() { return 0; }
#endif

	QString formatBytes(quint64 bytes)
	{
		double v = static_cast<double>(bytes);
		static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
		int u = 0;
		while (v >= 1024.0 && u < 4)
		{
			v /= 1024.0;
			++u;
		}
		return QString::number(v, 'f', 2) + " " + units[u];
	}
}
