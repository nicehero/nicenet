#ifndef ____NICE__HTTPSERVER____
#define ____NICE__HTTPSERVER____

#include "HttpParser.hpp"
#include "NoCopy.h"
#include "Type.h"
#include "CopyablePtr.hpp"
#include <functional>
#include <unordered_map>
#include "Server.h"

namespace nicehero
{
	class HttpConnection;
	class HttpServerImpl;
	class HttpRequest;
	class HttpResponse;
	using HttpHandler = std::function<AwaitableRet(CopyablePtr<HttpRequest> req, CopyablePtr<HttpResponse> res)>;
	class HttpServer
		:public NoCopy
	{
		friend class HttpConnection;
	public:
		HttpServer(const std::string& ip, ui16 port);
		virtual ~HttpServer();
		void start();

		void addHandler(
			const std::string& url,
			HttpHandler f
			);

		void removeHandler(const std::string& url);
		std::unique_ptr<HttpServerImpl> m_impl;
	protected:
		std::unordered_map<std::string, HttpHandler> m_handles;
	private:
	};

	class HttpRequest:
		public HttpRequestBase,NoCopy
	{
		friend class HttpConnection;
	public:
		~HttpRequest();
	protected:
		std::shared_ptr<HttpConnection> m_Connection;
	};
	class HttpResponse :
		public HttpResponseBase, NoCopy
	{
		friend class HttpConnection;
	public:
		~HttpResponse();
		void write(const std::string& str);
	protected:
		std::shared_ptr<HttpConnection> m_Connection;
	};


}
#ifdef NICE_HAS_CO_AWAIT
#define HTTP_HANDLER_PARAMS (nicehero::CopyablePtr<nicehero::HttpRequest> req, nicehero::CopyablePtr<nicehero::HttpResponse> res)->nicehero::AwaitableRet
#else
#define HTTP_HANDLER_PARAMS (nicehero::CopyablePtr<nicehero::HttpRequest> req, nicehero::CopyablePtr<nicehero::HttpResponse> res) 
#endif
#endif

