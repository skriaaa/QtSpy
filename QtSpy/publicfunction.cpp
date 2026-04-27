#include "publicfunction.h"
// c++ or lib
#include <typeinfo>

// qt
#include <QPoint>
#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QApplication>

QPoint convertGlobalPointToWidget(QPoint ptGlobal, QWidget* pTargetWidget)
{
	if (nullptr == pTargetWidget)
	{
		return ptGlobal;
	}
	QWidget* pWidget = pTargetWidget;
	for (; nullptr != pWidget; pWidget = pWidget->parentWidget())
	{
		QGraphicsProxyWidget* pProxyWidget = pWidget->graphicsProxyWidget();
		if (nullptr == pProxyWidget)
		{
			continue;
		}

		QGraphicsScene* pScene = pProxyWidget->scene();
		if (nullptr == pScene)
		{
			continue;
		}

		QList<QGraphicsView*> arrViews = pScene->views();
		if (arrViews.empty() || nullptr == arrViews.front())
		{
			continue;
		}

		QPoint ptScene = arrViews.front()->mapToScene(arrViews.front()->mapFromGlobal(ptGlobal)).toPoint();
		return pTargetWidget->mapFrom(pWidget, pProxyWidget->mapFromScene(ptScene).toPoint());
	}

	return pTargetWidget->mapFromGlobal(ptGlobal);
}

QPoint convertWidgetPointToGlobal(QPoint ptGlobal, QPoint ptWidget, QObject* pTarget)
{
	QWidget* pTargetWidget = dynamic_cast<QWidget*>(pTarget);
	if (nullptr == pTargetWidget)
	{
		return ptGlobal;
	}

	QWidget* pWidget = pTargetWidget;
	for (; nullptr != pWidget; pWidget = pWidget->parentWidget())
	{
		QGraphicsProxyWidget* pProxyWidget = pWidget->graphicsProxyWidget();
		if (nullptr == pProxyWidget)
		{
			continue;
		}

		QGraphicsScene* pScene = pProxyWidget->scene();
		if (nullptr == pScene)
		{
			continue;
		}

		QList<QGraphicsView*> arrViews = pScene->views();
		if (arrViews.empty() || nullptr == arrViews.front())
		{
			continue;
		}

		QPoint ptScene = pProxyWidget->mapToScene(pTargetWidget->mapTo(pWidget, ptWidget)).toPoint();
		return arrViews.front()->mapToGlobal(arrViews.front()->mapFromScene(ptScene));
	}

	return pTargetWidget->mapToGlobal(ptWidget);
}

QPoint MapToGlobal(QWidget* pWidget, QPoint pt)
{
	return convertWidgetPointToGlobal(pt, pt, pWidget);
}

QRect MapToGlobal(QWidget* pWidget, QRect rc)
{
	return QRect(MapToGlobal(pWidget, rc.topLeft()), MapToGlobal(pWidget, rc.bottomRight()));
}

QPoint MapFromGlobal(QWidget* pWidget, QPoint pt)
{
	return convertGlobalPointToWidget(pt, pWidget);
}

QRect MapFromGlobal(QWidget* pWidget, QRect rc)
{
	return QRect(MapFromGlobal(pWidget, rc.topLeft()), MapFromGlobal(pWidget, rc.bottomRight()));
}

QRect ScreenRect(QWidget* pWidget, QRect rc)
{
	if (nullptr == pWidget)
	{
		return QRect();
	}
	return MapToGlobal(pWidget, rc.isEmpty() ? pWidget->rect() : rc);
}

QRect ScreenRect(QGraphicsItem* pItem)
{
	if (nullptr == pItem)
	{
		return QRect();
	}

	auto geo = pItem->sceneBoundingRect();
	auto view = pItem->scene()->views().front();
	auto lt = view->mapToGlobal(view->mapFromScene(QPoint(geo.left(), geo.top())));
	return QRect(lt, QSize(geo.width(), geo.height()));
}

QString objectClass(QObject* object)
{
	if (object)
	{
		return object->metaObject()->className();
	}
	return QString();
}
QString objectName(QObject* object)
{
	if (object)
	{
		return object->objectName();
	}
	return QString();
}

QString pointerToHex(const void* pointer) {
	return "0x" + QString::number((uintptr_t)pointer, 16);
}

