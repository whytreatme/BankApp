#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

/**
 * @brief 数据库单例类 - 管理 SQLite 连接和表初始化
 *
 * 提供数据库初始化、连接管理的单例接口。
 * 使用 SQLite 作为数据库引擎，创建 User、Account、Transaction 三个核心表。
 */
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDebug>
#include <QUuid>
#include <QRandomGenerator>
#include <QCryptographicHash>

/**
 * @brief 数据库连接配置结构体
 */
struct DbConfig {
    QString host;
    int port;
    QString databaseName;
    QString username;
    QString password;
};

class Database
{
public:
    static Database& instance();

    /**
     * @brief 初始化数据库（连接 MySQL 并创建表）
     * @param config 数据库配置
     * @return true 表示初始化成功，false 表示失败
     */
    bool init(const DbConfig& config);

    /**
     * @brief 获取数据库连接
     */
    QSqlDatabase getDatabase();

    /**
     * @brief 关闭数据库连接
     */
    void close();

    /**
     * @brief 生成唯一的 16 位银行卡号
     */
    QString generateCardNumber();

private:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool createUserTable();
    bool createAccountTable();
    bool createTransactionTable();
    bool createDefaultAdmin();

    QSqlDatabase m_db;
    bool m_initialized;
};

#endif // DATABASE_H
