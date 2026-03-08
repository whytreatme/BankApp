#include "transactiondao.h"
#include "../database.h"
#include "../snowflake.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

TransactionDAO::TransactionDAO() {}

QSqlDatabase TransactionDAO::getDatabase() {
    return Database::instance().getDatabase();
}

qint64 TransactionDAO::insert(qint64 fromAccount, qint64 toAccount, double amount, const QString& type, const QString& remark) {
    qint64 id = Snowflake::instance().nextId();
    QSqlQuery query(getDatabase());
    query.prepare("INSERT INTO Transaction (id, from_account_id, to_account_id, amount, type, remark) "
                  "VALUES (:id, :from, :to, :amount, :type, :remark)");
    query.bindValue(":id", id);
    query.bindValue(":from", fromAccount > 0 ? QVariant(fromAccount) : QVariant(QVariant::LongLong));
    query.bindValue(":to", toAccount > 0 ? QVariant(toAccount) : QVariant(QVariant::LongLong));
    query.bindValue(":amount", amount);
    query.bindValue(":type", type);
    query.bindValue(":remark", remark);

    if (!query.exec()) {
        qCritical() << "Failed to insert transaction:" << query.lastError().text();
        return -1;
    }
    return id;
}

QList<QVariantMap> TransactionDAO::findByUserId(qint64 userId, int limit) {
    QList<QVariantMap> txns;
    QSqlQuery query(getDatabase());
    // 查询该用户账户相关的转出和转入
    query.prepare(R"(
        SELECT t.id, t.from_account_id, t.to_account_id, t.amount, t.type, t.remark, t.timestamp
        FROM Transaction t
        JOIN Account a ON (t.from_account_id = a.id OR t.to_account_id = a.id)
        WHERE a.user_id = :user_id
        ORDER BY t.timestamp DESC
        LIMIT :limit
    )");
    query.bindValue(":user_id", userId);
    query.bindValue(":limit", limit);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap m;
            m["id"] = QString::number(query.value(0).toLongLong());
            m["from_account"] = query.value(1).isNull() ? "" : QString::number(query.value(1).toLongLong());
            m["to_account"] = query.value(2).isNull() ? "" : QString::number(query.value(2).toLongLong());
            m["amount"] = query.value(3).toDouble();
            m["type"] = query.value(4).toString();
            m["remark"] = query.value(5).toString();
            m["timestamp"] = query.value(6).toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            txns.append(m);
        }
    }
    return txns;
}

QList<QVariantMap> TransactionDAO::findAll(int limit) {
    QList<QVariantMap> txns;
    QSqlQuery query(getDatabase());
    query.prepare("SELECT id, from_account_id, to_account_id, amount, type, remark, timestamp "
                  "FROM Transaction ORDER BY timestamp DESC LIMIT :limit");
    query.bindValue(":limit", limit);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap m;
            m["id"] = QString::number(query.value(0).toLongLong());
            m["from_account"] = query.value(1).isNull() ? "" : QString::number(query.value(1).toLongLong());
            m["to_account"] = query.value(2).isNull() ? "" : QString::number(query.value(2).toLongLong());
            m["amount"] = query.value(3).toDouble();
            m["type"] = query.value(4).toString();
            m["remark"] = query.value(5).toString();
            m["timestamp"] = query.value(6).toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            txns.append(m);
        }
    }
    return txns;
}
