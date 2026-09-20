#include "MemoryMonitorDlg.h"
#include "theme/QtSpyTheme.h"
#include "utils/ProcessInfo.h"
#include "utils/AllocProfiler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListView>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QCloseEvent>
#include <QDateTime>
#include <QAbstractListModel>
#include <QVector>
#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QStyledItemDelegate>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QStandardPaths>

// ---- 环形缓冲采样模型(无 Q_OBJECT, 仅重写虚函数, 避免在 cpp 内引 MOC) ----
class SampleModel : public QAbstractListModel
{
public:
	explicit SampleModel(int capacity = 4000, QObject* parent = nullptr)
		: QAbstractListModel(parent), m_capacity(capacity) {}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{ return parent.isValid() ? 0 : m_data.size(); }

	int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{ return parent.isValid() ? 0 : 1; }

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (!index.isValid() || role != Qt::DisplayRole)
			return {};
		if (index.row() < 0 || index.row() >= m_data.size())
			return {};
		const MemorySample& s = m_data.at(index.row());
		return formatRow(s);
	}

	void push(const MemorySample& s)
	{
		if (m_data.size() >= m_capacity)
		{
			beginRemoveRows(QModelIndex(), 0, 0);
			m_data.removeFirst();
			endRemoveRows();
		}
		int r = m_data.size();
		beginInsertRows(QModelIndex(), r, r);
		m_data.append(s);
		endInsertRows();
	}

	void clear()
	{
		beginResetModel();
		m_data.clear();
		endResetModel();
	}

	const MemorySample& at(int row) const { return m_data.at(row); }
	int size() const { return m_data.size(); }

private:
	static QString formatRow(const MemorySample& s)
	{
		// 时间戳 ms -> hh:mm:ss.zzz
		QDateTime dt = QDateTime::fromMSecsSinceEpoch(s.msecs);
		return QString("%1 | Private %2 | WS %3 | Handle %4")
			.arg(dt.toString("hh:mm:ss.zzz"))
			.arg(ProcessInfo::formatBytes(s.privateBytes))
			.arg(ProcessInfo::formatBytes(s.workingSet))
			.arg(s.handleCount);
	}
	QVector<MemorySample> m_data;
	int m_capacity;
};

// ---- 折线图: 画 Private 字节序列 ----
class MemoryChart : public QWidget
{
public:
	explicit MemoryChart(QWidget* parent = nullptr) : QWidget(parent)
	{
		setMinimumHeight(120);
	}
	void setModel(SampleModel* m) { m_model = m; update(); }
	void setBaseline(quint64 b) { m_baseline = b; update(); }
protected:
	void paintEvent(QPaintEvent*) override
	{
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		QRect r = rect().adjusted(0, 0, -1, -1);
		p.fillRect(r, QtSpyTheme::palette().contentBg);
		if (!m_model || m_model->size() < 1)
		{
			p.setPen(QtSpyTheme::palette().textSecondary);
			p.drawText(r, Qt::AlignCenter, "无数据");
			return;
		}
		// 纵轴范围: [0, max], 至少 1MB
		quint64 vmax = 0;
		for (int i = 0; i < m_model->size(); ++i)
			vmax = qMax(vmax, m_model->at(i).privateBytes);
		if (vmax < (1ULL << 20)) vmax = (1ULL << 20);

		// 基线参考线
		if (m_baseline > 0 && m_baseline <= vmax)
		{
			int y = r.top() + r.height() - int(quint64(r.height()) * m_baseline / vmax);
			p.setPen(QPen(QColor(QtSpyTheme::palette().border), 1, Qt::DashLine));
			p.drawLine(r.left(), y, r.right(), y);
		}

		// 折线
		QPainterPath path;
		int n = m_model->size();
		for (int i = 0; i < n; ++i)
		{
			quint64 v = m_model->at(i).privateBytes;
			int x = (n == 1) ? r.left() : r.left() + r.width() * i / (n - 1);
			int y = r.top() + r.height() - int(quint64(r.height()) * v / vmax);
			if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
		}
		p.setPen(QPen(QColor(QtSpyTheme::palette().accent), 2));
		p.drawPath(path);

		// 坐标标注
		p.setPen(QtSpyTheme::palette().textSecondary);
		p.drawText(r.adjusted(4, 2, 0, 0), Qt::AlignTop | Qt::AlignLeft, ProcessInfo::formatBytes(vmax));
		p.drawText(r.adjusted(4, 0, 0, -2), Qt::AlignBottom | Qt::AlignLeft, "0");
	}
private:
	SampleModel* m_model = nullptr;
	quint64 m_baseline = 0;
};

