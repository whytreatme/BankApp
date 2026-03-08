#include "transferwidget.h"
#include "tcpclient.h"
#include "protocolutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QTimer>

TransferWidget::TransferWidget(TcpClient* client, const QString& token, QWidget* parent)
    : QDialog(parent)
    , m_client(client)
    , m_token(token)
{
    setupUI();

    // 连接客户端响应信号
    connect(m_client, &TcpClient::responseReceived,
            this, &TransferWidget::onResponseReceived);

    // 设置对话框属性
    setModal(true);
    setWindowTitle("转账");
}

void TransferWidget::setupUI()
{
    // 创建输入框
    m_targetAccountEdit = new QLineEdit(this);
    m_targetAccountEdit->setPlaceholderText("请输入目标银行卡号 (16位)");
    m_targetAccountEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]{16}$"), this));
    m_targetAccountEdit->setMinimumHeight(55);
    m_targetAccountEdit->setStyleSheet("QLineEdit { font-size: 18px; padding-left: 10px; }");

    m_targetNameEdit = new QLineEdit(this);
    m_targetNameEdit->setPlaceholderText("请输入收款人真实姓名");
    m_targetNameEdit->setMinimumHeight(55);
    m_targetNameEdit->setStyleSheet("QLineEdit { font-size: 18px; padding-left: 10px; }");

    m_amountEdit = new QLineEdit(this);
    m_amountEdit->setPlaceholderText("请输入转账金额 (¥)");
    m_amountEdit->setMinimumHeight(55);
    m_amountEdit->setStyleSheet("QLineEdit { font-size: 18px; padding-left: 10px; }");

    QDoubleValidator* amountValidator = new QDoubleValidator(0.01, 999999999.99, 2, this);
    amountValidator->setNotation(QDoubleValidator::StandardNotation);
    m_amountEdit->setValidator(amountValidator);

    m_remarkEdit = new QLineEdit(this);
    m_remarkEdit->setPlaceholderText("请输入备注（可选）");
    m_remarkEdit->setMaxLength(200);
    m_remarkEdit->setMinimumHeight(55);
    m_remarkEdit->setStyleSheet("QLineEdit { font-size: 18px; padding-left: 10px; }");

    m_transferBtn = new QPushButton("确认转账", this);
    m_cancelBtn = new QPushButton("取消", this);
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setAlignment(Qt::AlignCenter);

    // 按钮样式
    m_transferBtn->setMinimumHeight(50);
    m_cancelBtn->setMinimumHeight(50);
    m_transferBtn->setStyleSheet("QPushButton { font-size: 16px; background-color: #4CAF50; color: white; border-radius: 5px; }");
    m_cancelBtn->setStyleSheet("QPushButton { font-size: 16px; border-radius: 5px; }");

    // 创建布局
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(25);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow("目标卡号:", m_targetAccountEdit);
    formLayout->addRow("收款人姓名:", m_targetNameEdit);
    formLayout->addRow("转账金额 (¥):", m_amountEdit);
    formLayout->addRow("备注:", m_remarkEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_transferBtn->setMinimumWidth(150);
    m_cancelBtn->setMinimumWidth(150);
    buttonLayout->addWidget(m_transferBtn);
    buttonLayout->addSpacing(30);
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addStretch();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_messageLabel);

    // 设置边距
    mainLayout->setContentsMargins(40, 40, 40, 40);

    // 设置对话框大小
    setMinimumSize(650, 450);
    resize(650, 450);

    // 连接按钮信号
    connect(m_transferBtn, &QPushButton::clicked,
            this, &TransferWidget::onTransferClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &TransferWidget::onCancelClicked);

    // 连接回车键
    connect(m_amountEdit, &QLineEdit::returnPressed,
            this, &TransferWidget::onTransferClicked);
    connect(m_remarkEdit, &QLineEdit::returnPressed,
            this, &TransferWidget::onTransferClicked);
}

