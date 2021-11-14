#ifndef ___NICE_COROAWAITABLE____
#define ___NICE_COROAWAITABLE____


#include "Type.h"
#include "Service.h"
#include <memory>
#include <tuple>

#define IMPL_AWAITABLE_FULL2(DO_SERVICE,TO_SERVICE) \
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
	RetType result_;
#define IMPL_AWAITABLE2(DO_SERVICE) \
	IMPL_AWAITABLE_FULL2(DO_SERVICE,nicehero::TO_MAIN)

namespace nicehero
{

	template<void(*f)(),ToService toService,class R, class... CtorArgTypes>
	struct Awaitable
	{
		Awaitable(CtorArgTypes... ctorArgs) : args(ctorArgs...)
		{
		}

		using RetType = R;
		IMPL_AWAITABLE(toService)

		std::tuple<CtorArgTypes...> args;
	};
	
	template<class... CtorArgTypes>
	class CoroAsyncBase {
	public:
		std::tuple<CtorArgTypes...> args;
	};
}

#define CORO_AWAITABLE(AWAITABLE_NAME,...) \
static void FUNCTAG_##AWAITABLE_NAME(){}\
typedef nicehero::Awaitable<FUNCTAG_##AWAITABLE_NAME,__VA_ARGS__> AWAITABLE_NAME;\
template<>\
AWAITABLE_NAME::RetType AWAITABLE_NAME::execute() 



#define CORO_AWAITABLE_EX(AWAITABLE_NAME,TO_SERVICE,R_TYPE,...) \
template<class R, class... CtorArgTypes>\
auto __##AWAITABLE_NAME##_EXECUTE(CtorArgTypes...)->R;\
template<nicehero::ToService toService,class R, class... CtorArgTypes>\
class AWAITABLE_NAME##_BASE {\
public:\
	AWAITABLE_NAME##_BASE(CtorArgTypes... ctorArgs) : args(std::move(ctorArgs)...){}\
	using RetType = R;\
	IMPL_AWAITABLE2(toService)\
	std::tuple<CtorArgTypes...> args;\
	RetType execute(){\
		return exeImpl( typename gens<sizeof ...(CtorArgTypes)>::type() );\
	}\
    template< int ...S >\
    RetType exeImpl( seq<S...>) {\
        return __##AWAITABLE_NAME##_EXECUTE<R,CtorArgTypes...>(std::move(std::get<S>(args)) ...);\
    }\
};\
typedef AWAITABLE_NAME##_BASE<TO_SERVICE,R_TYPE,__VA_ARGS__> AWAITABLE_NAME;\
template<>\
R_TYPE __##AWAITABLE_NAME##_EXECUTE

#endif





