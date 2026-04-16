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

    /**
     * @brief 插入新用户
     * @param username 用户名
     * @param cardNumber 16位卡号
     * @param passwordHash 密码哈希
     * @param salt 盐值
     * @param isAdmin 是否为管理员
     * @param isApproved 是否已批准
     * @param db 数据库连接
     * @return 成功返回新用户的 ID，失败返回 -1
     */
    qint64 insert(const QString& username, const QString& cardNumber, const QString& passwordHash, const QString& salt,
                  bool isAdmin = false, bool isApproved = false, QSqlDatabase& db);

    /**
     * @brief 插入新用户（带完整信息）
     * @param fullName 全名
     * @param idCard 身份证号
     * @param phone 电话
     * @param birthDate 出生日期
     * @param address 地址
     * @param cardNumber 16位卡号
     * @param username 用户名
     * @param passwordHash 密码哈希
     * @param salt 盐值
     * @param db 数据库连接
     * @return 成功返回新用户的 ID，失败返回 -1
     */
    qint64 insertWithDetails(const QString& fullName, const QString& idCard, const QString& phone,
                             const QString& birthDate, const QString& address,
                             const QString& cardNumber, const QString& username,
                             const QString& passwordHash, const QString& salt, QSqlDatabase& db);

    /**
     * @brief 根据用户名查询用户信息
     */
    bool findByUsername(const QString& username, qint64& id, QString& cardNumber, QString& passwordHash, QString& salt,
                       bool* isAdmin = nullptr, bool* isApproved = nullptr, QString* fullName = nullptr,
                       QString* phone = nullptr, QString* idCard = nullptr, QString* birthDate = nullptr, QSqlDatabase& db);

    /**
     * @brief 根据卡号查询用户信息
     */
    bool findByCardNumber(const QString& cardNumber, qint64& id, QString& username, QString& passwordHash, QString& salt,
                          bool* isAdmin = nullptr, bool* isApproved = nullptr, QString* fullName = nullptr,
                          QString* phone = nullptr, QString* idCard = nullptr, QString* birthDate = nullptr, QSqlDatabase& db);

    /**
     * @brief 检查用户名是否存在
     */
    bool exists(const QString& username, QSqlDatabase& db);

    /**
     * @brief 根据用户ID查询用户信息
     */
    bool findById(qint64 id, QString& username, QString& cardNumber, bool& isAdmin, bool& isApproved, QSqlDatabase& db);

    /**
     * @brief 根据卡号或用户名查询用户内部ID
     */
    bool findIdByCardOrUsername(const QString& val, qint64& id, QSqlDatabase& db);

    bool isAdmin(qint64 id, QSqlDatabase& db);
    bool isApproved(qint64 id, QSqlDatabase& db);
    bool setAdmin(qint64 id, bool isAdmin, QSqlDatabase& db);
    bool setApproved(qint64 id, bool isApproved, QSqlDatabase& db);

    QList<QVariantMap> getPendingUsers(QSqlDatabase& db);
    QList<QVariantMap> getAllUsers(QSqlDatabase& db);

    bool updateUserInfo(qint64 id, const QString& newUsername = QString(),
                       int isAdmin = -1, int isApproved = -1, QSqlDatabase& db);

    /**
     * @brief 用户修改个人信息
     */
    bool updateUserProfile(qint64 id, const QString& fullName, const QString& idCard,
                           const QString& phone, const QString& birthDate, QSqlDatabase& db);

    /**
     * @brief 更新用户密码
     */
    bool updatePassword(qint64 id, const QString& newPasswordHash, const QString& newSalt, QSqlDatabase& db);

    /**
     * @brief 获取用户密码哈希和盐（用于验证旧密码）
     */
    bool getPasswordInfo(qint64 id, QString& passwordHash, QString& salt, QSqlDatabase& db);

    /**
     * @brief 根据 ID 集合查询多个用户信息
     * @return QHash，键为用户 ID，值为包含 id, username, card_number 的 QVariantMap
     */
    QHash<qint64, QVariantMap> findByIds(const QSet<qint64>& ids, QSqlDatabase& db);
};

#endif // USERDAO_H