void TransferWidget::onTransferClicked()
{
    // 验证输入
    if (!validateInput()) {
        return;
    }

    // 检查连接状态
    if (!m_client->isConnected()) {
        showMessage("未连接到服务器！", false);
        return;
    }

    // 获取金额和备注
    QString targetAccount = m_targetAccountEdit->text().trimmed();
    QString targetName = m_targetNameEdit->text().trimmed();
    double amount = m_amountEdit->text().toDouble();
    QString remark = m_remarkEdit->text().trimmed();

    // 构造转账请求（type=4）
    QJsonObject requestData;
    requestData["to_account"] = targetAccount;
    requestData["to_name"] = targetName;
    requestData["amount"] = amount;
    requestData["remark"] = remark;

    // 发送转账请求
    m_client->sendRequest(4, m_token, requestData);

    // 显示处理中消息
    showMessage("正在转账...", true);
    m_transferBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
}

void TransferWidget::onCancelClicked()
{
    reject(); //  关闭对话框，返回 QDialog::Rejected
}

void TransferWidget::onResponseReceived(const QJsonObject& response)
{
    // 只处理转账相关的响应
    // 转账响应（成功或失败）都包含 amount 或 new_balance 字段
    // - 转账成功：{"status": "success", "msg": "转账成功", "new_balance": xxx}
    // - 转账失败：{"status": "error", "msg": "余额不足", "amount": xxx}
    // 其他响应如余额查询、交易记录查询不包含这两个字段，会被过滤掉
    if (!response.contains("amount") && !response.contains("new_balance")) {
        // 不是转账响应，忽略（可能是余额查询或交易记录查询的响应）
        return;
    }

    // 恢复按钮状态
    m_transferBtn->setEnabled(true);
    m_cancelBtn->setEnabled(true);

    // 检查响应状态
    QString status = response["status"].toString();
    QString msg = response["msg"].toString();

    if (status == "success") {
        // 转账成功
        showMessage(msg + "！", true);

        // 延迟关闭对话框（让用户看到成功消息）
        QTimer::singleShot(1000, this, [this]() {
            accept();  // 关闭对话框，返回 QDialog::Accepted
        });
    }
    else {
        // 转账失败
        showMessage(msg, false);
        
        // 如果是余额不足或其他业务错误，弹出警告框
        if (msg.contains("余额不足")) {
            QMessageBox::warning(this, "转账失败", "您的账户余额不足，请核对后再试。");
        } else {
            QMessageBox::warning(this, "转账失败", msg);
        }
    }
}

bool TransferWidget::validateInput()
{
    // 验证目标卡号
    QString targetCard = m_targetAccountEdit->text().trimmed();
    if (targetCard.length() != 16) {
        showMessage("请输入 16 位目标卡号！", false);
        m_targetAccountEdit->setFocus();
        return false;
    }

    // 验证目标姓名
    QString targetName = m_targetNameEdit->text().trimmed();
    if (targetName.isEmpty()) {
        showMessage("请输入收款人姓名！", false);
        m_targetNameEdit->setFocus();
        return false;
    }

    // 验证金额
    QString amountStr = m_amountEdit->text().trimmed();
    if (amountStr.isEmpty()) {
        showMessage("请输入转账金额！", false);
        m_amountEdit->setFocus();
        return false;
    }

    bool ok;
    double amount = amountStr.toDouble(&ok);
    if (!ok || amount <= 0) {
        showMessage("转账金额必须大于 0！", false);
        m_amountEdit->setFocus();
        return false;
    }

    // 验证金额格式（最多两位小数）
    if (amountStr.contains('.') && amountStr.split('.')[1].length() > 2) {
        showMessage("金额最多保留两位小数！", false);
        m_amountEdit->setFocus();
        return false;
    }

    return true;
}


void TransferWidget::showMessage(const QString& message, bool isSuccess)
{
    if (isSuccess) {
        m_messageLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    }
    else {
        m_messageLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    }
    m_messageLabel->setText(message);

    // 如果是错误消息，3 秒后清除
    if (!isSuccess) {
        QTimer::singleShot(3000, this, [this]() {
            m_messageLabel->setText("");
        });
    }
}
