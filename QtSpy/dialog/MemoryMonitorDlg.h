#pragma once
#include "qt_spydlg.h"
#include <QtCore/QtGlobal>
#include <QVector>

class QListView;
class QLabel;
class QSpinBox;
class QPushButton;
class QTabWidget;
class QTableWidget;
class QTimerEvent;
class QCloseEvent;
class QCheckBox;
class MemoryChart;
class SampleModel;

// 单次采样数据
struct MemorySample
{
	quint64 msecs;         // 时间戳(ms, 自监控起始)
	quint64 privateBytes;
	quint64 workingSet;
	quint64 handleCount;
};

class CMemoryMonitorDlg : public CXDialog
{
public:
	CMemoryMonitorDlg(QWidget* parent = nullptr);
	~CMemoryMonitorDlg();
protected:
	void timerEvent(QTimerEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
private:
	void initWidgets();
	// 内存占用页
	void startUsage();
	void stopUsage();
	void onUsageSample();
	void updateMetricsLabels(const struct MemorySample& s);
	// 内存分配页
	void startAlloc();
	void stopAlloc();
	void refreshCallSites();
	void showBacktraceDialog(quint64 stackHash);
	// 显示该符号包含的所有堆栈明细
	void showSymbolDetailDialog(const QString& symbol);
private:
	QTabWidget*   m_tabs        = nullptr;
	// ==== 内存占用页 ====
	QListView*    m_listView    = nullptr;
	SampleModel*  m_model       = nullptr;
	MemoryChart*  m_chart       = nullptr;
	QLabel*       m_lblPrivate  = nullptr;
	QLabel*       m_lblPeak     = nullptr;
	QLabel*       m_lblHandle   = nullptr;
	QLabel*       m_lblGdi      = nullptr;
	QLabel*       m_lblThreads  = nullptr;
	QSpinBox*     m_spinInterval = nullptr;
	QPushButton*  m_btnUsageStart = nullptr;
	QPushButton*  m_btnUsageReset = nullptr;
	int           m_nUsageTimer  = -1;
	int           m_nIntervalMs  = 1000;
	quint64       m_baselinePrivate = 0;
	quint64       m_peakPrivate     = 0;
	bool          m_bAutoScroll     = true;

	// ==== 内存分配页 ====
	QTableWidget* m_callSiteTable = nullptr;
	QLabel*       m_lblMode      = nullptr;
	QSpinBox*     m_spinMinSize  = nullptr;
	QPushButton*  m_btnAllocStart = nullptr;
	QCheckBox*    m_chkMergeBySymbol = nullptr;
	QVector<quint64> m_lastSiteHashes;  // 未合并模式: 每行 -> stackHash
	QVector<QString> m_lastRowSymbols;  // 合并模式: 每行 -> 符号(用于双击展开明细)
	int           m_nAllocTimer  = -1;
	bool          m_bAllocOn     = false;
};
