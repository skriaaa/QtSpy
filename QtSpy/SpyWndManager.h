#pragma once
#include <QObject>
#include <QList>
class CSpyMainWindow;
class CSpyWndManager : public QObject
{
	Q_OBJECT
public:
	CSpyWndManager(QObject* parent = nullptr);
	~CSpyWndManager();
protected:
	virtual bool eventFilter(QObject* watched, QEvent* event) override;
private:
	CSpyMainWindow* spyWnd();
private:
	CSpyMainWindow* m_pMainWnd{ nullptr };
};

