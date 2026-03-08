#include "userdao.h"
#include "../database.h"
#include "../snowflake.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>

UserDAO::UserDAO() {}

QSqlDatabase UserDAO::getDatabase() {
    return Database::instance().getDatabase();
}

QString UserDAO::insert(const QString& username, const QString& passwordHash, const QString& salt,
                        bool isAdmin, bool isApproved) {
    qint64 id = Snowflake::instance().nextId();
    QString cardNumber = Database::instance().generateCardNumber();

    QSqlQuery query(getDatabase());
    query.prepare("INSERT INTO User (id, card_number, username, password_hash, salt, is_admin, is_approved) "
                  "VALUES (:id, :card, :name, :hash, :salt, :admin, :approved)");
    query.bindValue(":id", id);
    query.bindValue(":card", cardNumber);
    query.bindValue(":name", username);
    query.bindValue(":hash", passwordHash);
    query.bindValue(":salt", salt);
    query.bindValue(":admin", isAdmin ? 1 : 0);
    query.bindValue(":approved", isApproved ? 1 : 0);

    if (!query.exec()) {
        qCritical() << "Failed to insert user:" << query.lastError().text();
        return QString();
    }
    return cardNumber;
}

QString UserDAO::insertWithDetails(const QString& fullName, const QString& idCard, const QString& phone,
                                   const QString& birthDate, const QString& address,
                                   const QString& passwordHash, const QString& salt) {
    qint64 id = Snowflake::instance().nextId();
    QString cardNumber = Database::instance().generateCardNumber();
    // 生成用户名：使用卡号后8位
    QString username = cardNumber.right(8);  // 简单使用卡号后8位作为用户名

    QSqlQuery query(getDatabase());
    query.prepare("INSERT INTO User (id, card_number, username, password_hash, salt, is_admin, is_approved, "
                  "full_name, id_card, phone, birth_date, address) "
                  "VALUES (:id, :card, :name, :hash, :salt, 0, 1, :fullname, :idcard, :phone, :birth, :addr)");
    query.bindValue(":id", id);
    query.bindValue(":card", cardNumber);
    query.bindValue(":name", username);
    query.bindValue(":hash", passwordHash);
    query.bindValue(":salt", salt);
    query.bindValue(":fullname", fullName);
    query.bindValue(":idcard", idCard);
    query.bindValue(":phone", phone);
    query.bindValue(":birth", birthDate);
    query.bindValue(":addr", address);

    if (!query.exec()) {
        qCritical() << "Failed to insert user with details:" << query.lastError().text();
        return QString();
    }
    return cardNumber;
}

bool UserDAO::findByUsername(const QString& username, qint64& id, QString& cardNumber, QString& passwordHash, QString& salt,
                            bool* isAdmin, bool* isApproved, QString* fullName, QString* phone, QString* idCard, QString* birthDate) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT id, card_number, password_hash, salt, is_admin, is_approved, full_name, phone, id_card, birth_date FROM User WHERE username = :name");
    query.bindValue(":name", username);

    if (query.exec() && query.next()) {
        id = query.value(0).toLongLong();
        cardNumber = query.value(1).toString();
        passwordHash = query.value(2).toString();
        salt = query.value(3).toString();
        if (isAdmin) *isAdmin = query.value(4).toBool();
        if (isApproved) *isApproved = query.value(5).toBool();
        if (fullName) *fullName = query.value(6).toString();
        if (phone) *phone = query.value(7).toString();
        if (idCard) *idCard = query.value(8).toString();
        if (birthDate) *birthDate = query.value(9).toString();
        return true;
    }
    return false;
}

bool UserDAO::findByCardNumber(const QString& cardNumber, qint64& id, QString& username, QString& passwordHash, QString& salt,
                              bool* isAdmin, bool* isApproved, QString* fullName, QString* phone, QString* idCard, QString* birthDate) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT id, username, password_hash, salt, is_admin, is_approved, full_name, phone, id_card, birth_date FROM User WHERE card_number = :card");
    query.bindValue(":card", cardNumber);

    if (query.exec() && query.next()) {
        id = query.value(0).toLongLong();
        username = query.value(1).toString();
        passwordHash = query.value(2).toString();
        salt = query.value(3).toString();
        if (isAdmin) *isAdmin = query.value(4).toBool();
        if (isApproved) *isApproved = query.value(5).toBool();
        if (fullName) *fullName = query.value(6).toString();
        if (phone) *phone = query.value(7).toString();
        if (idCard) *idCard = query.value(8).toString();
        if (birthDate) *birthDate = query.value(9).toString();
        return true;
    }
    return false;
}

bool UserDAO::exists(const QString& username) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT COUNT(*) FROM User WHERE username = :name");
    query.bindValue(":name", username);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}

bool UserDAO::findById(qint64 id, QString& username, QString& cardNumber, bool& isAdmin, bool& isApproved) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT username, card_number, is_admin, is_approved FROM User WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) {
        username = query.value(0).toString();
        cardNumber = query.value(1).toString();
        isAdmin = query.value(2).toBool();
        isApproved = query.value(3).toBool();
        return true;
    }
    return false;
}

