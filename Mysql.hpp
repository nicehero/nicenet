#ifndef ___NICE_MYSQL_HPP__
#define ___NICE_MYSQL_HPP__

#include <mysql.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <queue>
#include "NoCopy.h"
#include "Log.h"

namespace nicehero {

// ============================================================================
// MysqlError — lightweight error wrapper matching Mongo's bson_error_t pattern
// ============================================================================
struct MysqlError {
    unsigned int code = 0;
    std::string message;
    bool ok() const { return code == 0; }
};

// ============================================================================
// MysqlResult — RAII wrapper around MYSQL_RES, eagerly drains all rows
// ============================================================================
struct MysqlResult : public NoCopy {
    int               errCode = 0;
    std::string       errMsg;
    MYSQL_RES*        res  = nullptr;
    int               rowCount = 0;

    MysqlResult() = default;
    MysqlResult(int ec, const std::string& em, MYSQL_RES* r = nullptr)
        : errCode(ec), errMsg(em), res(r) {
        if (res) {
            rowCount = (int)mysql_num_rows(res);
        }
    }
    ~MysqlResult() {
        if (res) mysql_free_result(res);
    }
    bool ok() const { return errCode == 0; }
    int rows() const { return rowCount; }
};

using MysqlResultPtr = std::shared_ptr<MysqlResult>;

// ============================================================================
// MysqlConnection — RAII single connection wrapper (popped from pool)
// ============================================================================
class MysqlConnectionPool;  // fwd

struct MysqlConnection : public NoCopy {
    friend class MysqlConnectionPool;
    MYSQL*             handle = nullptr;
    MysqlConnectionPool& pool;
    MysqlConnection(MysqlConnectionPool& p, MYSQL* h) : handle(h), pool(p) {}
    ~MysqlConnection();
};

// ============================================================================
// MysqlConnectionPool — thread-safe connection pool
// ============================================================================
class MysqlConnectionPool : public NoCopy {
public:
    ~MysqlConnectionPool();

    bool init(const std::string& host,
              const std::string& user,
              const std::string& passwd,
              const std::string& dbname,
              int port = 3306,
              int poolSize = 64);

    // Execute INSERT/UPDATE/DELETE — returns error
    MysqlError execute(const std::string& sql);

    // Execute SELECT — returns result with row data
    MysqlResultPtr query(const std::string& sql);

    // Pop/push for manual use (RAII via MysqlConnection)
    std::shared_ptr<MysqlConnection> popConn();
    void pushConn(MYSQL* conn);

    const std::string& getDBName() const { return m_dbname; }

private:
    std::string         m_dbname;
    std::mutex          m_mutex;
    std::queue<MYSQL*>  m_pool;
    int                 m_poolSize = 0;
};

using MysqlPoolPtr = std::shared_ptr<MysqlConnectionPool>;

// ============================================================================
// Inline implementations
// ============================================================================

inline MysqlConnection::~MysqlConnection() {
    if (handle) {
        pool.pushConn(handle);
    }
}

inline MysqlConnectionPool::~MysqlConnectionPool() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_pool.empty()) {
        MYSQL* c = m_pool.front(); m_pool.pop();
        if (c) {
            mysql_close(c);
        }
    }
}

inline bool MysqlConnectionPool::init(
    const std::string& host,
    const std::string& user,
    const std::string& passwd,
    const std::string& dbname,
    int port,
    int poolSize)
{
    m_dbname = dbname;
    m_poolSize = poolSize;

    for (int i = 0; i < poolSize; ++i) {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) {
            nlogerr("mysql_init failed");
            return false;
        }
        // Force mysql_native_password for MySQL 5.6 compat
        unsigned int timeout = 5;
        mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);

        MYSQL* ret = mysql_real_connect(conn,
            host.c_str(), user.c_str(), passwd.c_str(),
            dbname.c_str(), port, nullptr, 0);
        if (!ret) {
            nlogerr("mysql_real_connect[%d] failed: %s", i, mysql_error(conn));
            mysql_close(conn);
            // Still return false if we can't connect at all
            if (i == 0) return false;
            continue;
        }
        // Enable auto-reconnect
        my_bool reconnect = 1;
        mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

        m_pool.push(conn);
    }
    if (m_pool.empty()) {
        nlogerr("No connections could be established");
        return false;
    }
    nlog("MySQL pool created: %zu connections to %s:%d/%s",
         m_pool.size(), host.c_str(), port, dbname.c_str());
    return true;
}

inline std::shared_ptr<MysqlConnection> MysqlConnectionPool::popConn() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pool.empty()) {
        return nullptr;
    }
    MYSQL* c = m_pool.front(); m_pool.pop();
    return std::make_shared<MysqlConnection>(*this, c);
}

inline void MysqlConnectionPool::pushConn(MYSQL* conn) {
    if (!conn) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pool.push(conn);
}

inline MysqlError MysqlConnectionPool::execute(const std::string& sql) {
    auto c = popConn();
    if (!c || !c->handle) {
        MysqlError err;
        err.code = 1;
        err.message = "pool empty";
        return err;
    }
    if (mysql_real_query(c->handle, sql.c_str(), (unsigned long)sql.size()) != 0) {
        MysqlError err;
        err.code = mysql_errno(c->handle);
        err.message = mysql_error(c->handle);
        return err;
    }
    // Consume any result set (for INSERT/UPDATE there may be none, but be safe)
    MYSQL_RES* res = mysql_store_result(c->handle);
    if (res) mysql_free_result(res);
    MysqlError err; // code=0 = success
    return err;
}

inline MysqlResultPtr MysqlConnectionPool::query(const std::string& sql) {
    auto c = popConn();
    if (!c || !c->handle) {
        return std::make_shared<MysqlResult>(1, "pool empty", nullptr);
    }
    if (mysql_real_query(c->handle, sql.c_str(), (unsigned long)sql.size()) != 0) {
        return std::make_shared<MysqlResult>(
            (int)mysql_errno(c->handle),
            mysql_error(c->handle),
            nullptr);
    }
    MYSQL_RES* res = mysql_store_result(c->handle);
    if (!res) {
        // No result set (unlikely for SELECT, but handle gracefully)
        return std::make_shared<MysqlResult>(0, "", nullptr);
    }
    return std::make_shared<MysqlResult>(0, "", res);
}

} // namespace nicehero

#endif
