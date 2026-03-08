#ifndef TRANSACTIONDAO_H
#define TRANSACTIONDAO_H

#include <QList>
#include <QVariantMap>
#include <QSqlDatabase>

class TransactionDAO
{
public:
    TransactionDAO();
    ~TransactionDAO() = default;

    /**
     * @brief 插入交易记录
     */
    qint64 insert(qint64 fromAccount, qint64 toAccount, double amount, const QString& type, const QString& remark = "");

    /**
     * @brief 查询用户的交易历史
     */
    QList<QVariantMap> findByUserId(qint64 userId, int limit = 20);

    /**
     * @brief 查询所有交易记录（管理员用）
     */
    QList<QVariantMap> findAll(int limit = 100);

private:
    QSqlDatabase getDatabase();
};

#endif // TRANSACTIONDAO_H
