#include "ConnectionInfo.h"
#include "utils/SymbolResolver.h"
#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>
#include <QMetaMethod>
#include <QRegularExpression>
#include <QFile>
#include <cstring>

namespace
{
	// QSlotObjectBase 无虚函数(qobjectdefs_impl.h), 布局固定:
	//   [0] QAtomicInt m_ref(4)
	//   [impl偏移] ImplFn m_impl —— 指向 QSlotObject<Func>::impl(PMF) 或
	//                              QFunctorSlotObject<Func>::impl(lambda/仿函数) 的实例化代码
	//   其后紧接派生类的 Func function —— PMF 或 lambda 闭包对象
	// impl 偏移随指针宽度对齐: x86=4, x64=8(ref 后补齐)
	constexpr int kPtrSize = sizeof(void*);
	constexpr int kSlotImplOffset = (8 == kPtrSize) ? 8 : 4;
	constexpr int kSlotFuncOffset = kSlotImplOffset + kPtrSize;

	// isSlotObject 连接(新式 connect: PMF/lambda/仿函数)没有 method index,
	// 从 slotObj 内部结构 + dbghelp 符号还原可读的槽描述:
	//   - PMF:       slotObj 后段是成员函数指针, 符号化即 "Class::method"
	//   - 虚 PMF:    指针落在 vcall thunk 上拿不到方法名, 退回 impl 符号里的 PMF 签名
	//   - lambda:    impl 符号名带 <lambda_hash>, 反查同 hash 的 operator() 得到定义处 file:line
	//   - 仿函数:    impl 符号的模板首参即类型名
	// strSourceOut: lambda 时接收从定义处文件截取的完整源码文本

	// 快速校验字节流是否为合法 UTF-8(无 BOM 时区分 UTF-8 与本地 ANSI/GBK)
	bool isValidUtf8(const QByteArray& bytes)
	{
		int i = 0;
		const int n = bytes.size();
		while (i < n)
		{
			const unsigned char c = (unsigned char)bytes.at(i);
			if (c < 0x80)
			{
				++i;
				continue;
			}
			int extra = 0;
			if ((c & 0xE0) == 0xC0)
				extra = 1;
			else if ((c & 0xF0) == 0xE0)
				extra = 2;
			else if ((c & 0xF8) == 0xF0)
				extra = 3;
			else
				return false;
			if (i + extra >= n)
				return false;
			for (int k = 1; k <= extra; ++k)
			{
				if (((unsigned char)bytes.at(i + k) & 0xC0) != 0x80)
					return false;
			}
			i += extra + 1;
		}
		return true;
	}

	// 源码文件全文缓存(连接表会反复刷新, 同一文件只读一次)
	QHash<QString, QString> g_sourceFileCache;
	QString readSourceFile(const QString& strFile)
	{
		const auto it = g_sourceFileCache.constFind(strFile);
		if (it != g_sourceFileCache.constEnd())
			return it.value();
		QString text;
		QFile file(strFile);
		if (file.open(QIODevice::ReadOnly))
		{
			const QByteArray raw = file.readAll();
			if (raw.startsWith("\xEF\xBB\xBF"))
				text = QString::fromUtf8(raw.constData() + 3, raw.size() - 3);
			else if (raw.startsWith("\xFF\xFE"))
				text = QString::fromUtf16(reinterpret_cast<const ushort*>(raw.constData() + 2), (raw.size() - 2) / 2);
			else if (isValidUtf8(raw))
				text = QString::fromUtf8(raw);
			else
				text = QString::fromLocal8Bit(raw);
			// 缓存也兜住"读失败"(空串), 避免每次刷新都重试打开
			g_sourceFileCache[strFile] = text;
		}
		return text;
	}

