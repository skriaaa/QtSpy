#include "SymbolResolver.h"

#ifdef Q_OS_WIN

#include <QMutex>
#include <QMutexLocker>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QFileInfo>
#include <QRegularExpression>
#include <atomic>

#include <Windows.h>
#include <Psapi.h>
#include <DbgHelp.h>

#pragma comment(lib, "Psapi.lib")

// ============================================================
// 进程内单份的 dbghelp 状态与缓存
// ============================================================
namespace {

QMutex g_mutex;
HANDLE g_hProc = nullptr;
std::atomic<bool> g_symbolsInited{ false };

// addr -> (symbol, fileLine), AllocProfiler 的 resolve 语义
QHash<quint64, QPair<QString, QString>> g_resolveCache;
// addr -> 规范化符号名(symbolName)
QHash<quint64, QString> g_nameCache;
// addr -> "文件:行"(fileLineOf)
QHash<quint64, QString> g_fileLineCache;

struct ModInfo { QString name; quint64 hMod = 0; };
// addr -> (模块basename, HMODULE), 省掉重复 GetModuleFileNameW
QHash<quint64, ModInfo> g_moduleCache;
// HMODULE -> 已完成的 lambda 枚举(hash -> file:line)
QHash<quint64, QHash<QString, QString>> g_lambdaMapCache;
// 已枚举过 lambda 符号的 HMODULE
QSet<quint64> g_lambdaEnumDone;

// 收集当前进程所有已加载模块的目录, 组成符号搜索路径(去重)
QString buildSearchPath()
{
	QStringList dirs;
	QSet<QString> seen;
	HMODULE mods[1024];
	DWORD cbNeeded = 0;
	HANDLE hProc = GetCurrentProcess();
	if (EnumProcessModules(hProc, mods, sizeof(mods), &cbNeeded))
	{
		int count = int(cbNeeded / sizeof(HMODULE));
		WCHAR buf[MAX_PATH];
		for (int i = 0; i < count; ++i)
		{
			if (!GetModuleFileNameExW(hProc, mods[i], buf, MAX_PATH))
				continue;
			QString full = QString::fromWCharArray(buf);
			QString d = QFileInfo(full).absolutePath();
			QString key = d.toLower();
			if (!seen.contains(key))
			{
				seen.insert(key);
				dirs.append(d);
			}
		}
	}
	return dirs.join(QLatin1Char(';'));
}

// 真正执行符号初始化的重活儿, 在工作线程跑, 避免阻塞注入方 UI 与目标进程
void doSymbolInit()
{
	g_hProc = GetCurrentProcess();
	// 关键: 不使用 DEFERRED_LOADS, 强制立即加载行号表; 加 LOAD_ANYTHING 让路径不完美时也尽力
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_LOAD_ANYTHING | SYMOPT_NO_PROMPTS | SYMOPT_FAIL_CRITICAL_ERRORS);
	QString searchPath = buildSearchPath();
	// 若 dbghelp 已被别人 SymInitialize 过, 我们的 Initialize 会失败, 走 SymRefreshModuleList 强刷
	BOOL ok = SymInitializeW(g_hProc, searchPath.isEmpty() ? nullptr : (PCWSTR)searchPath.utf16(), TRUE);
	if (!ok)
	{
		// 已被初始化, 追加我们的搜索路径后强刷所有模块
		WCHAR oldPath[4096] = { 0 };
		SymGetSearchPathW(g_hProc, oldPath, 4096);
		QString merged = QString::fromWCharArray(oldPath);
		if (!searchPath.isEmpty())
			merged = merged.isEmpty() ? searchPath : (merged + QLatin1Char(';') + searchPath);
		SymSetSearchPathW(g_hProc, (PCWSTR)merged.utf16());
		SymRefreshModuleList(g_hProc);
	}
	g_symbolsInited.store(true, std::memory_order_release);
}

// SymFromAddr 原始符号名(可能是 ILT/未 demangle), 失败返回空
QString rawSymbolName(DWORD64 addr)
{
	char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = { 0 };
	auto* sym = reinterpret_cast<PSYMBOL_INFO>(buf);
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);
	sym->MaxNameLen = MAX_SYM_NAME;
	DWORD64 displacement = 0;
	if (SymFromAddr(g_hProc, addr, &displacement, sym))
	{
		QString name = QString::fromLocal8Bit(sym->Name);
		if (0 < displacement)
		{
			// ILT thunk 符号形如 "ILT+405(?onSigFooQAEXHZ)", 名字本身不带偏移语义, 不拼
			if (!name.startsWith(QStringLiteral("ILT+")))
			{
				name += QStringLiteral("+0x%1").arg(displacement, 0, 16);
			}
		}
		return name;
	}
	return QString();
}

