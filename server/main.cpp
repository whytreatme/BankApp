#include <QCoreApplication>
#include <QDebug>
#include <QCommandLineParser>

#include "database.h"
#include "tcpreactor.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 设置应用信息
    QCoreApplication::setApplicationName("BankServer");
    QCoreApplication::setApplicationVersion("1.0.0");

    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("Bank Server - Qt6 Reactor Banking System");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  "Port to listen on (default: 9000)",
                                  "port", "9000");
    parser.addOption(portOption);

    QCommandLineOption dbHostOption("host", "MySQL host (default: 127.0.0.1)", "host", "127.0.0.1");
    QCommandLineOption dbPortOption("dbport", "MySQL port (default: 3306)", "dbport", "3306");
    QCommandLineOption dbNameOption("dbname", "MySQL database name", "dbname", "bank_db");
    QCommandLineOption dbUserOption("user", "MySQL username", "user", "bank");
    QCommandLineOption dbPassOption("pass", "MySQL password", "pass", "123456");

    parser.addOption(dbHostOption);
    parser.addOption(dbPortOption);
    parser.addOption(dbNameOption);
    parser.addOption(dbUserOption);
    parser.addOption(dbPassOption);

    parser.process(app);

    // 获取参数
    quint16 port = parser.value(portOption).toUShort();
    
    DbConfig config;
    config.host = parser.value(dbHostOption);
    config.port = parser.value(dbPortOption).toInt();
    config.databaseName = parser.value(dbNameOption);
    config.username = parser.value(dbUserOption);
    config.password = parser.value(dbPassOption);

    qDebug() << "========================================";
    qDebug() << "       Bank Server v1.0.0 (MySQL)";
    qDebug() << "========================================";
    qDebug() << "Configuration:";
    qDebug() << "  Server Port:" << port;
    qDebug() << "  MySQL Host: " << config.host;
    qDebug() << "  MySQL DB:   " << config.databaseName;
    qDebug() << "========================================\n";

    // ==================== Step 1: 初始化数据库 ====================
    qDebug() << "[Step 1] Initializing database...";
    if (!Database::instance().init(config)) {
        qCritical() << "ERROR: Failed to initialize database!";
        return 1;
    }
    qDebug() << "SUCCESS: Database initialized:" << config.databaseName;

    // ==================== Step 2: 启动 TCP Reactor ====================
    qDebug() << "\n[Step 2] Starting TCP Reactor...";
    TcpReactor reactor(port);
    if (!reactor.start()) {
        qCritical() << "ERROR: Failed to start TCP Reactor!";
        qCritical() << "Please check if port" << port << "is already in use.";
        return 1;
    }

    qDebug() << "\n========================================";
    qDebug() << "  Server is running!";
    qDebug() << "  Waiting for client connections...";
    qDebug() << "========================================\n";
    qDebug() << "Press Ctrl+C to stop the server.\n";

    // ==================== Step 3: 进入事件循环 ====================
    int exitCode = app.exec();

    // 清理资源
    qDebug() << "\nShutting down server...";
    reactor.stop();

    qDebug() << "Server stopped. Goodbye!";
    return exitCode;
}