	// 从 nBraceOpen 的 '{' 起做括号配对(跳过字符串/字符字面量与注释), 返回配对 '}' 的下标; 失败 -1
	int matchBrace(const QString& text, int nBraceOpen)
	{
		int depth = 0;
		const int n = text.size();
		for (int i = nBraceOpen; i < n; ++i)
		{
			const QChar ch = text.at(i);
			if (QLatin1Char('/') == ch && i + 1 < n)
			{
				if (QLatin1Char('/') == text.at(i + 1))
				{
					const int nl = text.indexOf(QChar('\n'), i);
					if (nl < 0)
						break;
					i = nl;  // for 的 ++ 再前进一格
					continue;
				}
				if (QLatin1Char('*') == text.at(i + 1))
				{
					const int end = text.indexOf(QLatin1String("*/"), i + 2);
					if (end < 0)
						break;
					i = end + 1;
					continue;
				}
			}
			if (QLatin1Char('"') == ch || QLatin1Char('\'') == ch)
			{
				const QChar quote = ch;
				for (++i; i < n; ++i)
				{
					if (QLatin1Char('\\') == text.at(i))
					{
						++i;
						continue;
					}
					if (quote == text.at(i))
						break;
				}
				continue;
			}
			if (QLatin1Char('{') == ch)
			{
				++depth;
			}
			else if (QLatin1Char('}') == ch)
			{
				--depth;
				if (0 == depth)
					return i;
			}
		}
		return -1;
	}

	// 读 strFile, 截取 nLine(1 基)处 lambda 的完整文本: 从捕获列表 '[' 到配对 '}'。
	// operator() 的行号通常落在 lambda 起始行, 向前找 '{'、再回溯 ']' ']' 的 '[';
	// 定位失败时退回该行前后几行原文。文件不存在/越界返回空
	QString lambdaSourceAt(const QString& strFile, int nLine)
	{
		const QString text = readSourceFile(strFile);
		if (text.isEmpty() || nLine < 1)
			return QString();
		// 行号 -> 该行起始偏移(超出总行数则放弃)
		int nLineStart = 0;
		for (int i = 1; i < nLine; ++i)
		{
			const int nl = text.indexOf(QChar('\n'), nLineStart);
			if (nl < 0)
				return QString();
			nLineStart = nl + 1;
		}

		// 1) 向前找 lambda 体 '{'(500 字内; 先撞到 ';' 说明 '{' 在行号之前, 改回溯找)
		int nBrace = -1;
		const int nFwdLimit = qMin(text.size(), nLineStart + 500);
		for (int i = nLineStart; i < nFwdLimit; ++i)
		{
			const QChar ch = text.at(i);
			if (QLatin1Char('{') == ch)
			{
				nBrace = i;
				break;
			}
			if (QLatin1Char(';') == ch)
				break;
		}
		if (nBrace < 0)
		{
			const int nBackLimit = qMax(0, nLineStart - 500);
			for (int i = nLineStart - 1; nBackLimit <= i; --i)
			{
				if (QLatin1Char('{') == text.at(i))
				{
					nBrace = i;
					break;
				}
			}
		}
		if (nBrace < 0)
			return text.mid(nLineStart).section(QChar('\n'), 0, 4).trimmed();

		// 2) 回溯捕获列表起始 '[': 先找 '{' 前最近的 ']', 再向前括号配对出 '['; 失败退到 '{' 所在行行首
		int nStart = -1;
		for (int i = nBrace - 1; qMax(0, nBrace - 300) <= i; --i)
		{
			if (QLatin1Char(']') == text.at(i))
			{
				int depth = 0;
				for (int j = i; 0 <= j; --j)
				{
					const QChar ch = text.at(j);
					if (QLatin1Char(']') == ch)
						++depth;
					else if (QLatin1Char('[') == ch)
					{
						--depth;
						if (0 == depth)
						{
							nStart = j;
							break;
						}
					}
				}
				break;
			}
		}
		if (nStart < 0)
		{
			nStart = text.lastIndexOf(QChar('\n'), nBrace);
			nStart = (nStart < 0) ? 0 : nStart + 1;
		}

		// 3) 截取 [capture](params) { ... } 到配对 '}'
		const int nEnd = matchBrace(text, nBrace);
		if (nEnd < 0)
			return text.mid(nStart).section(QChar('\n'), 0, 9).trimmed();
		QString snippet = text.mid(nStart, nEnd - nStart + 1).trimmed();
		if (4000 < snippet.size())
		{
			snippet.truncate(4000);
			snippet += QStringLiteral("\n... (过长截断)");
		}
		return snippet;
	}

