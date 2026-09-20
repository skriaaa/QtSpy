#pragma once
#include <QColor>
#include <QString>

class QWidget;

// QtSpy 界面调色板 —— 全部自绘/样式颜色的唯一来源
// 浅色 IDE 工具风; 将来深色主题 = 换一份 palette 值
struct SpyPalette
{
	// 颜色 token
	QColor windowBg        { 0xF5, 0xF6, 0xF8 };  // 对话框/面板底
	QColor contentBg       { 0xFF, 0xFF, 0xFF };  // 表格/树/输入框
	QColor altRow          { 0xF7, 0xF8, 0xFA };  // 表格交替行
	QColor border          { 0xD9, 0xDC, 0xE1 };  // 边框/分隔线
	QColor accent          { 0x2E, 0x7C, 0xF6 };  // 强调色(选中/焦点/曲线)
	QColor accentHover     { 0x4A, 0x90, 0xF9 };
	QColor accentPressed   { 0x1F, 0x5F, 0xD0 };
	QColor selectionBg     { 0xE8, 0xF1, 0xFE };  // 选中底(文字保持主色)
	QColor selectionText   { 0xFF, 0xFF, 0xFF };  // 强调底色上的文字(输入框选中文字等)
	QColor hoverBg         { 0xED, 0xF0, 0xF3 };  // 悬停底
	QColor textPrimary     { 0x1F, 0x23, 0x29 };
	QColor textSecondary   { 0x64, 0x6A, 0x73 };
	QColor textDisabled    { 0xA9, 0xAE, 0xB8 };
	QColor danger          { 0xE5, 0x45, 0x45 };  // 危险操作
	QColor highlightChange { 0xFF, 0xF4, 0xB4 };  // 属性面板变更高亮
	QColor spyHighlight    { 0x00, 0xFF, 0xE1 };  // 捕捉/选中目标的高亮指示(青)

	// 密度 token
	int controlHeight = 24;   // 按钮高度(px), 与输入框同高
	int inputHeight   = 24;   // 输入控件高度(px)
	int radius        = 3;    // 圆角(px)
};

namespace QtSpyTheme
{
	// 当前调色板(自绘代码取色入口)
	const SpyPalette& palette();

	// 生成完整 QSS 文本(模板 + palette token 替换)
	QString qss();

	// 挂载主题到 QtSpy 自有顶层窗口(QSS), 幂等。
	// 字体由 QSS 的 font-family/font-size 规则承担(QStyleSheetStyle 自动改写/维持
	// widget 字体, 目标程序改全局字体时同样会被 QSS 重新断言)。
	// !!! apply 内严禁 setFont —— 会与 QSS 字体机制打架, 造成 FontChange 无限递归
	// !!! 只允许挂 QtSpy 自有 widget 子树, 严禁 qApp->setStyleSheet/setStyle/setFont
	// !!! (QtSpy 注入在目标进程内, qApp 级设置会泄漏到目标程序 UI)
	void apply(QWidget* pWnd);
}
