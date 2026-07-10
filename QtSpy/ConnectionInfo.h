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
};

class CConnectionAanlyzer
{
    struct Connection {
        QPointer<QObject> endpoint;
        int signalIndex;
        int slotIndex;
        int type;
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

