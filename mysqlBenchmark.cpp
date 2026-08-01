#include "Service.h"
#include "Log.h"
#include "Clock.h"
#include "Mysql.hpp"
#include <atomic>
#include <vector>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef WIN32
#include <windows.h>
#endif

// ============================================================================
// TaskCoordinator — atomic timing coordination (same as mongo version)
// ============================================================================
class TaskCoordinator {
public:
    void recordStart() {
        ui64 now = nicehero::Clock::getInstance()->getMilliSeconds();
        ui64 expected = m_startTime.load(std::memory_order_relaxed);
        while (expected == 0 || now < expected) {
            if (m_startTime.compare_exchange_weak(expected, now,
                std::memory_order_release, std::memory_order_relaxed)) break;
        }
    }
    void recordEnd() {
        ui64 now = nicehero::Clock::getInstance()->getMilliSeconds();
        ui64 expected = m_endTime.load(std::memory_order_relaxed);
        while (now > expected) {
            if (m_endTime.compare_exchange_weak(expected, now,
                std::memory_order_release, std::memory_order_relaxed)) break;
        }
    }
    void markDone()  { m_doneCount.fetch_add(1, std::memory_order_acq_rel); }
    void addSuccess(int n) { m_successCount.fetch_add(n, std::memory_order_relaxed); }
    void addError(int n)   { m_errorCount.fetch_add(n, std::memory_order_relaxed); }

    int  getSuccess()   const { return m_successCount.load(); }
    int  getErrors()    const { return m_errorCount.load(); }
    bool isDone(int total) const { return m_doneCount.load() >= total; }
    ui64 getElapsedMs() const { return m_endTime.load() - m_startTime.load(); }

private:
    std::atomic<ui64> m_startTime{0};
    std::atomic<ui64> m_endTime{0};
    std::atomic<int>  m_doneCount{0};
    std::atomic<int>  m_successCount{0};
    std::atomic<int>  m_errorCount{0};
};

// ============================================================================
// Helpers
// ============================================================================
static std::vector<int> distributeOps(int totalOps, int threadNum) {
    std::vector<int> ops(threadNum, totalOps / threadNum);
    int remainder = totalOps % threadNum;
    for (int i = 0; i < remainder; ++i) ops[i] += 1;
    return ops;
}

struct PhaseResult {
    std::string name;
    int totalOps;
    int successOps;
    int errorOps;
    double elapsedMs;
    double qps() const {
        return elapsedMs > 0 ? (double)successOps / (elapsedMs / 1000.0) : 0.0;
    }
};

static void printResult(const PhaseResult& r) {
    nlog("  %-10s | %8d | %8d | %8d | %10.1f ms | %12.2f",
         r.name.c_str(), r.totalOps, r.successOps, r.errorOps,
         r.elapsedMs, r.qps());
}

// ============================================================================
// Benchmark context
// ============================================================================
struct BenchmarkContext {
    std::shared_ptr<nicehero::MysqlConnectionPool> pool;
    std::string tablename;
    int threadNum;
    int totalOps;
    int64_t idOffset = 0;
    std::vector<PhaseResult> results;
};

static void dropTable(std::shared_ptr<nicehero::MysqlConnectionPool> pool,
                      const std::string& tablename) {
    std::string sql = "DROP TABLE IF EXISTS `" + tablename + "`";
    auto err = pool->execute(sql);
    if (err.ok()) {
        nlog("  Dropped existing table '%s'", tablename.c_str());
    }
}

static void createTable(std::shared_ptr<nicehero::MysqlConnectionPool> pool,
                        const std::string& tablename) {
    std::string sql =
        "CREATE TABLE IF NOT EXISTS `" + tablename + "` ("
        "  id BIGINT PRIMARY KEY,"
        "  hello VARCHAR(50),"
        "  seq INT DEFAULT 0,"
        "  thread_id INT DEFAULT 0,"
        "  counter BIGINT DEFAULT 0"
        ") ENGINE=InnoDB";
    auto err = pool->execute(sql);
    if (!err.ok()) {
        nlogerr("CREATE TABLE failed: [%u] %s", err.code, err.message.c_str());
    }
}

static void doWarmup(std::shared_ptr<nicehero::MysqlConnectionPool> pool,
                     const std::string& tablename, int warmupOps) {
    nlog("Warming up (%d ops)...", warmupOps);
    int succ = 0, errs = 0;
    char buf[512];
    for (int i = 0; i < warmupOps; ++i) {
        snprintf(buf, sizeof(buf),
            "INSERT INTO `%s` (id, hello, seq, thread_id) VALUES (%lld,'warmup',%d,0)",
            tablename.c_str(), (long long)(9800000LL + i), i);
        auto err = pool->execute(buf);
        if (err.ok()) ++succ; else ++errs;
    }
    nlog("Warmup complete: %d ok, %d errors", succ, errs);
}

