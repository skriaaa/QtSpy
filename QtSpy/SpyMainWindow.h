#pragma once
#include "dialog/qt_spydlg.h"
class CWidgetSpyTree;
class QGraphicsItem;
class QLabel;
class QTimer;
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
class CSpyMainWindow;
class CSpyMainWindow : public CXDialog {
	friend class CFindWnd;
	friend class CSpyWndManager;
public:
	CSpyMainWindow(QWidget* parent = nullptr);
	CWidgetSpyTree* tree();

public:
	bool setTreeTarget(QPoint pt);
	bool setTreeTarget(QGraphicsItem* item);
	bool setTreeTarget(QWidget* widget);
	int currentItemCount();
	void showCenter();
protected:
	virtual bool eventFilter(QObject* watched, QEvent* event) override;
	virtual void keyPressEvent(QKeyEvent* event) override;
private:
	void initWindow();
	void initMenuBar();
	void initSpyTree();
	void clearSpyTree();
	void updateStatusInfo();
	bool selfEventFilter(QObject* object, QEvent* event);
	void locateCursorWidget(QPoint pt);

	bool showSystemInfo();
	bool showMemoryMonitor();
	bool showSystemFont();
	bool searchSpyTreeByName();
	bool showCursorLocate();
	bool findTarget();
	bool checkColor();

private:
	CWidgetSpyTree* m_pTree{ nullptr };
	QLabel* m_pStatusInfo{ nullptr };
	QTimer* m_pStatusTimer{ nullptr };
	QWidget* m_pSpyWidget{ nullptr };
	QGraphicsItem* m_pSpyViewItem{ nullptr };
	EScreenMouseAction m_eCursorAction = EScreenMouseAction::None;
};