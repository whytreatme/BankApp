#ifndef ADMINCONTROLLER_H
#define ADMINCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include "../service/adminservice.h"

/**
 * @brief 管理员控制器 - 处理管理员相关的请求
 *
 * 消息类型：
 * - Type 6: 审批用户
 * - Type 7: 获取待审批用户列表
 * - Type 8: 获取所有用户信息
 * - Type 9: 修改用户余额
 * - Type 10: 修改用户信息
 */
class AdminController : public QObject
{
    Q_OBJECT

public:
    explicit AdminController(QObject* parent = nullptr);
    ~AdminController() = default;

    /**
     * @brief 处理审批用户请求 (Type 6)
     * @param req 请求 JSON：{"user_id": 123, "approve": true/false}
     * @return 响应 JSON
     */
    QJsonObject approveUser(const QJsonObject& req);

    /**
     * @brief 处理获取待审批用户列表请求 (Type 7)
     * @param req 请求 JSON：{}
     * @return 响应 JSON
     */
    QJsonObject getPendingUsers(const QJsonObject& req);

    /**
     * @brief 处理获取所有用户信息请求 (Type 8)
     * @param req 请求 JSON：{}
     * @return 响应 JSON
     */
    QJsonObject getAllUsers(const QJsonObject& req);

    /**
     * @brief 处理修改用户余额请求 (Type 9)
     * @param req 请求 JSON：{"user_id": 123, "new_balance": 2000.00}
     * @return 响应 JSON
     */
    QJsonObject setUserBalance(const QJsonObject& req);

    /**
     * @brief 处理修改用户信息请求 (Type 10)
     * @param req 请求 JSON：{"user_id": 123, ...}
     * @return 响应 JSON
     */
    QJsonObject updateUserInfo(const QJsonObject& req);
    
    /**
     * @brief 处理添加用户请求 (Type 11)
     * @param req 请求 JSON：{"full_name": "姓名", ...}
     * @return 响应 JSON
     */
    QJsonObject createUser(const QJsonObject& req);
    
    /**
     * @brief 处理重置密码请求 (Type 12)
     * @param req 请求 JSON：{"user_id": 123}
     * @return 响应 JSON
     */
    QJsonObject resetPassword(const QJsonObject& req);

    /**
     * @brief 修改用户信息 (Type 14)
     */
    QJsonObject updateProfile(const QJsonObject& req);

private:
    AdminService m_service;
};

#endif // ADMINCONTROLLER_H
