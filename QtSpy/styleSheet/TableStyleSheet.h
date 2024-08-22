#pragma once
#include "stylemanager.h"
class QTableView;
class CTableStyleSheet:public CStyleSheetProxy
{
	Q_OBJECT
public:
	CTableStyleSheet();
	~CTableStyleSheet();
public:

	/// 设置行高
	void setRowHeight(int nHeight, int nRow = -1);

	/// 设置列宽
	void setColumnWidth(int nHeight, int nColumn = -1);

	/// 
private:
	QTableView* m_pTableView;
};

