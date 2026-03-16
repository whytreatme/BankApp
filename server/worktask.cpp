#include"worktask.h"
#include"protocolutils.h"




WorkTask::WorkTask(QTcpSocket* socket, 
                 quint8 type,
                 QString&& token,
                 QJsonObject&& req,
                 UserController* userCtrl,
                 AccountController* accountCtrl,
                 TransactionController* txnCtrl,
                 AdminController*  adminCtrl,
                 QString&&  dbId)
                : m_socket(socket),  m_type(type),
                  m_token(std::move(token)),
                  m_req(std::move(req)),
                  m_userCtrl(userCtrl), 
                  m_accountCtrl(accountCtrl),
                  m_txnCtrl(txnCtrl),
                  m_adminCtrl(adminCtrl),
                  m_dbId(std::move(dbId))
                 {setAutoDelete(false); }
                
void WorkTask::run(){
    
    try {
            switch (m_type) {
           /* case 1:  // 注册
                qDebug() << "Routing to UserController::registerUser";
                m_res = m_userCtrl.registerUser(m_req);
                break;
            */
            case 2:  // 登录
                qDebug() << "Routing to UserController::login";
                m_res = m_userCtrl->login(m_req);
                if (m_res["status"] == "success") {
                    QString newDbId = m_res["user_id"].toString();  // UUID 字符串
                    bool isAdmin = m_res.contains("is_admin") ? m_res["is_admin"].toBool() : false;
                    //TcpReactor::handleAuthSuccess(m_socket, newDbId, isAdmin);
                    emit AuthSuccess(m_socket, newDbId, isAdmin);

                    // 生成 Token 并返回（带 isAdmin 参数）
                    QString token = ProtocolUtils::generateToken(newDbId, isAdmin);
                    m_res["token"] = token;
                    qDebug() << ">>> DEBUG TOKEN FOR PYTHON:" << token;
                    qDebug() << "User" << newDbId << "logged in successfully, token generated"
                             << "(admin:" << isAdmin << ")";
                    
                }
                break;

            case 3:  // 查询余额
                qDebug() << "Routing to AccountController::getBalance for user" << m_dbId;
                m_res = m_accountCtrl->getBalance(m_dbId);
                break;

            case 4:  // 转账
                qDebug() << "Routing to AccountController::transfer for user" << m_dbId;
                m_res = m_accountCtrl->transfer(m_dbId, m_req);
                break;

            case 5:  // 查询交易流水
                qDebug() << "Routing to TransactionController::getTransactions for user" << m_dbId;
                m_res = m_txnCtrl->getTransactions(m_dbId, m_req);
                break;

            /*case 6:  // 审批用户（管理员）
                qDebug() << "Routing to AdminController::approveUser";
                m_res = m_adminCtrl.approveUser(m_req);
                break;
            */
            /*case 7:  // 获取待审批用户列表（管理员）
                qDebug() << "Routing to AdminController::getPendingUsers";
                m_res = m_adminCtrl.getPendingUsers(m_req);
                break;
            */
            case 8:  // 获取所有用户信息（管理员）
                qDebug() << "Routing to AdminController::getAllUsers";
                m_res = m_adminCtrl->getAllUsers(m_req);
                break;

            case 9:  // 修改用户余额（管理员）
                qDebug() << "Routing to AdminController::setUserBalance";
                m_res = m_adminCtrl->setUserBalance(m_req);
                break;

            case 10:  // 修改用户信息（管理员）
                qDebug() << "Routing to AdminController::updateUserInfo";
                m_res = m_adminCtrl->updateUserInfo(m_req);
                break;

            case 11:  // 添加用户（管理员）
                qDebug() << "Routing to AdminController::createUser";
                m_res = m_adminCtrl->createUser(m_req);
                break;

            case 12:  // 重置密码（管理员）
                qDebug() << "Routing to AdminController::resetPassword";
                m_res = m_adminCtrl->resetPassword(m_req);
                break;

            case 13:  // 用户修改密码
                qDebug() << "Routing to UserController::changePassword for user" << m_dbId;
                m_res = m_userCtrl->changePassword(m_req);
                break;

            case 14:  // 管理员修改个人信息
                qDebug() << "Routing to AdminController::updateProfile";
                m_res = m_adminCtrl->updateProfile(m_req);
                break;

            default:
                qWarning() << "Unknown message type:" << m_type;
                m_res = {{"status", "error"}, {"msg", "未知消息类型"}};
                break;
            }
        } catch (const std::exception& e) {
            qCritical() << "Exception handling message:" << e.what();
            m_res = {{"status", "error"}, {"msg", "服务器内部错误"}};
        }
        // 发送响应
       // sendResponse(socket, res);
       emit taskFinished(m_socket, m_res);
    
}
