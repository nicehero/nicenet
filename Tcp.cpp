#include "Tcp.h"
#include "Service.h"

#include <asio/asio.hpp>
#include <micro-ecc/uECC.h>
#include "Log.h"
#include "Message.h"
#include "Clock.h"
#include <map>
#include <asio/yield.hpp>
extern "C"
{
	extern void *sha3(const void *in, size_t inlen, void *md, int mdlen);
}

namespace nicehero
{
	TcpMessageParser& getTcpMessagerParse(const std::type_info& typeInfo)
	{
		static std::map<const std::type_info*, TcpMessageParser> gTcpMessageParse;
		return gTcpMessageParse[&typeInfo];
	}
	class TcpSessionImpl
	{
	public:
		TcpSessionImpl(TcpSession& session)
			:m_iocontext(getWorkerService()),m_socket(m_iocontext),m_session(session)
		{

		}
		asio::io_context& m_iocontext;
		asio::ip::tcp::socket m_socket;
		TcpSession& m_session;
	};

	class TcpServerImpl
	{
public:
		TcpServerImpl(asio::ip::address ip,ui16 port,TcpServer& server_)
			:m_acceptor(getWorkerService(),{ ip,port }),m_server(server_)
		{
		}
		~TcpServerImpl()
		{
			nlogerr("~TcpServerImpl()");
		}
		void accept()
		{
			std::shared_ptr<TcpSession> s = std::shared_ptr<TcpSession>(m_server.createSession());
			TcpSessionS* ss = dynamic_cast<TcpSessionS*>(s.get());
			if (ss)
			{
				ss->m_TcpServer = &m_server;
				ss->m_MessageParser = &getTcpMessagerParse(typeid(*ss));
			}
			m_acceptor.async_accept(s->m_impl->m_socket,
				[this,s](std::error_code ec) {
				if (ec)
				{
					nlogerr(ec.message().c_str());
				}
				s->init(m_server);
				accept();
			});
		}
		asio::ip::tcp::acceptor m_acceptor;
		TcpServer& m_server;
	};

	TcpServer::TcpServer(const std::string& ip, ui16 port)
	{
		asio::error_code ec;
		auto addr = asio::ip::address::from_string(ip, ec);
		if (ec)
		{
			nlogerr("TcpServer::TcpServer ip error:%s", ip.c_str());
		}
		try
		{
			m_impl = std::unique_ptr<TcpServerImpl>(new TcpServerImpl(addr, port,*this));
		}
		catch (asio::system_error & ec)
		{
			nlogerr("cannot open %s:%d", ip.c_str(), int(port));
			nlogerr("%s",ec.what());
		}
	}

	TcpServer::~TcpServer()
	{
		
	}

	TcpSessionS* TcpServer::createSession()
	{
		return new TcpSessionS();
	}

	void TcpServer::addSession(const tcpuid& uid, std::shared_ptr<TcpSession> session)
	{
		auto it = m_sessions.find(uid);
		if (it != m_sessions.end())
		{
			it->second->close();
		}
		m_sessions[uid] = session;
	}

	void TcpServer::removeSession(const tcpuid& uid, ui64 serialID)
	{
		auto it = m_sessions.find(uid);
		if (it != m_sessions.end() && it->second->m_serialID == serialID)
		{
			it->second->close();
			m_sessions.erase(uid);
		}
	}

	void TcpServer::accept()
	{
		m_impl->accept();
	}

	TcpSessionS::TcpSessionS()
	{

	}

	TcpSessionS::~TcpSessionS()
	{

	}

