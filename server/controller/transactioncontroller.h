#ifndef TRANSACTIONCONTROLLER_H
#define TRANSACTIONCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include "../service/transactionservice.h"

/**
 * @brief 交易控制器 - 处理交易相关的请求
 *
 * 职责：
 * - 参数验证
 * - 调用 Service 层
 * - 返回统一格式的 JSON 响应
 */
class TransactionController : public QObject
{
    Q_OBJECT

public:
    explicit TransactionController(QObject* parent = nullptr);
    ~TransactionController() = default;

    /**
     * @brief 处理交易历史查询请求
     * @param userId 用户 ID
     * @param req 请求 JSON：
     *             - "limit": 返回记录数限制 (int, 可选，默认 20)
     * @return 响应 JSON：
     *         成功：{
     *             "status": "success",
     *             "msg": "查询成功",
     *             "transactions": [...]
     *         }
     *         失败：{"status": "error", "msg": "错误原因"}
     */
    QJsonObject getTransactions(const QString& userId, const QJsonObject& req);

private:
    TransactionService m_service;

    /**
     * @brief 验证 limit 参数
     * @param req 请求 JSON
     * @param limit [out] 返回记录数限制
     * @return 验证通过返回 true，否则返回 false
     */
    bool validateLimit(const QJsonObject& req, int& limit);

    /**
     * @brief 构造错误响应
     */
    QJsonObject errorResponse(const QString& msg);
};

#endif // TRANSACTIONCONTROLLER_H
