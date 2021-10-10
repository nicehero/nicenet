cmake -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake" -DANDROID_NDK="%ANDROID_NDK_HOME%" -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI="arm64-v8a" -DCMAKE_MAKE_PROGRAM="%ANDROID_NDK_HOME%\prebuilt\windows-x86_64\bin\make.exe" ..
make install
PAUSE