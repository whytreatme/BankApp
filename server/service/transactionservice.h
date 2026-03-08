#ifndef TRANSACTIONSERVICE_H
#define TRANSACTIONSERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "../dao/transactiondao.h"
#include "../dao/accountdao.h"

/**
 * @brief 交易服务层 - 处理交易相关的业务逻辑
 *
 * 职责：
 * - 查询交易历史
 * - 格式化交易记录
 */
class TransactionService : public QObject
{
    Q_OBJECT

public:
    explicit TransactionService(QObject* parent = nullptr);
    ~TransactionService() = default;

    /**
     * @brief 查询交易历史
     * @param userId 用户 ID（UUID）
     * @param req 请求 JSON：
     *             - "limit": 返回记录数限制 (int, 可选，默认 20)
     * @return 响应 JSON：
     *         成功：{
     *             "status": "success",
     *             "msg": "查询成功",
     *             "transactions": [
     *                 {
     *                     "trans_id": 123,
     *                     "from_account": 1,
     *                     "to_account": 2,
     *                     "amount": 100.00,
     *                     "timestamp": "2026-01-01 12:00:00",
     *                     "remark": "备注",
     *                     "direction": "out"  // "out"=转出, "in"=转入
     *                 },
     *                 ...
     *             ]
     *         }
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject getTransactions(const QString& userId, const QJsonObject& req);

    /**
     * @brief 查询所有交易记录（管理员用）
     * @param req 请求参数（可包含 limit 字段）
     * @return JSON 响应
     */
    QJsonObject getAllTransactions(const QJsonObject& req);

private:
    TransactionDAO m_txnDao;
    AccountDAO m_accountDao;

    /**
     * @brief 构造错误响应
     */
    QJsonObject errorResponse(const QString& msg);

    /**
     * @brief 构造成功响应
     */
    QJsonObject successResponse(const QString& msg, const QJsonArray& transactions);
};

#endif // TRANSACTIONSERVICE_H
