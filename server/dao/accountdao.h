#ifndef ACCOUNTDAO_H
#define ACCOUNTDAO_H

#include <QString>
#include <QSqlDatabase>

class AccountDAO
{
public:
    AccountDAO();
    ~AccountDAO() = default;

    /**
     * @brief 创建账户
     */
    qint64 create(qint64 userId, double initialBalance = 0.0);

    /**
     * @brief 获取用户余额
     */
    bool getBalance(qint64 userId, double& balance);

    /**
     * @brief 更新余额（增减指定金额）
     */
    bool updateBalance(qint64 userId, double delta);

    /**
     * @brief 根据 user_id 获取 account_id
     */
    qint64 getAccountIdByUserId(qint64 userId);

    /**
     * @brief 根据 account_id 更新余额
     */
    bool updateBalanceByAccountId(qint64 accountId, double delta);

    /**
     * @brief 根据 account_id 获取 user_id
     */
    qint64 getUserIdByAccountId(qint64 accountId);

private:
    QSqlDatabase getDatabase();
};

#endif // ACCOUNTDAO_H
