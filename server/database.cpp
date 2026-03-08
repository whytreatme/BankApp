#include "database.h"
#include "snowflake.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDateTime>

Database::Database() : m_initialized(false) {}

Database::~Database() {
    close();
}

Database& Database::instance() {
    static Database inst;
    return inst;
}

bool Database::init(const DbConfig& config) {
    if (m_initialized) return true;

    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName(config.host);
    m_db.setPort(config.port);
    m_db.setDatabaseName(config.databaseName);
    m_db.setUserName(config.username);
    m_db.setPassword(config.password);

    if (!m_db.open()) {
        qCritical() << "Failed to connect to MySQL:" << m_db.lastError().text();
        return false;
    }

    if (!createUserTable() || !createAccountTable() || !createTransactionTable()) {
        return false;
    }

    if (!createDefaultAdmin()) {
        qWarning() << "Failed to create default admin";
    }

    m_initialized = true;
    return true;
}

QSqlDatabase Database::getDatabase() {
    return m_db;
}

void Database::close() {
    if (m_db.isOpen()) {
        m_db.close();
        m_initialized = false;
    }
}

bool Database::createUserTable() {
    QSqlQuery query(m_db);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS User (
            id BIGINT PRIMARY KEY,
            card_number CHAR(16) UNIQUE NOT NULL,
            username VARCHAR(50) UNIQUE NOT NULL,
            password_hash VARCHAR(128) NOT NULL,
            salt VARCHAR(64) NOT NULL,
            is_admin TINYINT(1) DEFAULT 0,
            is_approved TINYINT(1) DEFAULT 0,
            full_name VARCHAR(50),
            id_card VARCHAR(18),
            phone VARCHAR(15),
            birth_date DATE,
            address VARCHAR(255),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_card (card_number),
            INDEX idx_username (username)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";
    if (!query.exec(sql)) {
        qCritical() << "Failed to create User table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Database::createAccountTable() {
    QSqlQuery query(m_db);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS Account (
            id BIGINT PRIMARY KEY,
            user_id BIGINT NOT NULL,
            balance DECIMAL(18, 2) NOT NULL DEFAULT 0.00,
            status INT DEFAULT 1,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES User(id) ON DELETE CASCADE,
            INDEX idx_user_id (user_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";
    if (!query.exec(sql)) {
        qCritical() << "Failed to create Account table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool Database::createTransactionTable() {
    QSqlQuery query(m_db);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS Transaction (
            id BIGINT PRIMARY KEY,
            from_account_id BIGINT,
            to_account_id BIGINT,
            amount DECIMAL(18, 2) NOT NULL,
            type VARCHAR(20) NOT NULL,
            remark VARCHAR(255),
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_from (from_account_id),
            INDEX idx_to (to_account_id),
            INDEX idx_time (timestamp)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";
    if (!query.exec(sql)) {
        qCritical() << "Failed to create Transaction table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool Database::createDefaultAdmin() {
    QSqlQuery checkQuery(m_db);
    checkQuery.exec("SELECT COUNT(*) FROM User WHERE username = 'admin'");
    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) return true;

    qint64 id = Snowflake::instance().nextId();
    QString cardNumber = "6222000000000001"; // 固定管理卡号
    QString username = "admin";
    QString salt = QUuid::createUuid().toString(QUuid::WithoutBraces).left(16);
    QString password = "admin123";
    QByteArray hash = QCryptographicHash::hash((password + salt).toUtf8(), QCryptographicHash::Sha256);
    QString passwordHash = hash.toHex();

    QSqlQuery insertQuery(m_db);
    insertQuery.prepare("INSERT INTO User (id, card_number, username, password_hash, salt, is_admin, is_approved) "
                        "VALUES (:id, :card, :name, :hash, :salt, 1, 1)");
    insertQuery.bindValue(":id", id);
    insertQuery.bindValue(":card", cardNumber);
    insertQuery.bindValue(":name", username);
    insertQuery.bindValue(":hash", passwordHash);
    insertQuery.bindValue(":salt", salt);

    if (!insertQuery.exec()) {
        qCritical() << "Failed to insert admin user:" << insertQuery.lastError().text();
        return false;
    }

    // 创建账户
    QSqlQuery accQuery(m_db);
    accQuery.prepare("INSERT INTO Account (id, user_id, balance) VALUES (:id, :user_id, :balance)");
    accQuery.bindValue(":id", Snowflake::instance().nextId());
    accQuery.bindValue(":user_id", id);
    accQuery.bindValue(":balance", 0.00);
    return accQuery.exec();
}

QString Database::generateCardNumber() {
    // 简单实现：6222 + 12位随机数
    QString card = "6222";
    for(int i=0; i<12; ++i) {
        card.append(QString::number(QRandomGenerator::global()->bounded(10)));
    }
    return card;
}
