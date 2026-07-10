#include "ConnectionInfo.h"
#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>
#include <QMetaMethod>

CConnectionAanlyzer::CConnectionAanlyzer(QObject* object)
{
    m_pTargetObject = object;
    analyzeInBoundConnections(object);
    analyzeOutBoundConnections(object);
}

void CConnectionAanlyzer::analyzeInBoundConnections(QObject* object)
{
    QObjectPrivate* d = QObjectPrivate::get(object);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QObjectPrivate::ConnectionData* cd = d->connections.loadRelaxed();
    if (cd && cd->senders) {
        auto* senders = cd->senders;
#else
    if (d->senders) {
        auto* senders = d->senders;
#endif
        for (QObjectPrivate::Connection* s = senders; s; s = s->next) {
            if (!s->sender)
                continue;

            Connection conn;
            conn.endpoint = s->sender;
            conn.signalIndex = QMetaObjectPrivate::signal(s->sender->metaObject(), s->signal_index).methodIndex();
            if (s->isSlotObject) {
                conn.slotIndex = -1;
            }
            else {
                conn.slotIndex = s->method();
            }
            conn.type = s->connectionType;
            m_arrInBoundConnections.push_back(conn);
        }
    }
}

void CConnectionAanlyzer::analyzeOutBoundConnections(QObject* object)
{
    QObjectPrivate* d = QObjectPrivate::get(object);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QObjectPrivate::ConnectionData* cd = d->connections.loadRelaxed();
    if (!cd)
        return;

    auto cl = cd->signalVector.loadRelaxed();
#else
    if (!d->connectionLists)
        return;

    // HACK: the declaration of d->connectionsLists is not accessible for us...
    const auto cl = reinterpret_cast<QVector<QObjectPrivate::ConnectionList>*>(d->connectionLists);
#endif
    if (!cl)
        return;

    for (int signalIndex = 0; signalIndex < cl->count(); ++signalIndex) {
        const QObjectPrivate::Connection* c = cl->at(signalIndex).first;
        while (c) {
            if (!c->receiver) {
                c = c->nextConnectionList;
                continue;
            }

            Connection conn;
            conn.endpoint = c->receiver;
            conn.signalIndex = QMetaObjectPrivate::signal(object->metaObject(), signalIndex).methodIndex();
            if (c->isSlotObject)
                conn.slotIndex = -1;
            else
                conn.slotIndex = c->method();
            conn.type = c->connectionType;
            c = c->nextConnectionList;
            m_arrOutBoundConnections.push_back(conn);
        }
    }
}

QVector<QMetaMethod> CConnectionAanlyzer::objectSignals()
{
    QVector<QMetaMethod> arrSignals;
    const QMetaObject* metaObject = m_pTargetObject->metaObject();
    while (metaObject)
    {
        for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
            QMetaMethod&& method = metaObject->method(i);
            if (method.methodType() == QMetaMethod::Signal)
            {
                arrSignals.append(method);
            }
        }
        metaObject = metaObject->superClass();
    }
	return arrSignals;
}

QVector<QMetaMethod> CConnectionAanlyzer::objectSlots()
{
    QVector<QMetaMethod> arrSlots;
    const QMetaObject* metaObject = m_pTargetObject->metaObject();
    while (metaObject)
    {
        for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
            QMetaMethod&& method = metaObject->method(i);
            if (method.methodType() == QMetaMethod::Slot)
            {
                arrSlots.append(method);
            }
        }
        metaObject = metaObject->superClass();
    }
    return arrSlots;
}

QVector<ConnectionInfo> CConnectionAanlyzer::inBoundConnections()
{
	QVector<ConnectionInfo> arrConnections;
    for(auto connection : m_arrInBoundConnections)
    {
        ConnectionInfo info;
		info.pReceiver = m_pTargetObject;
        info.pSender = connection.endpoint;
        info.strSignal = connection.signalIndex != -1 ? connection.endpoint->metaObject()->method(connection.signalIndex).methodSignature() : QString();
        // isSlotObject(lambda/functor/函数指针) 时 slotIndex 为 -1, 这里给个可读标签, 否则槽列整列空白看着像漏取
        info.strSlot = connection.slotIndex != -1 ? m_pTargetObject->metaObject()->method(connection.slotIndex).methodSignature() : QStringLiteral("<functor>");
        switch (connection.type) {
        case Qt::AutoConnection:
            // Auto 在 emit 时按 sender/receiver 线程亲和性解析为 Direct 或 Queued, 这里预先解析
            info.strConnectType = (info.pSender && info.pReceiver && info.pSender->thread() == info.pReceiver->thread())
                ? "Auto(Direct)" : "Auto(Queued)";
            break;
        case Qt::DirectConnection:
            info.strConnectType = "DirectConnection";
            break;
        case Qt::QueuedConnection:
            info.strConnectType = "QueuedConnection";
            break;
        case Qt::BlockingQueuedConnection:
            info.strConnectType = "BlockingQueuedConnection";
            break;
        case Qt::UniqueConnection:
            info.strConnectType = "UniqueConnection";
            break;
        default:
            info.strConnectType = "Unknown";
            break;
        }
        arrConnections.append(info);
	}
	return arrConnections;
}

QVector<ConnectionInfo> CConnectionAanlyzer::outBoundConnections()
{
    QVector<ConnectionInfo> arrConnections;
    for (auto connection : m_arrOutBoundConnections)
    {
        ConnectionInfo info;
        info.pSender = m_pTargetObject;
        info.pReceiver = connection.endpoint;
        info.strSlot = connection.slotIndex != -1 ? connection.endpoint->metaObject()->method(connection.slotIndex).methodSignature() : QStringLiteral("<functor>");
        info.strSignal = connection.signalIndex != -1 ? m_pTargetObject->metaObject()->method(connection.signalIndex).methodSignature() : QString();
        switch (connection.type) {
        case Qt::AutoConnection:
            // Auto 在 emit 时按 sender/receiver 线程亲和性解析为 Direct 或 Queued, 这里预先解析
            info.strConnectType = (info.pSender && info.pReceiver && info.pSender->thread() == info.pReceiver->thread())
                ? "Auto(Direct)" : "Auto(Queued)";
            break;
        case Qt::DirectConnection:
            info.strConnectType = "Direct";
            break;
        case Qt::QueuedConnection:
            info.strConnectType = "Queued";
            break;
        case Qt::BlockingQueuedConnection:
            info.strConnectType = "Block";
            break;
        case Qt::UniqueConnection:
            info.strConnectType = "Unique";
            break;
        default:
            info.strConnectType = "Unknown";
            break;
        }
        arrConnections.append(info);
    }
    return arrConnections;
}
