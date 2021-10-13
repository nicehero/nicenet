mkdir ndkBuild
cd ndkBuild
cmake -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="%NDK_ROOT%\build\cmake\android.toolchain.cmake" -DANDROID_NDK="%NDK_ROOT%" -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI="arm64-v8a" -DCMAKE_MAKE_PROGRAM="%NDK_ROOT%\prebuilt\windows-x86_64\bin\make.exe" ..
make install
PAUSE