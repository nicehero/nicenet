#include "Service.h"
#include <asio/asio.hpp>
#include <random>

namespace nicehero{
	asio::io_context gService(1);
	asio::io_context gMultiWorkerService(WORK_THREAD_COUNT);
	asio::io_context gWorkerServices[WORK_THREAD_COUNT];
	asio::io_context gDBServices[DB_THREAD_COUNT];
	std::thread gMainThread;
}
static int checkCPUendian() {
	union {
		unsigned int a;
		unsigned char b;
	}c;
	c.a = 1;
	return (c.b == 1);

}
/*return 1 : little-endian, return 0:big-endian*/
void nicehero::start(bool background)
{
	if (checkCPUendian() == 0)
	{
		printf("only support little-endian");
		return;
	}
	for (int i = 0; i < WORK_THREAD_COUNT; ++i)
	{
		std::thread t([i] {
			asio::io_context::work work(gWorkerServices[i]);
			gWorkerServices[i].run();
		});
		t.detach();
		std::thread t2([] {
			asio::io_context::work work2(gMultiWorkerService);
			gMultiWorkerService.run();
		});
		t2.detach();
	}
	for (int i = 0; i < DB_THREAD_COUNT; ++i)
	{
		std::thread t([i] {
			asio::io_context::work work(gDBServices[i]);
			gDBServices[i].run();
		});
		t.detach();
	}

	if (background)
	{
		std::thread t([] {
			asio::io_context::work work(gService);
			gService.run();
		});
		gMainThread.swap(t);
	}
	else
	{
		asio::io_context::work work(gService);
		gService.run();
	}
}

void nicehero::stop()
{
	gService.stop();
	for (int i = 0; i < WORK_THREAD_COUNT; ++i)
	{
		gWorkerServices[i].stop();
	}
	gMultiWorkerService.stop();
	for (int i = 0; i < DB_THREAD_COUNT; ++i)
	{
		gDBServices[i].stop();
	}
}

void nicehero::post(std::function<void()> f, ToService to)
{
	switch (to)
	{
	case nicehero::TO_MAIN:
		{
			gService.post(f);
		}
		break;
	case nicehero::TO_WORKER:
		{
			getWorkerService().post(f);
		}
		break;
	case nicehero::TO_DB:
		{
			getDBService().post(f);
		}
		break;
	default:
		{
			gMultiWorkerService.post(f);
		}
		break;
	}
}

asio::io_context& nicehero::getWorkerService()
{
	static thread_local std::default_random_engine e;
	return gWorkerServices[e() % WORK_THREAD_COUNT];
}

asio::io_context& nicehero::getDBService()
{
	static thread_local std::default_random_engine e;
	return gDBServices[e() % DB_THREAD_COUNT];
}

void nicehero::joinMain()
{
	gMainThread.join();
}
