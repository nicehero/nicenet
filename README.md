# nicenet - A lightweight coroutine net library for C++

## These include:
* Support multi platform windows,linux,mac,android(ndk),ios
* Tcp Server and Client with auto encryption
* Kcp Server and Client with auto encryption
* Http Server
* MongoDB (need run dep/buildmongoc.py to install driver and include Mongo.hpp)
* Coroutine MongoDB method !(need cmake -DCORO=on and include MongoAsync.hpp)
* Support co_await by nicehero::Task<> !(need cmake -DCORO=on) !(Only support one layer in current)
* Better multi thread model and less context copy (Lock one session on same thread)
* One main thread + 8 work thread + 8 mutli work thread + 16 db thread
* Easy logging
* Easy binary proto for Tcp&Kcp and use proto/ProtoJson2Hpp.py auto build hpp proto code by json file
* Support ipv6


## how to install
Need GNU 11+,if platform windows recommend WinLibs of MinGW64:https://winlibs.com/
```
sudo apt-get install cmake libssl-dev libsasl2-dev
cd dep
python build.py
#if u need mongodb 
#python buildmongoc.py
cd ..
mkdir build
cd build
cmake -DCORO=ON .. #if platform=MinGW64 cmake -G"MinGW Makefiles" -DCORO=ON ..
#only add param -DCORO=ON then support coroutine
make install
cd ..
#client and server test
./buildCoroTest.sh #if platform=MinGW64 buildTestCoro.bat
./testServerCoro #if platform=MinGW64 testServerCoro.exe

#on another shell run test client
./testClientCoro #if platform=MinGW64 testClientCoro.exe

#MongoBenchmark
#./buildMongoBenchmark.sh
#./mongoBenchmark <threadNum> <dbconnection> <tablename>
```

## how to use

Set -DCMAKE_INSTALL_PREFIX=... u will get include files 
and lib file libnicenetCoro(or libnicenet if no use coroutine)

U can -lnicenetCoro link the library , include h and hpp file u need
,and add -std=c++20 -fcoroutines for gcc to support coroutine

## how to use coroutine
```c++
#include "nicenet.h"
using namespace nicehero;
//   awaitable function:                         
template<nicehero::ToService executer = TO_MULTIWORKER//executer thread,default:TO_MULTIWORKER
	,nicehero::ToService return_context = TO_MAIN//end return thread,default:TO_MAIN
	>
Task<int,executer,return_context> sync_add(int x,int y){
//return type: int,executer thread:executer,end return thread:return_context:
	//do something in the thread u want
	nlog("do sync_add in thread:%d",(int)gCurrentService);
	co_return x + y;
}
int main()
{
	nicehero::start(true);
	auto f = [/*use catch in coroutine must very careful*/]
	()->Task<bool,TO_MAIN,TO_MAIN>{
		//return Task and use co_await change a function to coroutine
		//the coroutine start run in TO_MAIN service thread
		nlog("step1 current thread:%d",(int)gCurrentService);
		//run in TO_MULTIWORKER,end to TO_MULTIWORKER
		int r = co_await sync_add<TO_MULTIWORKER,TO_MULTIWORKER>(1,1);
		nlog("step2 current thread:%d",(int)gCurrentService);
		r += co_await sync_add(2,2),//default:run in TO_MULTIWORKER,end to TO_MAIN
		nlog("step3 current thread:%d,r:%d",(int)gCurrentService,r);
		co_return true;
	};
	f();//do coroutine
	nicehero::joinMain();
	return 0;
}
```
* ##### Warning: co_await in co_await coroutine is NOT ALLOWED!
```c++
    //This is error!
    Task<...> a(...){
        ...
        co_return ...
    }
    Task<...> b(...){
        ...
        auto v = co_await a(...);
        ...
        co_return ...
    }
    Task<...> c(...){
        ...
        auto v = co_await b(...);
        ...
        co_return ...
    }
```
## how to make a http server
```c++
#include <Service.h>
#include <HttpServer.h>
int main(){
	std::string listenIP = "0.0.0.0";
#ifdef __IPV6__
	listenIP = "::";
#endif
	//          http                ip      ,port
	nicehero::HttpServer httpServer(listenIP,8080);
	nicehero::start();
	httpServer.addHandler("/", [] HTTP_HANDLER_PARAMS {
			res->write("hello world:\n");
			return true;
	});
	httpServer.start();
	nicehero::joinMain();
	return 0;
}
```

## how to make a tcp/kcp server and recv XData obj and reply
```c++
#include <Service.h>
#include <Tcp.h> //<Kcp.h> for kcp
#include <TestProtocol.h> //test protocol h file,read the file for detail
namespace Proto {
	extern const ui16 XDataID = 100; //protoID
}
class MyClient :public nicehero::TcpSessionS //nicehero::KcpSessionS for kcp
{
public:
};
class MyServer :public nicehero::TcpServer //nicehero::KcpServer for kcp
{
public:
	MyServer(const std::string& ip, ui16 port)
		:TcpServer(ip, port) //KcpServer(ip, port) for kcp
	{}
	virtual nicehero::TcpSessionS* createSession(); //virtual nicehero::KcpSessionS* createSession(); for kcp
};

auto MyServer::createSession()->nicehero::TcpSessionS* //nicehero::KcpSessionS* for kcp
{
	return new MyClient();
}
int main(){
	std::string listenIP = "0.0.0.0";
#ifdef __IPV6__
	listenIP = "::";
#endif
	//tcp/kcp server  ip    , port
	MyServer server(listenIP, 7000);
	server.accept();
	nicehero::joinMain();
	return 0;
}
using namespace Proto;
//                        protoID
SESSION_COMMAND(MyClient, XDataID)//use TCP_SESSION_COMMAND or KCP_SESSION_COMMAND for kcp tcp mix used
{
	XData recv;
	//unserialze to simple obj or protocol obj
	*msg >> recv;
	XData d;
	d.s1 = "aaa";
	d.s2 = nicehero::Binary(5, "xxxxx");//binary type
	session->sendMessage(d);//send to client
	return true;
}
```

## how to make a tcp/kcp client (sendMessage and recv SESSION_COMMAND same as server)
```c++
#include <Service.h>
#include <Tcp.h> //<Kcp.h> for kcp
#include <TestProtocol.h> //test protocol h file,read the file for detail
namespace Proto {
	extern const ui16 XDataID = 100; //protoID
}
class MyClient :public nicehero::TcpSessionC //nicehero::KcpSessionC for kcp
{
public:
};

int main(){
	nicehero::start(true);
	auto c = std::make_shared<MyClient>();//must use shared_ptr
	std::string serverIP = "127.0.0.1";//use ipv6 like "::1"
	//connect  ip      , port
	c->connect(serverIP, 7000);
	c->init();
	if (!c->m_isInit) // connect error
	{
		//do something
		return 0;
	}
	c->startRead();
	//do something with c
	nicehero::joinMain();
	return 0;
}

//SESSION_COMMAND(MyClient, XDataID) recv same as server
```

Any httpServer.addHandler callback or SESSION_COMMAND socket callback 
can use co_await nicehero::Task obj and use co_return instead of return.

Other useage such as MongoDB read example coro_server_test.cpp or mongoBenchmarkImpl.cpp

