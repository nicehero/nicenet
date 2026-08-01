#include "Service.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include <asio/yield.hpp>
#undef yield  // ASIO's yield macro conflicts with std::this_thread::yield
#ifdef WIN32
#include <windows.h>
#endif
#include <chrono>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <vector>
#include <thread>
#include "TestProtocol.h"
#include <mongoc/mongoc.h>
#include "Mongo.hpp"
#include "Clock.h"

// Required by TestProtocol.h (extern symbol definition)
namespace Proto {
    const ui16 XDataID = 100;
}

// ============================================================================
// TaskCoordinator - synchronizes benchmark tasks for accurate timing
// ============================================================================
class TaskCoordinator {
public:
    TaskCoordinator() = default;

    // Record the earliest start time across tasks (call at task entry)
    void recordStart() {
        ui64 now = nicehero::Clock::getInstance()->getMilliSeconds();
        ui64 expected = m_startTime.load(std::memory_order_relaxed);
        while (expected == 0 || now < expected) {
            if (m_startTime.compare_exchange_weak(expected, now, std::memory_order_release, std::memory_order_relaxed))
                break;
        }
    }

    // Record the latest end time across tasks (call at task exit)
    void recordEnd() {
        ui64 now = nicehero::Clock::getInstance()->getMilliSeconds();
        ui64 expected = m_endTime.load(std::memory_order_relaxed);
        while (now > expected) {
            if (m_endTime.compare_exchange_weak(expected, now, std::memory_order_release, std::memory_order_relaxed))
                break;
        }
    }

    // Called by each task on completion
    void markDone() {
        m_doneCount.fetch_add(1, std::memory_order_acq_rel);
    }

    void addSuccess(int n) { m_successCount.fetch_add(n, std::memory_order_relaxed); }
    void addError(int n)   { m_errorCount.fetch_add(n, std::memory_order_relaxed); }

    int  getSuccess()   const { return m_successCount.load(); }
    int  getErrors()    const { return m_errorCount.load(); }
    int  getDoneCount() const { return m_doneCount.load(); }
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
// Helper: distribute totalOps across threadNum threads with no truncation
// ============================================================================
static std::vector<int> distributeOps(int totalOps, int threadNum) {
    std::vector<int> ops(threadNum, totalOps / threadNum);
    int remainder = totalOps % threadNum;
    for (int i = 0; i < remainder; ++i) {
        ops[i] += 1;
    }
    return ops;
}

// ============================================================================
// Phase result
// ============================================================================
struct PhaseResult {
    std::string name;
    int totalOps;
    int successOps;
    int errorOps;
    double elapsedMs;
    double qps() const {
        if (elapsedMs <= 0.0) return 0.0;
        return (double)successOps / (elapsedMs / 1000.0);
    }
};

// ============================================================================
// Benchmark phases (defined before use)
// ============================================================================
struct BenchmarkContext;
static void runInsertPhase(BenchmarkContext& ctx);
static void runQueryPhase(BenchmarkContext& ctx);
static void runUpdatePhase(BenchmarkContext& ctx);

struct BenchmarkContext {
    std::shared_ptr<nicehero::MongoConnectionPool> pool;
    std::string tablename;
    int threadNum;
    int totalOps;
    int64_t idOffset = 0;  // shift _id range for multi-process runs
    std::vector<PhaseResult> results;

