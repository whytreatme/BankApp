#include "accountdao.h"
#include "../database.h"
#include "../snowflake.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

AccountDAO::AccountDAO() {}

QSqlDatabase AccountDAO::getDatabase() {
    QSqlDatabase w_db;
    if(!Database::getThreadConnection(w_db)){
        qCritical() << "DAO Error: Cannot get datbase access! ";
    }
    return w_db;
}

qint64 AccountDAO::create(qint64 userId, double initialBalance) {
    qint64 id = Snowflake::instance().nextId();
    QSqlQuery query(getDatabase());
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

bool AccountDAO::getBalance(qint64 userId, double& balance) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT balance FROM Account WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if (query.exec() && query.next()) {
        balance = query.value(0).toDouble();
        return true;
    }
    return false;
}

bool AccountDAO::updateBalance(qint64 userId, double delta) {
    QSqlQuery query(getDatabase());
    query.prepare("UPDATE Account SET balance = balance + :delta WHERE user_id = :user_id");
    query.bindValue(":delta", delta);
    query.bindValue(":user_id", userId);
    return query.exec();
}

qint64 AccountDAO::getAccountIdByUserId(qint64 userId) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT id FROM Account WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

bool AccountDAO::updateBalanceByAccountId(qint64 accountId, double delta) {
    QSqlQuery query(getDatabase());
    query.prepare("UPDATE Account SET balance = balance + :delta WHERE id = :id");
    query.bindValue(":delta", delta);
    query.bindValue(":id", accountId);
    return query.exec();
}

qint64 AccountDAO::getUserIdByAccountId(qint64 accountId) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT user_id FROM Account WHERE id = :id");
    query.bindValue(":id", accountId);
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}
