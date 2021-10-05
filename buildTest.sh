g++ -I./dep/include -I./dep/include/asio -L./dep/lib -L./ server_test.cpp TestProtocolCmd.cpp  -lnicenet -o testServer -Wl,-rpath=dep/lib
g++ -I./dep/include -I./dep/include/asio -L./dep/lib -L./ client_test.cpp TestProtocolCmd.cpp  -lnicenet -lws2_32 -o testClient -Wl,-rpath=dep/lib
