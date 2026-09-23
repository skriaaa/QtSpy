#pragma once
#include <QPointer>
class QMetaMethod;
struct ConnectionInfo
{
    QObject* pSender;
    QObject* pReceiver;
    QString strSignal;
    QString strSlot;
    QString strConnectType;
    // lambda 槽从定义处源码文件截取的完整文本, 供连接表 tooltip 展示(空=无/提取失败)
    QString strSlotSource;
};

class CConnectionAanlyzer
{
    struct Connection {
        QPointer<QObject> endpoint;
        int signalIndex;
        int slotIndex;
        int type;
        // isSlotObject 连接(lambda/PMF/仿函数)的槽列描述, dbghelp 符号还原;
        // analyze 时立即解析(slotObj 生命周期不保证到查询时, 不能存指针)
        QString strSlotDesc;
        // lambda 槽的源码文本(从 strSlotDesc 指向的定义处文件截取), 透传给连接表 tooltip
        QString strSlotSource;
    };
public:
	CConnectionAanlyzer(QObject* object);

    QVector<QMetaMethod> objectSignals();
    QVector<QMetaMethod> objectSlots();
    QVector<ConnectionInfo> inBoundConnections();
    QVector<ConnectionInfo> outBoundConnections();
private:
    void analyzeInBoundConnections(QObject* object);
	void analyzeOutBoundConnections(QObject* object);

	QObject* m_pTargetObject;
	QVector<Connection> m_arrInBoundConnections;
    QVector<Connection> m_arrOutBoundConnections;
};

