#pragma once

#include "qt_spydlg.h"
#include "ParamEditor.h"

#include <QHash>
#include <QMetaEnum>
#include <QMetaType>
#include <QPointer>
#include <QStyledItemDelegate>
#include <QVector>
#include <functional>

class QCheckBox;
class QLineEdit;
class QTableWidget;
class QTableWidgetItem;
class QTimer;

struct PROPERTY_ITEM_INFO
{
	int nPropertyIndex = -1;
	int nTypeId = QMetaType::UnknownType;
	QString strPropertyName;
	QMetaEnum metaEnum;
	bool bReadable = false;
	bool bWritable = false;
	bool bEditable = false;
	bool bFlag = false;
};

class CPropertyItemDelegate : public QStyledItemDelegate
{
public:
	using ParamTypeResolver = std::function<ParamEditor::ParamType(int)>;

	explicit CPropertyItemDelegate(QObject* pParent = nullptr);

	// 行号 -> 参数类型描述(属性面板按 m_vectorPropertyItems 提供)
	void setParamTypeResolver(const ParamTypeResolver& fnResolver);

	QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	void updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	void setEditorData(QWidget* pEditor, const QModelIndex& index) const override;
	void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;

private:
	ParamTypeResolver m_fnResolveParamType;
};

class CPropertyInspectorDlg : public CXDialog
{
public:
	explicit CPropertyInspectorDlg(QWidget* pParent = nullptr);

	bool setTargetObject(QObject* pTargetObject);

private:
	enum PROPERTY_COLUMN
	{
		PROPERTY_COLUMN_NAME = 0,
		PROPERTY_COLUMN_TYPE,
		PROPERTY_COLUMN_VALUE,
		PROPERTY_COLUMN_PERMISSION,
		PROPERTY_COLUMN_OWNER,
		PROPERTY_COLUMN_COUNT
	};

	void initWidgets();
	void buildPropertyRows();
	void refreshProperties(bool bHighlightChanges);
	void refreshBaseInfo();
	void refreshStatus();
	void refreshTargetCaption();
	void filterProperties(const QString& strKeyword);
	void applyPropertyEdit(QTableWidgetItem* pItem);
	void showPropertyContextMenu(const QPoint& ptPosition);
	void handleTargetDestroyed();
	bool isEditingPropertyRow(int nRow) const;

private:
	QPointer<QObject> m_pTargetObject;
	QLineEdit* m_pSearchEdit = nullptr;
	QCheckBox* m_pAutoRefreshCheckBox = nullptr;
	QTableWidget* m_pPropertyTable = nullptr;
	QTableWidget* m_pBaseInfoTable = nullptr;
	QTableWidget* m_pStatusTable = nullptr;
	QTimer* m_pRefreshTimer = nullptr;
	QVector<PROPERTY_ITEM_INFO> m_vectorPropertyItems;
	QHash<int, QString> m_mapPropertyValues;
	QHash<int, int> m_mapHighlightTicks;
	bool m_bUpdating = false;
};
