#ifndef PROTOCOLUTILS_H
#define PROTOCOLUTILS_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

/**
 * @brief 协议工具类 - 处理 TCP 消息的打包和解包
 *
 * 消息帧格式:
 * [4 bytes: Length] [1 byte: Type] [128 bytes: Token] [Variable: JSON]
 * - Length: 大端序 uint32，不包含自身 4 字节
 * - Type: uint8，0=响应, 1=注册, 2=登录, 3=查余额, 4=转账, 5=查流水
 * - Token: 128 字节定长，不足补 \0
 * - JSON: UTF-8 编码的 JSON 数据
 */
class ProtocolUtils
{
public:
    /**
     * @brief 打包消息
     * @param type 消息类型 (1-5)
     * @param token 认证 Token（可为空）
     * @param data JSON 数据体
     * @return 打包后的完整消息帧
     */
    static QByteArray packMessage(quint8 type, const QString& token, const QJsonObject& data);

    /**
     * @brief 解包消息（处理粘包/半包）
     * @param buffer 接收缓冲区 [in/out]，成功后会移除已处理的数据
     * @param type [out] 消息类型
     * @param token [out] Token（128 字节）
     * @param data [out] JSON 数据
     * @return true 表示提取到完整消息，false 表示数据不完整
     */
    static bool parseMessage(QByteArray& buffer, quint8& type, QString& token, QJsonObject& data);

    /**
     * @brief 生成认证 Token
     * 格式: base64(userId:timestamp:role:HMAC-SHA256(payload+SECRET_KEY))
     * @param userId 用户 ID
     * @param isAdmin 是否是管理员
     * @return 128 字节定长 Token
     */
    static QString generateToken(const QString& userId, bool isAdmin = false);

    /**
     * @brief 验证 Token
     * @param token Token 字符串
     * @param userId [out] 用户 ID (UUID)
     * @param isAdmin [out] 是否是管理员（可选）
     * @return true 表示 Token 有效
     */
    static bool verifyToken(const QString& token, QString& userId, bool* isAdmin = nullptr);

private:
    // HMAC 签名密钥（生产环境应从配置文件读取）
    static const QByteArray SECRET_KEY;

    // Token 有效期（秒），默认 24 小时
    static const qint64 TOKEN_VALIDITY_PERIOD = 86400;

    /**
     * @brief 计算 HMAC-SHA256 签名（只取前 16 字节以节省空间）
     * @param data 待签名数据
     * @param key 密钥
     * @return 十六进制签名字符串（32 个字符）
     */
    static QString hmacSha256(const QByteArray& data, const QByteArray& key);
};

#endif // PROTOCOLUTILS_H
