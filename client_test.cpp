#define ASIO_STANDALONE
#include "Service.h"
#include "Tcp.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include "Message.h"
#include "TestProtocol.h"
#include "Kcp.h"
#include "Clock.h"

class MyClient :public nicehero::TcpSessionC
{
public:
	void close();
};
class MyKcpClient : public nicehero::KcpSessionC
{
public:
};
void MyClient::close()
{
	nlog("client close");
	TcpSessionC::close();
}
std::shared_ptr<MyKcpClient> kcpc;
std::shared_ptr<MyClient> c;
int main(int argc, char* argv[])
{
	std::string serverIP = "127.0.0.1";
	if (argc > 1)
	{
		serverIP = argv[1];
	}
#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	nicehero::start(true);
	std::vector<std::shared_ptr<MyClient> > cs;
	kcpc = std::make_shared<MyKcpClient>();
	kcpc->connect(serverIP, 7001);
	kcpc->init();
	if (!kcpc->m_isInit)
	{
		nicehero::stop();
		Sleep(1000);
		nicehero::joinMain();
		return 0;
	}
	kcpc->startRead();
	std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(nicehero::gService);
	t->expires_from_now(std::chrono::seconds(1));
	t->async_wait([](std::error_code ec) {
		if (ec)
		{
			nlogerr(ec.message().c_str());
		}
		Proto::XData xxx;
		xxx.n1 = nicehero::Clock::getInstance()->getMilliSeconds();
		xxx.s1 = "66666666";
		xxx.s2 = nicehero::Binary(5, "v666v");
		kcpc->sendMessage(xxx);
	});


	for (int i = 0;i < 1; ++ i)
	{
		c = std::make_shared<MyClient>();
		cs.push_back(c);
		nicehero::post([=] {
			c->connect(serverIP, 7000);
			c->init();
			Proto::XData xxx;
			xxx.n1 = nicehero::Clock::getInstance()->getMilliSeconds();
			xxx.s1 = "66666666";
			xxx.s2 = nicehero::Binary(5, "v666v");
			c->sendMessage(xxx);
			ui32 dat[2] = { 32,0 };
			*(ui16*)(dat + 1) = 101;
			for (int i = 0; i < 100; ++i)
			{
				nicehero::Message msg2(dat, 8);
				c->sendMessage(msg2);
			}
			*(ui16*)(dat + 1) = 102;
			nicehero::Message msg3(dat, 8);
			c->sendMessage(msg3);
			c->startRead();
		});
	}
	nicehero::joinMain();
	return 0;
}

using namespace Proto;

TCP_SESSION_COMMAND(MyClient, XDataID)
{
	XData d;
	msg >> d;
	nlog("tcp ping:%dms",(i32)(nicehero::Clock::getInstance()->getMilliSeconds() - d.n1));

	std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(nicehero::gService);
	t->expires_from_now(std::chrono::seconds(1));
	t->async_wait([t](std::error_code ec) {
		Proto::XData xxx;
		xxx.n1 = nicehero::Clock::getInstance()->getMilliSeconds();
		xxx.s1 = "66666666";
		xxx.s2 = nicehero::Binary(5, "v666v");
		c->sendMessage(xxx);
	});

	return true;
}
KCP_SESSION_COMMAND(MyKcpClient, XDataID)
{
	XData d;
	msg >> d;
	nlog("\t\t\t\tkcp ping:%dms", (i32)(nicehero::Clock::getInstance()->getMilliSeconds() - d.n1));
	std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(nicehero::gService);
	t->expires_from_now(std::chrono::seconds(1));
	t->async_wait([t](std::error_code ec) {
		Proto::XData xxx;
		xxx.n1 = nicehero::Clock::getInstance()->getMilliSeconds();
		xxx.s1 = "66666666";
		xxx.s2 = nicehero::Binary(5, "v666v");
		kcpc->sendMessage(xxx);
	});
	return true;
}
