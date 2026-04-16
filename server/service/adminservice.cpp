#include "adminservice.h"
#include "userservice.h"
#include <QDebug>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include "../database.h"
#include <QSqlError>
#include <QSqlDatabase>
#include <stdexcept>

AdminService::AdminService(QObject* parent)
    : QObject(parent)
    , m_userDao()
    , m_accountDao()
{
}

QJsonObject AdminService::approveUser(const QJsonObject& req)
{
    if (!req.contains("user_id") || !req.contains("approve")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    qint64 userId = req["user_id"].toString().toLongLong();
    bool approve = req["approve"].toBool();

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    QString username, cardNumber;
    bool isAdmin, isApproved;

    if (!m_userDao.findById(userId, username, cardNumber, isAdmin, isApproved, db)) {
        return {{"status", "error"}, {"msg", "用户不存在"}};
    }

    if (!m_userDao.setApproved(userId, approve, db)) {
        return {{"status", "error"}, {"msg", "更新失败"}};
    }

    return {{"status", "success"}, {"msg", "操作成功"}};
}

QJsonObject AdminService::getPendingUsers(const QJsonObject& req)
{
    Q_UNUSED(req);

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    QList<QVariantMap> users = m_userDao.getPendingUsers(db);
    QJsonArray usersArray;
    for (const QVariantMap& user : users) {
        QJsonObject obj;
        obj["id"] = user["id"].toString();
        obj["card_number"] = user["card_number"].toString();
        obj["username"] = user["username"].toString();
        obj["created_at"] = user["created_at"].toString();
        usersArray.append(obj);
    }
    return {{"status", "success"}, {"users", usersArray}};
}

QJsonObject AdminService::getAllUsers(const QJsonObject& req)
{
    Q_UNUSED(req);

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    QList<QVariantMap> users = m_userDao.getAllUsers(db);
    QJsonArray usersArray;
    for (const QVariantMap& user : users) {
        QJsonObject obj;
        obj["id"] = user["id"].toString();
        obj["card_number"] = user["card_number"].toString();
        obj["username"] = user["username"].toString();
        obj["full_name"] = user["full_name"].toString();
        obj["phone"] = user["phone"].toString();
        obj["id_card"] = user["id_card"].toString();
        obj["birth_date"] = user["birth_date"].toString();
        obj["balance"] = user["balance"].toDouble();
        obj["is_admin"] = user["is_admin"].toBool();
        obj["is_approved"] = user["is_approved"].toBool();
        obj["created_at"] = user["created_at"].toString();
        usersArray.append(obj);
    }
    return {{"status", "success"}, {"users", usersArray}};
}

QJsonObject AdminService::setUserBalance(const QJsonObject& req)
{
    if (!req.contains("user_id") || !req.contains("new_balance")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    qint64 userId = req["user_id"].toString().toLongLong();
    double newBalance = req["new_balance"].toDouble();

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    double currentBalance;
    if (!m_accountDao.getBalance(userId, currentBalance, db)) {
        return {{"status", "error"}, {"msg", "账户查询失败"}};
    }

    if (!m_accountDao.updateBalance(userId, newBalance - currentBalance, db)) {
        return {{"status", "error"}, {"msg", "更新失败"}};
    }

    return {{"status", "success"}, {"msg", "余额已修改"}};
}

QJsonObject AdminService::updateUserInfo(const QJsonObject& req)
{
    if (!req.contains("user_id")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    qint64 userId = req["user_id"].toString().toLongLong();
    QString newUsername = req.value("username").toString();
    int isAdmin = req.contains("is_admin") ? (req["is_admin"].toBool() ? 1 : 0) : -1;
    int isApproved = req.contains("is_approved") ? (req["is_approved"].toBool() ? 1 : 0) : -1;

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    if (!m_userDao.updateUserInfo(userId, newUsername, isAdmin, isApproved, db)) {
        return {{"status", "error"}, {"msg", "更新失败"}};
    }

    return {{"status", "success"}, {"msg", "信息已更新"}};
}

QJsonObject AdminService::createUser(const QJsonObject& req)
{
    if (!req.contains("full_name") || !req.contains("id_card") ||
        !req.contains("phone") || !req.contains("birth_date") || !req.contains("address")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    QString fullName = req["full_name"].toString();
    QString idCard = req["id_card"].toString();
    QString phone = req["phone"].toString();
    QString birthDate = req["birth_date"].toString();
    QString address = req["address"].toString();

    // 生成随机6位数字密码
    QString password = QString::number(QRandomGenerator::global()->bounded(100000, 1000000));

    // 生成盐和密码哈希
    QByteArray saltBytes;
    for (int i = 0; i < 16; i++) {
        saltBytes.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    QString salt = saltBytes.toHex();
    qDebug() << "Generated salt for new user:" << salt << "length:" << salt.length();

    QString passwordHash = QCryptographicHash::hash((password + salt).toUtf8(), QCryptographicHash::Sha256).toHex();

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    // 开启事务
    if (!db.transaction()) {
        return {{"status", "error"}, {"msg", "系统错误：事务失败"}};
    }

    try {
        // 生成卡号
        QString cardNumber = UserService::generateCardNumber();

        // 生成用户名（使用卡号后8位）
        QString username = cardNumber.right(8);

        // 插入用户（返回 user_id）
        qint64 userId = m_userDao.insertWithDetails(fullName, idCard, phone, birthDate, address,
                                                     cardNumber, username, passwordHash, salt, db);
        if (userId == -1) {
            throw std::runtime_error("用户创建失败");
        }

        // 创建账户（初始余额为0）
        qint64 accountId = m_accountDao.create(userId, 0.0, db);
        if (accountId == -1) {
            throw std::runtime_error("账户创建失败");
        }

        // 提交事务
        if (!db.commit()) {
            throw std::runtime_error("系统错误：提交失败");
        }

        return {
            {"status", "success"},
            {"msg", "用户创建成功"},
            {"card_number", cardNumber},
            {"password", password}
        };
    }
    catch (const std::exception& e) {
        db.rollback();
        qCritical() << "createUser exception:" << e.what();
        return {{"status", "error"}, {"msg", "用户创建失败"}};
    }
    catch (...) {
        db.rollback();
        qCritical() << "createUser unknown exception";
        return {{"status", "error"}, {"msg", "用户创建失败"}};
    }
}

QJsonObject AdminService::resetPassword(const QJsonObject& req)
{
    if (!req.contains("user_id")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    qint64 userId = req["user_id"].toString().toLongLong();
    QString newPassword;

    if (req.contains("new_password")) {
        newPassword = req["new_password"].toString();
        if (newPassword.length() != 6) {
            return {{"status", "error"}, {"msg", "密码必须为6位数字"}};
        }
    } else {
        // 生成随机6位数字密码
        newPassword = QString::number(QRandomGenerator::global()->bounded(100000, 1000000));
    }

    // 生成新盐和密码哈希
    QByteArray saltBytes;
    for (int i = 0; i < 16; i++) {
        saltBytes.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    QString newSalt = saltBytes.toHex();

    QString newPasswordHash = QCryptographicHash::hash((newPassword + newSalt).toUtf8(), QCryptographicHash::Sha256).toHex();

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    if (!m_userDao.updatePassword(userId, newPasswordHash, newSalt, db)) {
        return {{"status", "error"}, {"msg", "密码修改失败"}};
    }

    return {
        {"status", "success"},
        {"msg", req.contains("new_password") ? "密码已修改" : "密码已重置"},
        {"password", newPassword}
    };
}

QJsonObject AdminService::updateProfile(const QJsonObject& req)
{
    if (!req.contains("user_id") || !req.contains("full_name") ||
        !req.contains("id_card") || !req.contains("phone") || !req.contains("birth_date")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    qint64 userId = req["user_id"].toString().toLongLong();
    QString fullName = req["full_name"].toString();
    QString idCard = req["id_card"].toString();
    QString phone = req["phone"].toString();
    QString birthDate = req["birth_date"].toString();

    if (fullName.isEmpty() || idCard.isEmpty() || phone.isEmpty() || birthDate.isEmpty()) {
        return {{"status", "error"}, {"msg", "所有字段都不能为空"}};
    }

    // 获取数据库连接
    QSqlDatabase db;
    if (!Database::getThreadConnection(db)) {
        return {{"status", "error"}, {"msg", "无法获取数据库连接"}};
    }

    if (!m_userDao.updateUserProfile(userId, fullName, idCard, phone, birthDate, db)) {
        return {{"status", "error"}, {"msg", "修改个人信息失败"}};
    }

    return {
        {"status", "success"},
        {"msg", "个人信息修改成功"},
        {"full_name", fullName},
        {"id_card", idCard},
        {"phone", phone},
        {"birth_date", birthDate}
    };
}
