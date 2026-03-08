#include "loginwidget.h"
#include "tcpclient.h"
#include "protocolutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QTimer>
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QRegularExpression>

LoginWidget::LoginWidget(TcpClient* client, QWidget* parent)
    : QWidget(parent)
    , m_client(client)
    , m_isAdminMode(false)
{
    setupUI();

    // 连接客户端响应信号
    connect(m_client, &TcpClient::responseReceived,
            this, &LoginWidget::onResponseReceived);

    // 连接连接状态信号
    connect(m_client, &TcpClient::connected,
            this, &LoginWidget::onConnected);

    connect(m_client, &TcpClient::disconnected, [this]() {
        m_connectionStatusLabel->setText("状态: 未连接");
        m_connectionStatusLabel->setStyleSheet("QLabel { color: red; }");
        m_loginBtn->setEnabled(false);
        m_switchModeBtn->setEnabled(true);
        m_connectBtn->setEnabled(true);
        m_connectBtn->setText("连接");
    });

    connect(m_client, &TcpClient::error, [this](const QString& msg) {
        showMessage("连接错误: " + msg, false);
        m_connectBtn->setEnabled(true);
        m_connectBtn->setText("连接");
    });
}

void LoginWidget::setupUI()
{
    // 创建窗口标题
    setWindowTitle("银行系统 - 登录");
    setMinimumSize(550, 450);

    // 创建服务器设置控件
    m_serverIpEdit = new QLineEdit(this);
    m_serverIpEdit->setPlaceholderText("例如: 127.0.0.1 或 localhost");
    m_serverIpEdit->setText("localhost");
    m_serverIpEdit->setMaxLength(50);
    m_serverIpEdit->setMinimumHeight(35);

    m_serverPortEdit = new QLineEdit(this);
    m_serverPortEdit->setPlaceholderText("例如: 9000");
    m_serverPortEdit->setText("9000");
    m_serverPortEdit->setValidator(new QIntValidator(1, 65535, this));
    m_serverPortEdit->setMinimumHeight(35);

    m_connectBtn = new QPushButton("连接", this);
    m_connectBtn->setMinimumHeight(35);

    m_connectionStatusLabel = new QLabel("状态: 未连接", this);
    m_connectionStatusLabel->setStyleSheet("QLabel { color: red; }");

    // 创建用户名密码控件
    m_modeLabel = new QLabel("当前模式: 用户登录（16位卡号）", this);
    m_modeLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
    
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入16位银行卡号");
    m_usernameEdit->setMaxLength(16);
    m_usernameEdit->setMinimumHeight(45);
    m_usernameEdit->setMinimumWidth(300);
    m_usernameEdit->setStyleSheet("QLineEdit { font-size: 16px; }");
    // 用户模式只允许16位数字
    m_usernameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]{16}$"), this));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("请输入6位数字密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMaxLength(50);
    m_passwordEdit->setMinimumHeight(45);
    m_passwordEdit->setMinimumWidth(300);
    m_passwordEdit->setStyleSheet("QLineEdit { font-size: 16px; }");

    m_loginBtn = new QPushButton("登录", this);
    m_loginBtn->setMinimumHeight(40);
    m_loginBtn->setEnabled(false);  // 初始禁用，连接后启用

    m_switchModeBtn = new QPushButton("切换为管理员登录", this);
    m_switchModeBtn->setMinimumHeight(40);
    m_switchModeBtn->setStyleSheet("QPushButton { background-color: #1976D2; color: white; }");

    m_messageLabel = new QLabel("", this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setAlignment(Qt::AlignCenter);

    // 创建布局
    QFormLayout* serverLayout = new QFormLayout();
    serverLayout->setFormAlignment(Qt::AlignCenter);
    serverLayout->setLabelAlignment(Qt::AlignRight);
    serverLayout->addRow("服务器地址:", m_serverIpEdit);
    serverLayout->addRow("端口:", m_serverPortEdit);

    QHBoxLayout* connectLayout = new QHBoxLayout();
    connectLayout->addWidget(m_connectBtn);
    connectLayout->addWidget(m_connectionStatusLabel);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setFormAlignment(Qt::AlignCenter);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setSpacing(15);
    formLayout->addRow(m_modeLabel);
    formLayout->addRow("用户名:", m_usernameEdit);
    formLayout->addRow("密码:", m_passwordEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_loginBtn);
    buttonLayout->addWidget(m_switchModeBtn);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch();
    mainLayout->addLayout(serverLayout);
    mainLayout->addLayout(connectLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_messageLabel);
    mainLayout->addStretch();

    // 设置边距
    mainLayout->setContentsMargins(40, 40, 40, 40);

    // 连接按钮信号
    connect(m_connectBtn, &QPushButton::clicked,
            this, &LoginWidget::onConnectClicked);
    connect(m_loginBtn, &QPushButton::clicked,
            this, &LoginWidget::onLoginClicked);
    connect(m_switchModeBtn, &QPushButton::clicked,
            this, &LoginWidget::onSwitchModeClicked);

    // 允许回车键登录
    m_passwordEdit->setEnabled(true);
    connect(m_passwordEdit, &QLineEdit::returnPressed,
            this, &LoginWidget::onLoginClicked);
}

