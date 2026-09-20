#include "theme/QtSpyTheme.h"

#include <QWidget>

const SpyPalette& QtSpyTheme::palette()
{
	static const SpyPalette s_palette;
	return s_palette;
}

void QtSpyTheme::apply(QWidget* pWnd)
{
	if (nullptr == pWnd)
	{
		return;
	}

	// 纵深防御: setStyleSheet 派生的 StyleChange/FontChange 事件会同步回到
	// CXDialog::event, 若样式机制出现意料外的相互触发, 这里截断递归
	static bool bReentrant = false;
	if (bReentrant)
	{
		return;
	}
	bReentrant = true;

	// 注意: 此处只挂 QSS, 严禁 setFont ——
	// QSS 的 font-family/font-size 规则由 QStyleSheetStyle 直接改写 widget 字体,
	// 若再 setFont, 两者(QFontDef 的 families 列表/resolve mask)不相等会触发
	// updateStyleSheetFont 改字体 + FontChange -> 自愈再 setFont 的无限递归(栈溢出)
	const QString strQss = qss();
	if (pWnd->styleSheet() != strQss)
	{
		pWnd->setStyleSheet(strQss);
	}

	bReentrant = false;
}

QString QtSpyTheme::qss()
{
	const SpyPalette& p = palette();

	// 颜色 token -> ${name} 占位符替换
	const struct
	{
		const char* name;
		QColor color;
	} arrColorTokens[] = {
		{ "windowBg",      p.windowBg },
		{ "contentBg",     p.contentBg },
		{ "altRow",        p.altRow },
		{ "border",        p.border },
		{ "accent",        p.accent },
		{ "accentHover",   p.accentHover },
		{ "accentPressed", p.accentPressed },
		{ "selectionBg",   p.selectionBg },
		{ "selectionText", p.selectionText },
		{ "hoverBg",       p.hoverBg },
		{ "textPrimary",   p.textPrimary },
		{ "textSecondary", p.textSecondary },
		{ "textDisabled",  p.textDisabled },
		{ "danger",        p.danger },
		{ "highlightChange", p.highlightChange },
	};
	// 密度 token -> ${name} 占位符替换
	const struct
	{
		const char* name;
		int value;
	} arrSizeTokens[] = {
		{ "controlHeight", p.controlHeight },
		{ "inputHeight",   p.inputHeight },
		{ "radius",        p.radius },
	};

	// QSS 模板: 关键属性(背景/文字/边框/选中)在根与每个基础控件类上写全,
	// 防止目标程序全局 QSS 的低特异性规则(如 QWidget{color:red})穿透
	QString strQss = R"(
QWidget {
	font-family: 'Microsoft YaHei UI', 'Microsoft YaHei', 'Segoe UI';
	font-size: 9pt;
	color: ${textPrimary};
	background-color: ${windowBg};
}

QDialog {
	background-color: ${windowBg};
}

QLabel {
	background: transparent;
	color: ${textPrimary};
}
QLabel:disabled {
	color: ${textDisabled};
}

QPushButton {
	background-color: ${contentBg};
	border: 1px solid ${border};
	border-radius: ${radius}px;
	min-height: ${controlHeight}px;
	padding: 2px 16px;
	color: ${textPrimary};
}
QPushButton:hover {
	background-color: ${hoverBg};
	border: 1px solid ${accent};
}
QPushButton:pressed {
	background-color: ${selectionBg};
	border: 1px solid ${accentPressed};
}
QPushButton:disabled {
	color: ${textDisabled};
	border: 1px solid ${border};
	background-color: ${windowBg};
}
QPushButton[spyClass="danger"] {
	color: ${danger};
}
QPushButton[spyClass="danger"]:hover {
	border: 1px solid ${danger};
	background-color: ${hoverBg};
}

QLineEdit, QTextEdit, QPlainTextEdit {
	background: ${contentBg};
	border: 1px solid ${border};
	border-radius: ${radius}px;
	min-height: ${inputHeight}px;
	padding: 2px 6px;
	color: ${textPrimary};
	selection-background-color: ${accent};
	selection-color: ${selectionText};
}
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
	border: 1px solid ${accent};
}
QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {
	color: ${textDisabled};
	background: ${windowBg};
	border: 1px solid ${border};
}

