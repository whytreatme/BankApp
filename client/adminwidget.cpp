#include "adminwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QCheckBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QDate>

AdminWidget::AdminWidget(TcpClient* client, QWidget* parent)
    : QWidget(parent)
    , m_client(client)
    , m_token(client->token())
{
    setupUI();

    // 连接响应信号
    connect(m_client, &TcpClient::responseReceived,
            this, &AdminWidget::onResponseReceived);

    // 自动加载所有用户列表
    onRefreshAllUsers();
}

void AdminWidget::setupUI()
{
    setWindowTitle("管理员控制台");
    setMinimumSize(900, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 创建Tab Widget
    m_tabWidget = new QTabWidget(this);

    setupAddUserTab();
    setupAllUsersTab();

    mainLayout->addWidget(m_tabWidget);

    // 监听tab切换事件，切换到"所有用户"时自动刷新
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        // 索引1是"所有用户"tab
        if (index == 1) {
            onRefreshAllUsers();
        }
    });

    // 返回按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* backBtn = new QPushButton("返回", this);
    backBtn->setMinimumSize(100, 35);
    backBtn->setStyleSheet("QPushButton { font-size: 14px; }");
    connect(backBtn, &QPushButton::clicked, this, &AdminWidget::closeRequested);
    buttonLayout->addWidget(backBtn);
    mainLayout->addLayout(buttonLayout);
}

void AdminWidget::setupAddUserTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // 标题
    QLabel* titleLabel = new QLabel("添加新用户", tab);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    layout->addWidget(titleLabel);
    layout->addSpacing(20);

    // 表单
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setSpacing(15);
    
    m_fullNameEdit = new QLineEdit(tab);
    m_fullNameEdit->setPlaceholderText("请输入用户真实姓名");
    m_fullNameEdit->setMinimumWidth(400);
    m_fullNameEdit->setMinimumHeight(35);
    formLayout->addRow("姓名:", m_fullNameEdit);
    
    m_idCardEdit = new QLineEdit(tab);
    m_idCardEdit->setPlaceholderText("请输入18位身份证号");
    m_idCardEdit->setMaxLength(18);
    m_idCardEdit->setMinimumWidth(400);
    m_idCardEdit->setMinimumHeight(35);
    formLayout->addRow("身份证号:", m_idCardEdit);
    
    m_phoneEdit = new QLineEdit(tab);
    m_phoneEdit->setPlaceholderText("请输入11位手机号");
    m_phoneEdit->setMaxLength(11);
    m_phoneEdit->setMinimumWidth(400);
    m_phoneEdit->setMinimumHeight(35);
    formLayout->addRow("手机号:", m_phoneEdit);
    
    m_birthDateEdit = new QDateEdit(tab);
    m_birthDateEdit->setCalendarPopup(true);
    m_birthDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_birthDateEdit->setDate(QDate::currentDate().addYears(-20)); 
    m_birthDateEdit->setMinimumWidth(400);
    m_birthDateEdit->setMinimumHeight(35);
    formLayout->addRow("出生年月:", m_birthDateEdit);
    
    m_addressEdit = new QLineEdit(tab);
    m_addressEdit->setPlaceholderText("请输入联系地址");
    m_addressEdit->setMinimumWidth(400);
    m_addressEdit->setMinimumHeight(35);
    formLayout->addRow("联系地址:", m_addressEdit);
    
    layout->addLayout(formLayout);
    layout->addSpacing(20);

    // 添加按钮
    m_addUserBtn = new QPushButton("添加用户", tab);
    m_addUserBtn->setMinimumHeight(40);
    m_addUserBtn->setStyleSheet("QPushButton { font-size: 14px; background-color: #4CAF50; color: white; }");
    connect(m_addUserBtn, &QPushButton::clicked, this, &AdminWidget::onAddUser);
    layout->addWidget(m_addUserBtn);

    // 状态标签
    m_addUserStatusLabel = new QLabel("", tab);
    m_addUserStatusLabel->setWordWrap(true);
    layout->addWidget(m_addUserStatusLabel);
    
    layout->addStretch();

    m_tabWidget->addTab(tab, "添加用户");
}

