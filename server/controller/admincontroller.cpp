#include "admincontroller.h"

AdminController::AdminController(QObject* parent)
    : QObject(parent)
    , m_service()
{
}

QJsonObject AdminController::approveUser(const QJsonObject& req)
{
    return m_service.approveUser(req);
}

QJsonObject AdminController::getPendingUsers(const QJsonObject& req)
{
    return m_service.getPendingUsers(req);
}

QJsonObject AdminController::getAllUsers(const QJsonObject& req)
{
    return m_service.getAllUsers(req);
}

QJsonObject AdminController::setUserBalance(const QJsonObject& req)
{
    return m_service.setUserBalance(req);
}

QJsonObject AdminController::updateUserInfo(const QJsonObject& req)
{
    return m_service.updateUserInfo(req);
}

QJsonObject AdminController::createUser(const QJsonObject& req)
{
    return m_service.createUser(req);
}

QJsonObject AdminController::resetPassword(const QJsonObject& req)
{
    return m_service.resetPassword(req);
}

QJsonObject AdminController::updateProfile(const QJsonObject& req)
{
    return m_service.updateProfile(req);
}