QCheckBox, QRadioButton {
	spacing: 5px;
	color: ${textPrimary};
	background: transparent;
}
QCheckBox:disabled, QRadioButton:disabled {
	color: ${textDisabled};
}
QCheckBox::indicator, QRadioButton::indicator {
	width: 13px;
	height: 13px;
}
QCheckBox::indicator:unchecked {
	border: 1px solid ${border};
	background: ${contentBg};
	border-radius: 2px;
}
QCheckBox::indicator:checked {
	border: 1px solid ${accent};
	background: ${accent};
	border-radius: 2px;
}
QCheckBox::indicator:disabled {
	border: 1px solid ${border};
	background: ${windowBg};
}
QRadioButton::indicator {
	border-radius: 7px;
}
QRadioButton::indicator:unchecked {
	border: 1px solid ${border};
	background: ${contentBg};
}
QRadioButton::indicator:checked {
	border: 1px solid ${accent};
	background: ${accent};
}

QSpinBox, QDoubleSpinBox {
	background: ${contentBg};
	border: 1px solid ${border};
	border-radius: ${radius}px;
	min-height: ${inputHeight}px;
	padding: 2px 4px;
	color: ${textPrimary};
}
QSpinBox:focus, QDoubleSpinBox:focus {
	border: 1px solid ${accent};
}
QSpinBox:disabled, QDoubleSpinBox:disabled {
	color: ${textDisabled};
	background: ${windowBg};
}
QSpinBox::up-button, QDoubleSpinBox::up-button {
	background: ${windowBg};
	border-left: 1px solid ${border};
	width: 16px;
}
QSpinBox::down-button, QDoubleSpinBox::down-button {
	background: ${windowBg};
	border-left: 1px solid ${border};
	width: 16px;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
	background: ${hoverBg};
}

QComboBox {
	background: ${contentBg};
	border: 1px solid ${border};
	border-radius: ${radius}px;
	min-height: ${inputHeight}px;
	padding: 2px 18px 2px 8px;
	color: ${textPrimary};
}
QComboBox:hover {
	border: 1px solid ${accentHover};
}
QComboBox:focus {
	border: 1px solid ${accent};
}
QComboBox:disabled {
	color: ${textDisabled};
	background: ${windowBg};
}
QComboBox::drop-down {
	subcontrol-origin: padding;
	subcontrol-position: top right;
	width: 16px;
	border-left: 1px solid ${border};
}
QComboBox::down-arrow {
	width: 0;
	height: 0;
	border-left: 4px solid transparent;
	border-right: 4px solid transparent;
	border-top: 4px solid ${textSecondary};
}
QComboBox QAbstractItemView {
	background: ${contentBg};
	border: 1px solid ${border};
	selection-background-color: ${selectionBg};
	selection-color: ${textPrimary};
}

QMenu {
	background: ${contentBg};
	color: ${textPrimary};
	border: 1px solid ${border};
	border-radius: ${radius}px;
	padding: 4px 0px;
}
QMenu::item {
	padding: 4px 24px;
	background: transparent;
	border-radius: ${radius}px;
}
QMenu::item:selected {
	background-color: ${selectionBg};
}
QMenu::item:disabled {
	color: ${textDisabled};
}
QMenu::separator {
	height: 1px;
	background: ${border};
	margin: 4px 8px;
}

QMenuBar {
	background: ${windowBg};
	color: ${textPrimary};
	border-bottom: 1px solid ${border};
}
QMenuBar::item {
	background: transparent;
	color: ${textPrimary};
	padding: 4px 10px;
}
QMenuBar::item:selected {
	background: ${hoverBg};
}
QMenuBar::item:pressed {
	background: ${selectionBg};
	border: 1px solid ${border};
}

QTabWidget::pane {
	border: 1px solid ${border};
	border-radius: ${radius}px;
	background: ${contentBg};
}
QTabBar::tab {
	background: transparent;
	color: ${textSecondary};
	border: 1px solid transparent;
	border-bottom: 1px solid ${border};
	padding: 4px 12px;
	margin-right: 2px;
	/* tab 等宽: 短标签(如单字"槽")也撑到统一最小宽度 */
	min-width: 56px;
}
QTabBar::tab:hover {
	color: ${textPrimary};
	background: ${hoverBg};
}
QTabBar::tab:selected {
	background: ${contentBg};
	color: ${accent};
	border: 1px solid ${border};
	border-bottom: 2px solid ${accent};
}