// ---- 工具: 字节格式化, >=10MB 时转 MB(2 位小数), 否则调用 formatBytes ----
static QString formatBytesForTable(quint64 bytes)
{
	if (bytes >= (10ULL << 20))  // 10 MB
		return QString::number(double(bytes) / double(1ULL << 20), 'f', 2) + " MB";
	return ProcessInfo::formatBytes(bytes);
}

// 文本从左侧省略, 保右侧文件名+行号可见(用于"文件:行"列)
class ElideLeftDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	void paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const override
	{
		QStyleOptionViewItem o(opt);
		initStyleOption(&o, idx);
		QString text = idx.data(Qt::DisplayRole).toString();
		QRect textRect = o.rect.adjusted(4, 0, -4, 0);
		QString elided = o.fontMetrics.elidedText(text, Qt::ElideLeft, textRect.width());
		// 用默认样式画背景/选中态, 但把 text 清掉自己画
		o.text.clear();
		o.widget->style()->drawControl(QStyle::CE_ItemViewItem, &o, p, o.widget);
		p->save();
		p->setPen(o.palette.color(o.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
		p->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
		p->restore();
	}
};

// ---- 表格 item: 数字列走数值排序, 靠 QTableWidgetItem::setData(Qt::UserRole, qlonglong) ----
class NumericItem : public QTableWidgetItem
{
public:
	NumericItem(const QString& text, quint64 value)
		: QTableWidgetItem(text)
	{
		setData(Qt::UserRole, QVariant::fromValue<qulonglong>(value));
	}
	bool operator<(const QTableWidgetItem& other) const override
	{
		return data(Qt::UserRole).toULongLong() < other.data(Qt::UserRole).toULongLong();
	}
};

// ---- 对话框 ----
CMemoryMonitorDlg::CMemoryMonitorDlg(QWidget* parent /*= nullptr*/)
	: CXDialog(parent)
{
	setWindowTitle("QtSpy · 内存监控");
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
	resize(816, 520);
	initWidgets();
}

CMemoryMonitorDlg::~CMemoryMonitorDlg()
{
	if (m_nUsageTimer != -1) killTimer(m_nUsageTimer);
	if (m_nAllocTimer != -1) killTimer(m_nAllocTimer);
}

void CMemoryMonitorDlg::initWidgets()
{
	auto* mainLayout = new QVBoxLayout(this);
	m_tabs = new QTabWidget(this);
	mainLayout->addWidget(m_tabs);

	// ============ 内存占用页 ============
	auto* pageUsage = new QWidget;
	auto* lUsage = new QVBoxLayout(pageUsage);

	// metrics 标签条: 5 个指标横排
	auto addMetric = [&](const QString& title, QLabel*& val) {
		auto* box = new QVBoxLayout;
		auto* t = new QLabel(title);
		t->setAlignment(Qt::AlignCenter);
		t->setStyleSheet("color:#888;");
		val = new QLabel("--");
		val->setAlignment(Qt::AlignCenter);
		val->setStyleSheet("QLabel{background:#2b2b2b;color:#7fd1b9;padding:6px;border-radius:3px;font-weight:bold;}");
		box->addWidget(t);
		box->addWidget(val);
		return box;
	};
	auto* metricRow = new QHBoxLayout;
	metricRow->addLayout(addMetric("Private", m_lblPrivate));
	metricRow->addLayout(addMetric("Peak", m_lblPeak));
	metricRow->addLayout(addMetric("Handle", m_lblHandle));
	metricRow->addLayout(addMetric("GDI", m_lblGdi));
	metricRow->addLayout(addMetric("Threads", m_lblThreads));
	lUsage->addLayout(metricRow);

	// 折线图 + 列表
	m_chart = new MemoryChart(pageUsage);
	m_model = new SampleModel(4000, pageUsage);
	m_chart->setModel(m_model);
	lUsage->addWidget(m_chart, 1);
	m_listView = new QListView(pageUsage);
	m_listView->setModel(m_model);
	m_listView->setUniformItemSizes(true);
	lUsage->addWidget(m_listView, 1);
	connect(m_listView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) {
		auto* sb = m_listView->verticalScrollBar();
		m_bAutoScroll = (v >= sb->maximum() - 2);
	});

	// 内存占用页控制条: 间隔 / 开始监控 / 重置
	auto* usageCtrl = new QHBoxLayout;
	usageCtrl->addWidget(new QLabel("间隔(ms):"));
	m_spinInterval = new QSpinBox;
	m_spinInterval->setRange(100, 60000);
	m_spinInterval->setSingleStep(100);
	m_spinInterval->setValue(m_nIntervalMs);
	connect(m_spinInterval, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int v) {
		m_nIntervalMs = v;
		if (m_nUsageTimer != -1)
		{
			killTimer(m_nUsageTimer);
			m_nUsageTimer = startTimer(m_nIntervalMs);
		}
	});
	usageCtrl->addWidget(m_spinInterval);
	m_btnUsageStart = new QPushButton("开始监控");
	connect(m_btnUsageStart, &QPushButton::clicked, this, [this]() {
		if (m_nUsageTimer == -1) startUsage(); else stopUsage();
	});
	usageCtrl->addWidget(m_btnUsageStart);
	m_btnUsageReset = new QPushButton("重置");
	m_btnUsageReset->setProperty("spyClass", "danger");  // QSS 属性选择器: 红字危险按钮
	connect(m_btnUsageReset, &QPushButton::clicked, this, [this]() {
		quint64 cur = ProcessInfo::queryProcessCurrentMemory();
		m_baselinePrivate = cur;
		m_peakPrivate = cur;
		m_model->clear();
		m_chart->setBaseline(m_baselinePrivate);
		m_chart->update();
	});
	usageCtrl->addWidget(m_btnUsageReset);
	usageCtrl->addStretch(1);
	lUsage->addLayout(usageCtrl);

	m_tabs->addTab(pageUsage, "内存占用");

	// ============ 内存分配页 ============
	auto* pageAlloc = new QWidget;
	auto* lAlloc = new QVBoxLayout(pageAlloc);

	// 列: 0符号 Interactive 200 | 1文件:行 Stretch (吃掉剩余空间, 左侧省略) | 2模块 120 | 3未释放 120 | 4已分配 120 | 5命中/剩余(次) 100
	m_callSiteTable = new QTableWidget(0, 6, pageAlloc);
	m_callSiteTable->setHorizontalHeaderLabels({ "符号", "文件:行", "模块", "未释放", "已分配", "命中/剩余(次)" });
	auto* hh = m_callSiteTable->horizontalHeader();
	hh->setStretchLastSection(false);
	hh->setMinimumSectionSize(120);  // 全局最小列宽 120, 也保证 Stretch 的文件:行列不会挤到 120 以下
	hh->setSectionResizeMode(0, QHeaderView::Fixed);
	hh->setSectionResizeMode(1, QHeaderView::Stretch);   // 文件:行 吃剩余空间
	hh->setSectionResizeMode(2, QHeaderView::Fixed);
	hh->setSectionResizeMode(3, QHeaderView::Fixed);
	hh->setSectionResizeMode(4, QHeaderView::Fixed);
	hh->setSectionResizeMode(5, QHeaderView::Fixed);
	m_callSiteTable->setColumnWidth(0, 140);
	m_callSiteTable->setColumnWidth(2, 120);
	m_callSiteTable->setColumnWidth(3, 120);
	m_callSiteTable->setColumnWidth(4, 120);
	m_callSiteTable->setColumnWidth(5, 120);  // 命中/剩余(次), 从 100 提到 120(全局最小限制)
	// 文件:行 列用左侧省略, 保尾部文件名+行号可见
	m_callSiteTable->setItemDelegateForColumn(1, new ElideLeftDelegate(m_callSiteTable));
	m_callSiteTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_callSiteTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_callSiteTable->setSortingEnabled(false);  // 刷新时手动按现存字节排序, 关掉 header 排序避免闪
	connect(m_callSiteTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
		bool merge = m_chkMergeBySymbol && m_chkMergeBySymbol->isChecked();
		if (merge)
		{
			// 合并模式下: 双击符号列打开明细; 其他列不响应
			if (col == 0 && row >= 0 && row < m_lastRowSymbols.size())
				showSymbolDetailDialog(m_lastRowSymbols[row]);
		}
		else
		{
			if (row >= 0 && row < m_lastSiteHashes.size())
				showBacktraceDialog(m_lastSiteHashes[row]);
		}
	});
	lAlloc->addWidget(m_callSiteTable);

	m_lblMode = new QLabel("符号: --", pageAlloc);
	m_lblMode->setStyleSheet("color:#888;");
	lAlloc->addWidget(m_lblMode);

	// 内存分配页控制条: minSize / 合并 / 开始监控
	auto* allocCtrl = new QHBoxLayout;
	allocCtrl->addWidget(new QLabel("minSize(B):"));
	m_spinMinSize = new QSpinBox;
	m_spinMinSize->setRange(0, 10485760);
	m_spinMinSize->setSingleStep(64);
	m_spinMinSize->setValue(0);
	AllocProfiler::setMinSize(0);
	connect(m_spinMinSize, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int v) {
		AllocProfiler::setMinSize(static_cast<quint64>(v));
	});
	allocCtrl->addWidget(m_spinMinSize);
	// 按符号合并开关: 打开后同符号的多个堆栈合并成一行, 前面加 ** 标识
	m_chkMergeBySymbol = new QCheckBox("按符号合并");
	connect(m_chkMergeBySymbol, &QCheckBox::toggled, this, [this](bool) { refreshCallSites(); });
	allocCtrl->addWidget(m_chkMergeBySymbol);
	m_btnAllocStart = new QPushButton("开始监控");
	connect(m_btnAllocStart, &QPushButton::clicked, this, [this]() {
		if (!m_bAllocOn) startAlloc(); else stopAlloc();
	});
	allocCtrl->addWidget(m_btnAllocStart);
	auto* btnAllocClear = new QPushButton("清空");
	connect(btnAllocClear, &QPushButton::clicked, this, [this]() {
		AllocProfiler::resetStats();
		m_callSiteTable->setRowCount(0);
		m_lastSiteHashes.clear();
	});
	allocCtrl->addWidget(btnAllocClear);
	allocCtrl->addStretch(1);
	lAlloc->addLayout(allocCtrl);

	m_tabs->addTab(pageAlloc, "内存分配");
}

