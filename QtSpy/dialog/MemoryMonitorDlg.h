#pragma once
#include "qt_spydlg.h"
class QListView;
class CMemoryMonitorDlg : public CXDialog
{
public:
	CMemoryMonitorDlg(QWidget* parent = nullptr);
protected:
	virtual void timerEvent(QTimerEvent* event);
private:
    void initWidgets();
	QListView* m_listView = nullptr;
	int m_nTimerID = -1;
	int m_nMemoryCount = 0;
	bool m_bTrace = true;
};

