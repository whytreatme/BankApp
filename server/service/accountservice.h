#ifndef ACCOUNTSERVICE_H
#define ACCOUNTSERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QSqlDatabase>
#include "../dao/accountdao.h"
#include "../dao/transactiondao.h"
#include "../dao/userdao.h"
#include "../database.h"

/**
 * @brief 账户服务层 - 处理账户相关的业务逻辑
 *
 * 职责：
 * - 查询账户余额
 * - 转账事务（原子性保证）
 */
class AccountService : public QObject
{
    Q_OBJECT

public:
    explicit AccountService(QObject* parent = nullptr);
    ~AccountService() = default;

    /**
     * @brief 查询余额
     * @param userId 用户 ID（UUID）
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "查询成功", "balance": 1000.00}
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject getBalance(const QString& userId);

    /**
     * @brief 转账
     * @param fromUserId 转出用户 ID（UUID）
     * @param req 请求 JSON：
     *             - "to_username": 目标用户名 (QString，与to_userid二选一)
     *             - "to_userid": 目标6位数字用户ID (QString，与to_username二选一)
     *             - "amount": 转账金额 (double)
     *             - "remark": 备注 (QString, 可选)
     * @return 响应 JSON：
     *         成功：{"status": "success", "msg": "转账成功", "new_balance": 900.00}
     *         失败：{"status": "error", "msg": "错误原因"}
     *
     * @note 转账使用 SQLite 事务保证原子性：
     *       BEGIN → 检查余额 → 扣款 → 加款 → 记录 → COMMIT
     *       任何步骤失败都会 ROLLBACK
     *
     * @note 用户通过 userid（6位数字）或 username（用户名）进行转账
     */
    QJsonObject transfer(const QString& fromUserId, const QJsonObject& req);

    /**
     * @brief 根据用户 ID 获取账户 ID
     * @param userId 用户 ID（UUID）
     * @return 成功返回 account_id，失败返回 -1
     */
    qint64 getAccountIdByUserId(const QString& userId);

private:
    AccountDAO m_accountDao;
    TransactionDAO m_txnDao;

    /**
     * @brief 构造错误响应
     */
    QJsonObject errorResponse(const QString& msg);

    /**
     * @brief 构造转账错误响应（包含amount字段，用于客户端识别）
     */
    QJsonObject transferErrorResponse(const QString& msg, double amount);

    /**
     * @brief 构造成功响应
     */
    QJsonObject successResponse(const QString& msg, const QJsonObject& data = QJsonObject());
};

#endif // ACCOUNTSERVICE_H
