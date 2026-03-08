#include "mainwindow.h"
#include "tcpclient.h"
#include "protocolutils.h"
#include "transferwidget.h"
#include "adminwidget.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QStatusBar>
#include <QJsonArray>
#include <QDateTime>

MainWindow::MainWindow(TcpClient* client, QWidget* parent)
    : QMainWindow(parent)
    , m_client(client)
{
    setupUI();

    connect(m_client, &TcpClient::responseReceived,
            this, &MainWindow::onResponseReceived);

    hide();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    // 创建窗口
    setWindowTitle("银行系统");
    resize(800, 600);


    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);


    m_stackedWidget = new QStackedWidget(this);

    // 创建用户视图
    m_userView = new QWidget(this);
    setupUserView();
    m_stackedWidget->addWidget(m_userView);


    for (int i = 0; i < m_txnTable->columnCount(); ++i) {
        m_txnTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }

    m_adminView = nullptr;


    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(m_stackedWidget);

    m_statusLabel = new QLabel("就绪", this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::setupUserView()
{
    // 创建控件
    m_userInfoLabel = new QLabel("未登录", this);
    m_userInfoLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; padding: 10px; }");

    m_balanceLabel = new QLabel("余额: ¥0.00", this);
    m_balanceLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #2e7d32; padding: 10px; }");

    m_transferBtn = new QPushButton("转账", this);
    m_transferBtn->setMinimumSize(100, 40);
    m_transferBtn->setStyleSheet("QPushButton { font-size: 14px; }");

    m_adminBtn = new QPushButton("管理", this);
    m_adminBtn->setMinimumSize(100, 40);
    m_adminBtn->setStyleSheet("QPushButton { font-size: 14px; background-color: #1976D2; color: white; }");
    m_adminBtn->hide();  // 默认隐藏，仅管理员可见

    m_changePasswordBtn = new QPushButton("修改密码", this);
    m_changePasswordBtn->setMinimumSize(100, 40);
    m_changePasswordBtn->setStyleSheet("QPushButton { font-size: 14px; }");

    m_refreshBtn = new QPushButton("刷新", this);
    m_refreshBtn->setMinimumSize(100, 40);
    m_refreshBtn->setStyleSheet("QPushButton { font-size: 14px; }");

    // 交易记录表格
    m_txnTable = new QTableWidget(this);
    m_txnTable->setColumnCount(5);
    m_txnTable->setHorizontalHeaderLabels({"交易流水号", "转出卡号", "转入卡号", "金额", "时间"});
    m_txnTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_txnTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_txnTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_txnTable->horizontalHeader()->setStretchLastSection(true);
    m_txnTable->verticalHeader()->setVisible(false);

    // 设置列宽
    m_txnTable->setColumnWidth(0, 80);   // 交易ID
    m_txnTable->setColumnWidth(1, 100);  // 转出账户
    m_txnTable->setColumnWidth(2, 100);  // 转入账户
    m_txnTable->setColumnWidth(3, 100);  // 金额
    m_txnTable->setColumnWidth(4, 180);  // 时间

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(m_userView);

    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(m_userInfoLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_balanceLabel);
    topLayout->addSpacing(20);
    topLayout->addWidget(m_transferBtn);
    topLayout->addWidget(m_adminBtn);
    topLayout->addWidget(m_changePasswordBtn);
    topLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(new QLabel("交易记录:", this));
    mainLayout->addWidget(m_txnTable);

    // 连接按钮信号
    connect(m_transferBtn, &QPushButton::clicked,
            this, &MainWindow::onTransferClicked);
    connect(m_adminBtn, &QPushButton::clicked,
            this, &MainWindow::onAdminClicked);
    connect(m_changePasswordBtn, &QPushButton::clicked,
            this, &MainWindow::onChangePasswordClicked);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &MainWindow::onRefreshClicked);
}

