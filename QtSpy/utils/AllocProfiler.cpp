#include "AllocProfiler.h"
#include "SymbolResolver.h"
#include <QMutex>
#include <QMutexLocker>
#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QFileInfo>
#include <algorithm>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <Psapi.h>
#include <detours/detours.h>
#include <DbgHelp.h>
#include <QThread>
#include <QRegularExpression>
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#pragma comment(lib, "Psapi.lib")
#endif

// ============================================================
// 平台无关: 无 Win 实现时全部为空操作
// ============================================================
#ifndef Q_OS_WIN
namespace AllocProfiler {
	void attach() {}
	void detach() {}
	bool isAttached() { return false; }
	bool symbolsReady() { return false; }
	QVector<SiteView> topSites(int, bool) { return {}; }
	void resetStats() {}
	void setMinSize(quint64) {}
	QVector<FrameInfo> backtraceForSite(quint64) { return {}; }
	void takeMark() {}
	void clearMark() {}
	QVector<SiteView> diffVsMark(int, bool) { return {}; }
	HeapSummary heapSnapshot() { return {}; }
}
#else

// ============================================================
// Win 实现
// ============================================================
namespace {

constexpr int kMaxFrames = 32;

typedef unsigned short (WINAPI *fnRtlCaptureStackBackTrace)(unsigned long, unsigned long, void**, unsigned long*);
fnRtlCaptureStackBackTrace backTraceFn()
{
	static fnRtlCaptureStackBackTrace p = []() -> fnRtlCaptureStackBackTrace {
		HMODULE m = GetModuleHandleW(L"ntdll.dll");
		return m ? reinterpret_cast<fnRtlCaptureStackBackTrace>(GetProcAddress(m, "RtlCaptureStackBackTrace")) : nullptr;
	}();
	return p;
}

// 挂 malloc (operator new 内部一定走 malloc 路径),
// Debug CRT 下 delete 只走 _free_dbg 不走 free, 所以 free 侧改挂 _free_dbg。
// _free_dbg 内部会兜底所有 free/delete 路径, 只挂它就够, 避免双拦。
typedef void* (__cdecl* MallocFunc) (size_t);
typedef void  (__cdecl* FreeFunc)   (void*);
typedef void  (__cdecl* FreeDbgFunc)(void*, int);
MallocFunc  TrueMalloc  = (MallocFunc)malloc;
FreeFunc    TrueFree    = (FreeFunc)  free;
FreeDbgFunc TrueFreeDbg = nullptr;   // 有值时挂它, 无值(Release CRT)则回退挂 TrueFree

// ---- 异步 free 队列 ----
// 全局 monotonic 序号 g_seq: alloc 事件与 free 事件都拿一个 seq。
// alloc 时把 seq 存进 g_addr[ptr].seq; free 事件入队时也带 seq。
// 消费者 recordFree 时对比: free.seq < g_addr[ptr].seq → 陈旧事件, 丢弃。
// 这样解决"free 事件延迟处理时, 同地址已被后续 alloc 复用"导致的误抵消。
constexpr int kFreeQCap  = 1 << 16;
constexpr int kFreeQMask = kFreeQCap - 1;
struct FreeEvent { void* ptr; LONG seq; };
static FreeEvent g_freeQ[kFreeQCap];
static volatile LONG g_freeHead = 0;
static volatile LONG g_freeTail = 0;
static volatile LONG g_seq = 0;   // 全局单调递增, alloc/free 都取一份
static HANDLE g_consumerThread   = nullptr;
static HANDLE g_consumerStopEvt  = nullptr;

inline LONG nextSeq() { return InterlockedIncrement(&g_seq); }


static thread_local int tls_depth = 0;  // 递归守卫: 跳过 profiler 自身分配

struct AddrInfo { quint64 hash; size_t size; LONG seq; };
struct SiteStats
{
	quint64 stackHash = 0;
	quint64 liveBytes = 0;
	quint64 liveCount = 0;
	quint64 totalBytes = 0;
	quint64 totalCount = 0;
	void*   frames[kMaxFrames] = { nullptr };
	int     frameCount = 0;
};

QMutex g_mutex;
QHash<quint64, SiteStats> g_sites;     // 按调用点 hash 聚合
QHash<void*, AddrInfo>    g_addr;      // addr -> (site hash, size) 用于 delete 配对
QSet<void*>  g_mark;                   // 快照 diff: 标记时刻所有 live 块地址
quint64 g_minSize   = 0;               // >=此大小才记(默认 0, 业务过滤已在 RA 层做)
bool    g_attached  = false;

// (业务模块判断缓存改成每线程 tiny cache, 见 tls_raCache)

HMODULE g_hMainExe = nullptr;          // 目标主 exe 模块, 业务帧优先选它

// 符号机制本体在 utils/SymbolResolver(dbghelp 进程内单份), 这里保留原有函数形态做转发
void ensureSymbolsAsync()
{
	SymbolResolver::ensureAsync();
}

// 取地址所属模块的 basename(小写)+HMODULE。缓存与实现在 SymbolResolver, 这里只做形态适配
QString moduleBasenameOf(DWORD64 addr, HMODULE* hModOut = nullptr)
{
	if (hModOut)
		*hModOut = reinterpret_cast<HMODULE>(SymbolResolver::moduleHandleOf(addr));
	return SymbolResolver::moduleBasename(addr);
}

// 是否为应跳过的非业务模块(Qt/系统/CRT/QtSpy 自身)
bool isSkipModule(const QString& name)
{
	if (name.isEmpty()) return false;
	static const char* kExact[] = { "ucrtbase.dll", "ntdll.dll", "kernel32.dll", "user32.dll", "gdi32.dll",
	                                "dbghelp.dll", "detours.dll", "qtspyd.dll", "qtspy.dll", nullptr };
	for (int i = 0; kExact[i]; ++i)
		if (name == QString::fromLatin1(kExact[i])) return true;
	static const char* kPrefix[] = { "qt5", "qt6", "qwindows", "vcruntime", "msvcp", "msvcr", "api-ms", "concrt", nullptr };
	for (int i = 0; kPrefix[i]; ++i)
		if (name.startsWith(QString::fromLatin1(kPrefix[i]))) return true;
	return false;
}

// hot path: 每线程一个 tiny 缓存(HMODULE -> bool), 命中率极高, 无锁
struct RACache
{
	static constexpr int N = 32;
	HMODULE keys[N] = { 0 };
	char    vals[N] = { 0 };  // 0=false, 1=true
	int     nextSlot = 0;
	bool lookup(HMODULE h, bool& out) const
	{
		for (int i = 0; i < N; ++i)
			if (keys[i] == h) { out = vals[i] != 0; return true; }
		return false;
	}
	void insert(HMODULE h, bool v)
	{
		keys[nextSlot] = h;
		vals[nextSlot] = v ? 1 : 0;
		nextSlot = (nextSlot + 1) % N;
	}
};
static thread_local RACache tls_raCache;

// hot path 版本: 只判断 RA 所在模块是否业务模块, 每线程 HMODULE 级 tiny 缓存, 完全无锁
bool isBusinessReturnAddress(void* ra)
{
	if (!ra) return false;
	HMODULE hMod = nullptr;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                        reinterpret_cast<LPCWSTR>(ra), &hMod))
		return false;
	if (!hMod)
		return false;
	bool cached = false;
	if (tls_raCache.lookup(hMod, cached))
		return cached;
	// 未缓存, 走一次完整判断
	WCHAR buf[MAX_PATH];
	DWORD n = GetModuleFileNameW(hMod, buf, MAX_PATH);
	bool business = false;
	if (n > 0)
	{
		QString full = QString::fromWCharArray(buf, int(n));
		int cut = qMax(full.lastIndexOf(QChar('/')), full.lastIndexOf(QChar('\\')));
		QString name = ((cut >= 0) ? full.mid(cut + 1) : full).toLower();
		business = !isSkipModule(name);
	}
	tls_raCache.insert(hMod, business);
	return business;
}

