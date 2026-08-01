@echo off
setlocal
set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

if not exist "dep\lib\libmysql.a" (
    echo [ERROR] MySQL not installed. Run: cd dep ^&^& python buildmysql.py
    exit /b 1
)
if not exist "libnicenet.dll.a" (
    echo [ERROR] libnicenet not built. Run buildOnMinGW.bat first.
    exit /b 1
)

echo ============================================================
echo  Building mysqlBenchmark.exe
echo ============================================================

g++ -O2 -Wall ^
    -I./dep/include -I./dep/include/asio -I./dep/include/mysql ^
    -L./dep/lib -L./ ^
    -DASIO_STANDALONE ^
    mysqlBenchmark.cpp ^
    -lnicenet -lpthread -lmysql ^
    -static-libgcc -static-libstdc++ ^
    -o mysqlBenchmark.exe ^
    -Wl,-rpath=dep/lib

if %ERRORLEVEL% NEQ 0 (
    echo [FAILED] Build failed
    pause
    exit /b %ERRORLEVEL%
)

echo [OK] mysqlBenchmark.exe built.
REM Copy libmysql.dll alongside the exe so it can be found at runtime
copy /Y "dep\lib\libmysql.dll" "libmysql.dll" > nul
echo [OK] libmysql.dll copied to current directory.
echo Usage: mysqlBenchmark.exe ^<threads^> ^<host^> ^<dbname^> ^<tablename^> [totalOps] [idOffset]
echo Example: mysqlBenchmark.exe 32 192.168.1.13 test mytable 100000 0
pause