// ============================================================================
// Phase 1: INSERT
// ============================================================================
static void runInsertPhase(BenchmarkContext& ctx) {
    nlog("--- INSERT Phase ---");
    auto opsPerThread = distributeOps(ctx.totalOps, ctx.threadNum);
    TaskCoordinator coord;

    for (int t = 0; t < ctx.threadNum; ++t) {
        int threadIdx = t;
        int myOps = opsPerThread[t];
        nicehero::post([&ctx, &coord, threadIdx, myOps]() {
            coord.recordStart();
            int succ = 0, errs = 0;
            char buf[512];
            nicehero::MysqlError lastErr;
            for (int i = 0; i < myOps; ++i) {
                int64_t id = ctx.idOffset + (threadIdx + 1) * 1000000LL + i;
                snprintf(buf, sizeof(buf),
                    "INSERT INTO `%s` (id,hello,seq,thread_id) VALUES (%lld,'world',%d,%d)",
                    ctx.tablename.c_str(), (long long)id, i, threadIdx);
                auto err = ctx.pool->execute(buf);
                if (err.ok()) ++succ; else { ++errs; lastErr = err; }
            }
            if (errs > 0 && succ == 0) {
                nlog("  [thread %d] INSERT error: [%u] %s",
                     threadIdx, lastErr.code, lastErr.message.c_str());
            }
            coord.addSuccess(succ);
            coord.addError(errs);
            coord.recordEnd();
            coord.markDone();
        }, nicehero::TO_DB);
    }

    while (!coord.isDone(ctx.threadNum))
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    PhaseResult r;
    r.name = "INSERT"; r.totalOps = ctx.totalOps;
    r.successOps = coord.getSuccess(); r.errorOps = coord.getErrors();
    r.elapsedMs = (double)coord.getElapsedMs();
    ctx.results.push_back(r);
    printResult(r);
}

// ============================================================================
// Phase 2: SELECT (by primary key)
// ============================================================================
static void runSelectPhase(BenchmarkContext& ctx) {
    nlog("--- SELECT Phase ---");
    auto opsPerThread = distributeOps(ctx.totalOps, ctx.threadNum);
    TaskCoordinator coord;

    for (int t = 0; t < ctx.threadNum; ++t) {
        int threadIdx = t;
        int myOps = opsPerThread[t];
        nicehero::post([&ctx, &coord, threadIdx, myOps]() {
            coord.recordStart();
            int succ = 0, errs = 0;
            char buf[256];
            for (int i = 0; i < myOps; ++i) {
                int64_t id = ctx.idOffset + (threadIdx + 1) * 1000000LL + i;
                snprintf(buf, sizeof(buf),
                    "SELECT id,hello,seq,thread_id,counter FROM `%s` WHERE id=%lld",
                    ctx.tablename.c_str(), (long long)id);
                auto r = ctx.pool->query(buf);
                if (r && r->ok()) {
                    ++succ;
                } else {
                    ++errs;
                }
            }
            coord.addSuccess(succ);
            coord.addError(errs);
            coord.recordEnd();
            coord.markDone();
        }, nicehero::TO_DB);
    }

    while (!coord.isDone(ctx.threadNum))
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    PhaseResult r;
    r.name = "SELECT"; r.totalOps = ctx.totalOps;
    r.successOps = coord.getSuccess(); r.errorOps = coord.getErrors();
    r.elapsedMs = (double)coord.getElapsedMs();
    ctx.results.push_back(r);
    printResult(r);
}

// ============================================================================
// Phase 3: UPDATE
// ============================================================================
static void runUpdatePhase(BenchmarkContext& ctx) {
    nlog("--- UPDATE Phase ---");
    auto opsPerThread = distributeOps(ctx.totalOps, ctx.threadNum);
    TaskCoordinator coord;

    for (int t = 0; t < ctx.threadNum; ++t) {
        int threadIdx = t;
        int myOps = opsPerThread[t];
        nicehero::post([&ctx, &coord, threadIdx, myOps]() {
            coord.recordStart();
            int succ = 0, errs = 0;
            char buf[256];
            for (int i = 0; i < myOps; ++i) {
                int64_t id = ctx.idOffset + (threadIdx + 1) * 1000000LL + i;
                snprintf(buf, sizeof(buf),
                    "UPDATE `%s` SET counter = counter + 1 WHERE id=%lld",
                    ctx.tablename.c_str(), (long long)id);
                auto err = ctx.pool->execute(buf);
                if (err.ok()) ++succ; else ++errs;
            }
            coord.addSuccess(succ);
            coord.addError(errs);
            coord.recordEnd();
            coord.markDone();
        }, nicehero::TO_DB);
    }

    while (!coord.isDone(ctx.threadNum))
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    PhaseResult r;
    r.name = "UPDATE"; r.totalOps = ctx.totalOps;
    r.successOps = coord.getSuccess(); r.errorOps = coord.getErrors();
    r.elapsedMs = (double)coord.getElapsedMs();
    ctx.results.push_back(r);
    printResult(r);
}

