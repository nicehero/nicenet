# nicehero

#need GNU 7.3.0 +

sudo apt-get install cmake libssl-dev libsasl2-dev

cd dep

python build.py

#if u need run mongodb

#python buildmongoc.py

cd ..

mkdir build

cd build

#cmake -G"your build tools" ..
#exsample mingw
#cmake -G"MinGW Makefiles" ..

make install

cd ..

#client and server test

#./testServer

#on another shell

#./testClient

#MongoBenchmark

#./buildMongoBenchmark.sh

#./mongoBenchmark <threadNum> <dbconnection> <tablename>

