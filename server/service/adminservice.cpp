#include "adminservice.h"
#include <QDebug>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QCryptographicHash>

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

    QString username, cardNumber;
    bool isAdmin, isApproved;

    if (!m_userDao.findById(userId, username, cardNumber, isAdmin, isApproved)) {
        return {{"status", "error"}, {"msg", "用户不存在"}};
    }

    if (!m_userDao.setApproved(userId, approve)) {
        return {{"status", "error"}, {"msg", "更新失败"}};
    }

    return {{"status", "success"}, {"msg", "操作成功"}};
}

QJsonObject AdminService::getPendingUsers(const QJsonObject& req)
{
    Q_UNUSED(req);
    QList<QVariantMap> users = m_userDao.getPendingUsers();
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
    QList<QVariantMap> users = m_userDao.getAllUsers();
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

    double currentBalance;
    if (!m_accountDao.getBalance(userId, currentBalance)) {
        return {{"status", "error"}, {"msg", "账户查询失败"}};
    }

    if (!m_accountDao.updateBalance(userId, newBalance - currentBalance)) {
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

    if (!m_userDao.updateUserInfo(userId, newUsername, isAdmin, isApproved)) {
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
    for (int i = 0; i < 16; i++) {  //生成16位随机数盐值
        saltBytes.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    QString salt = saltBytes.toHex();
    qDebug() << "Generated salt for new user:" << salt << "length:" << salt.length();
    
    QString passwordHash = QCryptographicHash::hash((password + salt).toUtf8(), QCryptographicHash::Sha256).toHex();

    // 插入用户
    QString cardNumber = m_userDao.insertWithDetails(fullName, idCard, phone, birthDate, address, passwordHash, salt);
    if (cardNumber.isEmpty()) {
        return {{"status", "error"}, {"msg", "用户创建失败"}};
    }

    // 获取用户ID并创建账户
    qint64 userId;
    if (m_userDao.findIdByCardOrUsername(cardNumber, userId)) {
        m_accountDao.create(userId, 0.0);  // 初始余额为0
    }

    return {
        {"status", "success"},
        {"msg", "用户创建成功"},
        {"card_number", cardNumber},
        {"password", password}
    };
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

    if (!m_userDao.updatePassword(userId, newPasswordHash, newSalt)) {
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

    if (!m_userDao.updateUserProfile(userId, fullName, idCard, phone, birthDate)) {
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
