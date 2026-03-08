#ifndef ACCOUNTCONTROLLER_H
#define ACCOUNTCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include "../service/accountservice.h"

/**
 * @brief 账户控制器 - 处理账户相关的请求
 *
 * 职责：
 * - 参数验证
 * - 调用 Service 层
 * - 返回统一格式的 JSON 响应
 */
class AccountController : public QObject
{
    Q_OBJECT

public:
    explicit AccountController(QObject* parent = nullptr);
    ~AccountController() = default;

    /**
     * @brief 处理余额查询请求
     * @param userId 用户 ID（UUID）
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "查询成功", "balance": 1000.00}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject getBalance(const QString& userId);

    /**
     * @brief 处理转账请求
     * @param userId 转出用户 ID（UUID）
     * @param req 请求 JSON：
     *             - "to_account": 目标账户 ID (qint64)
     *             - "to_username": 目标用户名 (QString)
     *             - "to_userid": 目标6位数字用户ID (QString)
     *             - "amount": 转账金额 (double)
     *             - "remark": 备注 (QString, 可选)
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "转账成功", "new_balance": 900.00}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject transfer(const QString& userId, const QJsonObject& req);

private:
    AccountService m_service;

    /**
     * @brief 验证转账参数
     * @param req 请求 JSON
     * @param toAccountId [out] 目标账户 ID
     * @param amount [out] 转账金额
     * @param remark [out] 备注
     * @return 验证通过返回 true，否则返回 false
     */
    bool validateTransferParams(
        const QJsonObject& req,
        qint64& toAccountId,
        double& amount,
        QString& remark
    );

    /**
     * @brief 构造错误响应
     */
    QJsonObject errorResponse(const QString& msg);

    /**
     * @brief 构造转账错误响应（包含amount字段，用于客户端识别）
     */
    QJsonObject transferErrorResponse(const QString& msg, double amount);
};

#endif // ACCOUNTCONTROLLER_H
