#ifndef TRANSFERWIDGET_H
#define TRANSFERWIDGET_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>

class TcpClient;

/**
 * @brief 转账对话框
 *
 * 功能：
 * - 支持通过账户ID或用户名转账
 * - 输入转账金额
 * - 输入备注（可选）
 * - 发送转账请求
 * - 显示转账结果
 */
class TransferWidget : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param client TCP 客户端指针
     * @param token 认证 Token
     * @param parent 父窗口
     */
    explicit TransferWidget(TcpClient* client, const QString& token, QWidget* parent = nullptr);

    ~TransferWidget() = default;

private slots:
    /**
     * @brief 处理转账按钮点击
     */
    void onTransferClicked();

    /**
     * @brief 处理取消按钮点击
     */
    void onCancelClicked();

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
     * @brief 验证输入
     * @return true 表示输入有效
     */
    bool validateInput();

    /**
     * @brief 显示消息提示
     * @param message 消息内容
     * @param isSuccess 是否为成功消息
     */
    void showMessage(const QString& message, bool isSuccess = true);

private:
    TcpClient* m_client;           // TCP 客户端
    QString m_token;               // 认证 Token

    // UI 控件
    QLineEdit* m_targetAccountEdit;   // 目标卡号输入框
    QLineEdit* m_targetNameEdit;      // 目标姓名输入框
    QLineEdit* m_amountEdit;          // 金额输入框
    QLineEdit* m_remarkEdit;          // 备注输入框
    QPushButton* m_transferBtn;       // 转账按钮
    QPushButton* m_cancelBtn;          // 取消按钮
    QLabel* m_messageLabel;           // 消息提示标签
};

#endif // TRANSFERWIDGET_H
