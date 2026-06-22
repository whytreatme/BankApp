#include "tcpreactor.h"
#include "protocolutils.h"
#include <QDebug>
#include <QDataStream>
#include "worktask.h"
#include <QThreadPool>

TcpReactor::TcpReactor(quint16 port, QObject* parent)
    : QObject(parent)
    , m_port(port)
    , m_server(nullptr)
{
    m_server = new QTcpServer(this);

    QThreadPool::globalInstance()->setMaxThreadCount(50); // 限制为 50 个，让任务排队，而不是疯狂开连接

    // 连接信号槽
    connect(m_server, &QTcpServer::newConnection,
            this, &TcpReactor::onNewConnection);
}

bool TcpReactor::start()
{
    if (!m_server->listen(QHostAddress::Any, m_port)) {
        qCritical() << "Failed to start server:" << m_server->errorString();
        return false;
    }

    qDebug() << "Server started successfully, listening on port" << m_port;
    return true;
}

void TcpReactor::stop()
{
    if (m_server->isListening()) {
        // 断开所有客户端连接
        for (QTcpSocket* socket : m_buffers.keys()) {
            socket->disconnectFromHost();
        }

        m_server->close();
        qDebug() << "Server stopped";
    }
}

void TcpReactor::onNewConnection()
{
    QTcpSocket* clientSocket = m_server->nextPendingConnection();
    if (!clientSocket) {
        return;
    }

    // 初始化该 Socket 的状态
    m_buffers[clientSocket] = QByteArray();
    m_authStatus[clientSocket] = false;
    m_socketToUserId[clientSocket] = "";
    m_socketIsAdmin[clientSocket] = false;

    qDebug() << "New client connected:" << clientSocket->peerAddress().toString()
             << "port" << clientSocket->peerPort();

    // 连接信号槽
    connect(clientSocket, &QTcpSocket::readyRead,
            this, &TcpReactor::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected,
            this, &TcpReactor::onDisconnected);
    connect(clientSocket, &QTcpSocket::errorOccurred,
            this, &TcpReactor::onSocketError);
}

void TcpReactor::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    // 累积数据到缓冲区
    m_buffers[socket] += socket->readAll();

    quint8 type;
    QString token;
    QJsonObject req;

    // 循环解析完整消息（处理粘包）
    while (ProtocolUtils::parseMessage(m_buffers[socket], type, token, req)) {
        qDebug() << "Received message - Type:" << type
                 << "from" << socket->peerAddress().toString();

        QJsonObject res;

        // 压力测试：tokenUserId 和 tokenIsAdmin 移到此處
        QString tokenDbId;
        bool tokenIsAdmin = false;

        // ==================== 认证中间件 ====================
        // type=1 (注册) 和 type=2 (登录) 不需要认证
        if (type != 1 && type != 2) {
            // 检查认证状态
            // if (!m_authStatus.value(socket, false)) {
            //     qWarning() << "Unauthorized request from" << socket->peerAddress().toString();
            //     res = {{"status", "error"}, {"msg", "未认证，请先登录"}};
            //     sendResponse(socket, res);
            //     continue;
            // }

            // 验证 Token 签名和角色
            if (!ProtocolUtils::verifyToken(token, tokenDbId, &tokenIsAdmin)) {
                qWarning() << "Invalid token from" << socket->peerAddress().toString();
                res = {{"status", "error"}, {"msg", "Token 无效"}};
                sendResponse(socket, res);
                continue;
            }

            // 验证 Token 中的 tokenDbId 是否与绑定的 dbId 一致
            // if (tokenDbId != m_socketToUserId[socket]) {
            //     qWarning() << "Token mismatch for" << socket->peerAddress().toString();
            //     res = {{"status", "error"}, {"msg", "Token 与用户不匹配"}};
            //     sendResponse(socket, res);
            //     continue;
            // }

            // 管理员权限检查（type 6-12, 14 需要管理员权限，type 13是用户修改密码）
            if ((type >= 6 && type <= 12) || type == 14) {
                if (!tokenIsAdmin || !m_socketIsAdmin[socket]) {
                    qWarning() << "Unauthorized admin request from user" << tokenDbId;
                    res = {{"status", "error"}, {"msg", "需要管理员权限"}};
                    sendResponse(socket, res);
                    continue;
                }
            }
        }

        // ==================== 路由分发 ====================
        // 壓力測試：userId 改為 tokenUserId
        QString dbId = tokenDbId;
        WorkTask *task = new WorkTask(socket, type, std::move(token), std::move(req), &m_userCtrl, &m_accountCtrl,
                 &m_txnCtrl, &m_adminCtrl, std::move(dbId));
       
        connect(task, &WorkTask::taskFinished, this, &TcpReactor::sendResponse, Qt::QueuedConnection);
        connect(task, &WorkTask::AuthSuccess, this, &TcpReactor::handleAuthSuccess, Qt::QueuedConnection);
        //QThreadPool::globalInstance()->start(task);
       task->run();
       
    }
}

void TcpReactor::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    qDebug() << "Client disconnected:" << socket->peerAddress().toString();

    // 清理该 Socket 的所有状态
    m_buffers.remove(socket);
    m_authStatus.remove(socket);
    m_socketToUserId.remove(socket);
    m_socketIsAdmin.remove(socket);

    socket->deleteLater();
}

void TcpReactor::onSocketError(QAbstractSocket::SocketError socketError)
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    qDebug() << "Socket error for" << socket->peerAddress().toString()
               << ":" << socket->errorString();
}

void TcpReactor::sendResponse(QPointer<QTcpSocket> socket, const QJsonObject& res)
{
    //WorkTask* task = qobject_cast<WorkTask*>(sender());
    //if (task) {
    //    task->deleteLater(); 
    //}

    
    if (!socket) return; //  如果 socket 已经死了，直接返回，不操作

    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Cannot send response: socket is not connected";
        return;
    }

    // 打包响应消息（type=0 表示响应）
    QByteArray packet = ProtocolUtils::packMessage(0, "", res);

    // 发送数据
    qint64 written = socket->write(packet);
    if (written != packet.size()) {
        qWarning() << "Failed to write complete packet:" << written << "/" << packet.size();
    }

    socket->flush();

    qDebug() << "Response sent - status:" << res["status"].toString()
             << "msg:" << res["msg"].toString();   

}

void TcpReactor::handleAuthSuccess(QPointer<QTcpSocket> socket, const QString& userId, bool isAdmin)
{
    if (!socket) return; 

    m_authStatus[socket] = true;
    m_socketToUserId[socket] = userId;
    m_socketIsAdmin[socket] = isAdmin;
    qDebug() << "Authentication successful for user" << userId
             << "(admin:" << isAdmin << ")"
             << ", socket" << socket->peerAddress().toString();
}
