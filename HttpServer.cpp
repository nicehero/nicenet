#include "HttpServer.h"
#include "Service.h"
#include <asio/asio.hpp>
#include "Log.h"
#include <list>
namespace nicehero
{
	class HttpConnection
		:public NoCopy,public std::enable_shared_from_this<HttpConnection>
	{
	public:
		HttpConnection()
			:m_socket(getWorkerService()) {
			m_IsSending = false;
		}
		void start()
		{
			auto self(shared_from_this());
			std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(getWorkerService());
			m_socket.async_wait(asio::ip::tcp::socket::wait_read,
				[&, self,t](std::error_code ec) {
				t->cancel();
				if (ec)
				{
					return;
				}
				char data_[NETWORK_BUF_SIZE];
				ui32 len = (ui32)self->m_socket.read_some(asio::buffer(data_), ec);
				if (ec)
				{
					return;
				}
				if (len < 1)
				{
					return;
				}
				char* text = data_;
				char* textend = text + len;
				if (m_buffer.size() > 0)
				{
					m_buffer.resize(m_buffer.size() + len);
					memcpy(&(m_buffer[m_buffer.size() - len]), data_, len);
					text = &(m_buffer[0]);
					textend = text + len;
				}
				CopyablePtr<HttpRequest> request = nicehero::make_copyable<HttpRequest>();
				nicehero::HttpRequestParser::ParseResult res = m_parser.parse(*request, text, textend);
				if (res == nicehero::HttpRequestParser::ParsingCompleted)
				{
					if (ui32(request->remainSize) > len)
					{
						return;
					}
					if (ui32(request->remainSize) > 0 && data_[len - ui32(request->remainSize)] == '\0')
					{
						request->remainSize -= 1;
					}
					if (ui32(request->remainSize) > 0)
					{
						m_buffer.resize(ui32(request->remainSize));
						memcpy(&(m_buffer[0]), data_ + len - ui32(request->remainSize), ui32(request->remainSize));
					}
					else
					{
						m_buffer.clear();
					}
					auto keepAlive = request->keepAlive;
					CopyablePtr<HttpResponse> response = nicehero::make_copyable<HttpResponse>();
					response->keepAlive = keepAlive;
					response->statusCode = 404;
					response->status = "nook";
					response->m_Connection = self;
					std::vector<std::string> ss;
					splitString(request->uri, ss, '?');
					request->uri = ss[0];
					if (ss.size() > 1)
					{
						request->paramsString = std::move(ss[ss.size() - 1]);
						std::vector<std::string> ss2;
						splitString(request->paramsString, ss2, '&');
						for (size_t i = 0; i < ss2.size(); ++ i)
						{
							std::vector<std::string> ss3;
							splitString(ss2[i], ss3, '=');
							if (ss3.size() > 1)
							{
								request->params[ss3[0]] = ss3[1];
							}
						}
					}
					auto it = m_Server->m_handles.find(request->uri);
					if (it == m_Server->m_handles.end())
					{
						return;
					}
					auto f = it->second;
					nicehero::post([f,request,response] {
						auto res = response;
						res->statusCode = 200;
						res->status = "OK";
						f(request, res);
					});
					if (keepAlive)
					{
						start();
					}
					return;
				}
				else if (res == nicehero::HttpRequestParser::ParsingError)
				{
					return;
				}
				start();
			});
			t->expires_from_now(std::chrono::seconds(10));
			t->async_wait([&,self](std::error_code ec) {
				if (!ec)
				{
					nlog("session connecting timeout");
					self->m_socket.close();
				}
			});
		}
		nicehero::HttpRequestParser m_parser;
		asio::ip::tcp::socket m_socket;
		HttpServer* m_Server;
		std::vector<char> m_buffer;
		std::list<std::string> m_sendBuffers;
		std::atomic_bool m_IsSending;

		void doSend()
		{
			m_IsSending = true;
			if (m_sendBuffers.size() < 1)
			{
				m_IsSending = false;
				return;
			}
			size_t sendSize = m_sendBuffers.front().size();
			auto self(shared_from_this());

			m_sendBuffers.front().size();
			asio::async_write(m_socket, asio::buffer(m_sendBuffers.front())
				, asio::transfer_at_least(m_sendBuffers.front().size())
				, [this, self, sendSize](asio::error_code ec, std::size_t sentSize) {
				if (ec)
				{
					return;
				}
				if (sentSize < sendSize)
				{
					memmove(&m_sendBuffers.front()[0], &m_sendBuffers.front()[sentSize], sendSize - sentSize);
					m_sendBuffers.front().resize(sendSize - sentSize);
					doSend();
					return;
				}
				m_sendBuffers.pop_front();
				if (m_sendBuffers.size() > 0)
				{
					doSend();
					return;
				}
				m_IsSending = false;
			});
		}
	};
	class HttpServerImpl
	{
	public:
		HttpServerImpl(asio::ip::address ip, ui16 port, HttpServer& server_)
			:m_acceptor(getWorkerService(), { ip,port }), m_server(server_)
		{
		}
		~HttpServerImpl()
		{
			nlogerr("~HttpServerImpl()");
		}
		void accept()
		{
			std::shared_ptr<HttpConnection> s = std::make_shared<HttpConnection>();
			s->m_Server = &m_server;
			m_acceptor.async_accept(s->m_socket,
				[this, s](std::error_code ec) {
				if (ec)
				{
					nlogerr(ec.message().c_str());
					return;
				}
				s->start();
				accept();
			});
		}
		asio::ip::tcp::acceptor m_acceptor;
		HttpServer& m_server;
	};


	HttpServer::HttpServer(const std::string& ip, ui16 port)
	{
		asio::error_code ec;
		auto addr = asio::ip::address::from_string(ip, ec);
		if (ec)
		{
			nlogerr("TcpServer::TcpServer ip error:%s", ip.c_str());
		}
		try
		{
			m_impl = std::unique_ptr<HttpServerImpl>(new HttpServerImpl(addr, port, *this));
		}
		catch (asio::system_error & ec)
		{
			nlogerr("cannot open %s:%d", ip.c_str(), int(port));
			nlogerr("%s", ec.what());
		}
	}

	HttpServer::~HttpServer()
	{

	}

	void HttpServer::start()
	{
		m_impl->accept();
	}

	void HttpServer::addHandler(const std::string& url, HttpHandler f)
	{
		m_handles[url] = f;
	}

	void HttpServer::removeHandler(const std::string& url)
	{
		m_handles.erase(url);
	}

	HttpRequest::~HttpRequest()
	{

	}

	HttpResponse::~HttpResponse()
	{
		if (m_Connection)
		{
			auto conn = m_Connection;
			auto ret = inspect();
			//nlog(ret.c_str());
			m_Connection->m_socket.get_io_context().post([conn,ret] {
				conn->m_sendBuffers.push_back(std::move(ret));
				if (!conn->m_IsSending)
				{
					conn->doSend();
				}
			});
		}
	}

	void HttpResponse::write(const std::string& str)
	{
		content.resize(content.size() + str.size());
		memcpy(&(content[content.size() - str.size()]), str.c_str(), str.size());
	}

}
