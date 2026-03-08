#include "tcpclient.h"
#include "protocolutils.h"

#include <QTcpSocket>
#include <QDebug>

TcpClient::TcpClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    // 连接 socket 的信号到槽
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onSocketError);
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::disconnected);
    connect(m_socket, &QTcpSocket::stateChanged, this, &TcpClient::onStateChanged);
}

TcpClient::~TcpClient()
{
    // 断开所有信号连接，防止在析构过程中触发槽
    if (m_socket) {
        m_socket->blockSignals(true);

        // 如果仍然连接，直接关闭（不等待，避免在析构函数中进入事件循环）
        if (m_socket->state() == QTcpSocket::ConnectedState) {
            m_socket->disconnectFromHost();
            // 立即中止连接，不等待
            m_socket->abort();
        }
    }
    // 注意：m_socket 会被 Qt 父子对象机制自动删除，不需要手动 delete
}

bool TcpClient::connectToHost(const QString& host, quint16 port)
{
    // 检查 socket 状态
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        qWarning() << "Already connected to server";
        emit error("Already connected to server");
        return false;
    }

    // 清空缓冲区
    m_buffer.clear();

    // 连接到服务器
    m_socket->connectToHost(host, port);
    qDebug() << "Connecting to" << host << ":" << port;

    // 注意：connectToHost 是异步的，实际连接结果通过 connected() 信号通知
    return true;
}

void TcpClient::disconnect()
{
    if (m_socket && m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void TcpClient::sendRequest(quint8 type, const QString& token, const QJsonObject& data)
{
    // 检查连接状态
    if (!isConnected()) {
        qWarning() << "Cannot send request: not connected";
        emit error("Not connected to server");
        return;
    }

    // 使用 ProtocolUtils 打包消息
    QByteArray packet = ProtocolUtils::packMessage(type, token, data);

    // 发送数据
    qint64 bytesWritten = m_socket->write(packet);
    if (bytesWritten == -1) {
        qWarning() << "Failed to send data:" << m_socket->errorString();
        emit error(QString("Failed to send data: %1").arg(m_socket->errorString()));
        return;
    }

    // 刷新缓冲区（确保数据立即发送）
    m_socket->flush();

    qDebug() << "Sent request:" << "type=" << type << "bytes=" << bytesWritten;
}

void TcpClient::onReadyRead()
{
    // 读取所有可用数据并追加到缓冲区
    QByteArray newData = m_socket->readAll();
    m_buffer.append(newData);

    qDebug() << "Received" << newData.size() << "bytes, buffer size:" << m_buffer.size();

    // 循环解析缓冲区中的消息（处理粘包/半包）
    quint8 type;
    QString token;
    QJsonObject response;

    while (ProtocolUtils::parseMessage(m_buffer, type, token, response)) {
        qDebug() << "Parsed message: type=" << type;

        // type=0 表示响应消息
        if (type == 0) {
            emit responseReceived(response);
        } else {
            qWarning() << "Received non-response message type:" << type;
        }
    }

    // 如果缓冲区还有剩余数据，说明是半包（数据不完整），等待下次接收
    if (!m_buffer.isEmpty()) {
        qDebug() << "Buffer has" << m_buffer.size() << "bytes remaining (incomplete packet)";
    }
}

void TcpClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString errorMsg = QString("Socket error: %1").arg(m_socket->errorString());
    qWarning() << errorMsg;
    emit error(errorMsg);
}

void TcpClient::onStateChanged(QAbstractSocket::SocketState state)
{
    QString stateStr;
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        stateStr = "Unconnected";
        break;
    case QAbstractSocket::HostLookupState:
        stateStr = "HostLookup";
        break;
    case QAbstractSocket::ConnectingState:
        stateStr = "Connecting";
        break;
    case QAbstractSocket::ConnectedState:
        stateStr = "Connected";
        break;
    case QAbstractSocket::BoundState:
        stateStr = "Bound";
        break;
    case QAbstractSocket::ClosingState:
        stateStr = "Closing";
        break;
    case QAbstractSocket::ListeningState:
        stateStr = "Listening";
        break;
    }

    qDebug() << "Socket state changed:" << stateStr;
}