// addr -> (模块basename, HMODULE), 带缓存(去重过的 GetModuleHandleExW + GetModuleFileNameW)
ModInfo lookupModule(quint64 addr)
{
	{
		QMutexLocker l(&g_mutex);
		auto it = g_moduleCache.find(addr);
		if (it != g_moduleCache.end())
			return it.value();
	}
	ModInfo info;
	HMODULE hMod = nullptr;
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(addr), &hMod);
	if (hMod)
	{
		info.hMod = reinterpret_cast<quint64>(hMod);
		WCHAR buf[MAX_PATH];
		DWORD n = GetModuleFileNameW(hMod, buf, MAX_PATH);
		if (0 < n)
		{
			QString full = QString::fromWCharArray(buf, int(n));
			int cut = qMax(full.lastIndexOf(QChar('/')), full.lastIndexOf(QChar('\\')));
			info.name = ((cut >= 0) ? full.mid(cut + 1) : full).toLower();
		}
	}
	QMutexLocker l(&g_mutex);
	g_moduleCache[addr] = info;
	return info;
}

} // namespace

// ============================================================
// 公开接口
// ============================================================
namespace SymbolResolver {

void ensureAsync()
{
	if (g_symbolsInited.load(std::memory_order_acquire))
		return;
	// 后台线程做符号加载, 主 exe + 所有业务 dll 的 pdb 都会走 LOAD_LINES 立即读入
	QThread* t = QThread::create([]() { doSymbolInit(); });
	QObject::connect(t, &QThread::finished, t, &QThread::deleteLater);
	t->start();
}

bool ready()
{
	return g_symbolsInited.load(std::memory_order_acquire);
}

void resolve(quint64 addr, QString& symbolOut, QString& fileLineOut)
{
	{
		QMutexLocker l(&g_mutex);
		auto it = g_resolveCache.find(addr);
		if (it != g_resolveCache.end())
		{
			symbolOut = it.value().first;
			fileLineOut = it.value().second;
			return;
		}
	}
	symbolOut.clear();
	fileLineOut.clear();
	// 符号未就绪, 只给地址; 不写缓存, 让就绪后重新解析
	if (!g_symbolsInited.load(std::memory_order_acquire))
	{
		symbolOut = QString("0x%1").arg(addr, 0, 16);
		return;
	}
	DWORD64 displacement = 0;
	char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
	auto* sym = reinterpret_cast<PSYMBOL_INFO>(buf);
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);
	sym->MaxNameLen = MAX_SYM_NAME;
	if (SymFromAddr(g_hProc, addr, &displacement, sym))
	{
		symbolOut = QString::fromLocal8Bit(sym->Name) + QString("+0x%1").arg(displacement, 0, 16);
		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(line);
		DWORD lineDisp = 0;
		if (SymGetLineFromAddr64(g_hProc, addr, &lineDisp, &line))
			fileLineOut = QString::fromLocal8Bit(line.FileName) + ":" + QString::number(line.LineNumber);
	}
	else
	{
		symbolOut = QString("0x%1").arg(addr, 0, 16);
	}
	QMutexLocker l(&g_mutex);
	g_resolveCache[addr] = qMakePair(symbolOut, fileLineOut);
}

QString symbolName(quint64 addr)
{
	{
		QMutexLocker l(&g_mutex);
		auto it = g_nameCache.find(addr);
		if (it != g_nameCache.end())
			return it.value();
	}
	// 符号未就绪不写缓存, 让就绪后重新解析
	if (!g_symbolsInited.load(std::memory_order_acquire))
	{
		return QString();
	}
	QString name = rawSymbolName(addr);
	// 增量链接 thunk(ILT+xx(...)): 是一条 E9 jmp rel32, 跟进目标地址拿真实符号
	// (thunk 的名字常是未 demangle 的, 目标处的名字是好的)
	if (name.startsWith(QStringLiteral("ILT+")))
	{
		if (0xE9 == *reinterpret_cast<const quint8*>(addr))
		{
			qint32 rel = *reinterpret_cast<const qint32*>(addr + 1);
			name = rawSymbolName(addr + 5 + rel);
		}
	}
	// 仍是 mangled 名('?')开头时手动 demangle
	if (name.startsWith(QLatin1Char('?')))
	{
		char out[1024] = { 0 };
		if (UnDecorateSymbolName(name.toLocal8Bit().constData(), out, sizeof(out), UNDNAME_COMPLETE))
			name = QString::fromLocal8Bit(out);
	}
	// 去掉 resolve 语义的 "+0x偏移" 尾巴(0 偏移本来也不拼)
	const int nPlus = name.indexOf(QLatin1String("+0x"));
	if (0 < nPlus)
	{
		name = name.left(nPlus);
	}
	QMutexLocker l(&g_mutex);
	g_nameCache[addr] = name;
	return name;
}

