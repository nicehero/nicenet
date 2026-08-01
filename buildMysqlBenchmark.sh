#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_ROOT"

# Check MySQL library
MYSQL_LIB=""
if [ -f "dep/lib/libmysql.a" ]; then
    MYSQL_LIB="mysql"
elif [ -f "dep/lib/libmysqlclient.so" ]; then
    MYSQL_LIB="mysqlclient"
else
    echo "[ERROR] MySQL not installed. Run: cd dep && python buildmysql.py"
    exit 1
fi

# Check libnicenet
if [ ! -f "libnicenet.a" ]; then
    echo "[ERROR] libnicenet not built. Run build.sh first."
    exit 1
fi

echo "============================================================"
echo " Building mysqlBenchmark"
echo "============================================================"

g++ -O2 -Wall \
    -I./dep/include -I./dep/include/asio -I./dep/include/mysql \
    -L./dep/lib -L./ \
    -DASIO_STANDALONE \
    mysqlBenchmark.cpp \
    -lnicenet -lpthread -l${MYSQL_LIB} \
    -static-libgcc -static-libstdc++ \
    -o mysqlBenchmark \
    -Wl,-rpath='$ORIGIN/dep/lib'

echo "[OK] mysqlBenchmark built."
echo "Usage: ./mysqlBenchmark <threads> <host> <dbname> <tablename> [totalOps] [idOffset]"
echo "Example: ./mysqlBenchmark 32 192.168.1.13 test mytable 100000 0"
