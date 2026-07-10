#pragma once
#include "qt_spydlg.h"

class QShowEvent;

class CStyleEditWnd : public CXDialog {
public:
	CStyleEditWnd(QWidget* parent = nullptr);
public:
	bool EditWidgetStyle(QWidget* pWidget);
protected:
	void initWidgets();
	void showKeys();
	void showEvent(QShowEvent* event) override;
public:
	QWidget* m_pTargetWidget;
};


