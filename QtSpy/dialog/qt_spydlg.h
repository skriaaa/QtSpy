#pragma once
#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTimer>
#include <QLineEdit>
#include <QStringListModel>
#include <qt_spyobject.h>
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

		}
		else {
			setWindowFlags(Qt::WindowStaysOnTopHint);
			raise();
			show();
		}
	}
};


class CSpyIndicatorWnd : public CXDialog {
public:
	CSpyIndicatorWnd(QWidget* parent = nullptr);
private:
	int m_nSpanPeriod;
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
	CStatusInfoWnd();
private:
	void UpdateStatusInfo();
protected:
	void keyReleaseEvent(QKeyEvent* event) override;
};

class CCursorLocateWnd : public CXDialog {
public:
	CCursorLocateWnd();
public:

protected:
	void paintEvent(QPaintEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override
	{
		update();
		QDialog::mouseMoveEvent(event);
	}
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
	template <typename T>
	bool AddInfo(T* pTarget, QEvent* event);
public:
	QSet<QObject*> m_arrMonitorObject;
	bool m_bShowEvent = true;
protected:
	void initWidget();
	bool eventFilter(QObject* pObject, QEvent* event) override;
private:
	template <typename T>
	QString EventInfo(T* pTarget, QEvent* event);
	QHash<QGraphicsScene*, CGraphicsItemSpy*> m_hashGraphicsSpy;
	bool m_bFilterEvent = false;
};

//class CResourceManageWnd : public CXDialog {
//public:
//	CResourceManageWnd(QWidget* parent = nullptr) : CXDialog(parent) {
//		setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowMinMaxButtonsHint);
//		setWindowTitle(_QStr("资源管理"));
//		auto mainLayout = new QVBoxLayout();
//		this->setLayout(mainLayout);
//		auto edit1 = new QLineEdit();
//		auto btnRegister1 = new QPushButton("加载资源文件");
//		QObject::connect(btnRegister1, &QPushButton::clicked, [&, edit1]() {
//			if (edit1) {
//				QString path = edit1->text();
//				char key[33] = { 0 };
//				GetUICore().uilRegisterResourceFile(path.toStdString().c_str(), key);
//			}
//			UpdateInfoTable();
//			});
//		auto sublayout1 = new QHBoxLayout();
//		sublayout1->addWidget(edit1);
//		sublayout1->addWidget(btnRegister1);
//		auto edit2 = new QLineEdit();
//		auto btnRegister2 = new QPushButton("加载资源数据");
//		QObject::connect(btnRegister2, &QPushButton::clicked, [&, edit2]() {
//			if (edit2) {
//				QString path = edit2->text();
//				char key[33] = { 0 };
//				std::ifstream file(path.toStdString().c_str(), std::ios::binary);
//				if (file.is_open()) {
//					file.seekg(0, std::ios::end);
//					std::streampos fileSize = file.tellg();
//					file.seekg(0, std::ios::beg);
//					std::vector<char> buffer(fileSize);
//					file.read(buffer.data(), fileSize);
//					file.close();
//					char key[33] = { 0 };
//					GetUICore().uilRegisterResourceData(buffer.data(), fileSize, key);
//				}
//			}
//			UpdateInfoTable();
//			});
//		auto sublayout2 = new QHBoxLayout();
//		sublayout2->addWidget(edit2);
//		sublayout2->addWidget(btnRegister2);
//		m_pInfoTable = new QTableWidget();
//		QStringList headers;
//		headers << _QStr("资源类型") << _QStr("内容") << _QStr("键值");
//		m_pInfoTable->setColumnCount(headers.size());
//		m_pInfoTable->setHorizontalHeaderLabels(headers);
//		m_pInfoTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
//		auto btnUnRegister = new QPushButton("卸载资源");
//		QObject::connect(btnUnRegister, &QPushButton::clicked, [&]() {
//			int row = m_pInfoTable->currentRow();
//			if (row != -1) {
//				QTableWidgetItem* item = m_pInfoTable->item(row, 2);
//				if (item) {
//					QString md5 = item->text();
//					GetCoreInst().uilUnRegisterResoure(md5.toStdString().c_str());
//				}
//			}
//			UpdateInfoTable();
//			});
//		mainLayout->addLayout(sublayout1);
//		mainLayout->addLayout(sublayout2);
//		mainLayout->addWidget(m_pInfoTable);
//		mainLayout->addWidget(btnUnRegister);
//		UpdateInfoTable();
//	}
//private:
//	void UpdateInfoTable() {
//		m_pInfoTable->clear();
//		m_pInfoTable->clearContents();
//		m_pInfoTable->setRowCount(0);
//		auto& mapResourceHandle = GetCoreInst().RefResourceHandleMap();
//		for (auto item : mapResourceHandle)
//		{
//			int row = m_pInfoTable->rowCount();
//			m_pInfoTable->insertRow(row);
//			if (item.second.data) {
//				auto col0 = new QTableWidgetItem("资源数据");
//				col0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//				auto col2 = new QTableWidgetItem(item.first);
//				col2->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//				m_pInfoTable->setItem(row, 0, col0);
//				m_pInfoTable->setItem(row, 2, col2);
//			}
//			else {
//				auto col0 = new QTableWidgetItem("资源文件");
//				col0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//				auto col1 = new QTableWidgetItem(item.second.file);
//				col1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//				auto col2 = new QTableWidgetItem(item.first);
//				col2->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//				m_pInfoTable->setItem(row, 0, col0);
//				m_pInfoTable->setItem(row, 1, col1);
//				m_pInfoTable->setItem(row, 2, col2);
//			}
//		}
//	}
//private:
//	QTableWidget* m_pInfoTable;
//};

//class CResourceCheckWnd : public CXDialog {
//public:
//	CResourceCheckWnd(QWidget* parent = nullptr) : CXDialog(parent) {
//		setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowMinMaxButtonsHint);
//		setWindowTitle(_QStr("check resoure"));
//		auto mainLayout = new QVBoxLayout();
//		this->setLayout(mainLayout);
//		auto edit = new QLineEdit();
//		mainLayout->addWidget(edit);
//		auto result = new QVBoxLayout();
//		mainLayout->addLayout(result);
//		auto btn1 = new QPushButton("check as image");
//		QObject::connect(btn1, &QPushButton::clicked, [edit, result, this]() {
//			auto item = result->takeAt(0);
//			if (item && item->widget()) {
//				item->widget()->deleteLater();
//			}
//			QString strUrl = edit->text();
//			QImage image(strUrl);
//			QString strTitle(_QStr("图像资源 (%1 x %2)"));
//			if (!image.isNull()) {
//				strTitle = strTitle.arg(image.width()).arg(image.height());
//			}
//			else {
//				strTitle = _QStr("Image资源未找到");
//			}
//			this->setWindowTitle(strTitle);
//			auto label = new QLabel();
//			label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//			label->setStyleSheet(_QStr("QLabel { background-image: url(%1); background-repeat: no-repeat; background-position: center; background-attachment: fixed; background-size: contain;}  ").arg(strUrl));
//			result->addWidget(label);
//			});
//		auto btn2 = new QPushButton("check as xml file");
//		QObject::connect(btn2, &QPushButton::clicked, [edit, result, this]() {
//			auto item = result->takeAt(0);
//			if (item && item->widget()) {
//				item->widget()->deleteLater();
//			}
//			QString strUrl = edit->text();
//			QFile file(strUrl);
//			if (file.open(QIODeviceBase::OpenModeFlag::ReadOnly)) {
//				this->setWindowTitle(file.fileName());
//				auto textEdit = new QPlainTextEdit();
//				QByteArray data = file.readAll();
//				QString strContent(data.constData());
//				textEdit->setPlainText(strContent);
//				result->addWidget(textEdit);
//			}
//			else {
//				this->setWindowTitle(_QStr("Xml资源未找到"));
//			}
//			});
//		mainLayout->addWidget(btn1);
//		mainLayout->addWidget(btn2);
//	}
//};

