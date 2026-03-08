#ifndef ADMINWIDGET_H
#define ADMINWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QDateEdit>
#include <QLabel>
#include "tcpclient.h"

/**
 * @brief 管理员管理界面
 *
 * 功能：
 * - Tab 1: 添加用户
 * - Tab 2: 所有用户管理（含重置密码）
 */
class AdminWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdminWidget(TcpClient* client, QWidget* parent = nullptr);
    ~AdminWidget() = default;

signals:
    void closeRequested();

private slots:
    void onResponseReceived(const QJsonObject& response);
    void onRefreshAllUsers();
    void onAddUser();
    void onResetPassword();
    void onModifyUserBalance();
    void onModifyPassword();
    void onModifyUserInfo();

private:
    void setupUI();
    void setupAddUserTab();
    void setupAllUsersTab();
    void updateAllUsersTable(const QJsonArray& users);

    TcpClient* m_client;
    QString m_token;

    // Tab widget
    QTabWidget* m_tabWidget;

    // Tab 1: 添加用户
    QLineEdit* m_fullNameEdit;
    QLineEdit* m_idCardEdit;
    QLineEdit* m_phoneEdit;
    QDateEdit* m_birthDateEdit;
    QLineEdit* m_addressEdit;
    QPushButton* m_addUserBtn;
    QLabel* m_addUserStatusLabel;

    // Tab 2: 所有用户
    QTableWidget* m_allUsersTable;
    QPushButton* m_refreshAllBtn;
    QPushButton* m_resetPasswordBtn;
    QPushButton* m_modifyBalanceBtn;
    QPushButton* m_modifyPasswordBtn;
    QPushButton* m_modifyInfoBtn;
    QLabel* m_allUsersStatusLabel;

    // 修改对话框
    void showModifyBalanceDialog(const QString& userId, const QString& username, double currentBalance);
    void showModifyPasswordDialog(const QString& userId, const QString& username);
    void showModifyInfoDialog(const QString& userId, const QString& username, const QString& fullName, 
                             const QString& phone, const QString& idCard, const QString& birthDate);
};

#endif // ADMINWIDGET_H