// 选业务代表帧: 从栈顶(最内层, f=0)向下扫, 取**第一个**业务帧(离 malloc 最近的业务代码)
// 业务帧定义: 属于主 exe, 或模块名不在 isSkipModule 列表里
int pickBusinessFrame(const SiteStats& s)
{
	for (int f = 0; f < s.frameCount; ++f)
	{
		HMODULE hMod = nullptr;
		QString name = moduleBasenameOf(reinterpret_cast<DWORD64>(s.frames[f]), &hMod);
		bool isBiz = (g_hMainExe && hMod == g_hMainExe) || (!name.isEmpty() && !isSkipModule(name));
		if (isBiz)
			return f;
	}
	return 0;
}

// 解析单个地址 -> (符号, 文件:行)。无源行时 fileLine 为空。实现在 SymbolResolver
void resolveSymbol(DWORD64 addr, QString& symbolOut, QString& fileLineOut)
{
	SymbolResolver::resolve(addr, symbolOut, fileLineOut);
}

// 由 SiteStats 构造一条 SiteView: 业务代表帧解析在此完成
AllocProfiler::SiteView buildView(const SiteStats& s)
{
	AllocProfiler::SiteView v;
	v.stackHash   = s.stackHash;
	v.liveBytes   = s.liveBytes;
	v.liveCount   = s.liveCount;
	v.totalBytes  = s.totalBytes;
	v.totalCount  = s.totalCount;
	if (s.frameCount > 0)
	{
		int fi = pickBusinessFrame(s);
		QString sym, fl;
		DWORD64 addr = reinterpret_cast<DWORD64>(s.frames[fi]);
		resolveSymbol(addr, sym, fl);
		// 若 file:line 落在 CRT / STL / MSVC / Qt 源码, 沿栈继续向外(fi+1)找下一个"业务模块且非上述源"的帧
		static const QRegularExpression qtVerRe(
			QStringLiteral("[\\\\/](qt\\d+\\.\\d+(?:\\.\\d+)?|qtbase|qtdeclarative|qt[\\\\/]src)[\\\\/]"),
			QRegularExpression::CaseInsensitiveOption);
		auto isNonBusinessSrc = [](const QString& p) {
			return p.contains(QLatin1String("\\vctools\\"), Qt::CaseInsensitive)
			    || p.contains(QLatin1String("\\vc\\tools\\msvc\\"), Qt::CaseInsensitive)
			    || p.contains(QLatin1String("\\microsoft visual studio\\"), Qt::CaseInsensitive)
			    || qtVerRe.match(p).hasMatch();
		};
		while (isNonBusinessSrc(fl))
		{
			int next = -1;
			for (int f = fi + 1; f < s.frameCount; ++f)
			{
				HMODULE hMod = nullptr;
				QString name = moduleBasenameOf(reinterpret_cast<DWORD64>(s.frames[f]), &hMod);
				bool isBiz = (g_hMainExe && hMod == g_hMainExe) || (!name.isEmpty() && !isSkipModule(name));
				if (isBiz) { next = f; break; }
			}
			if (next < 0) break;
			fi = next;
			addr = reinterpret_cast<DWORD64>(s.frames[fi]);
			resolveSymbol(addr, sym, fl);
		}
		if (sym.isEmpty())
			sym = QString("0x%1").arg(quint64(addr), 0, 16);
		v.module = moduleBasenameOf(addr);
		v.symbol = sym;
		v.fileLine = fl;
	}
	return v;
}

