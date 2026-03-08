#ifndef USERCONTROLLER_H
#define USERCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include "../service/userservice.h"

/**
 * @brief 用户控制器 - 处理用户相关的 HTTP 请求
 *
 * 职责：
 * - 参数验证
 * - 调用 Service 层
 * - 返回统一格式的 JSON 响应
 */
class UserController : public QObject
{
    Q_OBJECT

public:
    explicit UserController(QObject* parent = nullptr);
    ~UserController() = default;

    /**
     * @brief 处理用户注册请求
     * @param req 请求 JSON：{"username": "xxx", "password": "xxx"}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "注册成功", "user_id": 123}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject registerUser(const QJsonObject& req);

    /**
     * @brief 处理用户登录请求
     * @param req 请求 JSON：{"username": "xxx", "password": "xxx"}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "登录成功", "user_id": 123}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject login(const QJsonObject& req);
    
    /**
     * @brief 处理用户修改密码请求 (Type 13)
     * @param req 请求 JSON：{"user_id": "123", "old_password": "xxx", "new_password": "xxx"}
     * @return 响应 JSON
     */
    QJsonObject changePassword(const QJsonObject& req);

private:
    UserService m_service;

    /**
     * @brief 参数验证 - 验证用户名
     * @param username 用户名
     * @return 验证通过返回 true，否则返回 false
     */
    bool validateUsername(const QString& username);

    /**
     * @brief 参数验证 - 验证密码
     * @param password 密码
     * @return 验证通过返回 true，否则返回 false
     */
    bool validatePassword(const QString& password);

    /**
     * @brief 构造错误响应
     */
    QJsonObject errorResponse(const QString& msg);
};

#endif // USERCONTROLLER_H
