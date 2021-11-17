// asio_server.cpp : Defines the entry point for the console application.
//

#include "Service.h"
#include "HttpServer.h"

#include "Tcp.h"
#include "Log.h"
#include "TestProtocol.h"
#include "Clock.h"
#include "Kcp.h"
#include "CopyablePtr.hpp"

class MyClient :public nicehero::TcpSessionS
{
public:
	int recv101Num = 0;
};
class MyServer :public nicehero::TcpServer
{
public:
	MyServer(const std::string& ip, ui16 port)
		:TcpServer(ip, port)
	{}
	virtual nicehero::TcpSessionS* createSession();
};

nicehero::TcpSessionS* MyServer::createSession()
{
	return new MyClient();
}


class MyKcpSession : public nicehero::KcpSessionS
{
public:
};
class MyKcpServer :public nicehero::KcpServer
{
public:
	MyKcpServer(const std::string& ip, ui16 port)
		:KcpServer(ip, port)
	{}
	virtual nicehero::KcpSessionS* createSession();
};

nicehero::KcpSessionS* MyKcpServer::createSession()
{
	return new MyKcpSession();
}


#ifdef NICE_HAS_CO_AWAIT
#include "Mongo.hpp"
#include "MongoAsync.hpp"

nicehero::Task<int, nicehero::TO_MULTIWORKER,nicehero::TO_MAIN> async_add(int x, int y)
{
	return x + y;
}
nicehero::Task<int, nicehero::TO_MULTIWORKER> async_mul(int x,int y)
{
	return x * y;
}

using copy_int = nicehero::CopyablePtr<int>;
using unique_int = std::unique_ptr<int>;
nicehero::Task<copy_int, nicehero::TO_MULTIWORKER> CoroAsyncXX4(int x,int y)
{
	return nicehero::CopyablePtr<int>(new int(x + y + 400));
}

nicehero::Task<copy_int, nicehero::TO_MULTIWORKER> CoroAsyncXX5(unique_int x, int y)
{
	return nicehero::CopyablePtr<int>(new int(*x + y + 500));
}

#endif

int main(int argc, char* argv[])
{
	bool v6 = (argc > 1 && std::string(argv[1]) == "v6") ? true : false;
	nicehero::start(true);
#ifdef NICE_HAS_CO_AWAIT
	std::shared_ptr<nicehero::MongoConnectionPool> pool = std::make_shared<nicehero::MongoConnectionPool>();
	bool r = pool->init("mongodb://192.168.9.5:27018", "test");
	if (!r){
		pool = std::shared_ptr<nicehero::MongoConnectionPool>();
	}
#else
	int pool = 0;
#endif
	std::string listenIP = "0.0.0.0";
	if (v6)
	{
		listenIP = "::";
	}
	nicehero::HttpServer httpServer(listenIP,8080);
	httpServer.addHandler("/", [pool] HTTP_HANDLER_PARAMS
		{
			res->write("hello world:\n");
#ifdef NICE_HAS_CO_AWAIT
			//!!!Must copy first ,if use the catch of lambda  after co_await ,crash!
			//!!!or If u use catch of lambda and coroutine not start from the same thread(TO_MAIN in this sample),crash!
			auto nPool = pool;
			std::stringstream ss;
			int r = 0;
			if (r > 2) {
				co_return true;
			}
			auto xx = std::make_unique<int>(2);
			//auto xx = nicehero::make_copyable<int>(2);
			auto r2 = co_await CoroAsyncXX5(std::move(xx),7);
			(ss = std::stringstream()) << "co_await CoroAsyncXX5(2,7):" << *r2 << "\n";
			res->write(ss.str());
			auto cursor = co_await nicehero::MongoPoolFindAsync(nPool
				, "x"
				, NBSON("_id", BCON_INT32(1))
				, nicehero::Bson::createBsonPtr()
				, MONGOC_READ_PRIMARY);
			nlog("co_await nicehero::MongoPoolFindAsync ok");
			while (auto bobj = cursor->fetch()) {
				res->write(bobj->toJson());
			}
			
			if (r > 2) {
				co_return true;
			}
			r += co_await async_add(1,3);
			co_return true;
#else
			return true;
#endif
	});
	httpServer.addHandler("/v2", [pool] HTTP_HANDLER_PARAMS
		{
			res->write("hello world v2:\n");
			return true;
		});
	httpServer.start();
	MyServer tcpServer(listenIP, 7000);
	tcpServer.accept();
	MyKcpServer kcpServer(listenIP, 7001);
 	kcpServer.accept();
	nicehero::joinMain();
	
	return 0;
}
using namespace Proto;
TCP_SESSION_COMMAND(MyClient, XDataID)
{
	XData d;
	*msg >> d;
	d.s1 = "xxxx";
	std::string s;
	s.assign(d.s2.m_Data.get(), d.s2.m_Size);
#ifdef NICE_HAS_CO_AWAIT
	int r = co_await async_add(2,3);
	r += 1;
#endif
	nlog("tcp recv XData size:%d,%s", int(msg->getSize()), s.c_str());
	session->sendMessage(d);
	co_return true;
}


TCP_SESSION_COMMAND(MyClient, 101)
{
	MyClient& client = (MyClient&)*session.get();
	++client.recv101Num;
	nlog("recv101 recv101Num:%d", client.recv101Num);
#ifdef NICE_HAS_CO_AWAIT
	int r = co_await async_add(1,1);
	r += 1;
#endif
	co_return true;
}
static int numClients = 0;

TCP_SESSION_COMMAND(MyClient, 102)
{
	MyClient& client = (MyClient&)*session.get();
	++numClients;
	nlog("tcp recv102 recv101Num:%d,%d", client.recv101Num,numClients);
#ifdef NICE_HAS_CO_AWAIT
	int r = co_await async_add(1, 1);
	r += 1;
#endif
	co_return true;
}

KCP_SESSION_COMMAND(MyKcpSession, XDataID)
{
#ifdef NICE_HAS_CO_AWAIT
	int r = co_await async_add(1, 1);
	r += 1;
#endif
	XData d;
	*msg >> d;
	d.s1 = "xxxx";
	std::string s;
	s.assign(d.s2.m_Data.get(), d.s2.m_Size);
	nlog("kcp recv XData size:%d,%s", int(msg->getSize()),s.c_str());
	
	session->sendMessage(d);
	co_return true;
}