// ==== 内存占用页 ====
void CMemoryMonitorDlg::startUsage()
{
	m_nUsageTimer = startTimer(m_nIntervalMs);
	m_btnUsageStart->setText("停止监控");
}

void CMemoryMonitorDlg::stopUsage()
{
	if (m_nUsageTimer != -1)
	{
		killTimer(m_nUsageTimer);
		m_nUsageTimer = -1;
	}
	m_btnUsageStart->setText("开始监控");
}

void CMemoryMonitorDlg::onUsageSample()
{
	ProcessInfo::ProcessMemoryInfo info = ProcessInfo::queryProcessMemoryInfo();
	MemorySample s;
	s.msecs = QDateTime::currentMSecsSinceEpoch();
	s.privateBytes = info.privateBytes;
	s.workingSet = info.workingSet;
	s.handleCount = info.handleCount;
	m_model->push(s);
	m_peakPrivate = qMax(m_peakPrivate, info.privateBytes);
	m_chart->setBaseline(m_baselinePrivate);
	m_chart->update();
	updateMetricsLabels(s);
	if (m_bAutoScroll)
		m_listView->scrollToBottom();
}

void CMemoryMonitorDlg::updateMetricsLabels(const MemorySample& s)
{
	auto info = ProcessInfo::queryProcessMemoryInfo();
	m_lblPrivate->setText(ProcessInfo::formatBytes(s.privateBytes));
	m_lblPeak->setText(ProcessInfo::formatBytes(m_peakPrivate));
	m_lblHandle->setText(QString::number(s.handleCount));
	m_lblGdi->setText(QString::number(info.gdiObjects));
	m_lblThreads->setText(QString::number(info.threadCount));
}