bool UserDAO::findIdByCardOrUsername(const QString& val, qint64& id) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT id FROM User WHERE card_number = :val OR username = :val");
    query.bindValue(":val", val);
    if (query.exec() && query.next()) {
        id = query.value(0).toLongLong();
        return true;
    }
    return false;
}

bool UserDAO::isAdmin(qint64 id) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT is_admin FROM User WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec() && query.next() && query.value(0).toBool();
}

bool UserDAO::isApproved(qint64 id) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT is_approved FROM User WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec() && query.next() && query.value(0).toBool();
}

bool UserDAO::setAdmin(qint64 id, bool isAdmin) {
    QSqlQuery query(getDatabase());
    query.prepare("UPDATE User SET is_admin = :admin WHERE id = :id");
    query.bindValue(":admin", isAdmin ? 1 : 0);
    query.bindValue(":id", id);
    return query.exec();
}

bool UserDAO::setApproved(qint64 id, bool isApproved) {
    QSqlQuery query(getDatabase());
    query.prepare("UPDATE User SET is_approved = :approved WHERE id = :id");
    query.bindValue(":approved", isApproved ? 1 : 0);
    query.bindValue(":id", id);
    return query.exec();
}

QList<QVariantMap> UserDAO::getPendingUsers() {
    QList<QVariantMap> users;
    QSqlQuery query(getDatabase());
    query.exec("SELECT id, card_number, username, created_at FROM User WHERE is_approved = 0 ORDER BY created_at ASC");
    while (query.next()) {
        QVariantMap u;
        u["id"] = QString::number(query.value(0).toLongLong());
        u["card_number"] = query.value(1).toString();
        u["username"] = query.value(2).toString();
        u["created_at"] = query.value(3).toString();
        users.append(u);
    }
    return users;
}

QList<QVariantMap> UserDAO::getAllUsers() {
    QList<QVariantMap> users;
    QSqlQuery query(getDatabase());
    query.exec("SELECT u.id, u.card_number, u.username, u.full_name, u.is_admin, u.is_approved, u.created_at, a.balance, "
               "u.phone, u.id_card, u.birth_date "
               "FROM User u LEFT JOIN Account a ON u.id = a.user_id ORDER BY u.created_at ASC");
    while (query.next()) {
        QVariantMap u;
        u["id"] = QString::number(query.value(0).toLongLong());
        u["card_number"] = query.value(1).toString();
        u["username"] = query.value(2).toString();
        u["full_name"] = query.value(3).toString();
        u["is_admin"] = query.value(4).toBool();
        u["is_approved"] = query.value(5).toBool();
        u["created_at"] = query.value(6).toString();
        u["balance"] = query.value(7).toDouble();
        u["phone"] = query.value(8).toString();
        u["id_card"] = query.value(9).toString();
        u["birth_date"] = query.value(10).toString();
        users.append(u);
    }
    return users;
}

bool UserDAO::updateUserInfo(qint64 id, const QString& newUsername, int isAdmin, int isApproved) {
    QString sql = "UPDATE User SET id=id";
    if (!newUsername.isEmpty()) sql += ", username = :name";
    if (isAdmin != -1) sql += ", is_admin = :admin";
    if (isApproved != -1) sql += ", is_approved = :approved";
    sql += " WHERE id = :id";

    QSqlQuery query(getDatabase());
    query.prepare(sql);
    if (!newUsername.isEmpty()) query.bindValue(":name", newUsername);
    if (isAdmin != -1) query.bindValue(":admin", isAdmin);
    if (isApproved != -1) query.bindValue(":approved", isApproved);
    query.bindValue(":id", id);
    return query.exec();
}

bool UserDAO::updateUserProfile(qint64 id, const QString& fullName, const QString& idCard, const QString& phone, const QString& birthDate) {
    QSqlQuery query(getDatabase());
    query.prepare("UPDATE User SET full_name = :fullname, id_card = :idcard, phone = :phone, birth_date = :birth WHERE id = :id");
    query.bindValue(":fullname", fullName);
    query.bindValue(":idcard", idCard);
    query.bindValue(":phone", phone);
    query.bindValue(":birth", birthDate);
    query.bindValue(":id", id);
    return query.exec();
}

bool UserDAO::updatePassword(qint64 id, const QString& newPasswordHash, const QString& newSalt) {
    QSqlQuery query(getDatabase());
    query.prepare("UPDATE User SET password_hash = :hash, salt = :salt WHERE id = :id");
    query.bindValue(":hash", newPasswordHash);
    query.bindValue(":salt", newSalt);
    query.bindValue(":id", id);
    return query.exec();
}

bool UserDAO::getPasswordInfo(qint64 id, QString& passwordHash, QString& salt) {
    QSqlQuery query(getDatabase());
    query.prepare("SELECT password_hash, salt FROM User WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) {
        passwordHash = query.value(0).toString();
        salt = query.value(1).toString();
        return true;
    }
    return false;
}
