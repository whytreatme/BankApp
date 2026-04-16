#ifndef USERDAO_H
#define USERDAO_H

#include <QString>
#include <QVariantMap>
#include <QSqlDatabase>
#include <QList>

class UserDAO
{
public:
    UserDAO();
    ~UserDAO() = default;

    qint64 insert(QSqlDatabase& db, const QString& username, const QString& cardNumber, const QString& passwordHash, const QString& salt,
                  bool isAdmin = false, bool isApproved = false);

    qint64 insertWithDetails(QSqlDatabase& db, const QString& fullName, const QString& idCard, const QString& phone,
                             const QString& birthDate, const QString& address,
                             const QString& cardNumber, const QString& username,
                             const QString& passwordHash, const QString& salt);

    bool findByUsername(QSqlDatabase& db, const QString& username, qint64& id, QString& cardNumber, QString& passwordHash, QString& salt,
                       bool* isAdmin = nullptr, bool* isApproved = nullptr, QString* fullName = nullptr,
                       QString* phone = nullptr, QString* idCard = nullptr, QString* birthDate = nullptr);

    bool findByCardNumber(QSqlDatabase& db, const QString& cardNumber, qint64& id, QString& username, QString& passwordHash, QString& salt,
                          bool* isAdmin = nullptr, bool* isApproved = nullptr, QString* fullName = nullptr,
                          QString* phone = nullptr, QString* idCard = nullptr, QString* birthDate = nullptr);

    bool exists(QSqlDatabase& db, const QString& username);

    bool findById(QSqlDatabase& db, qint64 id, QString& username, QString& cardNumber, bool& isAdmin, bool& isApproved);

    bool findIdByCardOrUsername(QSqlDatabase& db, const QString& val, qint64& id);

    bool isAdmin(QSqlDatabase& db, qint64 id);
    bool isApproved(QSqlDatabase& db, qint64 id);
    bool setAdmin(QSqlDatabase& db, qint64 id, bool isAdmin);
    bool setApproved(QSqlDatabase& db, qint64 id, bool isApproved);

    QList<QVariantMap> getPendingUsers(QSqlDatabase& db);
    QList<QVariantMap> getAllUsers(QSqlDatabase& db);

    bool updateUserInfo(QSqlDatabase& db, qint64 id, const QString& newUsername = QString(),
                       int isAdmin = -1, int isApproved = -1);

    bool updateUserProfile(QSqlDatabase& db, qint64 id, const QString& fullName, const QString& idCard,
                           const QString& phone, const QString& birthDate);

    bool updatePassword(QSqlDatabase& db, qint64 id, const QString& newPasswordHash, const QString& newSalt);

    bool getPasswordInfo(QSqlDatabase& db, qint64 id, QString& passwordHash, QString& salt);

    QHash<qint64, QVariantMap> findByIds(QSqlDatabase& db, const QSet<qint64>& ids);
};

#endif // USERDAO_H
