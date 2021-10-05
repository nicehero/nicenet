#ifndef ___NICE_KCPSERVER____
#define ___NICE_KCPSERVER____
#include <string>
#include "Type.h"
#include <memory>
#include "Server.h"
#include <unordered_map>
#include <array>
#include <list>
#include <atomic>
#include "Message.h"
#include "NoCopy.h"
#include <functional>
#include "CopyablePtr.hpp"

namespace nicehero
{
	const i32 IKCP_OVERHEAD = 89;
	const i32 INVALID_CONV = (~0);

	class KcpSession;
	using kcpuid = std::string;
	using kcpcommand = bool(*)(KcpSession&, Message&);

	class KcpServer;
	class KcpSessionImpl;
	class KcpServerImpl;
	class KcpMessageParser
	{
	public:
		kcpcommand m_commands[65536] = {nullptr};
	};
	class KcpSessionCommand
	{
	public:
		KcpSessionCommand(const std::type_info & info,
			ui16 command,					
			kcpcommand fucnc			
			);
	};
	class KcpSession
		:public NoCopy,public std::enable_shared_from_this<KcpSession>
	{
		friend class KcpSessionImpl;
		friend class KcpSessionS;
		friend class KcpServer;
		friend class KcpServerImpl;
	public:
		KcpSession();
		virtual void init();
		virtual void init(KcpServer& server);
		virtual void init2(KcpServer& server);
		virtual void init3(KcpServer& server);
		virtual void close();
		virtual void setMessageParser(KcpMessageParser* messageParser);
		virtual void sendMessage(Message& msg,bool pureUdp = false);
		virtual void sendMessage(Serializable& msg, bool pureUdp = false);
		KcpMessageParser* m_MessageParser = nullptr;
		kcpuid& getUid();
	protected:
		void init_kcp();
		std::unique_ptr<KcpSessionImpl> m_impl;
		std::string m_hash;
		kcpuid m_uid;
		ui32 m_conv = INVALID_CONV;
		ui64 m_serialID;
		Message m_PreMsg;
		bool parseMsg(unsigned char* data, ui32 len);
		virtual void removeSelf();
		virtual void removeSelfImpl();
		void doPing();
		virtual void doRead();
		void doSend(Message& msg, bool pureUdp);
		std::atomic_bool m_IsSending;
		std::list<Message> m_SendList;
		virtual void handleMessage(CopyablePtr<Message> msg);
		bool m_Ready = true;
		std::atomic_bool m_closed;
	private:
		ui64 m_lastPingTime = 0;
		ui64 m_lastPongTime = 0;
	};
	class KcpSessionS
		:public KcpSession
	{
		friend class KcpServer;
		friend class KcpSessionImpl;
		friend class KcpServerImpl;
	public:
		KcpSessionS();
		virtual ~KcpSessionS();
		void init(KcpServer& server);
		void init2(KcpServer& server);
		void init3(KcpServer& server);
		void close();
	protected:
		KcpServer* m_KcpServer = nullptr;
		void removeSelf();
		void removeSelfImpl();
		void doRead()final;
	private:
		std::atomic_bool m_waitRemove;
	};
	class KcpSessionC
		:public KcpSession,public Server
	{
	public:
		KcpSessionC();
		virtual ~KcpSessionC();
		
		bool connect(const std::string& ip, ui16 port);
		void init(bool isAsync = false);
		void startRead();
		std::atomic<bool> m_isInit;
	protected:
		void doRead()final;
		void removeSelf();
	private:
		int checkServerSign(ui8* data_);//return 0 ok 1 error 2 warning
		std::shared_ptr<std::string> m_buffer;
		void startRead2();
		std::atomic<bool> m_isStartRead2;
	};
	class KcpServer
		:public Server
	{
		friend class KcpSession;
		friend class KcpSessionS;
	public:
		KcpServer(const std::string& ip,ui16 port );
		virtual ~KcpServer();

		std::unique_ptr<KcpServerImpl> m_impl;

		bool m_Started = false;

		virtual KcpSessionS* createSession();

		virtual void addSession(const kcpuid& uid, std::shared_ptr<KcpSession> session);
		virtual void removeSession(const kcpuid& uid,ui64 serialID);
		virtual void accept();
		std::unordered_map<kcpuid, std::shared_ptr<KcpSession> > m_sessions;
		
		ui32 getFreeUid();
		ui32 m_maxSessions = 10000;
		ui32 m_nextConv = 1;
	};
	KcpMessageParser& getKcpMessagerParse(const std::type_info& typeInfo);
	inline KcpSessionCommand::KcpSessionCommand(const std::type_info & info,  ui16 command, kcpcommand func )
	{
		KcpMessageParser& msgParser = getKcpMessagerParse(info);
		msgParser.m_commands[command] = func;
	}

}


#define KCP_SESSION_COMMAND(CLASS,COMMAND) \
static bool _##CLASS##_##COMMAND##FUNC(nicehero::KcpSession& session, nicehero::Message& msg);\
static nicehero::KcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
static bool _##CLASS##_##COMMAND##FUNC(nicehero::KcpSession& session, nicehero::Message& msg)


#ifndef SESSION_COMMAND
#define SESSION_COMMAND TCP_SESSION_COMMAND
#endif

#define KCP_SESSION_YIELDCMD(CLASS,COMMAND) \
namespace{\
class _##CLASS##_##COMMAND##TASK:asio::coroutine\
{\
	bool exe(nicehero::KcpSession& session, nicehero::Message& msg);\
};\
static nicehero::KcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
bool _##CLASS##_##COMMAND##TASK::exe(nicehero::KcpSession& session, nicehero::Message& msg){\
	reenter(this){
#endif