// 只用"业务帧连续段"参与 hash: 跳过前面的 CRT/malloc 内部帧, 从遇到的第一个业务帧开始累加,
// 遇到下一个非业务帧就停止(不包含)。这样同一段业务代码被不同上层调用时会合并成同一 site。
quint64 hashFrames(void* const* f, int n)
{
	quint64 h = 1469598103934665603ULL;  // FNV-1a offset
	bool started = false;
	for (int i = 0; i < n; ++i)
	{
		bool isBiz = isBusinessReturnAddress(f[i]);
		if (!started)
		{
			if (!isBiz) continue;      // 前置的 CRT/malloc 内部帧跳过
			started = true;
		}
		else if (!isBiz)
		{
			break;                      // 业务段结束
		}
		h ^= reinterpret_cast<quint64>(f[i]);
		h *= 1099511628211ULL;
	}
	return h;
}

void recordAlloc(void* ptr, size_t size, quint64 hash, LONG allocSeq, void* const* frames, int n)
{
	if (n <= 0 || hash == 0)
		return;
	QMutexLocker l(&g_mutex);
	auto& s = g_sites[hash];
	if (s.stackHash == 0)
	{
		s.stackHash = hash;
		s.frameCount = (n < kMaxFrames) ? n : kMaxFrames;
		for (int i = 0; i < s.frameCount; ++i)
			s.frames[i] = frames[i];
	}
	s.liveBytes  += size;  s.liveCount++;
	s.totalBytes += size;  s.totalCount++;
	// 地址复用: 若 ptr 已在表里, 说明前一次 alloc 的 free 事件被延迟/STALE 掉了没匹配上。
	// 地址能被 alloc 拿到说明前一次一定已经 free 过了, 直接把旧条目从对应 site 抵消掉。
	// (与 seq STALE 逻辑配合: STALE 保护"当前条目不被陈旧 free 误抵消", 这里补上"旧条目的会计")
	auto old = g_addr.find(ptr);
	if (old != g_addr.end())
	{
		auto sit = g_sites.find(old.value().hash);
		if (sit != g_sites.end())
		{
			if (sit.value().liveCount > 0) sit.value().liveCount--;
			if (sit.value().liveBytes >= old.value().size) sit.value().liveBytes -= old.value().size;
		}
	}
	g_addr[ptr] = AddrInfo{ hash, size, allocSeq };
	WCHAR msg[160];
	wsprintfW(msg, L"[AllocProfiler] recordAlloc ptr=%p size=%u hash=%08x seq=%ld live=%u\n",
	          ptr, unsigned(size), unsigned(hash & 0xFFFFFFFF), allocSeq, unsigned(s.liveCount));
	OutputDebugStringW(msg);
}