	void TcpSessionS::init(TcpServer& server)
	{
		auto self(shared_from_this());
		ui8 buff[PUBLIC_KEY_SIZE + 8 + HASH_SIZE + SIGN_SIZE] = {0};
		memcpy(buff, server.m_publicKey, PUBLIC_KEY_SIZE);
		ui64 now = nNow;
		*(ui64*)(buff + PUBLIC_KEY_SIZE) = now;
		sha3(buff, PUBLIC_KEY_SIZE + 8, buff + PUBLIC_KEY_SIZE + 8, HASH_SIZE);
		m_hash = std::string((const char*)(buff + PUBLIC_KEY_SIZE + 8),HASH_SIZE);
		if (uECC_sign(server.m_privateKey
			, buff + PUBLIC_KEY_SIZE + 8, HASH_SIZE
			, buff + PUBLIC_KEY_SIZE + 8 + HASH_SIZE
			, uECC_secp256k1()) != 1
			) 
		{
			nlogerr("uECC_sign() failed\n");
			return;
		}
// 		if (uECC_verify(buff, (const ui8*)(buff + PUBLIC_KEY_SIZE + 8), HASH_SIZE, buff + PUBLIC_KEY_SIZE + 8 + HASH_SIZE, uECC_secp256k1()) != 1)
// 		{
// 			nlogerr("error check hash2");
// 		}

		m_impl->m_socket.async_write_some(
			asio::buffer(buff, sizeof(buff)), 
			[&,self](std::error_code ec,size_t s) {
			if (ec)
			{
				nlogerr("%d\n", ec.value());
				return;
			}
			self->init2(server);
		});
	}

	void TcpSessionS::init2(TcpServer& server)
	{
		auto self(shared_from_this());
		std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(getWorkerService());
		m_impl->m_socket.async_wait(
			asio::ip::tcp::socket::wait_read,
			[&, self,t](std::error_code ec)	{
			t->cancel();
			if (ec)
			{
				return;
			}
			ui8 data_[PUBLIC_KEY_SIZE + SIGN_SIZE] = "";
			std::size_t len = m_impl->m_socket.read_some(
				asio::buffer(data_, sizeof(data_)), ec);
			if (ec)
			{
				return;
			}
			if (len < sizeof(data_))
			{
				return;
			}
			bool allSame = true;
			for (size_t i = 0; i < PUBLIC_KEY_SIZE; ++ i)
			{
				if (server.m_publicKey[i] != data_[i])
				{
					allSame = false;
					break;
				}
			}
			if (allSame)
			{
				return;
			}
			if (uECC_verify(data_, (const ui8*)m_hash.c_str(), HASH_SIZE, data_ + PUBLIC_KEY_SIZE, uECC_secp256k1()) != 1)
			{
				return;
			}
			m_uid = std::string((const char*)data_, PUBLIC_KEY_SIZE);
			static ui64 nowSerialID = 10000;
			m_serialID = nowSerialID++;
			nicehero::post([&,this, self] {
				server.addSession(m_uid, self);
				doRead();
			});

		});
		t->expires_from_now(std::chrono::seconds(2));
		t->async_wait([self](std::error_code ec) {
			if (!ec)
			{
				nlog("session connecting timeout");
				self->close();
			}
		});

	}


	void TcpSessionS::removeSelf()
	{
		auto self(shared_from_this());
		nicehero::post([&,self] {
			removeSelfImpl();
		});
	}

	void TcpSessionS::removeSelfImpl()
	{
		if (m_TcpServer)
		{
			m_TcpServer->removeSession(m_uid, m_serialID);
		}
	}

	TcpSession::TcpSession()
	{
		m_impl = std::unique_ptr<TcpSessionImpl>(new TcpSessionImpl(*this));
		m_IsSending = false;
	}

	void TcpSession::init(TcpServer& server)
	{

	}

	void TcpSession::init()
	{

	}

	void TcpSession::init2(TcpServer& server)
	{

	}

	void TcpSession::doRead()
	{
		auto self(shared_from_this());
		this->m_impl->m_socket.async_wait(asio::ip::tcp::socket::wait_read,
			[=](std::error_code ec) {
			if (ec)
			{
				self->removeSelf();
				return;
			}
			unsigned char data_[NETWORK_BUF_SIZE];
			ui32 len = (ui32)self->m_impl->m_socket.read_some(asio::buffer(data_), ec);
			if (ec)
			{
				self->removeSelf();
				return;
			}
			if (len > 0)
			{
				if (!parseMsg(data_, len))
				{
					self->removeSelf();
					return;
				}
			}
			doRead();
		});
	}

