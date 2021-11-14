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

struct async_add
{
	using RetType = int;
	async_add(int p) :init_(p) {}
	int init_;
	IMPL_AWAITABLE(nicehero::TO_DB)
};

int async_add::execute()
{
	return init_ + 1;
}
#include "MongoAsync.hpp"
#include "CoroAwaitable.hpp"

/*
void f1(){}
typedef nicehero::Awaitable<f1,nicehero::TO_DB,int,int,int> async_f1;
template<>
async_f1::RetType async_f1::execute()
{
	return std::get<0>(args) + std::get<1>(args);
}
*/
CORO_AWAITABLE(async_f1,nicehero::TO_DB,int,int,int)
{
	return std::get<0>(args) + std::get<1>(args);
}
CORO_AWAITABLE(async_f2,nicehero::TO_DB,int,int,int)
{
	return std::get<0>(args) * std::get<1>(args);
}

CORO_AWAITABLE_EX(CoroAsyncXX2,nicehero::TO_DB,int,int,int)
(int x,int y)
{
	return x + y + 200;
}

CORO_AWAITABLE_EX(CoroAsyncXX3,nicehero::TO_DB,int,int,int)
(int x,int y)
{
	return x + y + 300;
}
using copy_int = nicehero::CopyablePtr<int>;
using unique_int = std::unique_ptr<int>;
CORO_AWAITABLE_EX(CoroAsyncXX4,nicehero::TO_DB,copy_int,int,int)
(int x,int y)
{
	return nicehero::CopyablePtr<int>(new int(x + y + 400));
}
CORO_AWAITABLE_EX(CoroAsyncXX5,nicehero::TO_DB,copy_int,unique_int,int)
(unique_int x,int y)
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
			auto nPool = pool;
			std::stringstream ss;
			int r = 0;
			if (r > 2) {
				co_return true;
			}
			r = co_await async_f2(2,7);
			ss << "co_await async_f2(2,7):" << r << "\n";
			res->write(ss.str());
			
			r = co_await CoroAsyncXX2(2,7);
			(ss = std::stringstream()) << "co_await CoroAsyncXX2(2,7):" << r << "\n";
			res->write(ss.str());

			auto xx = std::make_unique<int>(2);
			//auto xx = nicehero::make_copyable<int>(2);
			auto r2 = co_await CoroAsyncXX5(std::move(xx),7);
			(ss = std::stringstream()) << "co_await CoroAsyncXX5(2,7):" << *r2 << "\n";
			res->write(ss.str());
			auto cursor = co_await nicehero::MongoPoolFindAsync3(nPool
				, "x"
				, NBSON("_id", BCON_INT32(1))
				, nicehero::Bson::createBsonPtr()
				, MONGOC_READ_PRIMARY);
			nlog("co_await nicehero::MongoPoolFindAsync ok");
			while (auto bobj = cursor->fetch()) {
				res->write(bobj->toJson());
			}
			
			r = co_await async_f1(1,2);
			(ss = std::stringstream()) << "co_await async_f1(1,2)" << r << "\n";
			res->write(ss.str());
			if (r > 2) {
				co_return true;
			}
			r += co_await async_add(1);
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
	msg >> d;
	d.s1 = "xxxx";
	std::string s;
	s.assign(d.s2.m_Data.get(), d.s2.m_Size);
#ifdef NICE_HAS_CO_AWAIT
	int r = co_await async_add(1);
	r += 1;
#endif
	nlog("tcp recv XData size:%d,%s", int(msg.getSize()),s.c_str());
	MyClient& client = (MyClient&)session;
	client.sendMessage(d);
	co_return true;
}

TCP_SESSION_COMMAND(MyClient, 101)
{
	MyClient& client = (MyClient&)session;
	//nlog("recv101 recv101Num:%d", client.recv101Num);
	++client.recv101Num;
#ifdef NICE_HAS_CO_AWAIT
	int r = co_await async_add(1);
	r += 1;
#endif
	co_return true;
}
static int numClients = 0;

TCP_SESSION_COMMAND(MyClient, 102)
{
	MyClient& client = (MyClient&)session;
	++numClients;
	nlog("tcp recv102 recv101Num:%d,%d", client.recv101Num,numClients);
	return true;
}

KCP_SESSION_COMMAND(MyKcpSession, XDataID)
{
	XData d;
	msg >> d;
	d.s1 = "xxxx";
	std::string s;
	s.assign(d.s2.m_Data.get(), d.s2.m_Size);
	nlog("kcp recv XData size:%d,%s", int(msg.getSize()),s.c_str());
	
	session.sendMessage(d);
	return true;
}
