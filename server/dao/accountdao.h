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
    qint64 create(qint64 userId, double initialBalance, QSqlDatabase& db);

    /**
     * @brief 获取用户余额
     */
    bool getBalance(qint64 userId, double& balance, QSqlDatabase& db);

    /**
     * @brief 更新余额（增减指定金额）
     */
    bool updateBalance(qint64 userId, double delta, QSqlDatabase& db);

    /**
     * @brief 根据 user_id 获取 account_id
     */
    qint64 getAccountIdByUserId(qint64 userId, QSqlDatabase& db);

    /**
     * @brief 根据 account_id 更新余额
     */
    bool updateBalanceByAccountId(qint64 accountId, double delta, QSqlDatabase& db);

    /**
     * @brief 根据 account_id 获取 user_id
     */
    qint64 getUserIdByAccountId(qint64 accountId, QSqlDatabase& db);

    /**
     * @brief 根据 Account ID 集合获取所有的 Account ID 到 User ID 映射
     * @return QHash，键为 Account ID，值为对应的 User ID
     */
    QHash<qint64, qint64> getUserIdsByAccountIds(const QSet<qint64>& accountIds, QSqlDatabase& db);
};

#endif // ACCOUNTDAO_H
