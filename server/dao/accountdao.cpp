#include "accountdao.h"
#include "../snowflake.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSet>
#include <QHash>

AccountDAO::AccountDAO() {}

qint64 AccountDAO::create(qint64 userId, double initialBalance, QSqlDatabase& db) {
    qint64 id = Snowflake::instance().nextId();
    QSqlQuery query(db);
    query.prepare("INSERT INTO Account (id, user_id, balance) VALUES (:id, :user_id, :balance)");
    query.bindValue(":id", id);
    query.bindValue(":user_id", userId);
    query.bindValue(":balance", initialBalance);

    if (!query.exec()) {
        qCritical() << "Failed to create account:" << query.lastError().text();
        return -1;
    }
    return id;
}

bool AccountDAO::getBalance(qint64 userId, double& balance, QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare("SELECT balance FROM Account WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if (query.exec() && query.next()) {
        balance = query.value(0).toDouble();
        return true;
    }
    return false;
}

bool AccountDAO::updateBalance(qint64 userId, double delta, QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare("UPDATE Account SET balance = balance + :delta WHERE user_id = :user_id");
    query.bindValue(":delta", delta);
    query.bindValue(":user_id", userId);
    return query.exec();
}

qint64 AccountDAO::getAccountIdByUserId(qint64 userId, QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare("SELECT id FROM Account WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

bool AccountDAO::updateBalanceByAccountId(qint64 accountId, double delta, QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare("UPDATE Account SET balance = balance + :delta WHERE id = :id");
    query.bindValue(":delta", delta);
    query.bindValue(":id", accountId);
    return query.exec();
}

qint64 AccountDAO::getUserIdByAccountId(qint64 accountId, QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare("SELECT user_id FROM Account WHERE id = :id");
    query.bindValue(":id", accountId);
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

QHash<qint64, qint64> AccountDAO::getUserIdsByAccountIds(const QSet<qint64>& accountIds, QSqlDatabase& db) {
    QHash<qint64, qint64> result;

    if (accountIds.isEmpty()) {
        return result;
    }

    // 构建 SQL IN 子句
    QStringList idStrings;
    for (qint64 id : accountIds) {
        idStrings.append(QString::number(id));
    }
    QString inClause = idStrings.join(",");

    QString sql = QString("SELECT id, user_id FROM Account WHERE id IN (%1)").arg(inClause);

    QSqlQuery query(db);
    if (query.exec(sql)) {
        while (query.next()) {
            qint64 accountId = query.value(0).toLongLong();
            qint64 userId = query.value(1).toLongLong();
            result[accountId] = userId;
        }
    }

    return result;
}
