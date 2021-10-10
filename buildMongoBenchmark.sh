g++ -g -Wall -std=c++14  -I./dep/include -I./dep/include/asio -L./dep/lib -lpthread -lsha3 -luECC -lmongoc-1.0 -lbson-1.0 -DASIO_STANDALONE Log.cpp Clock.cpp Server.cpp mongoBenchmark.cpp Tcp.cpp Service.cpp TestProtocolCmd.cpp -o mongoBenchmark -Wl,-rpath=dep/lib

