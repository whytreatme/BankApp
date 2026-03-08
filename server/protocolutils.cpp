#include "protocolutils.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QDateTime>

// 密钥：生产环境应从环境变量或配置文件读取
const QByteArray ProtocolUtils::SECRET_KEY = "BankAppSecretKey2024DoNotHardcode";

QByteArray ProtocolUtils::packMessage(quint8 type, const QString& token, const QJsonObject& data)
{
    // 1. 序列化 JSON 数据（紧凑格式，无空格）
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
    quint32 totalLen = 1 + 128 + jsonData.size();  // type + token + json

    // 4. 构造数据包
    QByteArray packet;
    packet.reserve(4 + totalLen);  // 预分配空间

    // 写入长度（大端序）
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
        return false;  // 数据不完整
    }

    // 2. 读取消息总长度（大端序）
    quint32 totalLen;
    memcpy(&totalLen, buffer.constData(), 4);
    totalLen = qFromBigEndian(totalLen);

    // 3. 检查缓冲区是否有完整消息
    if (buffer.size() < 4 + static_cast<int>(totalLen)) {
        return false;  // 数据不完整
    }

    // 4. 直接从 buffer 读取数据（不使用 QDataStream）
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

QString ProtocolUtils::generateToken(const QString& userId, bool isAdmin)
{
    // 1. 生成时间戳
    qint64 timestamp = QDateTime::currentSecsSinceEpoch();

    // 2. 构造 payload: userId:timestamp:role
    QString role = isAdmin ? "1" : "0";
    QString payload = QString("%1:%2:%3").arg(userId).arg(timestamp).arg(role);

    // 3. 计算 HMAC-SHA256 签名（只取前 15 字节 = 30 个十六进制字符）
    QString signature = hmacSha256(payload.toUtf8(), SECRET_KEY);

    // 4. 构造完整 token: userId:timestamp:role:signature
    QString fullToken = payload + ":" + signature;

    // 5. Base64 编码
    QByteArray tokenBytes = fullToken.toUtf8().toBase64();
    QString token = QString::fromUtf8(tokenBytes);

    // 6. 确保 128 字节定长
    while (token.size() < 128) {
        token.append('\0');  // 用 null 字符填充
    }

    return token.left(128);  // 确保不超过 128 字节
}

bool ProtocolUtils::verifyToken(const QString& token, QString& userId, bool* isAdmin)
{
    // 1. 移除填充的 null 字符
    QString cleanedToken = token;
    while (cleanedToken.endsWith('\0')) {
        cleanedToken.chop(1);
    }

    // 2. Base64 解码
    QByteArray decoded = QByteArray::fromBase64(cleanedToken.toUtf8());
    if (decoded.isEmpty()) {
        return false;  // Base64 解码失败
    }

    QString fullToken = QString::fromUtf8(decoded);

    // 3. 分割: userId:timestamp:role:signature
    QStringList parts = fullToken.split(':');
    if (parts.size() != 4) {
        return false;  // 格式错误（旧格式有3个部分，新格式有4个）
    }

    QString userIdStr = parts[0];
    QString timestampStr = parts[1];
    QString roleStr = parts[2];
    QString signature = parts[3];

    // 4. 验证签名
    QString payload = QString("%1:%2:%3").arg(userIdStr).arg(timestampStr).arg(roleStr);
    QString expectedSig = hmacSha256(payload.toUtf8(), SECRET_KEY);

    if (signature != expectedSig) {
        return false;  // 签名无效
    }

    // 5. 验证时间戳（检查是否过期）
    qint64 timestamp = timestampStr.toLongLong();
    qint64 current = QDateTime::currentSecsSinceEpoch();
    if (current - timestamp > TOKEN_VALIDITY_PERIOD) {
        return false;  // Token 已过期
    }

    // 6. 返回 userId (UUID) 和 role
    userId = userIdStr;
    if (isAdmin) {
        *isAdmin = (roleStr == "1");
    }
    return true;
}

QString ProtocolUtils::hmacSha256(const QByteArray& data, const QByteArray& key)
{
    // 简化的 HMAC-SHA256 实现
    // 标准 HMAC: H(K ^ opad || H(K ^ ipad || text))

    const int blockSize = 64;  // SHA-256 块大小

    // 1. 准备密钥
    QByteArray keyBlock = key;
    if (keyBlock.size() > blockSize) {
        keyBlock = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }
    while (keyBlock.size() < blockSize) {
        keyBlock.append('\0');
    }

    // 2. 生成 ipad 和 opad
    QByteArray iPadded;
    QByteArray oPadded;
    for (int i = 0; i < blockSize; ++i) {
        iPadded.append(static_cast<char>(keyBlock[i] ^ 0x36));  // ipad = 0x36
        oPadded.append(static_cast<char>(keyBlock[i] ^ 0x5c));  // opad = 0x5c
    }

    // 3. 计算内层哈希: H(K ^ ipad || data)
    QByteArray innerHash = QCryptographicHash::hash(
        iPadded + data,
        QCryptographicHash::Sha256
    );

    // 4. 计算外层哈希: H(K ^ opad || innerHash)
    QByteArray outerHash = QCryptographicHash::hash(
        oPadded + innerHash,
        QCryptographicHash::Sha256
    );

    // 5. 返回前 15 字节的十六进制字符串（30 个字符）
    // 这样可以保证 Base64 后不超过 64 字节
    QByteArray shortenedHash = outerHash.left(15);
    return QString::fromUtf8(shortenedHash.toHex());
}
