#include "accountcontroller.h"
#include <QDebug>

AccountController::AccountController(QObject* parent)
    : QObject(parent)
    , m_service()
{
}

QJsonObject AccountController::getBalance(const QString& userId)
{
    // 1. 用户 ID 验证
    if (userId.isEmpty()) {
        return errorResponse("无效的用户 ID");
    }

    qDebug() << "处理余额查询请求: userId =" << userId;

    // 2. 调用 Service 层
    QJsonObject res = m_service.getBalance(userId);

    // 3. 记录结果
    if (res["status"] == "success") {
        double balance = res["balance"].toDouble();
        qDebug() << "余额查询成功: userId =" << userId << ", balance =" << balance;
    } else {
        qWarning() << "余额查询失败: userId =" << userId << ", reason:" << res["msg"].toString();
    }

    return res;
}

QJsonObject AccountController::transfer(const QString& userId, const QJsonObject& req)
{
    // 1. 用户 ID 验证
    if (userId.isEmpty()) {
        return errorResponse("无效的用户 ID");
    }

    // 2. 参数验证
    qint64 toAccountId;
    double amount;
    QString remark;

    if (!validateTransferParams(req, toAccountId, amount, remark)) {
        return errorResponse("转账参数无效");
    }

    qDebug() << "处理转账请求: fromUserId =" << userId
             << ", toAccountId =" << toAccountId
             << ", amount =" << amount;

    // 3. 如果使用账号ID转账，先检查是否转账给自己
    if (toAccountId > 0) {
        qint64 fromAccountId = m_service.getAccountIdByUserId(userId);
        if (fromAccountId == toAccountId) {
            qWarning() << "转账失败：不能转账给自己";
            return transferErrorResponse("不能转账给自己", amount);
        }
    }
    // 如果使用用户名或userid转账（toAccountId == -1），安全检查将在 Service 层进行

    // 4. 调用 Service 层（支持 to_account、to_username 和 to_userid 三种方式）
    QJsonObject res = m_service.transfer(userId, req);

    // 5. 记录结果
    if (res["status"] == "success") {
        double newBalance = res["new_balance"].toDouble();
        qDebug() << "转账成功: userId =" << userId << ", new_balance =" << newBalance;
    } else {
        qWarning() << "转账失败: userId =" << userId << ", reason:" << res["msg"].toString();
    }

    return res;
}

bool AccountController::validateTransferParams(
    const QJsonObject& req,
    qint64& toAccountId,
    double& amount,
    QString& remark)
{
    // 1. 检查必填字段（支持 to_account、to_username 或 to_userid）
    if (!req.contains("amount")) {
        qWarning() << "转账参数缺失：amount";
        return false;
    }

    if (!req.contains("to_account") && !req.contains("to_username") && !req.contains("to_userid")) {
        qWarning() << "转账参数缺失：to_account、to_username 或 to_userid";
        return false;
    }

    // 2. 解析目标账户（支持三种方式）
    if (req.contains("to_account")) {
        // 方式1：通过账号ID转账
        bool ok1;
        toAccountId = req["to_account"].toVariant().toLongLong(&ok1);
        if (!ok1 || toAccountId <= 0) {
            qWarning() << "目标账户 ID 无效:" << req["to_account"];
            return false;
        }
    } else {
        // 方式2/3：通过用户名或userid转账，设置 toAccountId = -1 表示需要后续查询
        toAccountId = -1;
    }

    // 3. 解析转账金额
    amount = req["amount"].toDouble();
    if (amount <= 0) {
        qWarning() << "转账金额无效:" << amount;
        return false;
    }

    // 4. 金额精度检查（最多 2 位小数）
    QString amountStr = QString::number(amount, 'f', 2);
    if (QString::number(amount, 'f', 2) != QString::number(amount, 'f', 10).left(QString::number(amount, 'f', 10).indexOf('.') + 3).leftJustified(amountStr.length(), '0')) {
        // 简化检查：验证金额不超过 2 位小数
        double rounded = qRound(amount * 100.0) / 100.0;
        if (qFuzzyCompare(amount, rounded)) {
            amount = rounded;
        }
    }

    // 5. 金额上限检查（单笔转账不超过 100 万）
    if (amount > 1000000.0) {
        qWarning() << "转账金额超过上限:" << amount;
        return false;
    }

    // 6. 解析备注（可选）
    remark = req.value("remark").toString();
    if (remark.length() > 100) {
        remark = remark.left(100); // 限制备注长度
    }

    return true;
}

QJsonObject AccountController::errorResponse(const QString& msg)
{
    return {
        {"status", "error"},
        {"msg", msg}
    };
}

QJsonObject AccountController::transferErrorResponse(const QString& msg, double amount)
{
    return {
        {"status", "error"},
        {"msg", msg},
        {"amount", amount}
    };
}