// ==== 内存分配页 ====
void CMemoryMonitorDlg::startAlloc()
{
	AllocProfiler::attach();
	m_bAllocOn = true;
	m_nAllocTimer = startTimer(1000);  // 内存分配页固定 1s 刷新
	m_btnAllocStart->setText("停止监控");
}

void CMemoryMonitorDlg::stopAlloc()
{
	if (m_nAllocTimer != -1)
	{
		killTimer(m_nAllocTimer);
		m_nAllocTimer = -1;
	}
	AllocProfiler::detach();
	m_bAllocOn = false;
	m_btnAllocStart->setText("开始监控");
}

void CMemoryMonitorDlg::refreshCallSites()
{
	bool ready = AllocProfiler::symbolsReady();
	m_lblMode->setText(QString("符号: %1").arg(ready ? "就绪" : "加载中..."));

	QVector<AllocProfiler::SiteView> sites = AllocProfiler::topSites(500, true);
	m_lastSiteHashes.clear();
	m_lastRowSymbols.clear();

	bool merge = m_chkMergeBySymbol && m_chkMergeBySymbol->isChecked();
	if (!merge)
	{
		// 每个 SiteView 一行
		m_lastSiteHashes.reserve(sites.size());
		m_callSiteTable->setRowCount(sites.size());
		for (int i = 0; i < sites.size(); ++i)
		{
			const auto& v = sites[i];
			m_lastSiteHashes.append(v.stackHash);
			auto* itSym = new QTableWidgetItem(v.symbol);
			itSym->setToolTip(v.symbol);
			m_callSiteTable->setItem(i, 0, itSym);
			auto* itFl = new QTableWidgetItem(v.fileLine);
			itFl->setToolTip(v.fileLine);
			m_callSiteTable->setItem(i, 1, itFl);
			m_callSiteTable->setItem(i, 2, new QTableWidgetItem(v.module));
			m_callSiteTable->setItem(i, 3, new NumericItem(formatBytesForTable(v.liveBytes),  v.liveBytes));
			m_callSiteTable->setItem(i, 4, new NumericItem(formatBytesForTable(v.totalBytes), v.totalBytes));
			QString hitLive = QString("%1 / %2").arg(v.totalCount).arg(v.liveCount);
			m_callSiteTable->setItem(i, 5, new NumericItem(hitLive, v.totalCount));
		}
		return;
	}

	// 合并模式: 按 symbol 聚合. 保留每 symbol 内堆栈数, 用于显示 ** 标记
	struct Agg
	{
		QString symbol;
		QString fileLine;
		QString module;
		quint64 liveBytes = 0;
		quint64 liveCount = 0;
		quint64 totalBytes = 0;
		quint64 totalCount = 0;
		int     stackCount = 0;   // 汇总了多少不同 stackHash
	};
	QHash<QString, Agg> agg;
	QStringList order;  // 保持首次出现顺序(topSites 已按现存字节降序)
	for (const auto& v : sites)
	{
		auto it = agg.find(v.symbol);
		if (it == agg.end())
		{
			Agg a;
			a.symbol = v.symbol;
			a.fileLine = v.fileLine;
			a.module = v.module;
			a.liveBytes = v.liveBytes;
			a.liveCount = v.liveCount;
			a.totalBytes = v.totalBytes;
			a.totalCount = v.totalCount;
			a.stackCount = 1;
			agg.insert(v.symbol, a);
			order.append(v.symbol);
		}
		else
		{
			it.value().liveBytes  += v.liveBytes;
			it.value().liveCount  += v.liveCount;
			it.value().totalBytes += v.totalBytes;
			it.value().totalCount += v.totalCount;
			it.value().stackCount += 1;
		}
	}
	// 按 liveBytes 降序输出
	QList<Agg> rows;
	rows.reserve(order.size());
	for (const auto& k : order)
		rows.append(agg[k]);
	std::sort(rows.begin(), rows.end(), [](const Agg& a, const Agg& b) {
		return a.liveBytes > b.liveBytes;
	});
	m_lastRowSymbols.reserve(rows.size());
	m_callSiteTable->setRowCount(rows.size());
	for (int i = 0; i < rows.size(); ++i)
	{
		const auto& a = rows[i];
		m_lastRowSymbols.append(a.symbol);
		QString symText = (a.stackCount > 1) ? (QStringLiteral("*") + a.symbol) : a.symbol;
		auto* itSym = new QTableWidgetItem(symText);
		itSym->setToolTip(symText);
		m_callSiteTable->setItem(i, 0, itSym);
		auto* itFl = new QTableWidgetItem(a.fileLine);
		itFl->setToolTip(a.fileLine);
		m_callSiteTable->setItem(i, 1, itFl);
		m_callSiteTable->setItem(i, 2, new QTableWidgetItem(a.module));
		m_callSiteTable->setItem(i, 3, new NumericItem(formatBytesForTable(a.liveBytes),  a.liveBytes));
		m_callSiteTable->setItem(i, 4, new NumericItem(formatBytesForTable(a.totalBytes), a.totalBytes));
		QString hitLive = QString("%1 / %2").arg(a.totalCount).arg(a.liveCount);
		m_callSiteTable->setItem(i, 5, new NumericItem(hitLive, a.totalCount));
	}
}

