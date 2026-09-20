#pragma once
#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTimer>
#include <QLineEdit>
#include <QStringListModel>
#include <QSignalSpy>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPointer>
#include <QIcon>
#include <QPixmap>
#include "theme/QtSpyTheme.h"
#include <map>
class QListView;
class QCheckBox;
class QHBoxLayout;
class CXDialog : public QDialog {
public:
	CXDialog(QWidget* parent) : QDialog(parent) {
		// 统一补上最小化/最大化按钮(默认 QDialog 标题栏只有关闭);
		// 置顶由 showOnTop 的 WindowStaysOnTopHint 负责, 不用 Qt::Tool(其窄标题栏小按钮难看)
		setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
		// 窗口图标: 只保留 32x32 一份(系统槽位会自动缩放, 标题栏槽位小, 清晰度足够);
		// 只挂 QtSpy 自有窗口(严禁 qApp->setWindowIcon, 会泄漏到目标程序)
		setWindowIcon(QIcon(QStringLiteral(":/icons/resource/spy_icon.png")));
		// 主题挂载: 样式/字体只挂在 QtSpy 自有窗口子树上, 严禁 qApp 级设置(会泄漏到目标程序)
		QtSpyTheme::apply(this);
	}
	void showOnTop(bool bModal = false) {
		if (bModal)
		{
			exec();
		}
		else
		{
			setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
			raise();
			show();
		}
	}
	// 目标程序改全局样式/字体时自愈(见 qt_spydlg.cpp 实现)
	bool event(QEvent* event) override;
};


class QMenuBar;
// 菜单栏动作的浮窗提示: QMenuBar 默认不显示 QAction 的 toolTip(QMenu 才有 toolTipsVisible 开关),
// 用事件过滤器在 ToolTip 事件里手动弹 QToolTip
class CMenuBarTooltipFilter : public QObject {
public:
	explicit CMenuBarTooltipFilter(QMenuBar* pMenuBar);
protected:
	bool eventFilter(QObject* watched, QEvent* event) override;
};

class CSpyIndicatorWnd : public CXDialog {public:
	CSpyIndicatorWnd(QWidget* parent = nullptr);
	static CSpyIndicatorWnd& instance();
	static void showWnd(QRect rcArea, bool bHold = true);
	void show(bool bHold) ;
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	int m_nSpanPeriod;
	QTimer m_Timer;
};


class CMoveOrScaleWidgetWnd : public CXDialog {
public:
	CMoveOrScaleWidgetWnd(QWidget* parent = nullptr);
	void ControlTarget(QWidget* pTargetWidget) {
		m_pTargetWidget = pTargetWidget;
	}
	void GraphicsTarget(QGraphicsItem* pTargetItem) {
		m_pTargetItem = pTargetItem;
	}
private:
	void scaleGeometry(const QRect& rc);
	QGraphicsItem* m_pTargetItem{ nullptr };
	QWidget* m_pTargetWidget = nullptr;
	QLineEdit* m_pEditMoveStep = nullptr;
	QLineEdit* m_pEditScaleStep = nullptr;
	int m_nMoveStep = 1;
	int m_nScaleStep = 1;
};

class CListInfoWnd : public CXDialog {
public:
	CListInfoWnd(QWidget* parent = nullptr);
public:
	bool AddAttribute(QString strName, QString value);

public:
	bool AddInfo(QString strText, int row, int col);

	void ClearAll();
protected:

	void initWidgets();

	void InitTableWidget();
private:
	QTableWidget* m_pTableWidget;
};

class CLogTraceWnd;
struct ConnectionInfo;
class CSignalSpyWnd :public CXDialog {
	class CSignalSpy :public QSignalSpy {
	public:
		CSignalSpy(const QObject* obj, const QMetaMethod& signal);
		virtual int qt_metacall(QMetaObject::Call call, int methodId, void** a) override;
		void setTraceWnd(CLogTraceWnd* wnd);
	private:
		CLogTraceWnd* m_TraceWnd;
	};
public:
	CSignalSpyWnd(QWidget* parent = nullptr);
	~CSignalSpyWnd();

	void setTargetObject(QObject* target);
private:
	void ParseSignal(QObject* object);

	void initWidgets();

	void initTableWidget();

	void initContextMenu();

	void addMethodRow(QTableWidget* table, QMetaMethod* method);
	void addConnectionRow(ConnectionInfo* pInfo);

	void setContent();
	void clearContent();