QAbstractItemView {
	background: ${contentBg};
	color: ${textPrimary};
	border: 1px solid ${border};
	/* 视图获焦时的虚线框(PE_FrameFocusRect)由 outline 属性接管,
	   置 none 后不再回退基础样式的原生虚线 */
	outline: none;
	alternate-background-color: ${altRow};
	selection-background-color: ${selectionBg};
	selection-color: ${textPrimary};
}
QAbstractItemView::item {
	outline: none;
	border: none;
}
QAbstractItemView::item:hover {
	background-color: ${hoverBg};
}
/* 不写 color: 选中文字色由 QAbstractItemView 的 selection-color 负责;
   此处写 color 会顶掉 QTreeWidget::item 的 color:transparent, 造成
   树 item "样式绘制 + CTreeWidgetDelegate 自绘" 双重文字(重影) */
QAbstractItemView::item:selected {
	background-color: ${selectionBg};
}

QHeaderView {
	/* 表头视图本身是 QAbstractItemView, 会被上面 QAbstractItemView 的 1px 边框规则命中,
	   叠在外框内侧形成 2px 粗边; 分隔线由 QHeaderView::section 的 border-right/bottom 负责 */
	border: none;
}
QHeaderView::section {
	background-color: ${windowBg};
	color: ${textSecondary};
	border: none;
	border-right: 1px solid ${border};
	border-bottom: 1px solid ${border};
	padding: 4px 6px;
}
QHeaderView::section:hover {
	color: ${textPrimary};
}
QTableCornerButton::section {
	background-color: ${windowBg};
	border: none;
	/* 左上角与横/纵表头的交界线: 纵表头的 border-left 不存在, 由角按钮补右/下边 */
	border-right: 1px solid ${border};
	border-bottom: 1px solid ${border};
}

QTreeWidget {
	background: ${contentBg};
	border: 1px solid ${border};
	outline: 0px;
	show-decoration-selected: 1;
	selection-background-color: ${selectionBg};
	selection-color: ${textPrimary};
}
QTreeWidget::item {
	/* 文字由 CTreeWidgetDelegate 自绘, QSS 层保持透明避免双重绘制 */
	color: transparent;
	outline: 0px;
	height: 24px;
}
QTreeWidget::item:hover, QTreeWidget::branch:hover {
	background: ${hoverBg};
}
QTreeWidget::item:selected, QTreeWidget::branch:selected {
	background: ${selectionBg};
}
QTreeWidget::branch {
	padding-top: 2px;
}
QTreeWidget::branch:has-children:closed {
	image: url(:/icons/resource/tree_open.png);
}
QTreeWidget::branch:has-children:open {
	image: url(:/icons/resource/tree_close.png);
}

QTableWidget {
	background-color: ${contentBg};
	alternate-background-color: ${altRow};
	gridline-color: ${border};
	selection-background-color: ${selectionBg};
	selection-color: ${textPrimary};
	border: 1px solid ${border};
}
/* Tab pane 已提供 1px 外框, 内嵌表格去掉自身边框, 避免贴边叠成 2px 粗框 */
QTabWidget QTableWidget, QTabWidget QTableView, QTabWidget QTreeWidget {
	border: none;
}
QTableWidget::item {
	padding: 2px 4px;
	border: none;
	outline: none;
}
QTableWidget::item:hover {
	background-color: ${hoverBg};
}

QListView {
	background: ${contentBg};
	border: 1px solid ${border};
	outline: none;
}

QGroupBox {
	border: 1px solid ${border};
	border-radius: ${radius}px;
	margin-top: 10px;
	background: transparent;
}
QGroupBox::title {
	subcontrol-origin: margin;
	subcontrol-position: top left;
	left: 8px;
	padding: 0 3px;
	color: ${textSecondary};
}

QScrollBar:vertical {
	background: transparent;
	width: 8px;
	margin: 0px;
}
QScrollBar::handle:vertical {
	background: ${border};
	min-height: 20px;
	border-radius: 4px;
	margin: 2px;
}
QScrollBar::handle:vertical:hover {
	background: ${textDisabled};
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
	height: 0px;
}
QScrollBar:horizontal {
	background: transparent;
	height: 8px;
	margin: 0px;
}
QScrollBar::handle:horizontal {
	background: ${border};
	min-width: 20px;
	border-radius: 4px;
	margin: 2px;
}
QScrollBar::handle:horizontal:hover {
	background: ${textDisabled};
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
	width: 0px;
}

QToolTip {
	background: ${contentBg};
	color: ${textPrimary};
	border: 1px solid ${border};
	border-radius: ${radius}px;
	padding: 3px 6px;
}
)";

	for (const auto& token : arrColorTokens)
	{
		strQss.replace(QString("${%1}").arg(token.name), token.color.name());
	}
	for (const auto& token : arrSizeTokens)
	{
		strQss.replace(QString("${%1}").arg(token.name), QString::number(token.value));
	}

	return strQss;
}
