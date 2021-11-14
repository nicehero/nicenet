g++ -std=c++20 -fcoroutines -I./dep/include -I./dep/include/asio -L./dep/lib -L./ server_test.cpp TestProtocolCmd.cpp  -lnicenet -lpthread -likcp -lsha3 -luECC -lwinmm -lws2_32 -lmswsock -lbson-1.0 -lmongoc-1.0 -o testServer -Wl,-rpath=dep/lib:./
g++ -std=c++20 -fcoroutines -I./dep/include -I./dep/include/asio -L./dep/lib -L./ client_test.cpp TestProtocolCmd.cpp  -lnicenet -lpthread -likcp -lsha3 -luECC -lwinmm -lws2_32 -lmswsock -o testClient -Wl,-rpath=dep/lib:./