void AdminWidget::setupAllUsersTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // 标题和刷新按钮
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("所有用户管理", tab);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_refreshAllBtn = new QPushButton("刷新", tab);
    connect(m_refreshAllBtn, &QPushButton::clicked, this, &AdminWidget::onRefreshAllUsers);
    headerLayout->addWidget(m_refreshAllBtn);
    layout->addLayout(headerLayout);

    // 表格
    m_allUsersTable = new QTableWidget(tab);
    m_allUsersTable->setColumnCount(8);
    m_allUsersTable->setHorizontalHeaderLabels({
        "银行卡号", "用户姓名", "用户名", "余额", "管理员", "已审批", "注册时间", "选择"
    });
    m_allUsersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_allUsersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_allUsersTable->horizontalHeader()->setStretchLastSection(true);
    m_allUsersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_allUsersTable);

    // 操作按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_resetPasswordBtn = new QPushButton("随机重置", tab);
    m_modifyBalanceBtn = new QPushButton("修改余额", tab);
    m_modifyPasswordBtn = new QPushButton("修改密码", tab);
    m_modifyInfoBtn = new QPushButton("修改信息", tab);
    m_resetPasswordBtn->setEnabled(false);
    m_modifyBalanceBtn->setEnabled(false);
    m_modifyPasswordBtn->setEnabled(false);
    m_modifyInfoBtn->setEnabled(false);

    connect(m_resetPasswordBtn, &QPushButton::clicked, this, &AdminWidget::onResetPassword);
    connect(m_modifyBalanceBtn, &QPushButton::clicked, this, &AdminWidget::onModifyUserBalance);
    connect(m_modifyPasswordBtn, &QPushButton::clicked, this, &AdminWidget::onModifyPassword);
    connect(m_modifyInfoBtn, &QPushButton::clicked, this, &AdminWidget::onModifyUserInfo);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resetPasswordBtn);
    buttonLayout->addWidget(m_modifyPasswordBtn);
    buttonLayout->addWidget(m_modifyBalanceBtn);
    buttonLayout->addWidget(m_modifyInfoBtn);
    layout->addLayout(buttonLayout);

    // 状态标签
    m_allUsersStatusLabel = new QLabel("", tab);
    m_allUsersStatusLabel->setWordWrap(true);
    layout->addWidget(m_allUsersStatusLabel);

    connect(m_allUsersTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = m_allUsersTable->selectedItems().size() > 0;
        m_resetPasswordBtn->setEnabled(hasSelection);
        m_modifyBalanceBtn->setEnabled(hasSelection);
        m_modifyPasswordBtn->setEnabled(hasSelection);
        m_modifyInfoBtn->setEnabled(hasSelection);
    });

    m_tabWidget->addTab(tab, "所有用户");
}

/**
 * @brief 处理“添加用户”按钮点击事件。
 *
 * 从界面表单中获取用户输入的信息（姓名、身份证号、手机号、出生日期、联系地址），
 * 检查所有字段是否填写完整。如果有字段为空，则在状态标签中显示错误信息并返回。
 * 若输入完整，则将按钮设为不可用，状态标签显示“正在添加用户...”，
 * 并构造请求数据，通过 m_client 发送添加用户的请求（请求类型 11）。
 *
 * 请求参数：
 * - full_name: 用户真实姓名
 * - id_card: 身份证号
 * - phone: 手机号
 * - birth_date: 出生日期（yyyy-MM-dd 格式）
 * - address: 联系地址
 */
