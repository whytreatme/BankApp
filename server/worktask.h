#ifndef WORKTASK_H
#define WORKTASK_H   

#include<QObject>
#include<QTcpSocket>
#include<QJsonObject>
#include<QRunnable>  //用于线程池
#include"controller/usercontroller.h"
#include"controller/accountcontroller.h"
#include"controller/transactioncontroller.h"
#include"controller/admincontroller.h"

class WorkTask : public QObject, QRunnable{
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
        void taskFinished(QTcpSocket* socket, QJsonObject res);

    private:
        QTcpSocket*   m_socket;
        quint8        m_type;
        QString       m_token;
        QJsonOject    m_req;
        QJsonOject    m_res;
        QString       m_dbId;

        UserController*         m_userCtrl;
        AccountController*      m_accountCtrl;
        TransactionController*  m_txnCtrl;
        AdminController*        m_adminCtrl;
};



#endif