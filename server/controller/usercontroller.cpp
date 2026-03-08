#include "usercontroller.h"
#include <QDebug>
#include <QRegularExpression>

UserController::UserController(QObject* parent)
    : QObject(parent)
    , m_service()
{
}

QJsonObject UserController::registerUser(const QJsonObject& req)
{
    // 1. 参数验证
    if (!req.contains("username") || !req.contains("password")) {
        return errorResponse("缺少必要参数：username 或 password");
    }

    QString username = req["username"].toString();
    QString password = req["password"].toString();

    // 2. 格式验证
    if (!validateUsername(username)) {
        return errorResponse("用户名格式无效（长度 3-20 字符，仅限字母数字下划线）");
    }

    if (!validatePassword(password)) {
        return errorResponse("密码格式无效（长度 6-32 字符）");
    }

    qDebug() << "处理注册请求: username =" << username;

    // 3. 调用 Service 层
    QJsonObject res = m_service.registerUser(req);

    // 4. 记录结果
    if (res["status"] == "success") {
        qDebug() << "注册成功: username =" << username;
    } else {
        qWarning() << "注册失败: username =" << username << ", reason:" << res["msg"].toString();
    }

    return res;
}

QJsonObject UserController::login(const QJsonObject& req)
{
    // 1. 参数验证
    if (!req.contains("username") || !req.contains("password")) {
        return errorResponse("缺少必要参数：username 或 password");
    }

    QString username = req["username"].toString();
    QString password = req["password"].toString();

    // 2. 格式验证
    if (!validateUsername(username)) {
        return errorResponse("用户名格式无效");
    }

    if (!validatePassword(password)) {
        return errorResponse("密码格式无效");
    }

    qDebug() << "处理登录请求: username =" << username;

    // 3. 调用 Service 层
    QJsonObject res = m_service.login(req);

    // 4. 记录结果
    if (res["status"] == "success") {
        qDebug() << "登录成功: username =" << username;
    } else {
        qWarning() << "登录失败: username =" << username << ", reason:" << res["msg"].toString();
    }

    return res;
}

bool UserController::validateUsername(const QString& username)
{
    // 用户名：3-20 字符，仅限字母、数字、下划线
    if (username.length() < 3 || username.length() > 20) {
        return false;
    }

    // 检查是否只包含字母、数字、下划线
    QRegularExpression regex("^[a-zA-Z0-9_]+$");
    return regex.match(username).hasMatch();
}

bool UserController::validatePassword(const QString& password)
{
    // 密码：6-32 字符
    int length = password.length();
    return length >= 6 && length <= 32;
}

QJsonObject UserController::errorResponse(const QString& msg)
{
    return {
        {"status", "error"},
        {"msg", msg}
    };
}

QJsonObject UserController::changePassword(const QJsonObject& req)
{
    return m_service.changePassword(req);
}