QString fileLineOf(quint64 addr)
{
	{
		QMutexLocker l(&g_mutex);
		auto it = g_fileLineCache.find(addr);
		if (it != g_fileLineCache.end())
			return it.value();
	}
	// 符号未就绪不写缓存, 让就绪后重新解析
	if (!g_symbolsInited.load(std::memory_order_acquire))
	{
		return QString();
	}
	QString fileLine;
	{
		IMAGEHLP_LINE64 line = { 0 };
		line.SizeOfStruct = sizeof(line);
		DWORD disp = 0;
		if (SymGetLineFromAddr64(g_hProc, addr, &disp, &line))
			fileLine = QString::fromLocal8Bit(line.FileName) + ":" + QString::number(line.LineNumber);
	}
	QMutexLocker l(&g_mutex);
	g_fileLineCache[addr] = fileLine;
	return fileLine;
}

quint64 moduleHandleOf(quint64 addr)
{
	return lookupModule(addr).hMod;
}

QString moduleBasename(quint64 addr)
{
	return lookupModule(addr).name;
}

QString lambdaOperatorFileLine(quint64 addrInModule, const QString& strLambdaHash)
{
	if (strLambdaHash.isEmpty())
	{
		return QString();
	}
	const quint64 hMod = lookupModule(addrInModule).hMod;
	if (0 == hMod)
	{
		return QString();
	}

	// 该模块的 lambda 符号表(一趟 SymEnumSymbols)只建一次
	{
		QMutexLocker l(&g_mutex);
		if (g_lambdaEnumDone.contains(hMod))
			return g_lambdaMapCache[hMod].value(strLambdaHash);
	}
	if (!g_symbolsInited.load(std::memory_order_acquire))
	{
		return QString();
	}

	// MSVC 的 lambda 闭包类型符号名带 <lambda_十六进制> 后缀;
	// 枚举模块符号找 "<lambda_xxx>::operator()" 形态, 拿它地址的行号即 lambda 定义处
	static const QRegularExpression reHash(QStringLiteral("<lambda_[0-9a-f]+>"));
	struct EnumCtx { QHash<QString, QString>* map; } ctx = { nullptr };
	QHash<QString, QString> map;
	ctx.map = &map;
	SymEnumSymbols(g_hProc, hMod, nullptr, [](PSYMBOL_INFO sym, ULONG, PVOID user) -> BOOL {
		auto* c = reinterpret_cast<EnumCtx*>(user);
		// 名字形如 "<lambda_xxx>::operator()" 的是 lambda 调用运算符本体
		const QString name = QString::fromLocal8Bit(sym->Name);
		if (!name.contains(QLatin1String("::operator()")) || !name.contains(QLatin1String("<lambda_")))
		{
			return TRUE;
		}
		const auto match = reHash.match(name);
		if (!match.hasMatch())
		{
			return TRUE;
		}
		const QString hash = match.captured(0);
		if (c->map->contains(hash))
		{
			return TRUE;  // 同名符号(如 ILT/thunk)可能多个, 保留首个
		}
		IMAGEHLP_LINE64 line = { 0 };
		line.SizeOfStruct = sizeof(line);
		DWORD disp = 0;
		if (SymGetLineFromAddr64(g_hProc, sym->Address, &disp, &line))
		{
			(*c->map)[hash] = QString::fromLocal8Bit(line.FileName) + ":" + QString::number(line.LineNumber);
		}
		return TRUE;
	}, &ctx);

	QMutexLocker l(&g_mutex);
	g_lambdaMapCache[hMod] = map;
	g_lambdaEnumDone.insert(hMod);
	return map.value(strLambdaHash);
}

} // namespace

#else  // !Q_OS_WIN

namespace SymbolResolver {
void ensureAsync() {}
bool ready() { return false; }
void resolve(quint64, QString&, QString&) {}
QString symbolName(quint64) { return QString(); }
QString fileLineOf(quint64) { return QString(); }
QString moduleBasename(quint64) { return QString(); }
quint64 moduleHandleOf(quint64) { return 0; }
QString lambdaOperatorFileLine(quint64, const QString&) { return QString(); }
}

#endif