void recordFree(void* ptr, LONG freeSeq)
{
	if (!ptr)
		return;
	QMutexLocker l(&g_mutex);
	auto it = g_addr.find(ptr);
	if (it == g_addr.end())
		return;
	// 陈旧 free 事件: free 发生在这条 alloc 之前 → 说明这条 alloc 是 free 之后同地址复用, 忽略
	if (freeSeq < it.value().seq)
	{
		WCHAR msg[160];
		wsprintfW(msg, L"[AllocProfiler] recordFree STALE ptr=%p freeSeq=%ld allocSeq=%ld (skip)\n",
		          ptr, freeSeq, it.value().seq);
		OutputDebugStringW(msg);
		return;
	}
	quint64 hash = it.value().hash;
	size_t size  = it.value().size;
	g_addr.erase(it);
	auto sit = g_sites.find(hash);
	quint64 liveAfter = 0;
	if (sit != g_sites.end())
	{
		if (sit.value().liveCount > 0) sit.value().liveCount--;
		if (sit.value().liveBytes >= size) sit.value().liveBytes -= size;
		liveAfter = sit.value().liveCount;
	}
	WCHAR msg[160];
	wsprintfW(msg, L"[AllocProfiler] recordFree HIT ptr=%p size=%u hash=%08x live_after=%u\n",
	          ptr, unsigned(size), unsigned(hash & 0xFFFFFFFF), unsigned(liveAfter));
	OutputDebugStringW(msg);
}

} // namespace

// ---- 钩子: malloc/free ----
// 只有 RA 落在业务模块时才抓栈+记账; RA 不业务直接透传, 避免每次 malloc 都抓 32 帧
// 语义: 匹配"业务代码里直接调 new/malloc"; 业务 new 内部 Qt 再 malloc 的部分不算(RA 落在 Qt5Core)
void* __cdecl HookMalloc(size_t size)
{
	void* ra = _ReturnAddress();
	void* ptr = TrueMalloc(size);
	// 关键: alloc 事件的 seq 必须在 malloc 返回之后**立即**拿, 保证与"下一个 free 事件"严格全序
	// (若之前有 free 事件 seq=S1, 这次 alloc seq=S2 > S1; 后续同地址的 free 事件必然 seq=S3 > S2)
	LONG mySeq = nextSeq();
	if (tls_depth != 0 || !ptr || size < g_minSize)
		return ptr;
	tls_depth = 1;
	if (isBusinessReturnAddress(ra))
	{
		void* frames[kMaxFrames];
		fnRtlCaptureStackBackTrace bt = backTraceFn();
		int n = bt ? bt(1, kMaxFrames, frames, nullptr) : 0;
		quint64 h = (n > 0) ? hashFrames(frames, n) : 0;
		recordAlloc(ptr, size, h, mySeq, frames, n);
	}
	tls_depth = 0;
	return ptr;
}

