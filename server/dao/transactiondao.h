#ifndef TRANSACTIONDAO_H
#define TRANSACTIONDAO_H

#include <QList>
#include <QVariantMap>
#include <QMetaType>
#include <QSqlDatabase>

class TransactionDAO
{
public:
    TransactionDAO();
    ~TransactionDAO() = default;

    qint64 insert(QSqlDatabase& db, qint64 fromAccount, qint64 toAccount, double amount, const QString& type, const QString& remark = "");

    QList<QVariantMap> findByUserId(QSqlDatabase& db, qint64 userId, int limit = 20);

    QList<QVariantMap> findAll(QSqlDatabase& db, int limit = 100);
};

#endif // TRANSACTIONDAO_H
