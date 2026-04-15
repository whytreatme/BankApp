#ifndef TCPREACTOR_H
#define TCPREACTOR_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>
#include <QJsonObject>
#include <QPointer>
#include "controller/usercontroller.h"
#include "controller/accountcontroller.h"
#include "controller/transactioncontroller.h"
#include "controller/admincontroller.h"

/**
 * @brief TCP Reactor - 单线程事件循环处理并发连接
 *
 * 基于 Reactor 模式的 TCP 服务器：
 * - 使用 Qt 事件循环处理所有 I/O 事件
 * - 每个连接维护独立的接收缓冲区
 * - 认证状态和 Socket-UserId 映射
 * - 消息路由到对应的 Controller 处理
 */
class TcpReactor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param parent 父对象
     */
    explicit TcpReactor(quint16 port, QObject* parent = nullptr);
    ~TcpReactor() = default;

    /**
     * @brief 启动服务器
     * @return 成功返回 true，失败返回 false
     */
    bool start();

    /**
     * @brief 停止服务器
     */
    void stop();
   
    
private slots:
    /**
     * @brief 新连接槽函数 - QTcpServer::newConnection 触发
     */
    void onNewConnection();

    /**
     * @brief 数据可读槽函数 - QTcpSocket::readyRead 触发
     */
    void onReadyRead();

    /**
     * @brief 连接断开槽函数 - QTcpSocket::disconnected 触发
     */
    void onDisconnected();

    /**
     * @brief Socket 错误槽函数
     */
    void onSocketError(QAbstractSocket::SocketError socketError);

    /**
     * @brief 发送响应消息给客户端
     * @param socket 目标 Socket
     * @param res 响应 JSON（type=0 表示响应）
     */
    void sendResponse(QPointer<QTcpSocket> socket, const QJsonObject& res);

    /**
     * @brief 处理认证成功 - 设置认证状态和映射
     * @param socket 客户端 Socket
     * @param userId 用户 ID (UUID)
     * @param isAdmin 是否是管理员
     */
    void handleAuthSuccess(QPointer<QTcpSocket> socket, const QString& userId, bool isAdmin = false);


private:
   
    // ==================== 成员变量 ====================

    quint16 m_port;                           // 监听端口
    QTcpServer* m_server;                     // TCP 服务器

    // 连接管理
    QHash<QTcpSocket*, QByteArray> m_buffers;        // Socket → 接收缓冲区
    QHash<QTcpSocket*, bool> m_authStatus;           // Socket → 认证状态
    QHash<QTcpSocket*, QString> m_socketToUserId;    // Socket → UserId (UUID) 映射
    QHash<QTcpSocket*, bool> m_socketIsAdmin;        // Socket → IsAdmin 映射

    // Controller 层实例
    UserController m_userCtrl;
    AccountController m_accountCtrl;
    TransactionController m_txnCtrl;
    AdminController m_adminCtrl;
};

#endif // TCPREACTOR_H