inline void enqueueFree(void* ptr, LONG seq)
{
	if (!ptr) return;
	LONG idx = InterlockedIncrement(&g_freeHead) - 1;
	LONG tail = g_freeTail;
	if (idx - tail >= kFreeQCap)
	{
		g_freeQ[idx & kFreeQMask] = FreeEvent{ nullptr, 0 };
		return;
	}
	g_freeQ[idx & kFreeQMask] = FreeEvent{ ptr, seq };
}

void __cdecl HookFree(void* ptr)
{
	if (tls_depth == 0 && ptr)
	{
		tls_depth = 1;
		// free 的 seq 在 TrueFree 之前拿: 这个 seq 一定小于 "此地址被下一次 alloc 拿到" 的 seq
		LONG mySeq = nextSeq();
		enqueueFree(ptr, mySeq);
		tls_depth = 0;
	}
	TrueFree(ptr);
}

void __cdecl HookFreeDbg(void* ptr, int blockType)
{
	if (tls_depth == 0 && ptr)
	{
		tls_depth = 1;
		LONG mySeq = nextSeq();
		enqueueFree(ptr, mySeq);
		tls_depth = 0;
	}
	TrueFreeDbg(ptr, blockType);
}


// free 事件消费者线程: 定期批量取出队列里的 ptr, 走 recordFree
DWORD WINAPI freeConsumerProc(LPVOID)
{
	for (;;)
	{
		DWORD w = WaitForSingleObject(g_consumerStopEvt, 50);
		LONG head = g_freeHead;
		LONG tail = g_freeTail;
		int budget = 4096;
		while (tail < head && budget > 0)
		{
			FreeEvent ev = g_freeQ[tail & kFreeQMask];
			g_freeTail = ++tail;
			--budget;
			if (ev.ptr) recordFree(ev.ptr, ev.seq);
		}
		if (w == WAIT_OBJECT_0)
			break;
	}
	return 0;
}

// ---- 公开 API ----
namespace AllocProfiler {

// 定位目标进程实际使用的 CRT: 优先 ucrtbased / ucrtbase, 再回退 msvcr*d
static HMODULE findCrtModule()
{
	static const wchar_t* kNames[] = {
		L"ucrtbased.dll", L"ucrtbase.dll",
		L"msvcr120d.dll", L"msvcr120.dll",
		L"msvcr140d.dll", L"msvcr140.dll",
		L"msvcr110d.dll", L"msvcr100d.dll",
		nullptr
	};
	for (int i = 0; kNames[i]; ++i)
	{
		HMODULE h = GetModuleHandleW(kNames[i]);
		if (h) return h;
	}
	return nullptr;
}

void attach()
{
	if (g_attached)
		return;
	ensureSymbolsAsync();  // 后台加载 pdb, 不阻塞 UI(见下方 ensureSymbolsAsync 包装)
	g_hMainExe = GetModuleHandleW(nullptr);  // 原先在符号初始化线程里顺带设置, 改为主线程同步设置更确定

	// 优先从目标进程实际加载的 CRT 里取地址, 保证挂的是"它在用的那个"
	HMODULE hCrt = findCrtModule();
	if (hCrt)
	{
		auto pm  = GetProcAddress(hCrt, "malloc");
		auto pf  = GetProcAddress(hCrt, "free");
		auto pfd = GetProcAddress(hCrt, "_free_dbg");
		if (pm)  TrueMalloc  = reinterpret_cast<MallocFunc>(pm);
		if (pf)  TrueFree    = reinterpret_cast<FreeFunc>(pf);
		if (pfd) TrueFreeDbg = reinterpret_cast<FreeDbgFunc>(pfd);
	}

	// 启动 free 消费者线程 (只启一次)
	if (!g_consumerStopEvt) g_consumerStopEvt = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	ResetEvent(g_consumerStopEvt);
	g_freeHead = 0; g_freeTail = 0;
	if (!g_consumerThread)
		g_consumerThread = CreateThread(nullptr, 0, freeConsumerProc, nullptr, 0, nullptr);

	// Debug CRT: 挂 _free_dbg (delete 和 free 都会走它, 一次覆盖); Release CRT: 挂 free
	bool useDbg = (TrueFreeDbg != nullptr);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	LONG e1 = DetourAttach(&(PVOID&)TrueMalloc, HookMalloc);
	LONG e2 = useDbg ? DetourAttach(&(PVOID&)TrueFreeDbg, HookFreeDbg)
	                 : DetourAttach(&(PVOID&)TrueFree,    HookFree);
	LONG err = DetourTransactionCommit();
	g_attached = (err == NO_ERROR);
	// 诊断
	WCHAR crtName[MAX_PATH] = L"(none)";
	if (hCrt) GetModuleFileNameW(hCrt, crtName, MAX_PATH);
	WCHAR msg[512];
	wsprintfW(msg, L"[AllocProfiler] attach: crt=%s useDbg=%d e1=%ld e2=%ld commit=%ld attached=%d\n",
	          crtName, useDbg ? 1 : 0, e1, e2, err, g_attached ? 1 : 0);
	OutputDebugStringW(msg);
}

bool symbolsReady()
{
	return SymbolResolver::ready();}

void detach()
{
	if (!g_attached)
		return;
	bool useDbg = (TrueFreeDbg != nullptr);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)TrueMalloc, HookMalloc);
	if (useDbg) DetourDetach(&(PVOID&)TrueFreeDbg, HookFreeDbg);
	else        DetourDetach(&(PVOID&)TrueFree,    HookFree);
	DetourTransactionCommit();
	g_attached = false;

	// 停消费者线程, 等它退出后清资源
	if (g_consumerThread)
	{
		SetEvent(g_consumerStopEvt);
		WaitForSingleObject(g_consumerThread, 500);
		CloseHandle(g_consumerThread);
		g_consumerThread = nullptr;
	}
}

