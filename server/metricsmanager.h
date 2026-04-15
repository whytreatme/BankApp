#include <QAtomicInt>
#include <QAtomicInteger>
#include <QString>
#include <QElapsedTimer>

class MetricsManager {
public:
    static MetricsManager& instance() {
        static MetricsManager inst;
        return inst;
    }

    void setAppTimer(QElapsedTimer* timer) { m_appTimer = timer; }

    void recordSuccess(qint64 elapsedMs) { 
        m_successCount.fetchAndAddRelaxed(1); 
        m_totalTaskTime.fetchAndAddRelaxed(elapsedMs); 
        updateMaxTime(elapsedMs);
        checkAndPrint(); 
    }
    
    void recordFailure() { 
        m_failureCount.fetchAndAddRelaxed(1); 
        checkAndPrint(); 
    }

private:
    void checkAndPrint() {
        int currentTotal = m_totalCount.fetchAndAddRelaxed(1) + 1;

        // 每处理 20,000 个请求，在控制台报个平安（进度条）
        if (currentTotal % 20000 == 0) {
            qInfo().noquote() << "[PROGRESS] 压测中... 已处理请求:" << currentTotal;
        }

        // ✅ 核心：一旦达到 99900 次请求（容错100次），且还没打印过，立刻爆战报！
        if (currentTotal >= 99900 && m_reportPrinted.testAndSetRelaxed(0, 1)) {
            printReport(currentTotal);
        }
    }

    void printReport(int currentTotal) {
        int s = m_successCount.loadRelaxed();
        int f = m_failureCount.loadRelaxed();

        qInfo().noquote() << "\n========================================";
        qInfo().noquote() << "[FINAL REPORT] 压力测试终极战报";
        qInfo().noquote() << "实际收到请求 :" << currentTotal;
        qInfo().noquote() << "成功数       :" << s;
        qInfo().noquote() << "失败数       :" << f;
        
        if (currentTotal > 0) {
            qInfo().noquote() << "成功率       :" << QString::number((double)s / currentTotal * 100, 'f', 2) << "%";
        }

        if (s > 0) {
            qint64 totalTime = m_totalTaskTime.loadRelaxed();
            qInfo().noquote() << "单任务平均耗时:" << QString::number((double)totalTime / s, 'f', 2) << "ms";
            qInfo().noquote() << "单任务最大耗时:" << m_maxTaskTime.loadRelaxed() << "ms";
        }

        if (m_appTimer) {
            qint64 appElapsed = m_appTimer->elapsed();
            double qps = (double)s / (appElapsed / 1000.0);
            qInfo().noquote() << "压测总耗时   :" << appElapsed << "ms";
            qInfo().noquote() << "系统实际 QPS :" << QString::number(qps, 'f', 2) << " req/s";
        }
        qInfo().noquote() << "========================================\n";
    }

    void updateMaxTime(qint64 elapsedMs) {
        qint64 currentMax = m_maxTaskTime.loadRelaxed();
        while (elapsedMs > currentMax && 
               !m_maxTaskTime.testAndSetRelaxed(currentMax, elapsedMs)) {
            currentMax = m_maxTaskTime.loadRelaxed();
        }
    }

    QAtomicInt m_successCount{0};
    QAtomicInt m_failureCount{0};
    QAtomicInt m_totalCount{0};          
    QAtomicInt m_reportPrinted{0};       
    QAtomicInteger<qint64> m_totalTaskTime{0};
    QAtomicInteger<qint64> m_maxTaskTime{0};
    QElapsedTimer* m_appTimer{nullptr};
};