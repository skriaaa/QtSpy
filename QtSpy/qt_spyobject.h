#pragma once
#include <QRect>
#include <QWidget>
#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMainWindow>
#include <QTimer>
#include <QTextEdit>
#include "dialog/qt_spydlg.h"
class CWidgetSpyTree;
class QGraphicsItem;
enum class EScreenMouseAction {
	None,
	SearchWidget,
	SpyTarget,
	CheckColor,
};
enum class ETreeType {
	widgetTree = 0,
	viewTree = 1,
};
class CQtSpyObject;
class CSpyMainWindow : public CXDialog {
public:
	CSpyMainWindow(QWidget* parent = nullptr);

	CQtSpyObject* spyObject();
	void initWindow(CQtSpyObject* object);
	void setMenuBar(QMenuBar* menuBar);
	CWidgetSpyTree* tree();
	virtual void keyPressEvent(QKeyEvent* event) override;
	virtual bool eventFilter(QObject* object, QEvent* event) override;
protected:
	void initSpyTree();
	void clearSpyTree();
private:
	CWidgetSpyTree* m_pTree{ nullptr };
	CQtSpyObject* m_pSpyObject{ nullptr };
};

class CQtSpyObject : public QObject {
	friend class CFindWnd;
public:
	CQtSpyObject(QWidget* parent = nullptr);
	~CQtSpyObject();
public:
	int currentItemCount();
	virtual bool eventFilter(QObject* watched, QEvent* event) override;
public:
	bool setTreeTarget(QPoint pt);
	bool setTreeTarget(QGraphicsItem* item);
	bool setTreeTarget(QWidget* widget);
public:
	bool initToolWindow();
	bool ShowSystemInfo();
	bool ShowStatusInfo();
	bool showMemoryMonitor();
	bool ShowSystemFont();
	bool SearchSpyTreeByName();
	bool SearchSpyTreeByCursor();
	bool ShowCursorLocate();
	bool SpyTargetTree();
	bool CheckColor();
private:
	CSpyMainWindow* m_pMainWindow;
	QWidget* m_pSpyWidget;
	QGraphicsItem* m_pSpyViewItem{nullptr};
	QHash<void*, QTreeWidgetItem*> m_mapWidgetNode;
	EScreenMouseAction m_eCursorAction = EScreenMouseAction::None;
};
