@echo off
setlocal

REM ============================================================
REM  buildMongoBenchmarkV2.bat
REM  Builds the improved MongoDB benchmark (mongoBenchmarkV2)
REM ============================================================

set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

REM --- Check prerequisites ---
if not exist "dep\lib\libmongoc-static-1.0.a" (
    echo [ERROR] dep\lib not built. Run: cd dep ^&^& python build.py ^&^& python buildmongoc.py
    exit /b 1
)
if not exist "libnicenet.dll.a" (
    echo [ERROR] libnicenet not built. Run buildOnMinGW.bat first.
    exit /b 1
)

echo ============================================================
echo  Building mongoBenchmarkV2.exe
echo ============================================================

g++ -O2 -Wall ^
    -I./dep/include -I./dep/include/asio ^
    -L./dep/lib -L./ ^
    -DASIO_STANDALONE ^
    mongoBenchmarkV2.cpp ^
    -lnicenet -lpthread ^
    -lmongoc-static-1.0 -lbson-static-1.0 ^
    -ldnsapi -lbcrypt -lsecur32 -lcrypt32 -lz ^
    -lwinmm -lws2_32 -lmswsock ^
    -static-libgcc -static-libstdc++ ^
    -o mongoBenchmarkV2.exe ^
    -Wl,-rpath=dep/lib

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [FAILED] Build failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo [OK] mongoBenchmarkV2.exe built successfully.
echo.
echo Usage: mongoBenchmarkV2.exe ^<threadNum^> ^<mongoUrl^> ^<tablename^> [totalOps]
echo Example: mongoBenchmarkV2.exe 8 mongodb://192.168.1.100 mytable 100000
echo.
pause
