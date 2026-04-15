#ifndef WORKTASK_H
#define WORKTASK_H   

#include<QObject>
#include<QTcpSocket>
#include<QJsonObject>
#include <QPointer>
#include<QRunnable>  //用于线程池
#include"controller/usercontroller.h"
#include"controller/accountcontroller.h"
#include"controller/transactioncontroller.h"
#include"controller/admincontroller.h"

class WorkTask : public QObject, public QRunnable{
    Q_OBJECT

    public:
        WorkTask(QTcpSocket* socket, 
                 quint8 type,
                 QString&& token,
                 QJsonObject&& req,
                 UserController* userCtrl,
                 AccountController* accountCtrl,
                 TransactionController* txnCtrl,
                 AdminController*  adminCtrl,
                 QString&& dbId);
    
        void run() override;  //线程池执行入口
    signals:
        void taskFinished(QPointer<QTcpSocket> socket, QJsonObject res);
        void AuthSuccess(QPointer<QTcpSocket> socket, const QString userId, bool isAdmin);

    private:
        QPointer<QTcpSocket> m_socket; // 改为弱指针
        quint8        m_type;
        QString       m_token;
        QJsonObject   m_req;
        QJsonObject   m_res;
        QString       m_dbId;

        UserController*         m_userCtrl;
        AccountController*      m_accountCtrl;
        TransactionController*  m_txnCtrl;
        AdminController*        m_adminCtrl;
};



#endif