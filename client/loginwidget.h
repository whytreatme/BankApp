#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>

class TcpClient;

/**
 * @brief 登录/注册界面
 *
 * 功能：
 * - 用户登录
 * - 用户注册
 * - 登录成功后发出信号
 */
class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param client TCP 客户端指针
     * @param parent 父窗口
     */
    explicit LoginWidget(TcpClient* client, QWidget* parent = nullptr);

    ~LoginWidget() = default;

signals:
    /**
     * @brief 登录成功信号
     * @param userInfo 用户信息（包含 user_id, username 等）
     */
    void loginSuccess(const QJsonObject& userInfo);

private slots:
    /**
     * @brief 处理连接按钮点击
     */
    void onConnectClicked();

    /**
     * @brief 处理登录按钮点击
     */
    void onLoginClicked();

    /**
     * @brief 处理管理员/用户切换按钮点击
     */
    void onSwitchModeClicked();

    /**
     * @brief 处理服务器响应
     * @param response JSON 响应数据
     */
    void onResponseReceived(const QJsonObject& response);

    /**
     * @brief 处理连接状态变化
     */
    void onConnected();

private:
    /**
     * @brief 初始化 UI
     */
    void setupUI();

    /**
     * @brief 显示消息提示
     * @param message 消息内容
     * @param isSuccess 是否为成功消息
     */
    void showMessage(const QString& message, bool isSuccess = true);

private:
    TcpClient* m_client;           // TCP 客户端
    QLineEdit* m_serverIpEdit;     // 服务器IP输入框
    QLineEdit* m_serverPortEdit;   // 服务器端口输入框
    QLineEdit* m_usernameEdit;     // 用户名输入框
    QLineEdit* m_passwordEdit;     // 密码输入框
    QPushButton* m_connectBtn;     // 连接按钮
    QPushButton* m_loginBtn;       // 登录按钮
    QPushButton* m_switchModeBtn;  // 切换管理员/用户模式按钮
    QLabel* m_messageLabel;        // 消息提示标签
    QLabel* m_connectionStatusLabel; // 连接状态标签
    QLabel* m_modeLabel;           // 模式提示标签
    bool m_isAdminMode;            // 是否为管理员模式
};

#endif // LOGINWIDGET_H