void MainWindow::onLoginSuccess(const QJsonObject& userInfo)
{
    // 保存用户信息
    m_userIdStr = userInfo["user_id"].toString();
    m_username = userInfo["username"].toString();
    m_fullName = userInfo["full_name"].toString();
    m_cardNumber = userInfo["card_number"].toString();
    m_phone = userInfo["phone"].toString();
    m_idCard = userInfo["id_card"].toString();
    m_birthDate = userInfo["birth_date"].toString();
    m_isAdmin = userInfo.contains("is_admin") ? userInfo["is_admin"].toBool() : false;
    m_token = m_client->token();

    // 更新 UI
    if (m_isAdmin) {
        QString userLabel = QString("卡号: %1 | 姓名: %2 [管理员]").arg(m_userIdStr).arg(m_fullName);
        m_userInfoLabel->setText(userLabel);
        m_balanceLabel->hide();
        
        m_adminBtn->show();  // 显示管理员按钮
        m_transferBtn->hide();  // 管理员不能转账
        m_changePasswordBtn->hide(); // 管理员不能修改密码

        // 为管理员创建管理员视图（延迟创建，仅当需要时）
        if (!m_adminView) {
            m_adminView = new AdminWidget(m_client, this);
            m_stackedWidget->addWidget(m_adminView);

            // 连接返回信号
            connect(m_adminView, &AdminWidget::closeRequested,
                    this, &MainWindow::onBackClicked);
        }
    } else {
        QString userLabel = QString("卡号: %1 | 姓名: %2").arg(m_userIdStr).arg(m_fullName);
        m_userInfoLabel->setText(userLabel);
        m_balanceLabel->show();
        
        m_adminBtn->hide();
        m_transferBtn->show();  // 普通用户显示转账按钮
        m_changePasswordBtn->show(); // 普通用户显示修改密码按钮
    }

    // 显示主窗口
    show();

    // 自动查询余额和交易记录
    QTimer::singleShot(100, this, [this]() {
        requestBalance();
        requestTransactions();
    });

    // 设置定时器，每5秒自动刷新交易记录（用于检测新转入的交易）
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (m_client->isConnected()) {
            requestBalance();
            requestTransactions();
        }
    });
    m_refreshTimer->start(5000);  // 5秒刷新一次

    showStatusMessage("登录成功");
}

void MainWindow::requestBalance()
{
    if (!m_client->isConnected()) {
        showStatusMessage("未连接到服务器");
        return;
    }

    // 发送查询余额请求（type=3）
    QJsonObject requestData;
    m_client->sendRequest(3, m_token, requestData);

    // showStatusMessage("正在查询余额...");
}

void MainWindow::requestTransactions()
{
    if (!m_client->isConnected()) {
        showStatusMessage("未连接到服务器");
        return;
    }

    QJsonObject requestData;
    requestData["limit"] = 20; 
    m_client->sendRequest(5, m_token, requestData);

    // showStatusMessage("正在查询交易记录...");
}

void MainWindow::onTransferClicked()
{
    // 打开转账对话框
    TransferWidget transferDlg(m_client, m_token, this);
    transferDlg.exec();

    // 转账完成后刷新余额和交易记录
    if (transferDlg.result() == QDialog::Accepted) {
        requestBalance();
        requestTransactions();
    }
}

void MainWindow::onAdminClicked()
{
    // 切换到管理员视图
    if (m_adminView) {
        m_stackedWidget->setCurrentWidget(m_adminView);
        showStatusMessage("切换到管理员视图");
    }
}

