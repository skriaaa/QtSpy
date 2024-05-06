#pragma once
#include <QRect>
#include <QWidget>
#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMainWindow>
#include <QTimer>
#include <QTextEdit>
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
class CSpyMainWindow : public QDialog {
public:
	CSpyMainWindow();

	void initWindow();
	void setMenuBar(QMenuBar* menuBar);
	QTreeWidget* tree();
	virtual void keyPressEvent(QKeyEvent* event);
protected:
	void initSpyTree();
	void clearSpyTree();
private:
	QTreeWidget* m_pTree{ nullptr };
};

class CQtSpyObject : public QObject {
public:
	static CQtSpyObject& GetInstance();
public:
	CQtSpyObject();
	bool StartSpy(QWidget*);
	bool ShutdownSpy();
public:
	virtual bool eventFilter(QObject* watched, QEvent* event) override;
public:
	bool setTreeTarget(QPoint pt);
	bool setTreeTarget(QGraphicsItem* item);
	bool setTreeTarget(QWidget* widget);
public:
	bool initToolWindow();
	bool ClearSpyTree();
	bool AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode);
	bool AddSubSpyNode(QGraphicsItem* parent, QTreeWidgetItem* parentNode);
	bool ShowSystemInfo();
	bool ShowStatusInfo();
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
	std::map<QWidget*, QTreeWidgetItem*> m_mapWidgetNode;
	EScreenMouseAction m_eCursorAction = EScreenMouseAction::None;
};
