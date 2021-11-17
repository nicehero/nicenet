cd dep
python build.py
cd ..
mkdir buildCoro
cd buildCoro
cmake -G"MinGW Makefiles" -DCORO=ON  ..
make install
cd ..
PAUSE