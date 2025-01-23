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
#include <map>
#define _QStr(str) (str)
class QListView;
class CXDialog : public QDialog {
public:
	CXDialog(QWidget* parent) : QDialog(parent) {

	}
	void ShowOnTop(bool modal = false) {
		if (modal) {
			exec();
		}
		else {
			setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
			raise();
			show();
		}
	}
};


class CSpyIndicatorWnd : public CXDialog {
public:
	CSpyIndicatorWnd(QWidget* parent = nullptr);
	static CSpyIndicatorWnd& instance();
	static void showWnd(QRect rcArea, bool bHold = true);
	void show(bool bHold) ;
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

	void addItem(QTableWidget* table, int row, int col, const QMetaObject* metaObject, QMetaMethod* method);

	void setContent();
	void clearContent();

	CLogTraceWnd* traceWnd();
private:
	QTreeWidget* m_pSignalTree = nullptr;
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
	CLogTraceWnd(QWidget* parent = nullptr);
	bool AddInfo(QString strInfo);
private:
	void initWidgets();
public:
	QStringListModel m_listModel;
	QListView*		 m_listView;
	QStringList m_arrStrHas;
	QStringList m_arrStrNo;
	int		m_nCount{ 0 };
	bool	m_bTrace{ true };
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
	template <typename T>
	bool AddInfo(T* pTarget, QEvent* event);

protected:
	void initWidget();
	bool eventFilter(QObject* pObject, QEvent* event) override;
	template <typename T>
	QString EventInfo(T* pTarget, QEvent* event);

protected:

	QHash<QGraphicsScene*, CGraphicsItemSpy*> m_hashGraphicsSpy;
	QSet<QObject*> m_arrMonitorObject;
	bool m_bFilterEvent = false;
	bool m_bRunning = true;
};

class CQtSpyObject;
class CFindWnd : public CXDialog{
public:
	CFindWnd(CQtSpyObject* pSpyObject, QWidget* parent = nullptr);
	~CFindWnd();
protected:
	void initWidget();
protected:
	CQtSpyObject* m_pSpyObject{ nullptr };
	QList<QTreeWidgetItem*> m_arrTargetItem;
	int m_nCurrentIndex{ 0 };
};