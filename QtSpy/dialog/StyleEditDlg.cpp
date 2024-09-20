#include "StyleEditDlg.h"
// Qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QListView>
#include <QStandardItemModel>
#include <QGuiApplication>
#include <QScreen>
// this module
#include "stylemanager.h"
#include "publicfunction.h"

CStyleEditWnd::CStyleEditWnd(QWidget* parent /*= nullptr*/) : CXDialog(parent)
{
	m_pTargetWidget = nullptr;
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowMinMaxButtonsHint);
	initWidgets();
}

bool CStyleEditWnd::EditWidgetStyle(QWidget* pWidget)
{
	if (pWidget) {
		m_pTargetWidget = pWidget;
		auto edit = findChild<QPlainTextEdit*>();
		if (edit) {
			QStyle* style = m_pTargetWidget->style();
			edit->setPlainText(m_pTargetWidget->styleSheet());
		}
		return true;
	}
	return false;
}

void CStyleEditWnd::initWidgets()
{
	auto mainLayout = new QVBoxLayout();
	this->setLayout(mainLayout);
	auto edit = new QPlainTextEdit();
	mainLayout->addWidget(edit);

	QHBoxLayout* hLayout = new QHBoxLayout;
	mainLayout->addLayout(hLayout);
	QPushButton* btnTips = new QPushButton("keys");
	auto btnApply = new QPushButton("modify style");
	hLayout->addWidget(btnTips);
	hLayout->addWidget(btnApply);

	QObject::connect(btnTips, &QPushButton::clicked, [this]() {
		showKeys();
		});

	QObject::connect(btnApply, &QPushButton::clicked, [this]() {
		if (m_pTargetWidget) {
			auto editStyle = dynamic_cast<QPlainTextEdit*>(layout()->itemAt(0)->widget());
			QString strStyleSheet = editStyle->toPlainText();
			m_pTargetWidget->setStyleSheet(strStyleSheet);
			m_pTargetWidget->style()->unpolish(m_pTargetWidget);
			m_pTargetWidget->style()->polish(m_pTargetWidget);
			m_pTargetWidget->ensurePolished();
		}
		});
}

void CStyleEditWnd::showKeys()
{
	static QDialog* dlgKeys = nullptr;
	if(dlgKeys == nullptr)
	{
		dlgKeys = new QDialog;
		dlgKeys->setWindowFlag(Qt::WindowStaysOnTopHint);
		dlgKeys->setAttribute(Qt::WA_DeleteOnClose, false);
		dlgKeys->setLayout(new QHBoxLayout());
		{
			QListView* listSubCtrl = new QListView;
			listSubCtrl->setEditTriggers(false);
			dlgKeys->layout()->addWidget(listSubCtrl);
			QStandardItemModel* model = new QStandardItemModel;
			listSubCtrl->setModel(model);
			model->appendColumn({ new QStandardItem("SubControl") });
			for (int i = 0; i < queryEnumCount<CStyleSheetProxy::ESubCtrl>(); i++)
			{
				model->appendRow(new QStandardItem(queryEnumName<CStyleSheetProxy::ESubCtrl>(i)));
			}
		}

		{
			QListView* listPseudoStates = new QListView;
			listPseudoStates->setEditTriggers(false);
			dlgKeys->layout()->addWidget(listPseudoStates);
			QStandardItemModel* model = new QStandardItemModel;
			listPseudoStates->setModel(model);
			model->appendColumn({ new QStandardItem("PseudoStates") });
			for (int i = 0; i < queryEnumCount<CStyleSheetProxy::EPseudoStates>(); i++)
			{
				model->appendRow(new QStandardItem(queryEnumName<CStyleSheetProxy::EPseudoStates>(i)));
			}
		}
		dlgKeys->setGeometry(QRect(0, 0, 300, 400));
	}

	dlgKeys->setGeometry(
		QStyle::alignedRect(
			Qt::LeftToRight,
			Qt::AlignCenter,
			dlgKeys->size(),
			qApp->primaryScreen()->availableGeometry()
		));
	dlgKeys->raise();
	dlgKeys->show();
}
