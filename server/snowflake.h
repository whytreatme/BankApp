#ifndef SNOWFLAKE_H
#define SNOWFLAKE_H

#include <QtGlobal>
#include <QDateTime>
#include <QMutex>

/**
 * @brief 雪花算法 ID 生成器
 * 
 * 格式：1位符号位 + 41位时间戳 + 10位机器ID + 12位序列号
 */
class Snowflake
{
public:
    static Snowflake& instance() {
        static Snowflake inst;
        return inst;
    }

    /**
     * @brief 设置机器 ID (0-1023)
     */
    void setWorkerId(qint64 workerId) {
        if (workerId >= 0 && workerId <= MAX_WORKER_ID) {
            m_workerId = workerId;
        }
    }

    /**
     * @brief 生成下一个唯一 ID
     */
    qint64 nextId() {
        QMutexLocker locker(&m_mutex);

        qint64 timestamp = currentTimeMillis();

        if (timestamp < m_lastTimestamp) {
            // 时钟回拨处理，简单等待或报错
            return -1;
        }

        if (m_lastTimestamp == timestamp) {
            // 同一毫秒内，序列号递增
            m_sequence = (m_sequence + 1) & MAX_SEQUENCE;
            if (m_sequence == 0) {
                // 序列号溢出，等待下一毫秒
                timestamp = tilNextMillis(m_lastTimestamp);
            }
        } else {
            // 不同毫秒，序列号重置
            m_sequence = 0;
        }

        m_lastTimestamp = timestamp;

        // 拼接 ID
        return ((timestamp - EPOCH) << TIMESTAMP_SHIFT) |
               (m_workerId << WORKER_ID_SHIFT) |
               m_sequence;
    }

private:
    Snowflake() 
        : m_workerId(1)
        , m_sequence(0)
        , m_lastTimestamp(-1) 
    {}

    qint64 currentTimeMillis() {
        return QDateTime::currentMSecsSinceEpoch();
    }

    qint64 tilNextMillis(qint64 lastTimestamp) {
        qint64 timestamp = currentTimeMillis();
        while (timestamp <= lastTimestamp) {
            timestamp = currentTimeMillis();
        }
        return timestamp;
    }

    static const qint64 EPOCH = 1672531200000LL; // 2023-01-01 00:00:00

    static const qint64 WORKER_ID_BITS = 10;
    static const qint64 SEQUENCE_BITS = 12;

    static const qint64 MAX_WORKER_ID = (1LL << WORKER_ID_BITS) - 1;
    static const qint64 MAX_SEQUENCE = (1LL << SEQUENCE_BITS) - 1;

    static const qint64 WORKER_ID_SHIFT = SEQUENCE_BITS;
    static const qint64 TIMESTAMP_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS;

    qint64 m_workerId;
    qint64 m_sequence;
    qint64 m_lastTimestamp;
    QMutex m_mutex;
};

#endif // SNOWFLAKE_H
