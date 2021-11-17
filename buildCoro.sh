cd dep
python build.py
cd ..
mkdir buildCoro
cd buildCoro
cmake -DCORO=ON ..
make install
cd ..