	QString describeSlotObject(QtPrivate::QSlotObjectBase* slotObj, QString& strSourceOut)
	{
		if (nullptr == slotObj || !SymbolResolver::ready())
		{
			return QStringLiteral("<functor>");
		}
		const auto* pRaw = reinterpret_cast<const char*>(slotObj);
		const quintptr nImpl = *reinterpret_cast<const quintptr*>(pRaw + kSlotImplOffset);
		const QString strImpl = SymbolResolver::symbolName(nImpl);

		if (strImpl.contains(QLatin1String("QFunctorSlotObject")))
		{
			static const QRegularExpression reHash(QStringLiteral("<lambda_[0-9a-f]+>"));
			const auto match = reHash.match(strImpl);
			if (match.hasMatch())
			{
				const QString strFileLine = SymbolResolver::lambdaOperatorFileLine(nImpl, match.captured(0));
				if (strFileLine.isEmpty())
					return QStringLiteral("<lambda>");
				// "路径:行" 拆回 file + line, 顺手截取 lambda 源码(路径失效则源码为空, 只显示定位)
				const int nColon = strFileLine.lastIndexOf(QLatin1Char(':'));
				strSourceOut = lambdaSourceAt(strFileLine.left(nColon), strFileLine.midRef(nColon + 1).toInt());
				return QStringLiteral("<lambda> ") + strFileLine;
			}
			// 非 lambda 仿函数: 模板首参就是仿函数类型名
			const int nFrom = strImpl.indexOf(QLatin1String("QFunctorSlotObject<")) + int(strlen("QFunctorSlotObject<"));
			const int nTo = strImpl.indexOf(QLatin1Char(','), nFrom);
			const QString strType = (nTo > nFrom) ? strImpl.mid(nFrom, nTo - nFrom) : QString();
			return strType.isEmpty() ? QStringLiteral("<functor>") : QStringLiteral("<functor> ") + strType;
		}

		if (strImpl.contains(QLatin1String("QSlotObject")))
		{
			// PMF 首 sizeof(void*) 字节即函数地址(单继承直指函数, 多继承/虚槽指向 thunk)
			const quintptr nFunc = *reinterpret_cast<const quintptr*>(pRaw + kSlotFuncOffset);
			const QString strFunc = SymbolResolver::symbolName(nFunc);
			if (strFunc.contains(QLatin1String("`vcall'")))
			{
				// 虚槽: vcall thunk 只能给出类名, 从 impl 符号的 PMF 类型还原 "Class::(参数)"
				static const QRegularExpression rePmf(QStringLiteral("\\((?:__thiscall\\s+)?([^(:]+)::\\*\\)\\(([^)]*)\\)"));
				const auto match = rePmf.match(strImpl);
				if (match.hasMatch())
				{
					return QStringLiteral("%1::(virtual)(%2)").arg(match.captured(1).trimmed(), match.captured(2).trimmed());
				}
				return QStringLiteral("<virtual slot>");
			}
			if (!strFunc.isEmpty())
			{
				return strFunc;
			}
		}
		return QStringLiteral("<functor>");
	}
}

CConnectionAanlyzer::CConnectionAanlyzer(QObject* object)
{
    m_pTargetObject = object;
	// 连接分析依赖 dbghelp 符号(槽列的 lambda/PMF 还原), 幂等触发后台加载
	SymbolResolver::ensureAsync();
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
                conn.strSlotDesc = describeSlotObject(s->slotObj, conn.strSlotSource);
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
            if (c->isSlotObject) {
                conn.slotIndex = -1;
                conn.strSlotDesc = describeSlotObject(c->slotObj, conn.strSlotSource);
            }
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
        // isSlotObject(lambda/functor/函数指针) 时无 method index, 用符号还原的描述占位,
        // 符号不可用时退回 <functor> 标签(槽列整列空白看着像漏取)
        info.strSlot = connection.slotIndex != -1 ? m_pTargetObject->metaObject()->method(connection.slotIndex).methodSignature()
            : (connection.strSlotDesc.isEmpty() ? QStringLiteral("<functor>") : connection.strSlotDesc);
        info.strSlotSource = connection.strSlotSource;
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
        info.strSlot = connection.slotIndex != -1 ? connection.endpoint->metaObject()->method(connection.slotIndex).methodSignature()
            : (connection.strSlotDesc.isEmpty() ? QStringLiteral("<functor>") : connection.strSlotDesc);
        info.strSlotSource = connection.strSlotSource;
        info.strSignal = connection.signalIndex != -1 ? m_pTargetObject->metaObject()->method(connection.signalIndex).methodSignature() : QString();
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
