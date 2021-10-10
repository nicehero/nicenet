cd dep
python build.py
cd ..
mkdir build
cd build
cmake ..
make install
cd ..
buildTest.sh