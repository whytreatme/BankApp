// MetricsManager.h
#pragma once
#include <QAtomicInt>
#include <QAtomicInteger> // 用于 qint64
#include <algorithm>

class MetricsManager {
public:
    static MetricsManager& instance() {
        static MetricsManager inst;
        return inst;
    }

    void recordSuccess(qint64 elapsedMs) { 
        m_successCount.fetchAndAddRelaxed(1); 
        m_totalTaskTime.fetchAndAddRelaxed(elapsedMs); // 累加时间
        updateMaxTime(elapsedMs);
    }
    void recordFailure() { m_failureCount.fetchAndAddRelaxed(1); }
    
    void reset() {
        m_successCount = 0;
        m_failureCount = 0;
        m_totalTaskTime = 0;
        m_maxTaskTime = 0;
    }

    //int success() const { return m_successCount.loadRelaxed(); }
    //int failure() const { return m_failureCount.loadRelaxed(); }

    void printReport(qint64 totalAppTimeMs) {
        int s = m_successCount.loadRelaxed();
        int f = m_failureCount.loadRelaxed();
        int total = s + f;
        double qps = totalAppTimeMs > 0 ? (double)s / (totalAppTimeMs / 1000.0) : 0;

        // ✅ 使用 qInfo()，它在 Release 模式下依然会输出
        // .noquote() 是为了不让字符串带上多余的双引号
        qInfo().noquote() << "\n========================================";
        qInfo().noquote() << "[FINAL REPORT] 压力测试终极战报";
        qInfo().noquote() << "总请求数 :" << total;
        qInfo().noquote() << "成功数   :" << s << (s == 100000 ? " (✅ 完美通过)" : "");
        qInfo().noquote() << "失败数   :" << f;
        if (total > 0) {
            qInfo().noquote() << "成功率   :" << QString::number((double)s/total * 100, 'f', 2) << "%";
        }
        qInfo().noquote() << "实测 QPS :" << QString::number(qps, 'f', 2) << "req/s";
        qInfo().noquote() << "最大任务耗时 :" << m_maxTaskTime.loadRelaxed() << " ms";
        qInfo().noquote() << "========================================\n";
    }

private:
    void updateMaxTime(qint64 elapsedMs) {
        qint64 currentMax = m_maxTaskTime.loadRelaxed();
        while (elapsedMs > currentMax && 
               !m_maxTaskTime.testAndSetRelaxed(currentMax, elapsedMs)) {
            currentMax = m_maxTaskTime.loadRelaxed();
        }
    }

    QAtomicInt m_successCount{0};
    QAtomicInt m_failureCount{0};
    QAtomicInteger<qint64> m_totalTaskTime{0};
    QAtomicInteger<qint64> m_maxTaskTime{0};
};