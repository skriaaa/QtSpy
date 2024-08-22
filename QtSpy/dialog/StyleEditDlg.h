#pragma once
#include "qt_spydlg.h"

class CStyleEditWnd : public CXDialog {
public:
	CStyleEditWnd(QWidget* parent = nullptr);
public:
	bool EditWidgetStyle(QWidget* pWidget);
protected:
	void initWidgets();
	void showKeys();
public:
	QWidget* m_pTargetWidget;
};


