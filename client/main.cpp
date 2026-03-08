#include "mainwindow.h"
#include "loginwidget.h"
#include "tcpclient.h"

#include <QApplication>
#include <QMessageBox>
#include <QEventLoop>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    TcpClient client;

    LoginWidget login(&client);
    login.show();

    MainWindow mainWindow(&client);

    // 连接登录成功信号
    QObject::connect(&login, &LoginWidget::loginSuccess,
                     &mainWindow, &MainWindow::onLoginSuccess);

    return a.exec();
}