QString objectString(QObject* object)
{
	if (object == nullptr)
		return "";

	QString strText;
	if (OTo<QWidget>(object))
	{
		if (auto pWidget = OTo<QDialog>(object)) 
		{
			strText = pWidget->windowTitle();
		}
		else if (auto pWidget = OTo<QAbstractButton>(object))
		{
			strText = pWidget->text();
		}
		else if (auto pWidget = OTo<QLineEdit>(object))
		{
			strText = pWidget->text();
		}
		else if (auto pWidget = OTo<QLabel>(object))
		{
			strText = pWidget->text();
		}
		else if (auto pWidget = OTo<QComboBox>(object)) 
		{
			strText = pWidget->currentText();
		}
		else if (auto pWidget = OTo<QCheckBox>(object))
		{
			strText = pWidget->text();
		}

		strText += QString(" | %1 | %2").arg(object->objectName()).arg(pointerToHex(object));
	}
	else if(OTo<QLayout>(object))
	{
		strText += QString(" | %1 | %2").arg(object->objectName()).arg(pointerToHex(object));
	}

	QString strItemInfo = QString("%1(%2)").arg(objectClass(object)).arg(strText);
	if (OTo<QWidget>(object) && !OTo<QWidget>(object)->isVisible())
	{
		strItemInfo += "[hide]";
	}

	if (OTo<QWidget>(object) && !OTo<QWidget>(object)->isEnabled())
	{
		strItemInfo += "[disabled]";
	}

	return strItemInfo;
}

QString objectString(QGraphicsItem* pItem)
{
	if (nullptr == pItem)
	{
		return "";
	}

	QString strText = QString("%1 | %2").arg("graphicsItem").arg(pointerToHex(pItem));
	QString strItemInfo = QString("%1(%2)").arg(objectClass(To<QObject>(pItem))).arg(strText);
	if(!pItem->isVisible())
	{
		strItemInfo += "[hide]";
	}
	if(!pItem->isEnabled())
	{
		strItemInfo += "[disabled]";
	}
	return strItemInfo;
}

class CQssAnalyze
{
	struct QssInfo {
		QString strName;
		QString strInfo;
	};
public:
	QString QueryTargetQss(QWidget* widget)
	{
		QString strStyle;
		do
		{
			strStyle += widget->styleSheet();
			widget = OTo<QWidget>(widget->parent());
		} while (widget);
		
		Analyze(strStyle);
		QString strRet;
		for (auto info : m_arrInfo)
		{
			if (info.strName.indexOf(widget->metaObject()->className(), 0, Qt::CaseInsensitive) != -1 
				|| info.strName.isEmpty())
			{
				strRet += info.strName + info.strInfo + "\n";
			}
		}
		return strRet;
	}
private:
	void Analyze(QString strStyleSheet, int offset = 0)
	{
		QssInfo info;
		QString strSplitLeft = "{";
		QString strSplitRight = "}";
		int left = strStyleSheet.indexOf(strSplitLeft, offset);
		if (left == -1)
			return;
		info.strName = strStyleSheet.mid(offset, left - offset);	

		int right = strStyleSheet.indexOf(strSplitRight, left);
		if (right == -1)
			return;
		info.strInfo = strStyleSheet.mid(left, right - left + 1);
		
		m_arrInfo.push_back(info);

		Analyze(strStyleSheet, right + 1);
	}

	QVector<QssInfo> m_arrInfo;
};

QString styleSheet(QWidget* widget)
{
	CQssAnalyze analyze;
	return analyze.QueryTargetQss(widget);
}

QWidget* widgetAt(QPoint pt)
{
	auto pGraphicsViewItem = graphicsItemAt(pt);
	if(nullptr == pGraphicsViewItem)
	{
		return QApplication::widgetAt(pt);
	}

	if (To<QGraphicsProxyWidget>(pGraphicsViewItem))
	{
		auto pWidget = To<QGraphicsProxyWidget>(pGraphicsViewItem)->widget();
		if (!pWidget->children().empty())
		{
			auto pFind = pWidget->childAt(MapFromGlobal(pWidget, pt));
			if (nullptr != pFind)
			{
				pWidget = pFind;
			}
		}

		return pWidget;
	}

	return nullptr;
}

QGraphicsItem* graphicsItemAt(QPoint pt)
{
	auto pWidget = QApplication::widgetAt(pt);
	if (nullptr == pWidget)
	{
		return nullptr;
	}

	auto pView = OTo<QGraphicsView>(pWidget->parent());
	if(nullptr == pView)
	{
		return nullptr;
	}

	if(nullptr == pView->scene())
	{
		return nullptr;
	}

	return pView->scene()->itemAt(pView->mapToScene(pView->mapFromGlobal(pt)), QTransform());
}

