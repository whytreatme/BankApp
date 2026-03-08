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
     * @return 成功返回新用户的 16 位卡号，失败返回空字符串
     */
    QString insert(const QString& username, const QString& passwordHash, const QString& salt,
                   bool isAdmin = false, bool isApproved = false);
    
    /**
     * @brief 插入新用户（带完整信息）
     * @return 成功返回新用户的 16 位卡号，失败返回空字符串
     */
    QString insertWithDetails(const QString& fullName, const QString& idCard, const QString& phone,
                              const QString& birthDate, const QString& address,
                              const QString& passwordHash, const QString& salt);

    /**
     * @brief 根据用户名查询用户信息
     */
    bool findByUsername(const QString& username, qint64& id, QString& cardNumber, QString& passwordHash, QString& salt,
                       bool* isAdmin = nullptr, bool* isApproved = nullptr, QString* fullName = nullptr,
                       QString* phone = nullptr, QString* idCard = nullptr, QString* birthDate = nullptr);

    /**
     * @brief 根据卡号查询用户信息
     */
    bool findByCardNumber(const QString& cardNumber, qint64& id, QString& username, QString& passwordHash, QString& salt,
                          bool* isAdmin = nullptr, bool* isApproved = nullptr, QString* fullName = nullptr,
                          QString* phone = nullptr, QString* idCard = nullptr, QString* birthDate = nullptr);

    /**
     * @brief 检查用户名是否存在
     */
    bool exists(const QString& username);

    /**
     * @brief 根据用户ID查询用户信息
     */
    bool findById(qint64 id, QString& username, QString& cardNumber, bool& isAdmin, bool& isApproved);

    /**
     * @brief 根据卡号或用户名查询用户内部ID
     */
    bool findIdByCardOrUsername(const QString& val, qint64& id);

    bool isAdmin(qint64 id);
    bool isApproved(qint64 id);
    bool setAdmin(qint64 id, bool isAdmin);
    bool setApproved(qint64 id, bool isApproved);

    QList<QVariantMap> getPendingUsers();
    QList<QVariantMap> getAllUsers();

    bool updateUserInfo(qint64 id, const QString& newUsername = QString(),
                       int isAdmin = -1, int isApproved = -1);
    
    /**
     * @brief 用户修改个人信息
     */
    bool updateUserProfile(qint64 id, const QString& fullName, const QString& idCard, 
                           const QString& phone, const QString& birthDate);
    
    /**
     * @brief 更新用户密码
     */
    bool updatePassword(qint64 id, const QString& newPasswordHash, const QString& newSalt);
    
    /**
     * @brief 获取用户密码哈希和盐（用于验证旧密码）
     */
    bool getPasswordInfo(qint64 id, QString& passwordHash, QString& salt);

private:
    QSqlDatabase getDatabase();
};

#endif // USERDAO_H
