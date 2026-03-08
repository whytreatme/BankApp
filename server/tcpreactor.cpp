#include "tcpreactor.h"
#include "protocolutils.h"
#include <QDebug>
#include <QDataStream>

TcpReactor::TcpReactor(quint16 port, QObject* parent)
    : QObject(parent)
    , m_port(port)
    , m_server(nullptr)
{
    m_server = new QTcpServer(this);

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
    m_socketToUserId[clientSocket] = 0;
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

        // ==================== 认证中间件 ====================
        // type=1 (注册) 和 type=2 (登录) 不需要认证
        if (type != 1 && type != 2) {
            // 检查认证状态
            if (!m_authStatus.value(socket, false)) {
                qWarning() << "Unauthorized request from" << socket->peerAddress().toString();
                res = {{"status", "error"}, {"msg", "未认证，请先登录"}};
                sendResponse(socket, res);
                continue;
            }

            // 验证 Token 签名和角色
            QString tokenUserId;
            bool tokenIsAdmin = false;
            if (!ProtocolUtils::verifyToken(token, tokenUserId, &tokenIsAdmin)) {
                qWarning() << "Invalid token from" << socket->peerAddress().toString();
                res = {{"status", "error"}, {"msg", "Token 无效"}};
                sendResponse(socket, res);
                continue;
            }

            // 验证 Token 中的 userId 是否与绑定的 userId 一致
            if (tokenUserId != m_socketToUserId[socket]) {
                qWarning() << "Token mismatch for" << socket->peerAddress().toString();
                res = {{"status", "error"}, {"msg", "Token 与用户不匹配"}};
                sendResponse(socket, res);
                continue;
            }

            // 管理员权限检查（type 6-12, 14 需要管理员权限，type 13是用户修改密码）
            if ((type >= 6 && type <= 12) || type == 14) {
                if (!tokenIsAdmin || !m_socketIsAdmin[socket]) {
                    qWarning() << "Unauthorized admin request from user" << tokenUserId;
                    res = {{"status", "error"}, {"msg", "需要管理员权限"}};
                    sendResponse(socket, res);
                    continue;
                }
            }
        }

        // ==================== 路由分发 ====================
        QString userId = m_socketToUserId.value(socket, QString());

        try {
            switch (type) {
            case 1:  // 注册
                qDebug() << "Routing to UserController::registerUser";
                res = m_userCtrl.registerUser(req);
                break;

            case 2:  // 登录
                qDebug() << "Routing to UserController::login";
                res = m_userCtrl.login(req);
                if (res["status"] == "success") {
                    QString newUserId = res["user_id"].toString();  // UUID 字符串
                    bool isAdmin = res.contains("is_admin") ? res["is_admin"].toBool() : false;
                    handleAuthSuccess(socket, newUserId, isAdmin);

                    // 生成 Token 并返回（带 isAdmin 参数）
                    QString token = ProtocolUtils::generateToken(newUserId, isAdmin);
                    res["token"] = token;
                    qDebug() << "User" << newUserId << "logged in successfully, token generated"
                             << "(admin:" << isAdmin << ")";
                }
                break;

            case 3:  // 查询余额
                qDebug() << "Routing to AccountController::getBalance for user" << userId;
                res = m_accountCtrl.getBalance(userId);
                break;

            case 4:  // 转账
                qDebug() << "Routing to AccountController::transfer for user" << userId;
                res = m_accountCtrl.transfer(userId, req);
                break;

            case 5:  // 查询交易流水
                qDebug() << "Routing to TransactionController::getTransactions for user" << userId;
                res = m_txnCtrl.getTransactions(userId, req);
                break;

            case 6:  // 审批用户（管理员）
                qDebug() << "Routing to AdminController::approveUser";
                res = m_adminCtrl.approveUser(req);
                break;

            case 7:  // 获取待审批用户列表（管理员）
                qDebug() << "Routing to AdminController::getPendingUsers";
                res = m_adminCtrl.getPendingUsers(req);
                break;

            case 8:  // 获取所有用户信息（管理员）
                qDebug() << "Routing to AdminController::getAllUsers";
                res = m_adminCtrl.getAllUsers(req);
                break;

            case 9:  // 修改用户余额（管理员）
                qDebug() << "Routing to AdminController::setUserBalance";
                res = m_adminCtrl.setUserBalance(req);
                break;

            case 10:  // 修改用户信息（管理员）
                qDebug() << "Routing to AdminController::updateUserInfo";
                res = m_adminCtrl.updateUserInfo(req);
                break;

            case 11:  // 添加用户（管理员）
                qDebug() << "Routing to AdminController::createUser";
                res = m_adminCtrl.createUser(req);
                break;

            case 12:  // 重置密码（管理员）
                qDebug() << "Routing to AdminController::resetPassword";
                res = m_adminCtrl.resetPassword(req);
                break;

            case 13:  // 用户修改密码
                qDebug() << "Routing to UserController::changePassword for user" << userId;
                res = m_userCtrl.changePassword(req);
                break;

            case 14:  // 管理员修改个人信息
                qDebug() << "Routing to AdminController::updateProfile";
                res = m_adminCtrl.updateProfile(req);
                break;

            default:
                qWarning() << "Unknown message type:" << type;
                res = {{"status", "error"}, {"msg", "未知消息类型"}};
                break;
            }
        } catch (const std::exception& e) {
            qCritical() << "Exception handling message:" << e.what();
            res = {{"status", "error"}, {"msg", "服务器内部错误"}};
        }

        // 发送响应
        sendResponse(socket, res);
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

    qWarning() << "Socket error for" << socket->peerAddress().toString()
               << ":" << socket->errorString();
}

void TcpReactor::sendResponse(QTcpSocket* socket, const QJsonObject& res)
{
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

void TcpReactor::handleAuthSuccess(QTcpSocket* socket, const QString& userId, bool isAdmin)
{
    m_authStatus[socket] = true;
    m_socketToUserId[socket] = userId;
    m_socketIsAdmin[socket] = isAdmin;
    qDebug() << "Authentication successful for user" << userId
             << "(admin:" << isAdmin << ")"
             << ", socket" << socket->peerAddress().toString();
}
