#include "accountservice.h"
#include "../dao/userdao.h"
#include <QDebug>
#include <QJsonArray>
#include <QSqlDatabase>
#include <stdexcept>

AccountService::AccountService(QObject* parent)
    : QObject(parent)
    , m_accountDao()
    , m_txnDao()
{
}

QJsonObject AccountService::getBalance(const QString& userIdStr)
{
    qint64 userId = userIdStr.toLongLong();

    // 获取数据库连接
    thread_local QSqlDatabase db;
    if (!db.isValid() || !db.isOpen()) {
        if (!Database::getThreadConnection(db)) {
            return errorResponse("无法获取数据库连接");
        }
    }

    double balance;
    if (!m_accountDao.getBalance(db, userId, balance)) {
        return errorResponse("余额查询失败");
    }

    return successResponse("查询成功", {{"balance", balance}});
}

QJsonObject AccountService::transfer(const QString& fromUserIdStr, const QJsonObject& req)
{
    qint64 fromUserId = fromUserIdStr.toLongLong();
    if (!req.contains("amount")) {
        return errorResponse("缺少必要参数：amount");
    }

    double amount = req["amount"].toDouble();
    QString targetName = req["to_name"].toString().trimmed();

    // 获取数据库连接
    thread_local QSqlDatabase db;
    if (!db.isValid() || !db.isOpen()) {
        if (!Database::getThreadConnection(db)) {
            return errorResponse("无法获取数据库连接");
        }
    }

    UserDAO userDao;

    qint64 toUserId = 0;
    QString dummyUsername, dummyHash, dummySalt, actualFullName;

    if (req.contains("to_account")) { // 这里前端传的是卡号
        userDao.findByCardNumber(db, req["to_account"].toString(), toUserId, dummyUsername, dummyHash, dummySalt,
                                  nullptr, nullptr, &actualFullName, nullptr, nullptr, nullptr);
    }

    if (toUserId == 0) {
        return transferErrorResponse("目标卡号不存在", amount);
    }

    if (!targetName.isEmpty() && actualFullName != targetName) {
        return transferErrorResponse("目标卡号与姓名不匹配", amount);
    }

    if (toUserId == fromUserId) {
        return transferErrorResponse("不能转账给自己", amount);
    }

    if (userDao.isAdmin(db, toUserId)) {
        return transferErrorResponse("不能转账给管理员", amount);
    }

    qint64 toAccountId = m_accountDao.getAccountIdByUserId(db, toUserId);
    qint64 fromAccountId = m_accountDao.getAccountIdByUserId(db, fromUserId);

    if (toAccountId < 0 || fromAccountId < 0) {
        return transferErrorResponse("账户不存在", amount);
    }

    // 开启事务
    if (!db.transaction()) {
        return transferErrorResponse("系统错误：事务失败", amount);
    }

    try {
        double fromBalance;

        if (!m_accountDao.getBalance(db, fromUserId, fromBalance)) {
            throw std::runtime_error("账户查询失败");
        }

        if (fromBalance < amount) {
            throw std::runtime_error("余额不足");
        }

        if (!m_accountDao.updateBalance(db, fromUserId, -amount) ||
            !m_accountDao.updateBalanceByAccountId(db, toAccountId, amount)) {
            throw std::runtime_error("转账失败");
        }

        if (m_txnDao.insert(db, fromAccountId, toAccountId, amount, "transfer",
                            req.value("remark").toString()) < 0) {
            throw std::runtime_error("记录交易失败");
        }

        if (!db.commit()) {
            throw std::runtime_error("系统错误：提交失败");
        }

        double newBalance;
        m_accountDao.getBalance(db, fromUserId, newBalance);
        return successResponse("转账成功", {{"new_balance", newBalance}});

    } catch (const std::exception& e) {
        db.rollback();
        qCritical() << "Transfer exception:" << e.what();
        return transferErrorResponse("转账异常", amount);
    } catch (...) {
        db.rollback();
        return transferErrorResponse("未知转账异常", amount);
    }
}

QJsonObject AccountService::errorResponse(const QString& msg)
{
    return {{"status", "error"}, {"msg", msg}};
}

QJsonObject AccountService::transferErrorResponse(const QString& msg, double amount)
{
    return {{"status", "error"}, {"msg", msg}, {"amount", amount}};
}

QJsonObject AccountService::successResponse(const QString& msg, const QJsonObject& data)
{
    QJsonObject result = {{"status", "success"}, {"msg", msg}};
    for (auto it = data.begin(); it != data.end(); ++it) {
        result[it.key()] = it.value();
    }
    return result;
}

qint64 AccountService::getAccountIdByUserId(const QString& userIdStr)
{
    thread_local QSqlDatabase db;
    if (!db.isValid() || !db.isOpen()) {
        if (!Database::getThreadConnection(db)) {
            qCritical() << "Failed to get database connection in getAccountIdByUserId";
            return -1;
        }
    }

    return m_accountDao.getAccountIdByUserId(db, userIdStr.toLongLong());
}
