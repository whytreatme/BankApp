#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>
#include <QJsonObject>

class TcpClient;
class AdminWidget;

/**
 * @brief 主窗口 - 银行系统主界面
 *
 * 功能：
 * - 显示用户信息和余额
 * - 显示交易记录
 * - 打开转账对话框
 * - 自动查询余额和交易历史
 * - 管理员界面切换
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param client TCP 客户端指针
     * @param parent 父窗口
     */
    explicit MainWindow(TcpClient* client, QWidget* parent = nullptr);

    ~MainWindow();

public slots:
    /**
     * @brief 登录成功后调用（初始化用户信息）
     * @param userInfo 用户信息（包含 user_id, username 等）
     */
    void onLoginSuccess(const QJsonObject& userInfo);

private slots:
    /**
     * @brief 请求查询余额
     */
    void requestBalance();

    /**
     * @brief 请求查询交易历史
     */
    void requestTransactions();

    /**
     * @brief 处理转账按钮点击
     */
    void onTransferClicked();

    /**
     * @brief 处理管理员按钮点击
     */
    void onAdminClicked();

    /**
     * @brief 处理返回按钮点击（从管理员界面返回用户界面）
     */
    void onBackClicked();

    /**
     * @brief 处理刷新按钮点击
     */
    void onRefreshClicked();
    
    /**
     * @brief 处理修改密码按钮点击
     */
    void onChangePasswordClicked();

    /**
     * @brief 处理服务器响应
     * @param response JSON 响应数据
     */
    void onResponseReceived(const QJsonObject& response);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUI();

    /**
     * @brief 创建用户界面
     */
    void setupUserView();

    /**
     * @brief 更新交易记录表格
     * @param transactions 交易记录数组
     */
    void updateTransactionTable(const QJsonArray& transactions);

    /**
     * @brief 显示状态消息
     * @param message 消息内容
     */
    void showStatusMessage(const QString& message);

private:
    TcpClient* m_client;           // TCP 客户端
    QString m_token;               // 认证 Token
    QString m_userIdStr;           // 用户 ID (字符串格式的雪花ID)
    QString m_cardNumber;          // 16位银行卡号
    QString m_username;            // 用户名
    QString m_fullName;            // 用户姓名
    QString m_phone;               // 手机号
    QString m_idCard;              // 身份证号
    QString m_birthDate;           // 出生日期
    bool m_isAdmin;                // 是否是管理员

    // 视图切换
    QStackedWidget* m_stackedWidget;   // 堆栈窗口，用于切换视图
    QWidget* m_userView;               // 用户视图
    AdminWidget* m_adminView;          // 管理员视图

    // 用户视图控件
    QLabel* m_userInfoLabel;       // 用户信息标签
    QLabel* m_balanceLabel;        // 余额标签
    QTableWidget* m_txnTable;      // 交易记录表格
    QPushButton* m_transferBtn;    // 转账按钮
    QPushButton* m_adminBtn;       // 管理员按钮
    QPushButton* m_changePasswordBtn; // 修改密码按钮
    QPushButton* m_refreshBtn;     // 刷新按钮
    QPushButton* m_backBtn;        // 返回按钮（管理员界面用）

    QLabel* m_statusLabel;         // 状态栏标签
    QTimer* m_refreshTimer;        // 自动刷新定时器
};

#endif // MAINWINDOW_H