// ============================================================================
// Main runner
// ============================================================================
static int runBenchmark(const std::string& host, int port,
                        const std::string& user, const std::string& passwd,
                        const std::string& dbname, const std::string& tablename,
                        int threadNum, int totalOps, int64_t idOffset) {
    nicehero::start(true);

    auto pool = std::make_shared<nicehero::MysqlConnectionPool>();
    nlog("Connecting to mysql://%s:%d/%s ...", host.c_str(), port, dbname.c_str());
    if (!pool->init(host, user, passwd, dbname, port, 64)) {
        nlogerr("FATAL: Failed to connect to MySQL at %s:%d", host.c_str(), port);
        nicehero::stop();
        return 1;
    }
    nlog("Connected OK, pool size=64");

    // Only process 0 cleans the slate; others just ensure table exists
    if (idOffset == 0) {
        dropTable(pool, tablename);
    }
    createTable(pool, tablename);  // CREATE IF NOT EXISTS is safer
    // Brief wait for other processes if we just created the table
    if (idOffset == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    nlog("==============================================");
    nlog("  MySQL Benchmark");
    nlog("  Host:      %s:%d", host.c_str(), port);
    nlog("  Database:  %s", dbname.c_str());
    nlog("  Table:     %s", tablename.c_str());
    nlog("  Threads:   %d  (DB thread pool: 16)", threadNum);
    nlog("  Total ops: %d per phase", totalOps);
    nlog("  ID offset: %lld", (long long)idOffset);
    nlog("  Phases:    INSERT -> SELECT -> UPDATE");
    nlog("==============================================");

    BenchmarkContext ctx;
    ctx.pool      = pool;
    ctx.tablename = tablename;
    ctx.threadNum = threadNum;
    ctx.totalOps  = totalOps;
    ctx.idOffset  = idOffset;

    doWarmup(pool, tablename, std::min(1000, totalOps / 10));

    runInsertPhase(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runSelectPhase(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runUpdatePhase(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Summary
    nlog("==============================================");
    nlog("  SUMMARY");
    nlog("  %-10s | %8s | %8s | %8s | %12s | %12s",
         "Phase", "Total", "Success", "Errors", "Time(ms)", "QPS");
    nlog("  -----------+----------+----------+----------+--------------+-------------");
    double totalQps = 0;
    for (auto& r : ctx.results) {
        printResult(r);
        totalQps += r.qps();
    }
    nlog("  ---------------------------------------------");
    nlog("  Average QPS across phases: %.2f", totalQps / ctx.results.size());
    nlog("==============================================");

    nicehero::stop();
    return 0;
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 5) {
        nlogerr("Usage: mysqlBenchmark <threadNum> <host> <dbname> <tablename> [totalOps] [idOffset]");
        nlogerr("  Default port=3306, user=root, passwd from env MYSQL_PWD or empty");
        nlogerr("");
        nlogerr("Multi-process example:");
        nlogerr("  start mysqlBenchmark 32 192.168.1.13 test mytable 100000 0");
        nlogerr("  start mysqlBenchmark 32 192.168.1.13 test mytable 100000 100000000");
        return 1;
    }

    int threadNum = atoi(argv[1]);
    if (threadNum < 1)  threadNum = 1;
    if (threadNum > 1000) threadNum = 1000;

    std::string host      = argv[2];
    std::string dbname    = argv[3];
    std::string tablename = argv[4];
    int totalOps = (argc >= 6) ? atoi(argv[5]) : 100000;
    if (totalOps < 100) totalOps = 100;
    if (totalOps > 10000000) totalOps = 10000000;

    int64_t idOffset = (argc >= 7) ? strtoll(argv[6], nullptr, 10) : 0;

    const char* envPwd = getenv("MYSQL_PWD");
    if (!envPwd) {
        nlogerr("MYSQL_PWD environment variable must be set");
        return 1;
    }
    std::string passwd = envPwd;

    return runBenchmark(host, 3306, "root", passwd,
                        dbname, tablename, threadNum, totalOps, idOffset);
}
