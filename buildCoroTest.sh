g++ -std=c++20 -fcoroutines -I./dep/include -I./dep/include/asio -L./dep/lib -L./ server_test.cpp TestProtocolCmd.cpp  -lnicenetCoro -lpthread -likcp -lsha3 -luECC -o testServerCoro -Wl,-rpath=dep/lib:./
g++ -std=c++20 -fcoroutines -I./dep/include -I./dep/include/asio -L./dep/lib -L./ client_test.cpp TestProtocolCmd.cpp  -lnicenetCoro -lpthread -likcp -lsha3 -luECC -o testClientCoro -Wl,-rpath=dep/lib:./
