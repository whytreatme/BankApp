#ifndef USERSERVICE_H
#define USERSERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include "../dao/userdao.h"
#include "../dao/accountdao.h"

/**
 * @brief 用户服务层 - 处理用户相关的业务逻辑
 *
 * 职责：
 * - 用户注册（包括创建账户）
 * - 用户登录验证
 * - 密码哈希和盐生成
 */
class UserService : public QObject
{
    Q_OBJECT

public:
    explicit UserService(QObject* parent = nullptr);
    ~UserService() = default;

    /**
     * @brief 用户注册
     * @param req 请求 JSON：{"username": "xxx", "password": "xxx"}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "注册成功", "user_id": 123}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject registerUser(const QJsonObject& req);

    /**
     * @brief 用户登录
     * @param req 请求 JSON：{"username": "xxx", "password": "xxx"}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "登录成功", "user_id": 123}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject login(const QJsonObject& req);

    /**
     * @brief 密码哈希 - SHA256(password + salt)
     * @param password 明文密码
     * @param salt 盐值
     * @return 密码哈希值（十六进制字符串）
     */
    static QString hashPassword(const QString& password, const QString& salt);

    /**
     * @brief 生成随机盐（16 字节）
     * @return 随机盐值（十六进制字符串，32 个字符）
     */
    static QString generateSalt();
    
    /**
     * @brief 用户修改密码（需验证旧密码）
     * @param req 请求 JSON：{"user_id": "123", "old_password": "xxx", "new_password": "xxx"}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "密码修改成功"}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject changePassword(const QJsonObject& req);

    
    static QString generateCardNumber();

private:
    UserDAO m_userDao;
    AccountDAO m_accountDao;

};

#endif // USERSERVICE_H