QString normalStyleSheet()
{
	QString strStyleSheet = R"(
/* ========================================================================== */
/* Global & Common Styles                                                    */
/* ========================================================================== */

QWidget {
	font-family: 'Microsoft YaHei', 'Segoe UI';
	font-size: 9pt;
	color: black;
	background-color: #f0f0f0;
}

QDialog {
	background-color: #f0f0f0;
}


/* ========================================================================== */
/* QLabel Styles                                                             */
/* ========================================================================== */

QLabel {
	background: transparent;
}


/* ========================================================================== */
/* QPushButton Styles                                                        */
/* ========================================================================== */

QPushButton {
	background-color: #e1e1e1;
	border: 1px solid #adadad;
	padding: 4px;
	min-width: 60px;
	font-size: 9pt;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QPushButton:hover {
	background-color: #e5f1fb;
	border: 1px solid #0078d7;
}

QPushButton:pressed {
	background-color: #cce4f7;
	border: 1px solid #005499;
}


/* ========================================================================== */
/* QLineEdit Styles                                                          */
/* ========================================================================== */

QLineEdit {
	border: 1px solid #7a7a7a;
	background: white;
	selection-background-color: #0078d7;
	font-size: 9pt;
	color: black;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}


/* ========================================================================== */
/* QTextEdit & QPlainTextEdit Styles                                         */
/* ========================================================================== */

QTextEdit, QPlainTextEdit {
	background-color: white;
	border: 1px solid #7a7a7a;
	selection-background-color: #0078d7;
	selection-color: white;
	font-size: 9pt;
	color: black;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QTextEdit:focus, QPlainTextEdit:focus {
	border: 1px solid #0078d7;
}


/* ========================================================================== */
/* QCheckBox & QRadioButton Styles                                           */
/* ========================================================================== */

QCheckBox, QRadioButton {
	spacing: 5px;
	font-size: 9pt;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QCheckBox::indicator, QRadioButton::indicator {
	width: 13px;
	height: 13px;
}

QCheckBox::indicator:unchecked {
	border: 1px solid #7a7a7a;
	background: white;
}

QCheckBox::indicator:checked {
	border: 1px solid #0078d7;
	background: #0078d7;
}

QRadioButton::indicator:unchecked {
	border: 1px solid #7a7a7a;
	background: white;
	border-radius: 7px;
}

QRadioButton::indicator:checked {
	border: 1px solid #0078d7;
	background: #0078d7;
	border-radius: 7px;
}


/* ========================================================================== */
/* QSpinBox & QDoubleSpinBox Styles                                          */
/* ========================================================================== */

QSpinBox, QDoubleSpinBox {
	background: white;
	border: 1px solid #7a7a7a;
	padding: 2px;
	font-size: 9pt;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QSpinBox::up-button, QDoubleSpinBox::up-button {
	background: #e1e1e1;
	border-left: 1px solid #7a7a7a;
}

QSpinBox::down-button, QDoubleSpinBox::down-button {
	background: #e1e1e1;
	border-left: 1px solid #7a7a7a;
}


/* ========================================================================== */
/* QComboBox Styles                                                          */
/* ========================================================================== */

QComboBox {
	background: white;
	border: 1px solid #7a7a7a;
	padding: 2px 18px 2px 3px;
	font-size: 9pt;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QComboBox::drop-down {
	subcontrol-origin: padding;
	subcontrol-position: top right;
	width: 15px;
	border-left: 1px solid #7a7a7a;
}

QComboBox::down-arrow {
	width: 0;
	height: 0;
	border-left: 4px solid transparent;
	border-right: 4px solid transparent;
	border-top: 4px solid #606060;
}

QComboBox QAbstractItemView {
	background: white;
	border: 1px solid #7a7a7a;
	selection-background-color: #cce8ff;
	selection-color: black;
}


/* ========================================================================== */
/* QScrollBar Styles                                                         */
/* ========================================================================== */

QScrollBar:vertical {
	background: #f0f0f0;
	width: 6px;
	margin: 0px;
}

QScrollBar::handle:vertical {
	background: #cdcdcd;
	min-height: 20px;
	border-radius: 0px;
}

QScrollBar::handle:vertical:hover {
	background: #a6a6a6;
}

QScrollBar::add-line:vertical {
	height: 0px;
	subcontrol-position: bottom;
	subcontrol-origin: margin;
}

QScrollBar::sub-line:vertical {
	height: 0px;
	subcontrol-position: top;
	subcontrol-origin: margin;
}

QScrollBar:horizontal {
	background: #f0f0f0;
	height: 6px;
	margin: 0px;
}

QScrollBar::handle:horizontal {
	background: #cdcdcd;
	min-width: 20px;
	border-radius: 0px;
}

QScrollBar::handle:horizontal:hover {
	background: #a6a6a6;
}


/* ========================================================================== */
/* QMenu & QMenuBar Styles                                                   */
/* ========================================================================== */

QMenu {
	background: white;
	color: black;
	border: 1px solid #cccccc;
	font-family: 'Microsoft YaHei', 'Segoe UI';
	font-size: 9pt;
	padding: 0px;
	margin: 0px;
	border-radius: 0px;
}

QMenu::item {
	padding: 4px 10px;
	background: transparent;
	color: black;
	border-radius: 0px;
}

QMenu::item:hover {
	background-color: #90c8f6;
	color: black;
	border-radius: 0px;
}

QMenu::item:selected {
	background-color: #90c8f6;
	color: black;
	border-radius: 0px;
}

QMenu::item:disabled {
	color: #404040;
}

QMenu::item:selected:disabled {
	color: #404040;
}

QMenuBar {
	background: #f0f0f0;
	color: black;
	border-bottom: 1px solid #dcdcdc;
	font-family: 'Microsoft YaHei', 'Segoe UI';
	font-size: 9pt;
}

QMenuBar::item {
	background: transparent;
	color: black;
	padding: 4px 10px;
}

QMenuBar::item:selected {
	background: #cce8ff;
}

QMenuBar::item:pressed {
	background: #99c9ef;
}


/* ========================================================================== */
/* QTabWidget & QTabBar Styles                                               */
/* ========================================================================== */

QTabWidget::pane {
	border: 1px solid #7a7a7a;
	background: white;
	padding: 2px;
}

QTabWidget::tab-bar {
	alignment: left;
}

QTabBar::tab {
	background: #e1e1e1;
	border: 1px solid #adadad;
	padding: 4px 10px;
	margin-right: 2px;
	font-size: 9pt;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QTabBar::tab:selected {
	background: white;
	border-bottom: 0px;
}

QTabBar::tab:hover {
	background: #e5f1fb;
}


/* ========================================================================== */
/* QAbstractView & QHeaderView Styles (Base for Tree/Table/List)             */
/* ========================================================================== */

QAbstractView {
	background: white;
	color: rgba(15,15,15,255);
	border: 1px solid #7a7a7a;
	font-size: 9pt;
}

QAbstractView::item {
	outline: none;
	border: none;
}

QAbstractView::item:hover {
	background-color: rgba(205, 232, 255, 255);
	color: black;
}

QAbstractView::item:selected {
	background-color: rgba(217, 217, 217, 255);
	color: black;
}

QHeaderView::section {
	background-color: #f0f0f0;
	color: black;
	border: 1px solid #dcdcdc;
	padding: 4px;
	font-size: 9pt;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}


/* ========================================================================== */
/* QTreeWidget Styles                                                        */
/* ========================================================================== */

QTreeWidget {
	background:rgba(255,255,255,255);
	color:black;
	font:9pt Microsoft YaHei;
	border:1px solid rgba(185,185,185,255);
	outline:0px;
    show-decoration-selected:1;
}

QTreeWidget::item{
	outline:0px;
	border:3px;
	height: 25px;
	color:transparent;
}

QTreeWidget::item:hover,QTreeWidget::branch:hover{
	background:rgba(235,235,235,255);
}

QTreeWidget::item:selected,QTreeWidget::branch:selected {
	background:rgba(204,232,255,255);
}

QTreeWidget::branch {
	padding-top:2px;
}

QTreeWidget::branch:has-children:closed {
	image: url(:/icons/resource/tree_open.png);
}

QTreeWidget::branch:has-children:open {
	image: url(:/icons/resource/tree_close.png);
}

/* ========================================================================== */
/* QTableWidget Styles                                                       */
/* ========================================================================== */

QTableWidget {
	background-color: white;
	alternate-background-color: #f7f7f7;
	gridline-color: #d0d0d0;
	selection-background-color: #cce8ff;
	selection-color: black;
	border: 1px solid #7a7a7a;
	font-size: 9pt;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QTableWidget::item {
	padding: 2px;
	border: none;
	outline: none;
}

QTableWidget::item:hover {
	background-color: #e5f3ff;
}

QTableWidget::item:selected {
	background-color: #cce8ff;
	color: black;
}

QTableWidget QTableCornerButton::section {
	background-color: #f0f0f0;
	border: 1px solid #dcdcdc;
}


/* ========================================================================== */
/* QListView Styles                                                          */
/* ========================================================================== */

QListView {
	background-color: white;
	border: 1px solid #7a7a7a;
	selection-background-color: #cce8ff;
	selection-color: black;
	font-size: 9pt;
	outline: none;
	padding: 0px;
	margin: 0px;
	font-family: 'Microsoft YaHei', 'Segoe UI';
}

QListView::item {
	padding: 2px;
	border: none;
	color: black;
	background: white;
}

QListView::item:hover {
	background-color: #e5f3ff;
}

QListView::item:selected {
	background-color: #cce8ff;
	color: black;
}

QListView::item:selected:!active {
	background-color: #d9d9d9;
}
)";
	return strStyleSheet;
}