void AdminWidget::onAddUser()
{
    QString fullName = m_fullNameEdit->text().trimmed();
    QString idCard = m_idCardEdit->text().trimmed();
    QString phone = m_phoneEdit->text().trimmed();
    QString birthDate = m_birthDateEdit->date().toString("yyyy-MM-dd");
    QString address = m_addressEdit->text().trimmed();

    if (fullName.isEmpty() || idCard.isEmpty() || phone.isEmpty() || 
        address.isEmpty()) {
        m_addUserStatusLabel->setText("错误: 所有字段都必须填写！");
        m_addUserStatusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    m_addUserStatusLabel->setText("正在添加用户...");
    m_addUserStatusLabel->setStyleSheet("QLabel { color: blue; }");
    m_addUserBtn->setEnabled(false);

    QJsonObject req;
    req["full_name"] = fullName;
    req["id_card"] = idCard;
    req["phone"] = phone;
    req["birth_date"] = birthDate;
    req["address"] = address;
    m_client->sendRequest(11, m_token, req);  // Type 11: 添加用户
}

void AdminWidget::onResetPassword()
{
    int row = m_allUsersTable->currentRow();
    if (row < 0) return;

    QString userId = m_allUsersTable->item(row, 0)->data(Qt::UserRole).toString();
    QString username = m_allUsersTable->item(row, 2)->text(); // 2 is username

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认重置密码",
        QString("确定要重置用户 '%1' 的密码吗？").arg(username),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_allUsersStatusLabel->setText("正在重置密码...");

        QJsonObject req;
        req["user_id"] = userId;
        m_client->sendRequest(12, m_token, req);  // Type 12: 重置密码
    }
}

void AdminWidget::onRefreshAllUsers()
{
    m_allUsersStatusLabel->setText("正在加载...");
    m_refreshAllBtn->setEnabled(false);

    QJsonObject req;
    m_client->sendRequest(8, m_token, req);  // Type 8: 获取所有用户
}

void AdminWidget::onModifyUserBalance()
{
    int row = m_allUsersTable->currentRow();
    if (row < 0) return;

    QString userId = m_allUsersTable->item(row, 0)->data(Qt::UserRole).toString();
    QString username = m_allUsersTable->item(row, 2)->text(); // index 2
    double currentBalance = m_allUsersTable->item(row, 3)->text().toDouble(); // index 3

    showModifyBalanceDialog(userId, username, currentBalance);
}

void AdminWidget::showModifyBalanceDialog(const QString& userId, const QString& username, double currentBalance)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString("修改 %1 的余额").arg(username));
    dialog.setMinimumWidth(300);

    QFormLayout* layout = new QFormLayout(&dialog);

    QLabel* infoLabel = new QLabel(QString("当前余额: %1").arg(currentBalance, 0, 'f', 2));
    layout->addRow(infoLabel);

    QLineEdit* balanceEdit = new QLineEdit(QString::number(currentBalance, 'f', 2));
    layout->addRow("新余额:", balanceEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定");
    QPushButton* cancelBtn = new QPushButton("取消");
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addRow(buttonLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        double newBalance = balanceEdit->text().toDouble();

        m_allUsersStatusLabel->setText("正在修改余额...");

        QJsonObject req;
        req["user_id"] = userId;
        req["new_balance"] = newBalance;
        m_client->sendRequest(9, m_token, req);  // Type 9: 修改用户余额
    }
}

void AdminWidget::onModifyPassword()
{
    int row = m_allUsersTable->currentRow();
    if (row < 0) return;

    QString userId = m_allUsersTable->item(row, 0)->data(Qt::UserRole).toString();
    QString username = m_allUsersTable->item(row, 2)->text(); // index 2

    showModifyPasswordDialog(userId, username);
}

void AdminWidget::showModifyPasswordDialog(const QString& userId, const QString& username)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString("修改 %1 的密码").arg(username));
    dialog.setMinimumWidth(300);

    QFormLayout* layout = new QFormLayout(&dialog);

    QLineEdit* passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("请输入6位数字新密码");
    layout->addRow("新密码:", passwordEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定");
    QPushButton* cancelBtn = new QPushButton("取消");
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addRow(buttonLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString newPassword = passwordEdit->text().trimmed();

        if (newPassword.length() != 6) {
            QMessageBox::warning(this, "输入错误", "密码必须为6位数字！");
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
            QMessageBox::warning(this, "输入错误", "密码必须全部为数字！");
            return;
        }

        m_allUsersStatusLabel->setText("正在修改密码...");

        QJsonObject req;
        req["user_id"] = userId;
        req["new_password"] = newPassword;
        m_client->sendRequest(12, m_token, req);  // Type 12: 重置/修改密码
    }
}

