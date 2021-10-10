cd dep
python build.py
cd ..
mkdir build
cd build
cmake -G"MinGW Makefiles" ..
make install
cd ..
buildTest.bat