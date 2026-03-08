#include "userservice.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>

UserService::UserService(QObject* parent)
    : QObject(parent)
    , m_userDao()
    , m_accountDao()
{
}

QJsonObject UserService::registerUser(const QJsonObject& req)
{
    if (!req.contains("username") || !req.contains("password")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    QString username = req["username"].toString();
    QString password = req["password"].toString();

    if (password.length() != 6) {
        return {{"status", "error"}, {"msg", "密码必须为6位数字"}};
    }

    bool isDigitOnly = true;
    for (const QChar &ch : password) {
        if (!ch.isDigit()) {
            isDigitOnly = false;
            break;
        }
    }
    if (!isDigitOnly) {
        return {{"status", "error"}, {"msg", "密码必须全部为数字"}};
    }

    if (m_userDao.exists(username)) {
        return {{"status", "error"}, {"msg", "用户名已存在"}};
    }

    QString salt = generateSalt();
    QString passwordHash = hashPassword(password, salt);

    QString cardNumber = m_userDao.insert(username, passwordHash, salt, false, false);
    if (cardNumber.isEmpty()) {
        return {{"status", "error"}, {"msg", "用户创建失败"}};
    }

    qint64 userId;
    m_userDao.findIdByCardOrUsername(username, userId);
    
    // 为用户创建账户
    m_accountDao.create(userId, 1000.0);

    return {
        {"status", "success"},
        {"msg", "注册成功，请等待管理员审批"},
        {"card_number", cardNumber}
    };
}

QJsonObject UserService::login(const QJsonObject& req)
{
    if (!req.contains("username") || !req.contains("password")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    QString account = req["username"].toString(); // 可能是用户名或卡号
    QString password = req["password"].toString();

    qint64 id;
    QString username, cardNumber, storedHash, salt, fullName, phone, idCard, birthDate;
    bool isAdmin, isApproved;

    // 尝试按卡号找
    bool found = m_userDao.findByCardNumber(account, id, username, storedHash, salt, &isAdmin, &isApproved, &fullName, &phone, &idCard, &birthDate);
    if (!found) {
        // 尝试按用户名找
        found = m_userDao.findByUsername(account, id, cardNumber, storedHash, salt, &isAdmin, &isApproved, &fullName, &phone, &idCard, &birthDate);
    }

    if (!found) {
        return {{"status", "error"}, {"msg", "用户不存在"}};
    }

    if (hashPassword(password, salt) != storedHash) {
        return {{"status", "error"}, {"msg", "密码错误"}};
    }

    if (!isAdmin && !isApproved) {
        return {{"status", "error"}, {"msg", "账户未审批"}};
    }

    return {
        {"status", "success"},
        {"msg", "登录成功"},
        {"user_id", QString::number(id)},
        {"card_number", cardNumber},
        {"username", username},
        {"full_name", fullName},
        {"phone", phone},
        {"id_card", idCard},
        {"birth_date", birthDate},
        {"is_admin", isAdmin}
    };
}

QString UserService::hashPassword(const QString& password, const QString& salt)
{
    return QCryptographicHash::hash((password + salt).toUtf8(), QCryptographicHash::Sha256).toHex();
}

QString UserService::generateSalt()
{
    QByteArray salt;
    for (int i = 0; i < 16; i++) {
        salt.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    return salt.toHex();
}

QJsonObject UserService::changePassword(const QJsonObject& req)
{
    if (!req.contains("user_id") || !req.contains("old_password") || !req.contains("new_password")) {
        return {{"status", "error"}, {"msg", "缺少必要参数"}};
    }

    qint64 userId = req["user_id"].toString().toLongLong();
    QString oldPassword = req["old_password"].toString();
    QString newPassword = req["new_password"].toString();

    if (newPassword.length() != 6) {
        return {{"status", "error"}, {"msg", "新密码必须为6位数字"}};
    }

    bool isDigitOnly = true;
    for (const QChar &ch : newPassword) {
        if (!ch.isDigit()) {
            isDigitOnly = false;
            break;
        }
    }
    if (!isDigitOnly) {
        return {{"status", "error"}, {"msg", "新密码必须全部为数字"}};
    }

    // 获取当前密码哈希和盐
    QString currentHash, currentSalt;
    if (!m_userDao.getPasswordInfo(userId, currentHash, currentSalt)) {
        return {{"status", "error"}, {"msg", "用户不存在"}};
    }

    // 验证旧密码
    if (hashPassword(oldPassword, currentSalt) != currentHash) {
        return {{"status", "error"}, {"msg", "旧密码错误"}};
    }

    // 生成新密码哈希
    QString newSalt = generateSalt();
    QString newHash = hashPassword(newPassword, newSalt);

    // 更新密码
    if (!m_userDao.updatePassword(userId, newHash, newSalt)) {
        return {{"status", "error"}, {"msg", "密码修改失败"}};
    }

    return {{"status", "success"}, {"msg", "密码修改成功"}};
}