	bool TcpSession::parseMsg(unsigned char* data, ui32 len)
	{
		if (len > (ui32)NETWORK_BUF_SIZE)
		{
			return false;
		}
		Message& prevMsg = m_PreMsg;
		auto self(shared_from_this());
		if (prevMsg.m_buff == nullptr)
		{
			if (len < 4)
			{
// 				nlog("TcpSession::parseMsg len < 4");
				memcpy(&prevMsg.m_writePoint, data, len);
				prevMsg.m_buff = (unsigned char*)&prevMsg.m_writePoint;
				prevMsg.m_readPoint = len;
				return true;
			}
			ui32 msgLen = *((ui32*)data);
			if (msgLen > MSG_SIZE)
			{
				return false;
			}
			if (msgLen <= len)
			{
				auto recvMsg = make_copyable<Message>(data, *((ui32*)data));
				
// 				if (m_MessageParser && m_MessageParser->m_commands[recvMsg->getMsgID()] == nullptr)
// 				{
// 					nlogerr("TcpSession::parseMsg err 1");
// 				}

				nicehero::post([self,recvMsg] {
					self->handleMessage(recvMsg);
				});
				if (msgLen < len)
				{
					return parseMsg( data + msgLen, len - msgLen);
				}
				else
				{
					return true;
				}
			}
			else
			{
				prevMsg.m_buff = new unsigned char[msgLen];
				memcpy(prevMsg.m_buff, data, len);
				prevMsg.m_writePoint = len;
				return true;
			}
		}
		ui32 msgLen = 0;
		ui32 cutSize = 0;
		if (prevMsg.m_buff == (unsigned char*)&prevMsg.m_writePoint)
		{
			if (prevMsg.m_readPoint + len < 4)
			{
// 				nlog("TcpSession::parseMsg prevMsg.m_readPoint + len < 4");
				memcpy(((unsigned char*)&prevMsg.m_writePoint) + prevMsg.m_readPoint
					, data, len);
				prevMsg.m_readPoint = prevMsg.m_readPoint + len;
				return true;
			}
			cutSize = 4 - prevMsg.m_readPoint;
			memcpy(((unsigned char*)&prevMsg.m_writePoint) + prevMsg.m_readPoint
				, data, cutSize);
			msgLen = prevMsg.m_writePoint;
			prevMsg.m_buff = new unsigned char[msgLen];
			memcpy(prevMsg.m_buff, &msgLen, 4);
			prevMsg.m_readPoint = 4;
			prevMsg.m_writePoint = 4;
		}
		msgLen = prevMsg.getSize();
		if (msgLen > MSG_SIZE)
		{
			return false;
		}
		if (len + prevMsg.m_writePoint - cutSize >= msgLen)
		{
// 			ui32 oldWritePoint = 0;//test value
// 			oldWritePoint = prevMsg.m_writePoint;//test value
			memcpy(prevMsg.m_buff + prevMsg.m_writePoint, data + cutSize, msgLen - prevMsg.m_writePoint);
			data = data + cutSize + (msgLen - prevMsg.m_writePoint);
			len = len - cutSize - (msgLen - prevMsg.m_writePoint);
			auto recvMsg = CopyablePtr<Message>();
			recvMsg->swap(prevMsg);
// 			if (m_MessageParser && m_MessageParser->m_commands[recvMsg->getMsgID()] == nullptr)
// 			{
// 				nlogerr("TcpSession::parseMsg err 2");
// 			}

			nicehero::post([this,self,recvMsg] {
				self->handleMessage(recvMsg);
			});
			if (len > 0)
			{
				return parseMsg( data, len);
			}
			return true;
		}
// 		nlog("TcpSession::parseMsg else");
		memcpy(prevMsg.m_buff + prevMsg.m_writePoint, data + cutSize, len - cutSize);
		prevMsg.m_writePoint += len - cutSize;
		return true;
	}

	void TcpSession::removeSelf()
	{
	}

	void TcpSession::removeSelfImpl()
	{

	}

