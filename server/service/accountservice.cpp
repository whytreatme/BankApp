#include "accountservice.h"
#include <QDebug>
#include <QSqlError>
#include <QJsonArray>
#include "../database.h"

AccountService::AccountService(QObject* parent)
    : QObject(parent)
    , m_accountDao()
    , m_txnDao()
{
}

QJsonObject AccountService::getBalance(const QString& userIdStr)
{
    qint64 userId = userIdStr.toLongLong();
    double balance;
    if (!m_accountDao.getBalance(userId, balance)) {
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
    UserDAO userDao;
    
    qint64 toUserId = 0;
    QString dummyUsername, dummyHash, dummySalt, actualFullName;
    
    if (req.contains("to_account")) { // 这里前端传的是卡号
        userDao.findByCardNumber(req["to_account"].toString(), toUserId, dummyUsername, dummyHash, dummySalt, nullptr, nullptr, &actualFullName);
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

    if (userDao.isAdmin(toUserId)) {
        return transferErrorResponse("不能转账给管理员", amount);
    }

    qint64 toAccountId = m_accountDao.getAccountIdByUserId(toUserId);
    qint64 fromAccountId = m_accountDao.getAccountIdByUserId(fromUserId);

    if (toAccountId < 0 || fromAccountId < 0) {
        return transferErrorResponse("账户不存在", amount);
    }

    QSqlDatabase db = Database::instance().getDatabase();
    if (!db.transaction()) {
        return transferErrorResponse("系统错误：事务失败", amount);
    }

    try {
        double fromBalance;
        
        if (!m_accountDao.getBalance(fromUserId, fromBalance)) {
            db.rollback();
            return transferErrorResponse("账户查询失败", amount);
        }

        if (fromBalance < amount) {
            db.rollback();
            return transferErrorResponse("余额不足", amount);
        }

        if (!m_accountDao.updateBalance(fromUserId, -amount) ||
            !m_accountDao.updateBalanceByAccountId(toAccountId, amount)) {
            db.rollback();
            return transferErrorResponse("转账失败", amount);
        }

        if (m_txnDao.insert(fromAccountId, toAccountId, amount, "transfer", req.value("remark").toString()) < 0) {
            db.rollback();
            return transferErrorResponse("记录交易失败", amount);
        }

        if (!db.commit()) {
            return transferErrorResponse("系统错误：提交失败", amount);
        }

        double newBalance;
        m_accountDao.getBalance(fromUserId, newBalance);
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
    return m_accountDao.getAccountIdByUserId(userIdStr.toLongLong());
}