void AdminWidget::onModifyUserInfo()
{
    int row = m_allUsersTable->currentRow();
    if (row < 0) return;

    QString userId = m_allUsersTable->item(row, 0)->data(Qt::UserRole).toString();
    QString username = m_allUsersTable->item(row, 2)->text();
    QString fullName = m_allUsersTable->item(row, 1)->text();
    
    QString phone = m_allUsersTable->item(row, 1)->data(Qt::UserRole).toString();
    QString idCard = m_allUsersTable->item(row, 1)->data(Qt::UserRole + 1).toString();
    QString birthDate = m_allUsersTable->item(row, 1)->data(Qt::UserRole + 2).toString();

    showModifyInfoDialog(userId, username, fullName, phone, idCard, birthDate);
}

void AdminWidget::showModifyInfoDialog(const QString& userId, const QString& username, const QString& fullName, 
                                     const QString& phone, const QString& idCard, const QString& birthDate)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString("修改用户 %1 (%2) 的信息").arg(fullName).arg(username));
    dialog.setMinimumWidth(500);
    dialog.setStyleSheet("QDialog { background-color: white; } "
                         "QLineEdit, QDateEdit { padding: 8px; border: 1px solid #ccc; border-radius: 4px; min-width: 300px; }"
                         "QLabel { font-weight: bold; color: #333; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    QLabel* headerLabel = new QLabel("请更新用户信息：", &dialog);
    headerLabel->setStyleSheet("font-size: 16px; color: #1976D2; margin-bottom: 10px;");
    mainLayout->addWidget(headerLabel);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    formLayout->setLabelAlignment(Qt::AlignRight);

    QLineEdit* fullNameEdit = new QLineEdit(fullName, &dialog);
    fullNameEdit->setPlaceholderText("请输入真实姓名");
    formLayout->addRow("真实姓名:", fullNameEdit);

    QLineEdit* phoneEdit = new QLineEdit(phone, &dialog);
    phoneEdit->setMaxLength(11);
    phoneEdit->setPlaceholderText("请输入11位手机号");
    formLayout->addRow("手机号:", phoneEdit);

    QLineEdit* idCardEdit = new QLineEdit(idCard, &dialog);
    idCardEdit->setMaxLength(18);
    idCardEdit->setPlaceholderText("请输入18位身份证号");
    formLayout->addRow("身份证号:", idCardEdit);

    QDateEdit* birthDateEdit = new QDateEdit(&dialog);
    birthDateEdit->setCalendarPopup(true);
    birthDateEdit->setDisplayFormat("yyyy-MM-dd");
    QDate bDate = QDate::fromString(birthDate, "yyyy-MM-dd");
    if (bDate.isValid()) {
        birthDateEdit->setDate(bDate);
    } else {
        birthDateEdit->setDate(QDate::currentDate().addYears(-20));
    }
    formLayout->addRow("出生日期:", birthDateEdit);

    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("保存修改");
    QPushButton* cancelBtn = new QPushButton("取消");
    
    okBtn->setMinimumSize(120, 40);
    cancelBtn->setMinimumSize(100, 40);
    
    okBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; font-weight: bold; }"
                         "QPushButton:hover { background-color: #45a049; }");
    cancelBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; border: none; font-weight: bold; }"
                             "QPushButton:hover { background-color: #da190b; }");

    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_allUsersStatusLabel->setText("正在同步更新用户信息...");

        QJsonObject req;
        req["user_id"] = userId;
        req["full_name"] = fullNameEdit->text().trimmed();
        req["phone"] = phoneEdit->text().trimmed();
        req["id_card"] = idCardEdit->text().trimmed();
        req["birth_date"] = birthDateEdit->date().toString("yyyy-MM-dd");
        
        m_client->sendRequest(14, m_token, req); 
    }
}