	void TcpSession::handleMessage(CopyablePtr<Message> msg)
	{
		if (m_MessageParser)
		{
			if (m_MessageParser->m_commands[msg->getMsgID()] == nullptr)
			{
				nlogerr("TcpSession::handleMessage undefined msg:%d", ui32(msg->getMsgID()));
				return;
			}
			m_MessageParser->m_commands[msg->getMsgID()](*this, *msg.get());
		}
	}

	void TcpSession::close()
	{
		m_impl->m_socket.close();
	}

	void TcpSession::setMessageParser(TcpMessageParser* messageParser)
	{
		m_MessageParser = messageParser;
	}

	std::string& TcpSession::getUid()
	{
		return m_uid;
	}

	void TcpSession::doSend(Message& msg)
	{
		auto self(shared_from_this());
		std::shared_ptr<Message> msg_ = std::make_shared<Message>();
		msg_->swap(msg);
		m_impl->m_iocontext.post([this,self, msg_] {
			//same thread ,no need lock
			m_SendList.emplace_back();
			m_SendList.back().swap(*msg_);
			if (m_IsSending)
			{
				return;
			}
			doSend();
		});
	}

	void TcpSession::doSend()
	{
		auto self(shared_from_this());
		m_IsSending = true;
		while (m_SendList.size() > 0 && m_SendList.front().m_buff == nullptr)
		{
			m_SendList.pop_front();
		}
		if (m_SendList.empty())
		{
			m_IsSending = false;
			return;
		}
		ui8* data = m_SendList.front().m_buff;
		ui32 size_ = m_SendList.front().getSize();
		if (m_SendList.size() > 1 && size_ <= (ui32)NETWORK_BUF_SIZE)
		{
			ui8 data2[NETWORK_BUF_SIZE];
			size_ = 0;
			while (m_SendList.size() > 0)
			{
				Message& msg = m_SendList.front();
				if (size_ + msg.getSize() > (ui32)NETWORK_BUF_SIZE)
				{
					break;
				}
				memcpy(data2 + size_, msg.m_buff, msg.getSize());
				size_ += msg.getSize();
				m_SendList.pop_front();
			}
			asio::async_write(m_impl->m_socket,asio::buffer(data2, size_)
				, asio::transfer_at_least(size_)
				, [this,self, size_](asio::error_code ec, std::size_t s) {
				if (ec)
				{
					removeSelf();
					return;
				}
				if (s < size_)
				{
					nlogerr("async_write buffer(data2, size_) err s:%d < size_:%d", int(s), int(size_));
					removeSelf();
					return;
				}
				if (m_SendList.size() > 0)
				{
					doSend();
					return;
				}
				m_IsSending = false;
			});
		}
		else
		{
			asio::async_write(m_impl->m_socket
				, asio::buffer(data, size_)
				, asio::transfer_at_least(size_)
				, [this, self, size_](asio::error_code ec, std::size_t s) {
				if (ec)
				{
					removeSelf();
					return;
				}
				if (s < size_)
				{
					nlogerr("async_write buffer(data2, size_) err s:%d < size_:%d", int(s), int(size_));
					removeSelf();
					return;
				}
				m_SendList.pop_front();
				if (m_SendList.size() > 0)
				{
					doSend();
					return;
				}
				m_IsSending = false;
			});
		}

	}

	void TcpSession::sendMessage(Message& msg)
	{
		doSend(msg);
	}

	void TcpSession::sendMessage(const Serializable& msg)
	{
		Message msg_;
		msg.toMsg(msg_);
		sendMessage(msg_);
	}

	TcpSessionC::TcpSessionC()
	{
		m_isInit = false;
		m_impl = std::unique_ptr<TcpSessionImpl>(new TcpSessionImpl(*this));
	}

	TcpSessionC::~TcpSessionC()
	{

	}

