#include "transactioncontroller.h"
#include "../dao/userdao.h"
#include "../database.h"     
#include <QSqlDatabase>      
#include <QDebug>

TransactionController::TransactionController(QObject* parent)
    : QObject(parent)
    , m_service()
{
}

QJsonObject TransactionController::getTransactions(const QString& userId, const QJsonObject& req)
{
    // 1. 用户 ID 验证
    if (userId.isEmpty()) {
        return errorResponse("无效的用户 ID");
    }

    // 2. 参数验证
    int limit;
    if (!validateLimit(req, limit)) {
        return errorResponse("limit 参数无效（范围：1-100）");
    }

    qDebug() << "处理交易历史查询请求: userId =" << userId << ", limit =" << limit;

  

    // 3. 检查用户是否是管理员
    QSqlDatabase db;  // <--- 必须先声明 db 变量
    if (!Database::getThreadConnection(db)) {
        return errorResponse("无法获取数据库连接");
    }

    UserDAO userDao;
    QString username, cardNumber;
    bool isAdmin = false, isApproved = false;
    
    // 把上面获取到的 db 传给 findById 的第一个参数
    bool userExists = userDao.findById(db, userId.toLongLong(), username, cardNumber, isAdmin, isApproved);
    // 4. 根据用户身份调用不同的 Service 方法
    QJsonObject res;
    if (userExists && isAdmin) {
        // 管理员：查询所有交易记录
        qDebug() << "管理员查询所有交易记录";
        res = m_service.getAllTransactions(req);
    } else {
        // 普通用户：只查询自己的交易记录
        res = m_service.getTransactions(userId, req);
    }

    // 5. 记录结果
    if (res["status"] == "success") {
        QJsonArray transactions = res["transactions"].toArray();
        qDebug() << "交易历史查询成功: userId =" << userId << ", 记录数 =" << transactions.size();
    } else {
        qWarning() << "交易历史查询失败: userId =" << userId << ", reason:" << res["msg"].toString();
    }

    return res;
}

bool TransactionController::validateLimit(const QJsonObject& req, int& limit)
{
    // 默认值
    limit = 20;

    // 如果请求中包含 limit 参数
    if (req.contains("limit")) {
        int requestedLimit = req["limit"].toInt(0);

        // 验证范围
        if (requestedLimit <= 0 || requestedLimit > 100) {
            qWarning() << "limit 参数超出范围:" << requestedLimit;
            return false;
        }

        limit = requestedLimit;
    }

    return true;
}

QJsonObject TransactionController::errorResponse(const QString& msg)
{
    return {
        {"status", "error"},
        {"msg", msg}
    };
}