bool isAttached() { return g_attached; }

void resetStats()
{
	QMutexLocker l(&g_mutex);
	g_sites.clear();
	g_addr.clear();
	g_mark.clear();
}

void setMinSize(quint64 bytes) { g_minSize = bytes; }

QVector<SiteView> topSites(int n, bool byLive)
{
	QList<SiteStats> snap;
	{
		QMutexLocker l(&g_mutex);
		snap = g_sites.values();
	}
	std::sort(snap.begin(), snap.end(), [byLive](const SiteStats& a, const SiteStats& b) {
		return byLive ? a.liveBytes > b.liveBytes : a.totalBytes > b.totalBytes;
	});
	QVector<SiteView> out;
	int cnt = qMin(n, snap.size());
	out.reserve(cnt);
	for (int i = 0; i < cnt; ++i)
		out.append(buildView(snap[i]));
	return out;
}

QVector<FrameInfo> backtraceForSite(quint64 stackHash)
{
	SiteStats s;
	{
		QMutexLocker l(&g_mutex);
		auto it = g_sites.find(stackHash);
		if (it == g_sites.end())
			return {};
		s = it.value();
	}
	QVector<FrameInfo> out;
	out.reserve(s.frameCount);
	for (int f = 0; f < s.frameCount; ++f)
	{
		DWORD64 addr = reinterpret_cast<DWORD64>(s.frames[f]);
		FrameInfo fi;
		fi.address = addr;
		HMODULE hMod = nullptr;
		fi.module = moduleBasenameOf(addr, &hMod);
		fi.business = (g_hMainExe && hMod == g_hMainExe) || (!fi.module.isEmpty() && !isSkipModule(fi.module));
		resolveSymbol(addr, fi.symbol, fi.fileLine);
		out.append(fi);
	}
	return out;
}

void takeMark()
{
	QMutexLocker l(&g_mutex);
	g_mark.clear();
	for (auto it = g_addr.constBegin(); it != g_addr.constEnd(); ++it)
		g_mark.insert(it.key());
}

void clearMark()
{
	QMutexLocker l(&g_mutex);
	g_mark.clear();
}

