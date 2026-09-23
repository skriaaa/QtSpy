#pragma once

#include <QComboBox>
#include <QFont>
#include <QLineEdit>
#include <QMetaEnum>
#include <QMetaType>
#include <QPushButton>
#include <QString>
#include <QVariant>
#include <QWidget>

class QMetaProperty;
class QShowEvent;

// 参数/属性编辑公共设施: 按 (metatype + 枚举信息) 创建编辑器, 完成值与文本互转。
// 服务两处:
//   1. 属性面板(CPropertyItemDelegate): 编辑器嵌表格单元格, 值列以文本为载体(paramText 往返)
//   2. 发送信号弹窗: 逐形参编辑, 直接取 QVariant(paramValue) 后经 QGenericArgument invoke
// 类型覆盖: 标量/字符串/枚举/flags/几何(QPoint,QSize,QRect 系)/QColor/QFont/QCursor/
//           QSizePolicy/日期时间/QKeySequence/QUrl/QStringList/QByteArray/指针(仅发信号)
namespace ParamEditor
{
	// 参数类型描述(属性 / 信号形参通用)
	struct ParamType
	{
		int nTypeId = QMetaType::UnknownType;   // QVariant userType / QMetaType id
		QString strTypeName;                     // 规范化类型名(invoke 的 QGenericArgument 需与形参名一致)
		QMetaEnum metaEnum;                       // 枚举/flags 时有效
		bool bFlag = false;                      // QFlags(值可按 key|key 组合)
		bool bPointer = false;                   // 指针形参(QObject*/char* 等, 仅发信号场景编辑)
	};

	// 属性 -> ParamType(含枚举/flags 判定)
	ParamType fromProperty(const QMetaProperty& metaProperty);
	// 该类型是否有编辑器; bAllowPointer 控制指针类型(属性面板 false, 发信号 true)
	bool isEditable(const ParamType& type, bool bAllowPointer = false);

	// 值 -> 文本(值列显示 / 编辑器内容; 支持类型给出精确可逆格式)
	QString valueToText(const ParamType& type, const QVariant& value);
	// 文本 -> 值(编辑提交时解析; 失败返回无效 QVariant)
	QVariant textToValue(const ParamType& type, const QString& strText);

	// 编辑器创建与取值: 编辑器统一实现 paramText/paramValue 双属性
	QWidget* createEditor(const ParamType& type, QWidget* pParent);
	void setEditorValue(QWidget* pEditor, const QVariant& value);
	QVariant editorValue(QWidget* pEditor);

	// ---- 编辑器实现(头文件声明供 AUTOMOC, 外部只经 createEditor 使用) ----

	// 文本型: 整数/浮点/字符串/QChar/QByteArray/QDate/QTime/QKeySequence/QUrl/
	//          flags("A|B" 或数值)/指针(十六进制地址)
	class CParamLineEdit : public QLineEdit
	{
		Q_OBJECT
		Q_PROPERTY(QString paramText READ text WRITE setText)
		Q_PROPERTY(QVariant paramValue READ paramValue WRITE setParamValue)
	public:
		explicit CParamLineEdit(const ParamType& type, QWidget* pParent = nullptr);
	private:
		QVariant paramValue() const;
		void setParamValue(const QVariant& value);
		ParamType m_type;
	};

	// 复合值弹窗型: 按钮弹出逐字段表单(字段名标注) ——
	// QPoint(F)/QSize(F)/QRect(F)/QSizePolicy/QDateTime/QStringList
	// 属性面板场景(带 "qtspyAutoOpenDialog" 属性)双击单元格直接弹窗, 免去再点一次按钮
	class CParamStructButton : public QPushButton
	{
		Q_OBJECT
		Q_PROPERTY(QString paramText READ paramText WRITE setParamText)
		Q_PROPERTY(QVariant paramValue READ paramValue WRITE setParamValue)
	public:
		explicit CParamStructButton(const ParamType& type, QWidget* pParent = nullptr);
	protected:
		void showEvent(QShowEvent* event) override;
	private:
		QString paramText() const;
		void setParamText(const QString& strText);
		QVariant paramValue() const;
		void setParamValue(const QVariant& value);
		void openDialog();
		ParamType m_type;
		QVariant m_value;
		QString m_strRawText;        // 文本解析失败时保留原文(显示/提交不丢数据)
		bool m_bAutoOpenDone = false;
	};

	// 下拉型: bool / 枚举(key 列表, userData 存数值)
	class CParamComboBox : public QComboBox
	{
		Q_OBJECT
		Q_PROPERTY(QString paramText READ currentText WRITE setCurrentText)
		Q_PROPERTY(QVariant paramValue READ paramValue WRITE setParamValue)
	public:
		// bBoolMode: false/true 两项; 否则按 metaEnum 的 key 填充
		CParamComboBox(const ParamType& type, bool bBoolMode, QWidget* pParent = nullptr);
	private:
		QVariant paramValue() const;
		void setParamValue(const QVariant& value);
		bool m_bBoolMode = false;
	};

	// 颜色: 十六进制文本 + 选色按钮
	class CParamColorEditor : public QWidget
	{
		Q_OBJECT
		Q_PROPERTY(QString paramText READ paramText WRITE setParamText)
		Q_PROPERTY(QVariant paramValue READ paramValue WRITE setParamValue)
	public:
		explicit CParamColorEditor(QWidget* pParent = nullptr);
	private:
		QString paramText() const;
		void setParamText(const QString& strText);
		QVariant paramValue() const;
		void setParamValue(const QVariant& value);
		QLineEdit* m_pTextEdit = nullptr;
	};

	// 光标形状: 下拉(Qt::CursorShape 的 22 种形状)
	class CParamCursorCombo : public QComboBox
	{
		Q_OBJECT
		Q_PROPERTY(QString paramText READ paramText WRITE setParamText)
		Q_PROPERTY(QVariant paramValue READ paramValue WRITE setParamValue)
	public:
		explicit CParamCursorCombo(QWidget* pParent = nullptr);
	private:
		QString paramText() const;
		void setParamText(const QString& strText);
		QVariant paramValue() const;
		void setParamValue(const QVariant& value);
	};

	// 字体: 选择按钮(文本即紧凑描述 "family,pt[,bold][,italic]")
	// 属性面板场景(带 "qtspyAutoOpenDialog" 属性)双击单元格直接弹字体对话框
	class CParamFontButton : public QPushButton
	{
		Q_OBJECT
		Q_PROPERTY(QString paramText READ paramText WRITE setParamText)
		Q_PROPERTY(QVariant paramValue READ paramValue WRITE setParamValue)
	public:
		explicit CParamFontButton(QWidget* pParent = nullptr);
	protected:
		void showEvent(QShowEvent* event) override;
	private:
		QString paramText() const;
		void setParamText(const QString& strText);
		QVariant paramValue() const;
		void setParamValue(const QVariant& value);
		void openDialog();
		QFont m_font;
		bool m_bAutoOpenDone = false;
	};
}
