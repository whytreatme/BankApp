#include "protocolutils.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QDateTime>
#include <QtEndian>



QByteArray ProtocolUtils::packMessage(quint8 type, const QString& token, const QJsonObject& data)
{
    // 1. 序列化 JSON 数据
    QByteArray jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);

    // 2. 准备 Token 字段（128 字节定长，不足补 \0）
    QByteArray tokenBytes = token.toUtf8();
    if (tokenBytes.size() > 128) {
        tokenBytes = tokenBytes.left(128);  // 截断
    }
    while (tokenBytes.size() < 128) {
        tokenBytes.append('\0');  // 填充
    }

    // 3. 计算消息总长度（不包含长度字段自身的 4 字节）
    quint32 totalLen = 1 + 128 + jsonData.size(); 

    // 4. 构造数据包
    QByteArray packet;
    packet.reserve(4 + totalLen);  // 预分配空间

    // 写入长度
    quint32 bigEndianLen = qToBigEndian(totalLen);
    packet.append(reinterpret_cast<const char*>(&bigEndianLen), 4);

    // 写入消息类型
    packet.append(static_cast<char>(type));

    // 写入 Token（128 字节）
    packet.append(tokenBytes);

    // 写入 JSON 数据
    packet.append(jsonData);

    return packet;
}

bool ProtocolUtils::parseMessage(QByteArray& buffer, quint8& type, QString& token, QJsonObject& data)
{
    // 1. 检查是否有足够的数据读取长度字段
    if (buffer.size() < 4) {
        return false;  
    }

    // 2. 读取消息总长度
    quint32 totalLen;
    memcpy(&totalLen, buffer.constData(), 4);
    totalLen = qFromBigEndian(totalLen);

    // 3. 检查缓冲区是否有完整消息
    if (buffer.size() < 4 + static_cast<int>(totalLen)) {
        return false;  // 数据不完整
    }

    // 4. 直接从 buffer 读取数据
    int offset = 4;  // 跳过长度字段

    // 读取消息类型（1 字节）
    type = static_cast<quint8>(buffer[offset]);
    offset += 1;

    // 读取 Token（128 字节）
    QByteArray tokenBytes = QByteArray::fromRawData(buffer.constData() + offset, 128);
    token = QString::fromUtf8(tokenBytes).trimmed();  // 移除填充的 \0
    offset += 128;

    // 读取 JSON 数据
    int jsonSize = static_cast<int>(totalLen) - 1 - 128;  // 总长 - type(1) - token(128)
    QByteArray jsonData = QByteArray::fromRawData(buffer.constData() + offset, jsonSize);
    data = QJsonDocument::fromJson(jsonData).object();

    // 5. 从缓冲区移除已处理的数据
    buffer.remove(0, 4 + static_cast<int>(totalLen));

    return true;
}


