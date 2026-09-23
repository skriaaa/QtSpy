#pragma once
#include <QtCore/QtGlobal>
#include <QString>

// dbghelp 符号解析公共设施(进程内单份)。
// 抽自 AllocProfiler: dbghelp 一个进程只能 SymInitialize 一次,
// 内存监控/连接分析等多处需要"地址 -> 符号/file:line"时统一走这里, 避免重复初始化互相踩。
// 所有接口线程安全(内部缓存带锁); 符号加载在后台线程, 未就绪时 resolve 类接口返回空。
namespace SymbolResolver
{
	// 幂等: 后台线程初始化 dbghelp 并加载 pdb, 不阻塞 UI 与目标进程
	void ensureAsync();
	// 符号是否已就绪
	bool ready();

	// 地址 -> 符号("name+0x偏移", 无符号时为 "0x地址") + "文件:行"(无源行为空)
	void resolve(quint64 addr, QString& symbolOut, QString& fileLineOut);
	// 只要符号名: 增量链接 ILT thunk 自动跟 E9 跳转取真实符号; 未 demangle 自动 UnDecorate
	// 无符号/未就绪返回空串
	QString symbolName(quint64 addr);
	// 只要 "文件:行", 无返回空串
	QString fileLineOf(quint64 addr);

	// 地址所属模块 basename(小写, 如 "targetapp.exe"), 带缓存; 失败返回空
	QString moduleBasename(quint64 addr);
	// 地址所属模块 HMODULE(失败 0), 与 moduleBasename 共享缓存
	quint64 moduleHandleOf(quint64 addr);

	// lambda 反查: strLambdaHash 形如 "<lambda_十六进制>"(取自 QFunctorSlotObject 符号名),
	// addrInModule 用于定位所属模块; 在该模块符号表中找 <lambda_hash>::operator() 的
	// "文件:行"(即 lambda 定义处), 找不到返回空。按模块缓存, 单趟 SymEnumSymbols。
	QString lambdaOperatorFileLine(quint64 addrInModule, const QString& strLambdaHash);
};