QVector<SiteView> diffVsMark(int n, bool byLive)
{
	// 聚合: 当前仍 live、且不在 g_mark 里的块, 按 hash 聚合
	QHash<quint64, SiteStats> agg;
	QList<SiteStats> snap;
	{
		QMutexLocker l(&g_mutex);
		for (auto it = g_addr.constBegin(); it != g_addr.constEnd(); ++it)
		{
			if (g_mark.contains(it.key()))
				continue;
			quint64 h = it.value().hash;
			size_t sz = it.value().size;
			auto& s = agg[h];
			if (s.stackHash == 0)
			{
				s.stackHash = h;
				auto sit = g_sites.find(h);
				if (sit != g_sites.end())
				{
					s.frameCount = sit.value().frameCount;
					for (int i = 0; i < s.frameCount; ++i)
						s.frames[i] = sit.value().frames[i];
				}
			}
			s.liveBytes += sz;  s.liveCount++;
			s.totalBytes += sz; s.totalCount++;
		}
		snap = agg.values();
	}
	std::sort(snap.begin(), snap.end(), [byLive](const SiteStats& a, const SiteStats& b) {
		return byLive ? a.liveBytes > b.liveBytes : a.totalBytes > b.totalBytes;
	});
	QVector<SiteView> out;
	int cnt = qMin(n, snap.size());
	out.reserve(cnt);
	for (int i = 0; i < cnt; ++i)
		out.append(buildView(snap[i]));
	return out;
}

// 收集所有堆里当前活动块的**用户数据地址**到 out
// 注意: __try/__except 段里不能有需要栈展开的对象
static void collectHeapLiveAddrs(QSet<void*>& out)
{
	HANDLE heaps[256];
	DWORD got = GetProcessHeaps(256, heaps);
	for (DWORD i = 0; i < got && i < 256; ++i)
	{
		if (!HeapLock(heaps[i]))
			continue;
		__try
		{
			PROCESS_HEAP_ENTRY entry;
			ZeroMemory(&entry, sizeof(entry));
			while (HeapWalk(heaps[i], &entry))
			{
				if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
					out.insert(entry.lpData);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		HeapUnlock(heaps[i]);
	}
}

int reapDeadAllocs()
{
	// Debug CRT 的 malloc 返回的 ptr 前面有 _CrtMemBlockHeader (32B), HeapAlloc 拿到的是 header 起始
	// 所以 HeapWalk 报的 lpData = ptr - 32; 我们的 g_addr key 是用户 ptr.
	// 简化: 建 heap 活动地址 set, 校验 g_addr 时同时判 (addr) 和 (addr - 32) 是否在里面。
	QSet<void*> live;
	collectHeapLiveAddrs(live);
	if (live.isEmpty())
		return 0;

	QMutexLocker l(&g_mutex);
	int reaped = 0;
	for (auto it = g_addr.begin(); it != g_addr.end(); )
	{
		void* p = it.key();
		bool alive = live.contains(p) ||
		             live.contains(reinterpret_cast<char*>(p) - 0x20) ||   // Debug CRT header 32B
		             live.contains(reinterpret_cast<char*>(p) - 0x30);     // 保险: 某些配置对齐到 48
		if (alive) { ++it; continue; }
		// 已死: 抵消对应 site
		auto sit = g_sites.find(it.value().hash);
		if (sit != g_sites.end())
		{
			if (sit.value().liveCount > 0) sit.value().liveCount--;
			if (sit.value().liveBytes >= it.value().size) sit.value().liveBytes -= it.value().size;
		}
		it = g_addr.erase(it);
		++reaped;
	}
	return reaped;
}

// ---- HeapWalk 堆快照 ----
// 注意: 本函数含 __try/__except, 不能有需要栈展开的对象(POD 局部变量 OK)
HeapSummary heapSnapshot()
{
	HeapSummary sum;
	HANDLE heaps[256];
	DWORD got = GetProcessHeaps(256, heaps);
	for (DWORD i = 0; i < got && i < 256; ++i)
	{
		if (!HeapLock(heaps[i]))
			continue;
		__try
		{
			PROCESS_HEAP_ENTRY entry;
			ZeroMemory(&entry, sizeof(entry));
			while (HeapWalk(heaps[i], &entry))
			{
				if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
				{
					sum.blockCount++;
					sum.totalBytes += entry.cbData;
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		HeapUnlock(heaps[i]);
	}
	return sum;
}

} // namespace AllocProfiler

#endif // Q_OS_WIN