// 符号明细弹窗: 列出该符号下所有独立 stackHash 的分配统计
// 列布局: 文件:行 / 未释放 / 已分配 / 命中/剩余(次) (删掉符号 & 模块列)
void CMemoryMonitorDlg::showSymbolDetailDialog(const QString& symbol)
{
	// 从当前所有 site 里筛出符号匹配的
	QVector<AllocProfiler::SiteView> all = AllocProfiler::topSites(500, true);
	QVector<AllocProfiler::SiteView> match;
	match.reserve(all.size());
	for (const auto& v : all)
		if (v.symbol == symbol)
			match.append(v);

	auto* dlg = new CXDialog(this);  // 用 CXDialog 构造以挂载主题
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->setWindowTitle(QString("QtSpy · 符号明细 - %1").arg(symbol));
	dlg->resize(700, 400);
	auto* lay = new QVBoxLayout(dlg);

	auto* table = new QTableWidget(match.size(), 4, dlg);
	table->setHorizontalHeaderLabels({ "文件:行", "未释放", "已分配", "命中/剩余(次)" });
	auto* hh = table->horizontalHeader();
	hh->setStretchLastSection(false);
	hh->setMinimumSectionSize(100);
	hh->setSectionResizeMode(0, QHeaderView::Stretch);
	hh->setSectionResizeMode(1, QHeaderView::Fixed);
	hh->setSectionResizeMode(2, QHeaderView::Fixed);
	hh->setSectionResizeMode(3, QHeaderView::Fixed);
	table->setColumnWidth(1, 120);
	table->setColumnWidth(2, 120);
	table->setColumnWidth(3, 120);
	table->setItemDelegateForColumn(0, new ElideLeftDelegate(table));
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);

	// 用一个 vector 缓存每行对应的 stackHash, 供双击展开回溯
	auto* rowHashes = new QVector<quint64>();
	rowHashes->reserve(match.size());
	for (int i = 0; i < match.size(); ++i)
	{
		const auto& v = match[i];
		rowHashes->append(v.stackHash);
		auto* itFl = new QTableWidgetItem(v.fileLine);
		itFl->setToolTip(v.fileLine);
		table->setItem(i, 0, itFl);
		table->setItem(i, 1, new NumericItem(formatBytesForTable(v.liveBytes),  v.liveBytes));
		table->setItem(i, 2, new NumericItem(formatBytesForTable(v.totalBytes), v.totalBytes));
		QString hitLive = QString("%1 / %2").arg(v.totalCount).arg(v.liveCount);
		table->setItem(i, 3, new NumericItem(hitLive, v.totalCount));
	}
	// 双击某行 -> 打开该 stackHash 的完整调用回溯
	connect(table, &QTableWidget::cellDoubleClicked, dlg, [this, rowHashes](int row, int) {
		if (row >= 0 && row < rowHashes->size())
			showBacktraceDialog(rowHashes->at(row));
	});
	connect(dlg, &QDialog::destroyed, [rowHashes]() { delete rowHashes; });
	lay->addWidget(table);
	dlg->show();
}