	bool TcpSessionC::connect(const std::string& ip, ui16 port)
	{
		asio::error_code ec;
		auto addr = asio::ip::address::from_string(ip, ec);
		if (ec)
		{
			nlogerr("TcpSessionC::TcpSessionC ip error:%s", ip.c_str());
			return false;
		}
		m_impl->m_socket.connect({addr,port} , ec);
		if (ec)
		{
			nlogerr("TcpSessionC::TcpSessionC connect error:%s", ec.message().c_str());
			return false;
		}
		return true;
	}

	void TcpSessionC::init(bool isAsync)
	{
		std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(getWorkerService());
		auto f = [&, t](std::error_code ec) {
			t->cancel();
			if (ec)
			{
				nlogerr("TcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			ui8 data_[PUBLIC_KEY_SIZE + 8 + HASH_SIZE + SIGN_SIZE] = "";
			std::size_t len = m_impl->m_socket.read_some(
				asio::buffer(data_, sizeof(data_)), ec);
			if (ec)
			{
				nlogerr("TcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			if (len < sizeof(data_))
			{
				nlogerr("server sign data len err");
				return;
			}
			if (checkServerSign(data_) == 1)
			{
				nlogerr("server sign err");
				return;
			}
			ui8 sendSign[PUBLIC_KEY_SIZE + SIGN_SIZE] = { 0 };
			memcpy(sendSign, m_publicKey, PUBLIC_KEY_SIZE);
			if (uECC_sign(m_privateKey
				, (const ui8*)data_ + PUBLIC_KEY_SIZE + 8, HASH_SIZE
				, sendSign + PUBLIC_KEY_SIZE
				, uECC_secp256k1()) != 1)
			{
				nlogerr("uECC_sign() failed\n");
				return;
			}
			m_uid = std::string((const char*)data_, PUBLIC_KEY_SIZE);
			static ui64 nowSerialID = 10000;
			m_serialID = nowSerialID++;
			m_impl->m_socket.write_some(asio::buffer(sendSign, PUBLIC_KEY_SIZE + SIGN_SIZE), ec);
			if (ec)
			{
				nlogerr("TcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			m_isInit = true;
			m_MessageParser = &getTcpMessagerParse(typeid(*this));
		};
		if (isAsync)
		{
			m_impl->m_socket.async_wait(
				asio::ip::tcp::socket::wait_read,f);
			t->expires_from_now(std::chrono::seconds(2));
			t->async_wait([&](std::error_code ec) {
				if (!ec)
				{
					close();
				}
			});
		}
		else
		{
			f(std::error_code());
		}
	}


	void TcpSessionC::startRead()
	{
		doRead();
	}

	void TcpSessionC::removeSelf()
	{
		close();
	}

	int TcpSessionC::checkServerSign(ui8* data_)
	{
		bool allSame = true;
		for (size_t i = 0; i < PUBLIC_KEY_SIZE; ++i)
		{
			if (m_publicKey[i] != data_[i])
			{
				allSame = false;
				break;
			}
		}
		if (allSame)
		{
			nlogerr("same publicKey");
			return 1;
		}
		ui64& serverTime = *(ui64*)(data_ + PUBLIC_KEY_SIZE);
		int ret = 0;
		if (nNow > serverTime + 10 || nNow < serverTime - 10)
		{
			nlogerr("your time is diff from serverTime");
			ret = 2;
		}
		ui64 checkHash[HASH_SIZE / 8] = { 0 };
		sha3(data_, PUBLIC_KEY_SIZE + 8, checkHash, HASH_SIZE);
		allSame = true;
		for (size_t i = 0; i < HASH_SIZE / 8; ++i)
		{
			if (checkHash[i] != ((ui64*)(data_ + PUBLIC_KEY_SIZE + 8))[i])
			{
				allSame = false;
			}
		}
		if (!allSame)
		{
			nlogerr("error check hash");
			return 1;
		}
		if (uECC_verify(data_, (const ui8*)(data_ + PUBLIC_KEY_SIZE + 8), HASH_SIZE, data_ + PUBLIC_KEY_SIZE + 8 + HASH_SIZE, uECC_secp256k1()) != 1)
		{
			nlogerr("error check hash2");
			return 1;
		}
		return ret;
	}


}

