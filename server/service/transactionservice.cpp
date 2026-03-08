#include "transactionservice.h"
#include "../dao/userdao.h"
#include <QDebug>
#include <QJsonArray>

TransactionService::TransactionService(QObject* parent)
    : QObject(parent)
    , m_txnDao()
    , m_accountDao()
{
}

QJsonObject TransactionService::getTransactions(const QString& userIdStr, const QJsonObject& req)
{
    qint64 userId = userIdStr.toLongLong();
    int limit = req.value("limit").toInt(20);
    if (limit > 100) limit = 100;

    QList<QVariantMap> txnList = m_txnDao.findByUserId(userId, limit);
    QJsonArray transactions;
    UserDAO userDao;

    for (const QVariantMap& txn : txnList) {
        QJsonObject obj;
        obj["trans_id"] = txn["id"].toString();
        obj["amount"] = txn["amount"].toDouble();
        obj["timestamp"] = txn["timestamp"].toString();
        obj["remark"] = txn["remark"].toString();
        obj["type"] = txn["type"].toString();

        qint64 fromAccId = txn["from_account"].toLongLong();
        qint64 toAccId = txn["to_account"].toLongLong();

        qint64 fromUserId = m_accountDao.getUserIdByAccountId(fromAccId);
        qint64 toUserId = m_accountDao.getUserIdByAccountId(toAccId);

        QString fromUsername, fromCard, toUsername, toCard;
        bool admin, approved;

        if (userDao.findById(fromUserId, fromUsername, fromCard, admin, approved)) {
            obj["from_username"] = fromUsername;
            obj["from_card_number"] = fromCard;
        }
        if (userDao.findById(toUserId, toUsername, toCard, admin, approved)) {
            obj["to_username"] = toUsername;
            obj["to_card_number"] = toCard;
        }

        transactions.append(obj);
    }

    return {{"status", "success"}, {"msg", "查询成功"}, {"transactions", transactions}};
}

QJsonObject TransactionService::getAllTransactions(const QJsonObject& req)
{
    int limit = req.value("limit").toInt(100);
    QList<QVariantMap> txnList = m_txnDao.findAll(limit);
    QJsonArray transactions;
    UserDAO userDao;

    for (const QVariantMap& txn : txnList) {
        QJsonObject obj;
        obj["trans_id"] = txn["id"].toString();
        obj["amount"] = txn["amount"].toDouble();
        obj["timestamp"] = txn["timestamp"].toString();
        obj["remark"] = txn["remark"].toString();

        qint64 fromAccId = txn["from_account"].toLongLong();
        qint64 toAccId = txn["to_account"].toLongLong();

        qint64 fromUserId = m_accountDao.getUserIdByAccountId(fromAccId);
        qint64 toUserId = m_accountDao.getUserIdByAccountId(toAccId);

        QString u, c; bool a, ap;
        if (userDao.findById(fromUserId, u, c, a, ap)) obj["from_card_number"] = c;
        if (userDao.findById(toUserId, u, c, a, ap)) obj["to_card_number"] = c;

        transactions.append(obj);
    }
    return {{"status", "success"}, {"msg", "查询成功"}, {"transactions", transactions}};
}

QJsonObject TransactionService::errorResponse(const QString& msg)
{
    return {{"status", "error"}, {"msg", msg}};
}

QJsonObject TransactionService::successResponse(const QString& msg, const QJsonArray& transactions)
{
    return {{"status", "success"}, {"msg", msg}, {"transactions", transactions}};
}