void CMemoryMonitorDlg::showBacktraceDialog(quint64 stackHash)
{
	QVector<AllocProfiler::FrameInfo> frames = AllocProfiler::backtraceForSite(stackHash);
	auto* dlg = new CXDialog(this);  // 用 CXDialog 构造以挂载主题
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->setWindowTitle(QString("QtSpy · 调用回溯 (0x%1)").arg(stackHash, 0, 16));
	dlg->resize(560, 360);
	auto* lay = new QVBoxLayout(dlg);
	auto* list = new QListWidget(dlg);
	list->setWordWrap(true);
	// Qt 源目录用正则匹配, 避免硬编码版本号
	static const QRegularExpression qtVerRe(
		QStringLiteral("[\\\\/](qt\\d+\\.\\d+(?:\\.\\d+)?|qtbase|qtdeclarative|qt[\\\\/]src)[\\\\/]"),
		QRegularExpression::CaseInsensitiveOption);
	for (const auto& f : frames)
	{
		// CRT / STL / MSVC / Qt 源码不展示
		if (f.fileLine.contains(QStringLiteral("\\vctools\\"), Qt::CaseInsensitive)
		 || f.fileLine.contains(QStringLiteral("\\vc\\tools\\msvc\\"), Qt::CaseInsensitive)
		 || f.fileLine.contains(QStringLiteral("\\microsoft visual studio\\"), Qt::CaseInsensitive)
		 || qtVerRe.match(f.fileLine).hasMatch())
			continue;
		QString line = QString("%1 [%2] %3 | %4 | %5")
			.arg(f.business ? QStringLiteral("▶") : QStringLiteral(" "))
			.arg(f.module)
			.arg(QString("0x%1").arg(f.address, 0, 16))
			.arg(f.symbol)
			.arg(f.fileLine.isEmpty() ? QString("(无源行)") : f.fileLine);
		auto* item = new QListWidgetItem(line, list);
		item->setData(Qt::UserRole, f.fileLine);  // 原始 "file:line" 存 UserData, 双击时用
	}
	// 双击: 解析 UserData 里的 "路径:行号", 优先 VS Code 跳转, 回退系统默认打开
	connect(list, &QListWidget::itemDoubleClicked, dlg, [](QListWidgetItem* it) {
		QString fl = it->data(Qt::UserRole).toString();
		if (fl.isEmpty()) return;
		// 从右往左找最后一个 ':' 作为 line 分隔; 避开盘符冒号(如 "C:\...")
		int col = fl.lastIndexOf(QLatin1Char(':'));
		if (col <= 2) return;  // "C:\..." 里的第一个冒号在 idx=1, 跳过
		QString file = fl.left(col);
		QString lineNum = fl.mid(col + 1);
		bool ok = false;
		int lineN = lineNum.toInt(&ok);
		if (!ok || !QFileInfo(file).exists()) return;
		// 优先 VS Code (code -g file:line)
		QString code = QStandardPaths::findExecutable(QStringLiteral("code"));
		if (!code.isEmpty())
		{
			QProcess::startDetached(code, { QStringLiteral("-g"), QString("%1:%2").arg(file).arg(lineN) });
			return;
		}
		// 回退: 用系统默认关联程序打开(可能无法跳行)
		QDesktopServices::openUrl(QUrl::fromLocalFile(file));
	});
	lay->addWidget(list);
	dlg->show();
}

void CMemoryMonitorDlg::timerEvent(QTimerEvent* event)
{
	int id = event->timerId();
	if (id == m_nUsageTimer) onUsageSample();
	else if (id == m_nAllocTimer) refreshCallSites();
	CXDialog::timerEvent(event);
}

void CMemoryMonitorDlg::closeEvent(QCloseEvent* event)
{
	if (m_bAllocOn) AllocProfiler::detach();
	if (m_nUsageTimer != -1) { killTimer(m_nUsageTimer); m_nUsageTimer = -1; }
	if (m_nAllocTimer != -1) { killTimer(m_nAllocTimer); m_nAllocTimer = -1; }
	CXDialog::closeEvent(event);
}
