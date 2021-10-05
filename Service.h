#ifndef ____SERVICE___
#define ____SERVICE___

#include <functional>

namespace std
{
	class thread;
};
namespace asio
{
	class io_context;
}

namespace nicehero
{
	enum ToService
	{
		TO_MAIN,
		TO_WORKER,
		TO_DB,
		TO_MULTIWORKER,
	};
	const int WORK_THREAD_COUNT = 8;
	const int DB_THREAD_COUNT = 16;
	extern asio::io_context gService;
	extern asio::io_context gWorkerServices[nicehero::WORK_THREAD_COUNT];
	extern asio::io_context gMultiWorkerService;
	extern asio::io_context gDBServices[nicehero::DB_THREAD_COUNT];
	extern std::thread gMainThread;
	void start(bool background = false);
	void stop();
	void post(std::function<void()> f, ToService to = TO_MAIN);
	asio::io_context& getWorkerService();
	asio::io_context& getDBService();
}

#endif