    // _id = idOffset + (threadIdx + 1) * 1000000 + seq
    // idOffset ensures no collision when running multiple processes
};

static void printResult(const PhaseResult& r) {
    nlog("  %-10s | %8d | %8d | %8d | %10.1f ms | %12.2f",
         r.name.c_str(), r.totalOps, r.successOps, r.errorOps,
         r.elapsedMs, r.qps());
}

// ============================================================================
// PHASE 1: INSERT
// ============================================================================
static void runInsertPhase(BenchmarkContext& ctx) {
    nlog("--- INSERT Phase ---");
    auto opsPerThread = distributeOps(ctx.totalOps, ctx.threadNum);
    TaskCoordinator coord;

    for (int t = 0; t < ctx.threadNum; ++t) {
        int threadIdx = t;
        int myOps = opsPerThread[t];
        nicehero::post([&ctx, &coord, threadIdx, myOps]() {
            coord.recordStart();  // atomic min-start-time

            int succ = 0, errs = 0;
            bson_error_t lastErr = {0,0,{0}};
            for (int i = 0; i < myOps; ++i) {
                auto doc = NBSON_T(
                    "_id",   BCON_INT64(ctx.idOffset + (threadIdx + 1) * 1000000LL + i),
                    "hello", BCON_UTF8("world"),
                    "seq",   BCON_INT32(i),
                    "thread", BCON_INT32(threadIdx)
                );
                bson_error_t error = ctx.pool->insert(ctx.tablename, doc);
                if (error.code == 0) ++succ; else { ++errs; lastErr = error; }
            }
            if (errs > 0 && succ == 0) {
                nlog("  [thread %d] insert error: code=%d msg=%s",
                     threadIdx, lastErr.code, lastErr.message);
            }
            coord.addSuccess(succ);
            coord.addError(errs);
            coord.recordEnd();  // atomic max-end-time
            coord.markDone();
        }, nicehero::TO_DB);
    }

    while (!coord.isDone(ctx.threadNum)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    PhaseResult r;
    r.name       = "INSERT";
    r.totalOps   = ctx.totalOps;
    r.successOps = coord.getSuccess();
    r.errorOps   = coord.getErrors();
    r.elapsedMs  = (double)coord.getElapsedMs();
    ctx.results.push_back(r);
    printResult(r);
}

// ============================================================================
// PHASE 2: QUERY (find by _id)
// ============================================================================
static void runQueryPhase(BenchmarkContext& ctx) {
    nlog("--- QUERY Phase ---");
    auto opsPerThread = distributeOps(ctx.totalOps, ctx.threadNum);
    TaskCoordinator coord;

    for (int t = 0; t < ctx.threadNum; ++t) {
        int threadIdx = t;
        int myOps = opsPerThread[t];
        nicehero::post([&ctx, &coord, threadIdx, myOps]() {
            coord.recordStart();

            int succ = 0, errs = 0;
            for (int i = 0; i < myOps; ++i) {
                auto cursor = ctx.pool->find(ctx.tablename,
                    NBSON_T("_id", BCON_INT64(ctx.idOffset + (threadIdx + 1) * 1000000LL + i)),
                    nicehero::Bson(nullptr)
                );
                if (cursor && cursor->m_err == 0) {
                    ++succ;
                    while (cursor->fetch()) {}  // drain cursor
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

    while (!coord.isDone(ctx.threadNum)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    PhaseResult r;
    r.name       = "QUERY";
    r.totalOps   = ctx.totalOps;
    r.successOps = coord.getSuccess();
    r.errorOps   = coord.getErrors();
    r.elapsedMs  = (double)coord.getElapsedMs();
    ctx.results.push_back(r);
    printResult(r);
}

// ============================================================================
// PHASE 3: UPDATE (increment a counter field)
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
            for (int i = 0; i < myOps; ++i) {
                auto upd = NBSON("counter", BCON_INT64(1));
                bson_error_t error = ctx.pool->update(ctx.tablename,
                    NBSON_T("_id", BCON_INT64(ctx.idOffset + (threadIdx + 1) * 1000000LL + i)),
                    *upd
                );
                if (error.code == 0) ++succ; else ++errs;
            }
            coord.addSuccess(succ);
            coord.addError(errs);
            coord.recordEnd();
            coord.markDone();
        }, nicehero::TO_DB);
    }

    while (!coord.isDone(ctx.threadNum)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    PhaseResult r;
    r.name       = "UPDATE";
    r.totalOps   = ctx.totalOps;
    r.successOps = coord.getSuccess();
    r.errorOps   = coord.getErrors();
    r.elapsedMs  = (double)coord.getElapsedMs();
    ctx.results.push_back(r);
    printResult(r);
}

// ============================================================================
// Cleanup: drop collection from previous run
// ============================================================================
static void dropCollection(std::shared_ptr<nicehero::MongoConnectionPool> pool,
                           const std::string& dbname, const std::string& tablename) {
    auto client = mongoc_client_pool_pop(pool->m_poolImpl);
    if (!client) {
        nlog("  [warn] Could not get client for cleanup");
        return;
    }
    bson_error_t error;
    auto coll = mongoc_client_get_collection(client, dbname.c_str(), tablename.c_str());
    if (coll) {
        if (mongoc_collection_drop(coll, &error)) {
            nlog("  Dropped existing collection '%s'", tablename.c_str());
        } else if (error.code != 26) {  // 26 = NamespaceNotFound (ok, first run)
            nlog("  [warn] Drop collection failed: code=%d msg=%s", error.code, error.message);
        }
        mongoc_collection_destroy(coll);
    }
    mongoc_client_pool_push(pool->m_poolImpl, client);
}

// ============================================================================
// Warmup: run N operations before benchmark to stabilize connections
// ============================================================================
static void doWarmup(std::shared_ptr<nicehero::MongoConnectionPool> pool,
                     const std::string& tablename, int warmupOps) {
    nlog("Warming up (%d ops)...", warmupOps);
    int succ = 0, errs = 0;
    for (int i = 0; i < warmupOps; ++i) {
        // Use a separate _id range (98xxxxxx) so warmup data doesn't collide with benchmark
        auto doc = NBSON_T(
            "_id",   BCON_INT64(9800000LL + i),
            "hello", BCON_UTF8("warmup"),
            "warmup", BCON_BOOL(true)
        );
        bson_error_t error = pool->insert(tablename, doc);
        if (error.code == 0) ++succ; else ++errs;
    }
    nlog("Warmup complete: %d ok, %d errors", succ, errs);
}

// ============================================================================
// Main benchmark runner
// ============================================================================
static int runBenchmark(const std::string& mongoUrl, const std::string& dbname,
                        const std::string& tablename, int threadNum, int totalOps,
                        int64_t idOffset = 0) {
    // --- Init framework ---
    nicehero::start(true);

    auto pool = std::make_shared<nicehero::MongoConnectionPool>();
    nlog("Connecting to %s / %s ...", mongoUrl.c_str(), dbname.c_str());
    if (!pool->init(mongoUrl, dbname)) {
        nlogerr("FATAL: Failed to connect to MongoDB at %s", mongoUrl.c_str());
        nicehero::stop();
        return 1;
    }
    nlog("Connected OK.");

    // --- Cleanup from previous runs ---
    dropCollection(pool, dbname, tablename);

    // --- Print config ---
    nlog("==============================================");
    nlog("  MongoDB Benchmark V2");
    nlog("  URL:       %s", mongoUrl.c_str());
    nlog("  Database:  %s", dbname.c_str());
    nlog("  Table:     %s", tablename.c_str());
    nlog("  Threads:   %d  (DB thread pool: 16)", threadNum);
    nlog("  Total ops: %d per phase", totalOps);
    nlog("  ID offset: %lld", (long long)idOffset);
    nlog("  Phases:    INSERT -> QUERY -> UPDATE");
    nlog("==============================================");

    BenchmarkContext ctx;
    ctx.pool      = pool;
    ctx.tablename = tablename;
    ctx.threadNum = threadNum;
    ctx.totalOps  = totalOps;
    ctx.idOffset  = idOffset;

    // --- Warmup ---
    doWarmup(pool, tablename, std::min(1000, totalOps / 10));

    // --- Run phases ---
    runInsertPhase(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runQueryPhase(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runUpdatePhase(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // --- Summary ---
    nlog("==============================================");
    nlog("  SUMMARY");
    nlog("  %-10s | %8s | %8s | %8s | %12s | %12s",
         "Phase", "Total", "Success", "Errors", "Time(ms)", "QPS");
    nlog("  -----------+----------+----------+----------+--------------+-------------");
    double totalQps = 0.0;
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
    setvbuf(stdout, NULL, _IONBF, 0);  // disable stdout buffering for real-time output

    if (argc < 4) {
        nlogerr("Usage: mongoBenchmarkV2 <threadNum> <mongoUrl> <tablename> [totalOps] [idOffset]");
        nlogerr("  threadNum  : number of concurrent task posters (1-1000)");
        nlogerr("  mongoUrl   : MongoDB connection string");
        nlogerr("  tablename  : collection name (shared across processes)");
        nlogerr("  totalOps   : total operations per phase (default: 100000)");
        nlogerr("  idOffset   : _id offset for multi-process (default: 0)");
        nlogerr("                e.g. offset=0, 100000000, 200000000 for 3 processes");
        nlogerr("");
        nlogerr("Multi-process example:");
        nlogerr("  start mongoBenchmarkV2 32 mongodb://192.168.1.100 mytable 100000 0");
        nlogerr("  start mongoBenchmarkV2 32 mongodb://192.168.1.100 mytable 100000 100000000");
        nlogerr("  start mongoBenchmarkV2 32 mongodb://192.168.1.100 mytable 100000 200000000");
        return 1;
    }

    int threadNum = atoi(argv[1]);
    if (threadNum < 1)  threadNum = 1;
    if (threadNum > 1000) threadNum = 1000;

    std::string mongoUrl  = argv[2];
    std::string tablename = argv[3];
    int totalOps = (argc >= 5) ? atoi(argv[4]) : 100000;
    if (totalOps < 100)   totalOps = 100;
    if (totalOps > 10000000) totalOps = 10000000;

    int64_t idOffset = (argc >= 6) ? strtoll(argv[5], nullptr, 10) : 0;

    return runBenchmark(mongoUrl, "benchmark", tablename, threadNum, totalOps, idOffset);
}
