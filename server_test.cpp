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


int main(int argc, char* argv[])
{
	bool v6 = (argc > 1 && std::string(argv[1]) == "v6") ? true : false;
	nicehero::start(true);
	std::string listenIP = "0.0.0.0";
	if (v6)
	{
		listenIP = "::";
	}
	nicehero::HttpServer httpServer(listenIP,8080);
	httpServer.addHandler("/", [] HTTP_HANDLER_PARAMS
		{
			res->write("hello world:\n");
			return true;
	});
	httpServer.addHandler("/v2", [] HTTP_HANDLER_PARAMS
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
	nlog("tcp recv XData size:%d,%s", int(msg.getSize()),s.c_str());
	MyClient& client = (MyClient&)session;
	client.sendMessage(d);
	return true;
}

TCP_SESSION_COMMAND(MyClient, 101)
{
	MyClient& client = (MyClient&)session;
	//nlog("recv101 recv101Num:%d", client.recv101Num);
	++client.recv101Num;
	return true;
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