void AdminWidget::onResponseReceived(const QJsonObject& response)
{
    QString status = response["status"].toString();
    QString msg = response["msg"].toString();

    if (status == "success") {
        // Type 8: 获取所有用户
        if (response.contains("users")) {
            QJsonArray users = response["users"].toArray();
            updateAllUsersTable(users);
            m_allUsersStatusLabel->setText(QString("加载成功，共 %1 个用户").arg(users.size()));
            m_refreshAllBtn->setEnabled(true);
        }
        // Type 11: 添加用户成功
        else if (response.contains("card_number") && response.contains("password")) {
            QString cardNumber = response["card_number"].toString();
            QString password = response["password"].toString();
            QString successMsg = QString("用户创建成功！\n卡号: %1\n初始密码: %2\n请妥善保管！").arg(cardNumber).arg(password);
            m_addUserStatusLabel->setText(successMsg);
            m_addUserStatusLabel->setStyleSheet("QLabel { color: green; }");
            m_addUserBtn->setEnabled(true);
            
            // 清空表单
            m_fullNameEdit->clear();
            m_idCardEdit->clear();
            m_phoneEdit->clear();
            m_birthDateEdit->setDate(QDate::currentDate().addYears(-20));
            m_addressEdit->clear();
            
            // 显示详细信息对话框
            QMessageBox::information(this, "用户创建成功", successMsg);
        }
        // Type 12: 修改/重置密码成功
        else if (response.contains("password") && (msg.contains("重置") || msg.contains("修改"))) {
            QString newPassword = response["password"].toString();
            QString successMsg = QString("密码操作成功！\n新密码: %1\n请妥善保管！").arg(newPassword);
            m_allUsersStatusLabel->setText(msg);
            QMessageBox::information(this, "操作成功", successMsg);
            onRefreshAllUsers();  // 刷新列表
        }
        // Type 9: 修改余额成功
        else if (msg.contains("余额")) {
            m_allUsersStatusLabel->setText(msg);
            onRefreshAllUsers();  // 刷新列表
        }
        // Type 14: 修改个人信息成功
        else if (msg.contains("个人信息")) {
            m_allUsersStatusLabel->setText(msg);
            onRefreshAllUsers();  // 刷新列表
        }
        else {
            m_addUserStatusLabel->setText(msg);
            m_addUserBtn->setEnabled(true);
            m_allUsersStatusLabel->setText(msg);
            m_refreshAllBtn->setEnabled(true);
        }
    }
    else {
        // 错误处理
        m_addUserStatusLabel->setText("错误: " + msg);
        m_addUserStatusLabel->setStyleSheet("QLabel { color: red; }");
        m_addUserBtn->setEnabled(true);
        m_allUsersStatusLabel->setText("错误: " + msg);
        m_refreshAllBtn->setEnabled(true);

        QMessageBox::warning(this, "操作失败", msg);
    }
}

void AdminWidget::updateAllUsersTable(const QJsonArray& users)
{
    m_allUsersTable->setRowCount(0);

    for (const QJsonValue& value : users) {
        QJsonObject user = value.toObject();
        
        // 跳过管理员账号，不显示在普通用户列表中
        if (user["is_admin"].toBool()) {
            continue;
        }

        int row = m_allUsersTable->rowCount();
        m_allUsersTable->insertRow(row);

        m_allUsersTable->setItem(row, 0, new QTableWidgetItem(user["card_number"].toString()));
        // 保存内部 ID 到用户角色数据中
        m_allUsersTable->item(row, 0)->setData(Qt::UserRole, user["id"].toString());
        m_allUsersTable->setItem(row, 1, new QTableWidgetItem(user["full_name"].toString()));
        // 存储额外信息用于修改对话框
        m_allUsersTable->item(row, 1)->setData(Qt::UserRole, user["phone"].toString());
        m_allUsersTable->item(row, 1)->setData(Qt::UserRole + 1, user["id_card"].toString());
        m_allUsersTable->item(row, 1)->setData(Qt::UserRole + 2, user["birth_date"].toString());
        
        m_allUsersTable->setItem(row, 2, new QTableWidgetItem(user["username"].toString()));
        m_allUsersTable->setItem(row, 3, new QTableWidgetItem(QString::number(user["balance"].toDouble(), 'f', 2)));
        m_allUsersTable->setItem(row, 4, new QTableWidgetItem(user["is_admin"].toBool() ? "是" : "否"));
        m_allUsersTable->setItem(row, 5, new QTableWidgetItem(user["is_approved"].toBool() ? "是" : "否"));
        m_allUsersTable->setItem(row, 6, new QTableWidgetItem(user["created_at"].toString()));
        m_allUsersTable->setItem(row, 7, new QTableWidgetItem("✓"));
    }

    m_allUsersTable->resizeColumnsToContents();
}
