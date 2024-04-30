#pragma once
#include <QRect>
#include <QWidget>
#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMainWindow>
#include <QTimer>
#include <QTextEdit>

enum class EScreenMouseAction {
	None,
	SearchWidget,
	SpyTarget,
	CheckColor,
};

class CSpyMainWindow : public QDialog {
public:
	CSpyMainWindow();

	QWidget* centralWidget();
	bool setMenuBar(QMenuBar* menuBar);
	bool setCentralWidget(QWidget*);
	virtual void keyPressEvent(QKeyEvent* event);
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
	bool BuildSpyTree(QWidget*);
private:
	bool initToolWindow();
	bool ClearSpyTree();
	bool AddSubSpyNode(QWidget* parent, QTreeWidgetItem* parentNode);
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
	std::map<QWidget*, QTreeWidgetItem*> m_mapWidgetNode;
	QTreeWidget* m_pSpyTree;
	EScreenMouseAction m_eCursorAction = EScreenMouseAction::None;
};
