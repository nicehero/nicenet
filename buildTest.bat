g++ -I./dep/include -I./dep/include/asio -L./dep/lib -L./ server_test.cpp TestProtocolCmd.cpp  -lnicenet -lpthread -likcp -lsha3 -luECC -lwinmm -lws2_32 -lmswsock -o testServer -Wl,-rpath=dep/lib:./
g++ -I./dep/include -I./dep/include/asio -L./dep/lib -L./ client_test.cpp TestProtocolCmd.cpp  -lnicenet -lpthread -likcp -lsha3 -luECC -lwinmm -lws2_32 -lmswsock -o testClient -Wl,-rpath=dep/lib:./