void MainWindow::onChangePasswordClicked()
{
    // 创建修改密码对话框
    QDialog dialog(this);
    dialog.setWindowTitle("修改密码");
    dialog.setMinimumWidth(400);

    QFormLayout* layout = new QFormLayout(&dialog);

    QLineEdit* oldPasswordEdit = new QLineEdit(&dialog);
    oldPasswordEdit->setEchoMode(QLineEdit::Password);
    oldPasswordEdit->setPlaceholderText("请输入旧密码");
    layout->addRow("旧密码:", oldPasswordEdit);

    QLineEdit* newPasswordEdit = new QLineEdit(&dialog);
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    newPasswordEdit->setPlaceholderText("请输入6位数字密码");
    layout->addRow("新密码:", newPasswordEdit);

    QLineEdit* confirmPasswordEdit = new QLineEdit(&dialog);
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText("请再次输入6位数字密码");
    layout->addRow("确认密码:", confirmPasswordEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定", &dialog);
    QPushButton* cancelBtn = new QPushButton("取消", &dialog);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addRow(buttonLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString oldPassword = oldPasswordEdit->text().trimmed();
        QString newPassword = newPasswordEdit->text().trimmed();
        QString confirmPassword = confirmPasswordEdit->text().trimmed();

        // 验证输入
        if (oldPassword.isEmpty() || newPassword.isEmpty() || confirmPassword.isEmpty()) {
            QMessageBox::warning(this, "输入错误", "所有字段都必须填写！");
            return;
        }

        if (newPassword.length() != 6) {
            QMessageBox::warning(this, "输入错误", "新密码必须为6位数字！");
            return;
        }

        bool isDigitOnly = true;
        for (const QChar &ch : newPassword) {
            if (!ch.isDigit()) {
                isDigitOnly = false;
                break;
            }
        }
        if (!isDigitOnly) {
            QMessageBox::warning(this, "输入错误", "新密码必须全部为数字！");
            return;
        }

        if (newPassword != confirmPassword) {
            QMessageBox::warning(this, "输入错误", "两次输入的新密码不一致！");
            return;
        }

        // 发送修改密码请求
        showStatusMessage("正在修改密码...");
        QJsonObject req;
        req["user_id"] = m_userIdStr;
        req["old_password"] = oldPassword;
        req["new_password"] = newPassword;
        m_client->sendRequest(13, m_token, req);  // Type 13: 修改密码
    }
}

void MainWindow::onBackClicked()
{
    // 返回用户视图
    m_stackedWidget->setCurrentWidget(m_userView);
    showStatusMessage("返回用户视图");

    // 刷新用户数据
    requestBalance();
    requestTransactions();
}

void MainWindow::onRefreshClicked()
{
    requestBalance();
    requestTransactions();
}

void MainWindow::onResponseReceived(const QJsonObject& response)
{
    QString msg = response["msg"].toString();
    QString status = response["status"].toString();

    // 处理查询余额响应
    if (response.contains("balance")) {
        double balance = response["balance"].toDouble();
        m_balanceLabel->setText(QString("余额: ¥%1").arg(balance, 0, 'f', 2));
    }

    // 处理查询交易历史响应
    if (response.contains("transactions")) {
        QJsonArray transactions = response["transactions"].toArray();
        int prevCount = m_txnTable->rowCount();
        updateTransactionTable(transactions);
        int newCount = transactions.size();

        // 只有在记录数变化或手动刷新时才显示消息
        if (newCount != prevCount) {
            showStatusMessage(QString("已加载 %1 条交易记录").arg(newCount));
        }
    }

    // 处理转账响应
    if (response.contains("new_balance")) {
        double newBalance = response["new_balance"].toDouble();
        m_balanceLabel->setText(QString("余额: ¥%1").arg(newBalance, 0, 'f', 2));
        showStatusMessage("余额已更新");
    }
    
    // 处理修改密码响应
    if (status == "success" && msg.contains("密码修改成功")) {
        showStatusMessage(msg);
        QMessageBox::information(this, "成功", "密码修改成功！请使用新密码重新登录。");
    } else if (status == "error" && msg.contains("密码")) {
        showStatusMessage("错误: " + msg);
        QMessageBox::warning(this, "操作失败", msg);
    }
}

void MainWindow::updateTransactionTable(const QJsonArray& transactions)
{
    // 清空表格
    m_txnTable->setRowCount(0);

    // 填充数据
    for (int i = 0; i < transactions.size(); ++i) {
        QJsonObject txn = transactions[i].toObject();

        int row = m_txnTable->rowCount();
        m_txnTable->insertRow(row);

        // 交易ID
        m_txnTable->setItem(row, 0, new QTableWidgetItem(QString::number(txn["trans_id"].toVariant().toLongLong())));

        QString fromCard = txn["from_card_number"].toString();
        m_txnTable->setItem(row, 1, new QTableWidgetItem(fromCard.isEmpty() ? "系统" : fromCard));

        QString toCard = txn["to_card_number"].toString();
        m_txnTable->setItem(row, 2, new QTableWidgetItem(toCard.isEmpty() ? "系统" : toCard));

        double amount = txn["amount"].toDouble();
        QTableWidgetItem* amountItem = new QTableWidgetItem(QString("¥%1").arg(amount, 0, 'f', 2));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_txnTable->setItem(row, 3, amountItem);

        // 时间列 - 转换为本地时区
        QString timestampStr = txn["timestamp"].toString();
        QDateTime timestampUtc = QDateTime::fromString(timestampStr, Qt::ISODate);
        QDateTime timestampLocal = timestampUtc.toLocalTime();
        QString localTimestamp = timestampLocal.toString("yyyy-MM-dd hh:mm:ss");
        m_txnTable->setItem(row, 4, new QTableWidgetItem(localTimestamp));
    }
}

void MainWindow::showStatusMessage(const QString& message)
{
    // 如果当前就是要显示的消息，不重复更新
    if (m_statusLabel->text() == message) {
        return;
    }

    m_statusLabel->setText(message);

    // 3 秒后清除消息
    QTimer::singleShot(3000, this, [this]() {
        m_statusLabel->setText("就绪");
    });
}
