#ifndef ____SERVICE___
#define ____SERVICE___

#include <functional>

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
	void start(bool background = false);
	void stop();
	void post(std::function<void()> f, ToService to = TO_MAIN);
	asio::io_context& getWorkerService();
	asio::io_context& getDBService();
	
	void joinMain();
}
#if !__has_include(<experimental/coroutine>)
#define STDCORO std
#else
#define STDCORO std::experimental
#endif
#define IMPL_AWAITABLE_FULL(DO_SERVICE,TO_SERVICE) \
	using call_back = std::function<void()>;\
	bool await_ready() const { return false; }\
	RetType await_resume() { return std::move(result_); }\
	void await_suspend(STDCORO::coroutine_handle<> handle) {\
		nicehero::post([handle, this]() {\
			result_ = execute();\
			if (DO_SERVICE != TO_SERVICE)nicehero::post([handle,this]{ handle.resume(); },TO_SERVICE);\
			else handle.resume();\
		}, DO_SERVICE);\
	}\
	RetType execute();\
	RetType result_;
#define IMPL_AWAITABLE(DO_SERVICE) \
	IMPL_AWAITABLE_FULL(DO_SERVICE,nicehero::TO_MAIN)
#endif
