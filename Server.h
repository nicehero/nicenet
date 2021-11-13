#ifndef __NICE_SERVER___
#define __NICE_SERVER___

#include "Type.h"
#include <string>
#include "NoCopy.h"
#ifdef NICE_HAS_CO_AWAIT
#if !__has_include(<experimental/coroutine>)
#include <coroutine>
#else
#include <experimental/coroutine>
#endif
#endif
namespace nicehero
{
	const ui32 PUBLIC_KEY_SIZE = 64;
	const ui32 HASH_SIZE = 32;
	const ui32 SIGN_SIZE = 64;
	const ui32 PRIVATE_KEY_SIZE = 32;
	class Server
		:public NoCopy
	{
	public:
		Server();

		bool SetPrivateKeyString(const std::string& privateKeyString);
		std::string GetPrivateKeyString();
		std::string GetPublicKeyString();
		ui8 m_privateKey[PRIVATE_KEY_SIZE] = { 0 };
		ui8 m_publicKey[PUBLIC_KEY_SIZE] = { 0 };
	protected:
	private:

	};

#ifdef NICE_HAS_CO_AWAIT
	struct AwaitableRet
	{
		struct promise_type {
			auto get_return_object() { return AwaitableRet(true); }
#if !__has_include(<experimental/coroutine>)
			auto initial_suspend() { return std::suspend_never{}; }
			auto final_suspend() noexcept { return std::suspend_never{}; }
#else
			auto initial_suspend() { return std::experimental::suspend_never{}; }
			auto final_suspend() noexcept { return std::experimental::suspend_never{}; }
#endif
			void unhandled_exception() { std::terminate(); }
			//void return_void() {}
			void return_value(bool value) { _return_value = value; }
			bool _return_value;
		};
		bool ret;
		AwaitableRet(bool r) :ret(r) {}
		AwaitableRet() :ret(true) {}
	};
	struct VAwaitableRet
	{
		struct promise_type {
			auto get_return_object() { return VAwaitableRet(); }
#if !__has_include(<experimental/coroutine>)
			auto initial_suspend() { return std::suspend_never{}; }
			auto final_suspend() { return std::suspend_never{}; }
#else
			auto initial_suspend() { return std::experimental::suspend_never{}; }
			auto final_suspend() { return std::experimental::suspend_never{}; }
#endif
			void unhandled_exception() { std::terminate(); }
			void return_void() {}
		};
	};
#else
	using AwaitableRet = bool;
	using VAwaitableRet = void;
#endif
}

#endif

