#ifndef ADMINSERVICE_H
#define ADMINSERVICE_H

#include <QObject>
#include <QJsonObject>
#include "../dao/userdao.h"
#include "../dao/accountdao.h"
#include <QSqlDatabase>

/**
 * @brief 管理员服务层 - 处理管理员相关的业务逻辑
 *
 * 职责：
 * - 审批用户注册
 * - 查询所有用户信息
 * - 修改用户余额
 * - 修改用户信息
 */
class AdminService : public QObject
{
    Q_OBJECT

public:
    explicit AdminService(QObject* parent = nullptr);
    ~AdminService() = default;

    /**
     * @brief 审批用户注册
     * @param req 请求 JSON：{"user_id": 123, "approve": true/false}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "用户已批准/拒绝"}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject approveUser(const QJsonObject& req);

    /**
     * @brief 获取待审批用户列表
     * @param req 请求 JSON：{}
     * @return 响应 JSON：
     *         成功：{"status": "success", "users": [{user_id, username, created_at}, ...]}
     */
    QJsonObject getPendingUsers(const QJsonObject& req);

    /**
     * @brief 获取所有用户信息（管理员查询用）
     * @param req 请求 JSON：{}
     * @return 响应 JSON：
     *         成功：{"status": "success", "users": [{user_id, username, balance, is_admin, is_approved, created_at}, ...]}
     */
    QJsonObject getAllUsers(const QJsonObject& req);

    /**
     * @brief 修改用户余额（管理员权限）
     * @param req 请求 JSON：{"user_id": 123, "new_balance": 2000.00}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "余额已修改"}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject setUserBalance(const QJsonObject& req);

    /**
     * @brief 修改用户信息（管理员权限）
     * @param req 请求 JSON：{"user_id": 123, "username": "newname", "is_admin": 0/1, "is_approved": 0/1}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "用户信息已更新"}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject updateUserInfo(const QJsonObject& req);
    
    /**
     * @brief 管理员添加新用户
     * @param req 请求 JSON：{"full_name": "姓名", "id_card": "身份证", "phone": "手机", "birth_date": "出生年月", "address": "地址"}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "用户创建成功", "card_number": "6222...", "password": "123456"}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject createUser(const QJsonObject& req);
    
    /**
     * @brief 管理员重置用户密码
     * @param req 请求 JSON：{"user_id": 123}
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "密码已重置", "password": "新密码"}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject resetPassword(const QJsonObject& req);

    /**
     * @brief 修改用户信息 (Type 14)
     */
    QJsonObject updateProfile(const QJsonObject& req);  


private:
    UserDAO m_userDao;
    AccountDAO m_accountDao;
};

#endif // ADMINSERVICE_H