	CLogTraceWnd* traceWnd();
private:
	QTreeWidget* m_pSignalTree = nullptr;
	QTableWidget* m_pConnectionTable{ nullptr };
	QTableWidget* m_pSignalTable{ nullptr };
	QTableWidget* m_pSlotTable{ nullptr };
	QObject* m_pTargetObject{ nullptr };
	std::map<QMetaMethod*, CSignalSpy*> m_arrSignal;
	CLogTraceWnd* m_pTraceWnd{ nullptr };
};

class CStatusInfoWnd : public CListInfoWnd {
public:
	CStatusInfoWnd(QWidget* parent = nullptr);
private:
	void UpdateStatusInfo();
protected:
	void keyReleaseEvent(QKeyEvent* event) override;
};

class CCursorLocateWnd : public CXDialog {
public:
	CCursorLocateWnd(QWidget* parent = nullptr);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
};

class CLogTraceWnd :public CXDialog {
public:
	CLogTraceWnd(QWidget* parent = nullptr, bool bShowBreakCheck = true);
	bool AddInfo(QString strInfo);
private:
	void initWidgets();
	void appendPendingLog(QString strInfo, int nGeneration);
	void flushPendingLogs();
protected:
	QCheckBox* createBreakCheck();
public:
	QStringListModel m_listModel;
	QListView*		 m_listView;
	QHBoxLayout* m_pControlLayout{ nullptr };
	QTimer m_timerFlushLog;
	QStringList m_listPendingLog;
	int m_nLogGeneration{ 0 };
	QStringList m_arrStrHas;
	QStringList m_arrStrNo;
	int		m_nCount{ 0 };
	bool	m_bTrace{ true };
	bool	m_bOnlyLog{ false };
	bool	m_bBreakOnTrace{ false };
};

class CEventTraceWnd;
class CGraphicsItemSpy :public QGraphicsItem
{
public:
	CGraphicsItemSpy(QGraphicsScene* pScene, CEventTraceWnd* pWnd):m_pWnd(pWnd)
	{ 
		pScene->addItem(this);
	}
	~CGraphicsItemSpy() { 
		if(scene())
		{
			scene()->removeItem(this);

			for (auto pItem : m_arrMonitorItems)
			{
				pItem->removeSceneEventFilter(this);
			}
		}
	}
public:
	void addTargetItem(QGraphicsItem* pItem)
	{
		pItem->installSceneEventFilter(this);
		m_arrMonitorItems.insert(pItem);
	}
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /* = nullptr */) override { Q_UNUSED(painter); Q_UNUSED(option); Q_UNUSED(widget) }
	QRectF boundingRect() const override { return QRectF(); }
	bool sceneEventFilter(QGraphicsItem* watched, QEvent* event) override;
private:
	QSet<QGraphicsItem*> m_arrMonitorItems;
	CEventTraceWnd* m_pWnd;
};
class CEventTraceWnd : public CLogTraceWnd{
public:
	CEventTraceWnd(QWidget* parent = nullptr);
	~CEventTraceWnd();

public:
	bool MonitorWidget(QObject* pWidget);
	void setRunning(bool bRun);
	// 设定监控目标集: 自身 / 子树全部。窗内"包含子组件"开关在两者间切换,
	// 取代原独立菜单项"事件跟踪All"
	void setTargets(const QList<QObject*>& arrSelf, const QList<QObject*>& arrSubTree);
	template <typename T>
	bool AddInfo(T* pTarget, QEvent* event);

protected:
	void initWidget();
	bool eventFilter(QObject* pObject, QEvent* event) override;
	template <typename T>
	QString EventInfo(T* pTarget, QEvent* event);

private:
	void applyTargets(const QList<QObject*>& arrTargets);
	void resetMonitors();

protected:

	QHash<QGraphicsScene*, CGraphicsItemSpy*> m_hashGraphicsSpy;
	QSet<QObject*> m_arrMonitorObject;
	QList<QObject*> m_arrTargetSelf;
	QList<QObject*> m_arrTargetAll;
	bool m_bFilterEvent = false;
	bool m_bRunning = true;
};

class CSpyMainWindow;
class CFindWnd : public CXDialog{
public:
	CFindWnd(CSpyMainWindow* parent = nullptr);
	CFindWnd(QTreeWidget* pTree, QWidget* parent);
	~CFindWnd();
protected:
	void initWidget();
protected:
	QPointer<QTreeWidget> m_pTargetTree;
	QList<QTreeWidgetItem*> m_arrTargetItem;
	int m_nCurrentIndex{ 0 };
	QLineEdit* m_pEdit{ nullptr };
	// 当前查找结果对应的关键字; 与输入框不一致时, 上一个/下一个视为重新查找
	QString m_strKeyword;
};
