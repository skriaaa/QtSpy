#include "MemoryMonitorDlg.h"
#include <QHBoxLayout>
#include <QListView>
#include <QPushButton>
#include <QTime>
#include <QStringListModel>
#include "utils/ProcessInfo.h"
#include "utils/LogRecorder.h"
#include <QMutex>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <detours/detours.h>
#include <DbgHelp.h>

static HANDLE s_hMainThread;
//dbghelper
static QMutex g_cs;
void InitSymbolsOnce() {
	static bool s_bInited = false;
	if (s_bInited)
	{
		return;
	}

	s_hMainThread = GetCurrentThread();
	SymInitialize(GetCurrentProcess(), NULL, TRUE);
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
	s_bInited = true;
}
// 获取调用者信息
void GetCallerInfo(void* address, char* buffer, size_t bufferSize) {
	DWORD64 displacement = 0;
	char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	if (SymFromAddr(GetCurrentProcess(), (DWORD64)address, &displacement, pSymbol)) {
		snprintf(buffer, bufferSize, "%s+0x%llX", pSymbol->Name, displacement);
	}
	else {
		DWORD nId = GetLastError();
		Q_UNUSED(nId);
		snprintf(buffer, bufferSize, "0x%p", address);
	}
}

// detours
// 原始函数指针
typedef void* (__cdecl* MallocFunc)(size_t);
typedef void(__cdecl* FreeFunc)(void*);

MallocFunc TrueMalloc = (MallocFunc)malloc;
FreeFunc TrueFree = (FreeFunc)free;
static thread_local int tls_recursion_depth = 0;

// Hook 函数
void* __cdecl HookMalloc(size_t size) {
	void* ptr = TrueMalloc(size);
	if (0 == tls_recursion_depth && s_hMainThread == GetCurrentThread())
	{
		tls_recursion_depth++;
		char szLog[MAX_SYM_NAME] = { 0 };
		GetCallerInfo(_ReturnAddress(), szLog, MAX_SYM_NAME);
		LogRecorder().addLog(QString("[Hook][malloc]address: %1 | size: %2 | %3").arg((quintptr)ptr, QT_POINTER_SIZE * 2, 16, QChar('0')).arg(size).arg(szLog));
		tls_recursion_depth--;
	}
	return ptr;
}

void __cdecl HookFree(void* ptr) {
	if (0 == tls_recursion_depth && s_hMainThread == GetCurrentThread())
	{
		tls_recursion_depth++;
		char szLog[MAX_SYM_NAME] = { 0 };
		GetCallerInfo(_ReturnAddress(), szLog, MAX_SYM_NAME);
		LogRecorder().addLog(QString("[Hook][free]address: %1 | %2").arg((quintptr)ptr, QT_POINTER_SIZE * 2, 16, QChar('0')).arg(szLog));
		tls_recursion_depth--;
	}
	TrueFree(ptr);
}
#endif

void attachMemory()
{
	InitSymbolsOnce();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)TrueMalloc, HookMalloc);
	DetourAttach(&(PVOID&)TrueFree, HookFree);
	DetourTransactionCommit();
}
void detachMemory()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)TrueMalloc, HookMalloc);  // 恢复原函数
	DetourDetach(&(PVOID&)TrueFree, HookFree);
	DetourTransactionCommit();
}

CMemoryMonitorDlg::CMemoryMonitorDlg(QWidget* parent /*= nullptr*/):CXDialog(parent)
{
	setWindowTitle("内存监控");
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::Popup | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint | Qt::CustomizeWindowHint);
	resize(320, 380);
	initWidgets();
}

void CMemoryMonitorDlg::initWidgets()
{
	setLayout(new QVBoxLayout());

	m_listView = new QListView;
	m_listView->setModel(new QStringListModel());
	connect(m_listView, &QListView::clicked, [=]() {
		m_bTrace = false;
		});

	QPushButton* btn = new QPushButton("开始监控");
	connect(btn, &QPushButton::clicked, [=]() {
		if (m_nTimerID == -1)
		{
			m_nTimerID = startTimer(100);
			btn->setText("停止监控");
		}
		else
		{
			killTimer(m_nTimerID);
			m_nTimerID = -1;
			btn->setText("开始监控");
		}
		});
	QPushButton* btnTrace = new QPushButton("Trace");
	connect(btnTrace, &QPushButton::clicked, [=]() {
		m_bTrace = !m_bTrace;
		});

	QPushButton* btnAttach = new QPushButton("Attach");
	connect(btnAttach, &QPushButton::clicked, [=]() {
		m_bAttach = !m_bAttach;
		if(m_bAttach)
		{
			btnAttach->setText("Detach");
			attachMemory();
		}
		else
		{
			btnAttach->setText("Attach");
			detachMemory();
		}
	});

	layout()->addWidget(m_listView);
	layout()->addWidget(btnTrace);
	layout()->addWidget(btn);
	layout()->addWidget(btnAttach);
}

void CMemoryMonitorDlg::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == m_nTimerID)
	{
		int nMemoryCount = ProcessInfo::queryProcessCurrentMemory();
		int nDiffer = nMemoryCount - m_nMemoryCount;
		m_nMemoryCount = nMemoryCount;
		QStringListModel* model = static_cast<QStringListModel*>(m_listView->model());
		model->insertRow(model->rowCount());

		QString strRowString = QString("%1 | 当前内存占用 %2 (MByte)| 变化值 %3 (MByte)").arg(QTime().currentTime().toString("hh:mm:ss::zzz")).arg(m_nMemoryCount).arg(nDiffer);
		model->setData(model->index(model->rowCount() - 1), strRowString);
		if (m_bTrace)
		{
			m_listView->scrollTo(model->index(model->rowCount() - 1));
		}
	}
}