void LoginWidget::onLoginClicked()
{
    // 获取输入
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();

    // 验证输入
    if (username.isEmpty() || password.isEmpty()) {
        showMessage("用户名和密码不能为空！", false);
        return;
    }

    // 检查连接状态
    if (!m_client->isConnected()) {
        showMessage("未连接到服务器，请稍后重试！", false);
        return;
    }

    // 构造登录请求（type=2）
    QJsonObject requestData;
    requestData["username"] = username;
    requestData["password"] = password;

    // 发送登录请求（不需要 Token）
    m_client->sendRequest(2, "", requestData);

    // 显示处理中消息
    showMessage("正在登录...", true);
    m_loginBtn->setEnabled(false);
    m_switchModeBtn->setEnabled(false);
}

void LoginWidget::onSwitchModeClicked()
{
    m_isAdminMode = !m_isAdminMode;
    
    if (m_isAdminMode) {
        // 切换为管理员模式
        m_modeLabel->setText("当前模式: 管理员登录");
        m_modeLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        m_usernameEdit->setPlaceholderText("请输入管理员用户名");
        m_usernameEdit->setMaxLength(50);
        m_usernameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_]{3,20}$"), this));
        m_switchModeBtn->setText("切换为用户登录");
        m_switchModeBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
    } else {
        // 切换为用户模式
        m_modeLabel->setText("当前模式: 用户登录（16位卡号）");
        m_modeLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
        m_usernameEdit->setPlaceholderText("请输入16位银行卡号");
        m_usernameEdit->setMaxLength(16);
        m_usernameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]{16}$"), this));
        m_switchModeBtn->setText("切换为管理员登录");
        m_switchModeBtn->setStyleSheet("QPushButton { background-color: #1976D2; color: white; }");
    }
    
    m_usernameEdit->clear();
    m_passwordEdit->clear();
}

void LoginWidget::onResponseReceived(const QJsonObject& response)
{
    // 恢复按钮状态
    m_loginBtn->setEnabled(true);
    m_switchModeBtn->setEnabled(true);

    // 检查响应状态
    QString status = response["status"].toString();
    QString msg = response["msg"].toString();

    if (status == "success") {
        // 登录成功
        if (response.contains("token")) {
            QString token = response["token"].toString();
            m_client->setToken(token);

            // 提取用户信息
            QJsonObject userInfo = response;
            userInfo["username"] = m_usernameEdit->text().trimmed();
            userInfo["is_admin"] = response.contains("is_admin") ? response["is_admin"].toBool() : false;

            showMessage(msg + "！", true);

            // 延迟发射信号（让用户看到成功消息）
            QTimer::singleShot(500, this, [this, userInfo]() {
                emit loginSuccess(userInfo);
                // 登录成功后隐藏登录界面
                hide();
            });
        }
        else {
            showMessage(msg + "！", true);
        }
    }
    else {
        showMessage(msg, false);
    }
}

void LoginWidget::onConnectClicked()
{
    // 获取服务器地址和端口
    QString host = m_serverIpEdit->text().trimmed();
    QString portStr = m_serverPortEdit->text().trimmed();

    // 验证输入
    if (host.isEmpty() || portStr.isEmpty()) {
        showMessage("请输入服务器地址和端口！", false);
        return;
    }

    bool ok;
    quint16 port = portStr.toUShort(&ok);
    if (!ok || port == 0) {
        showMessage("端口号无效（1-65535）！", false);
        return;
    }

    // 如果已经连接，先断开
    if (m_client->isConnected()) {
        m_client->disconnect();
    }

    // 禁用连接按钮
    m_connectBtn->setEnabled(false);
    m_connectBtn->setText("连接中...");
    showMessage("正在连接到服务器 " + host + ":" + QString::number(port) + "...", true);

    // 连接到服务器
    m_client->connectToHost(host, port);
}

void LoginWidget::onConnected()
{
    // 更新连接状态
    m_connectionStatusLabel->setText("状态: 已连接");
    m_connectionStatusLabel->setStyleSheet("QLabel { color: green; }");

    // 启用登录和切换按钮
    m_loginBtn->setEnabled(true);
    m_switchModeBtn->setEnabled(true);

    // 禁用连接按钮（保持已连接状态）
    m_connectBtn->setText("已连接");
    m_connectBtn->setEnabled(false);

    m_serverIpEdit->setEnabled(false);
    m_serverPortEdit->setEnabled(false);

    showMessage("已连接到服务器", true);
}

void LoginWidget::showMessage(const QString& message, bool isSuccess)
{
    if (isSuccess) {
        m_messageLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    }
    else {
        m_messageLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    }
    m_messageLabel->setText(message);

    // 3 秒后清除消息
    if (!isSuccess || message.contains("正在")) {
        QTimer::singleShot(3000, this, [this]() {
            m_messageLabel->setText("");
        });
    }
}
