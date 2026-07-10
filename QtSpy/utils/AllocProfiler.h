#pragma once
#include <QtCore/QtGlobal>
#include <QString>
#include <QVector>

// 按调用点聚合的分配分析器(detours hook malloc/free, 全线程覆盖)
// 设计要点:
//  - hot path 只抓栈+聚合, 不做符号解析(解析延后到 topSites 显示时)
//  - 默认只统计 >= minSize 的块, 避免海量小分配拖垮
//  - free 配对靠 addr->(hash,size) 表, 有条目上限, 超限只记 gross
//  - thread_local 递归守卫跳过 profiler 自身分配
namespace AllocProfiler
{
	struct SiteView
	{
		quint64 stackHash  = 0;  // 用于 backtraceForSite 反查全栈
		QString module;     // 调用点所在模块 basename
		QString symbol;     // 调用点符号 "func+0x.."
		QString fileLine;   // 源文件:行
		quint64 liveBytes  = 0;
		quint64 liveCount  = 0;
		quint64 totalBytes = 0;
		quint64 totalCount = 0;
	};

	// 单帧解析结果(全栈展开用)
	struct FrameInfo
	{
		QString module;    // 所属模块 basename
		QString symbol;    // 符号 "func+0x.."
		QString fileLine;  // 源文件:行
		quint64 address = 0;
		bool    business = false;  // 是否业务帧(主 exe 或非跳过模块)
	};

	// 安装/卸载 malloc/free 钩子(detours)
	void attach();
	void detach();
	bool isAttached();

	// 符号初始化是否已完成(attach 后异步进行, UI 可据此提示"符号加载中")
	bool symbolsReady();

	// 返回 top-N 调用点(byLive=true 按现存字节, false 按累计字节); 代表帧取业务代码, 符号在此解析
	QVector<SiteView> topSites(int n, bool byLive);

	// 清零聚合统计(不卸载钩子)
	void resetStats();

	// 调参: 仅统计 >= minSize 字节的分配(默认 4096)
	void setMinSize(quint64 bytes);

	// 返回某调用点的完整回溯(逐帧解析), 双击行时调一次
	QVector<FrameInfo> backtraceForSite(quint64 stackHash);

	// === 快照 diff: 标记 vs 现在, 找新增且未释放的块 ===
	void takeMark();                                   // 快照当前所有 live 块地址
	void clearMark();
	QVector<SiteView> diffVsMark(int n, bool byLive);  // 现在仍 live、且不在 mark 里的块, 按调用点聚合

	// === 平台工具(HeapWalk) ===
	struct HeapSummary { quint64 totalBytes = 0; quint64 blockCount = 0; };
	// HeapWalk 枚举进程所有堆的活动块, 粗粒度, 扰动远小于 hook
	HeapSummary heapSnapshot();

	// 用 HeapWalk 校验 g_addr, 把不在活动块里的条目当作已 free 抵消。
	// 用于 Debug CRT 下 delete 走 _free_dbg 绕过 free hook 的场景。
	// 返回被抵消的条目数, 供诊断。
	int reapDeadAllocs();
}

