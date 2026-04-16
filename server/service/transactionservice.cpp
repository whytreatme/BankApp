#include "transactionservice.h"
#include "../database.h"
#include <QDebug>
#include <QJsonArray>
#include <QSet>
#include <QSqlDatabase>

TransactionService::TransactionService(QObject* parent)
    : QObject(parent)
    , m_txnDao()
    , m_accountDao()
    , m_userDao()
{
}

QJsonObject TransactionService::getTransactions(const QString& userIdStr, const QJsonObject& req)
{
    qint64 userId = userIdStr.toLongLong();
    int limit = req.value("limit").toInt(20);
    if (limit > 100) limit = 100;

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return errorResponse("无法获取数据库连接");
    }

    // 第 1 步：获取交易记录
    QList<QVariantMap> txnList = m_txnDao.findByUserId(userId, limit, db);

    // 第 2 步：收集所有 Account ID
    QSet<qint64> accountIds;
    for (const QVariantMap& txn : txnList) {
        qint64 fromAccId = txn["from_account"].toLongLong();
        qint64 toAccId = txn["to_account"].toLongLong();
        if (fromAccId > 0) accountIds.insert(fromAccId);
        if (toAccId > 0) accountIds.insert(toAccId);
    }

    // 第 3 步：批量查询 Account ID 到 User ID 的映射
    QHash<qint64, qint64> accountIdToUserId = m_accountDao.getUserIdsByAccountIds(accountIds, db);

    // 第 4 步：收集所有需要查询的 User ID
    QSet<qint64> userIds;
    for (qint64 userId : accountIdToUserId.values()) {
        if (userId > 0) userIds.insert(userId);
    }

    // 第 5 步：批量查询所有用户信息
    QHash<qint64, QVariantMap> userInfoMap = m_userDao.findByIds(userIds, db);

    // 第 6 步：组装 JSON 响应（全部从内存中查找，无数据库查询）
    QJsonArray transactions;
    for (const QVariantMap& txn : txnList) {
        QJsonObject obj;
        obj["trans_id"] = txn["id"].toString();
        obj["amount"] = txn["amount"].toDouble();
        obj["timestamp"] = txn["timestamp"].toString();
        obj["remark"] = txn["remark"].toString();
        obj["type"] = txn["type"].toString();

        qint64 fromAccId = txn["from_account"].toLongLong();
        qint64 toAccId = txn["to_account"].toLongLong();

        // 从映射中查找 User ID
        qint64 fromUserId = accountIdToUserId.value(fromAccId, -1);
        qint64 toUserId = accountIdToUserId.value(toAccId, -1);

        // 从 User 信息 Hash 中直接查找，无数据库查询
        if (userInfoMap.contains(fromUserId)) {
            const QVariantMap& userInfo = userInfoMap[fromUserId];
            obj["from_username"] = userInfo["username"].toString();
            obj["from_card_number"] = userInfo["card_number"].toString();
        }
        if (userInfoMap.contains(toUserId)) {
            const QVariantMap& userInfo = userInfoMap[toUserId];
            obj["to_username"] = userInfo["username"].toString();
            obj["to_card_number"] = userInfo["card_number"].toString();
        }

        transactions.append(obj);
    }

    return {{"status", "success"}, {"msg", "查询成功"}, {"transactions", transactions}};
}

QJsonObject TransactionService::getAllTransactions(const QJsonObject& req)
{
    int limit = req.value("limit").toInt(100);

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return errorResponse("无法获取数据库连接");
    }

    // 第 1 步：获取交易记录
    QList<QVariantMap> txnList = m_txnDao.findAll(limit, db);

    // 第 2 步：收集所有 Account ID
    QSet<qint64> accountIds;
    for (const QVariantMap& txn : txnList) {
        qint64 fromAccId = txn["from_account"].toLongLong();
        qint64 toAccId = txn["to_account"].toLongLong();
        if (fromAccId > 0) accountIds.insert(fromAccId);
        if (toAccId > 0) accountIds.insert(toAccId);
    }

    // 第 3 步：批量查询 Account ID 到 User ID 的映射
    QHash<qint64, qint64> accountIdToUserId = m_accountDao.getUserIdsByAccountIds(accountIds, db);

    // 第 4 步：收集所有需要查询的 User ID
    QSet<qint64> userIds;
    for (qint64 userId : accountIdToUserId.values()) {
        if (userId > 0) userIds.insert(userId);
    }

    // 第 5 步：批量查询所有用户信息
    QHash<qint64, QVariantMap> userInfoMap = m_userDao.findByIds(userIds, db);

    // 第 6 步：组装 JSON 响应（全部从内存中查找，无数据库查询）
    QJsonArray transactions;
    for (const QVariantMap& txn : txnList) {
        QJsonObject obj;
        obj["trans_id"] = txn["id"].toString();
        obj["amount"] = txn["amount"].toDouble();
        obj["timestamp"] = txn["timestamp"].toString();
        obj["remark"] = txn["remark"].toString();

        qint64 fromAccId = txn["from_account"].toLongLong();
        qint64 toAccId = txn["to_account"].toLongLong();

        // 从映射中查找 User ID
        qint64 fromUserId = accountIdToUserId.value(fromAccId, -1);
        qint64 toUserId = accountIdToUserId.value(toAccId, -1);

        // 从 User 信息 Hash 中直接查找，无数据库查询
        if (userInfoMap.contains(fromUserId)) {
            obj["from_card_number"] = userInfoMap[fromUserId]["card_number"].toString();
        }
        if (userInfoMap.contains(toUserId)) {
            obj["to_card_number"] = userInfoMap[toUserId]["card_number"].toString();
        }

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
