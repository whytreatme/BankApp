#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QString>

/**
 * @brief TCP 客户端类 - 封装网络通信细节
 *
 * 功能：
 * - 连接到服务器
 * - 发送请求（非阻塞）
 * - 接收响应（通过信号）
 * - 自动处理粘包/半包问题
 * - Token 管理
 */
class TcpClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit TcpClient(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TcpClient();

    /**
     * @brief 连接到服务器
     * @param host 服务器地址（IP 或域名）
     * @param port 服务器端口
     * @return true 表示连接请求已发出（实际连接状态通过 connected 信号通知）
     */
    bool connectToHost(const QString& host, quint16 port);

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 发送请求（非阻塞）
     * @param type 消息类型 (1-5)
     * @param token 认证 Token（登录成功后必须提供）
     * @param data JSON 数据体
     */
    void sendRequest(quint8 type, const QString& token, const QJsonObject& data);

    /**
     * @brief 设置 Token（登录成功后调用）
     * @param token Token 字符串
     */
    void setToken(const QString& token) { m_token = token; }

    /**
     * @brief 获取当前 Token
     * @return Token 字符串
     */
    QString token() const { return m_token; }

    /**
     * @brief 检查是否已连接
     * @return true 表示已连接
     */
    bool isConnected() const { return m_socket && m_socket->state() == QTcpSocket::ConnectedState; }

signals:
    /**
     * @brief 连接成功信号
     */
    void connected();

    /**
     * @brief 断开连接信号
     */
    void disconnected();

    /**
     * @brief 收到响应信号
     * @param response JSON 响应数据
     */
    void responseReceived(const QJsonObject& response);

    /**
     * @brief 错误信号
     * @param msg 错误消息
     */
    void error(const QString& msg);

private slots:
    /**
     * @brief 处理 socket readyRead 信号（接收数据）
     */
    void onReadyRead();

    /**
     * @brief 处理 socket 错误信号
     * @param socketError socket 错误码
     */
    void onSocketError(QAbstractSocket::SocketError socketError);

    /**
     * @brief 处理连接状态变化
     */
    void onStateChanged(QAbstractSocket::SocketState state);

private:
    QTcpSocket* m_socket;      // TCP Socket
    QByteArray m_buffer;       // 接收缓冲区（用于处理粘包/半包）
    QString m_token;           // 认证 Token
};

#endif // TCPCLIENT_H
