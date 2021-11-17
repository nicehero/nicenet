#ifndef ___NICE_TCPSERVER____
#define ___NICE_TCPSERVER____
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
#include "Task.hpp"



namespace nicehero
{
	class TcpSession;
	using tcpuid = std::string;
	using TcpSessionPtr = std::shared_ptr<TcpSession>;
	using MessagePtr = CopyablePtr<Message>;
	using TcpTask = Task<bool,TO_MAIN>;
	using tcpcommand = TcpTask(*)(TcpSessionPtr,MessagePtr);
	//typedef std::function<bool(TcpSession&, Message&)> tcpcommand;
	class TcpServer;
	class TcpSessionImpl;
	class TcpServerImpl;
	class TcpMessageParser
	{
	public:
		tcpcommand m_commands[65536] = {nullptr};
	};
	class TcpSessionCommand
	{
	public:
		TcpSessionCommand(const std::type_info & info,	
			ui16 command,					
			tcpcommand fucnc			
			);
	};
	class TcpSession
		:public NoCopy,public std::enable_shared_from_this<TcpSession>
	{
		friend class TcpSessionImpl;
		friend class TcpServer;
		friend class TcpServerImpl;
	public:
		TcpSession();
		virtual void init();
		virtual void init(TcpServer& server);
		virtual void init2(TcpServer& server);
		virtual void close();
		virtual void setMessageParser(TcpMessageParser* messageParser);
		virtual void sendMessage(Message& msg);
		virtual void sendMessage(const Serializable& msg);
		TcpMessageParser* m_MessageParser = nullptr;
		tcpuid& getUid();
	protected:
		std::unique_ptr<TcpSessionImpl> m_impl;
		std::string m_hash;
		tcpuid m_uid;
		ui64 m_serialID;
		Message m_PreMsg;
		bool parseMsg(unsigned char* data, ui32 len);
		virtual void removeSelf();
		virtual void removeSelfImpl();
		void doRead();
		void doSend(Message& msg);
		void doSend();
		std::atomic_bool m_IsSending;
		std::list<Message> m_SendList;
		virtual void handleMessage(MessagePtr msg);
	};
	class TcpSessionS
		:public TcpSession
	{
		friend class TcpServer;
		friend class TcpSessionImpl;
		friend class TcpServerImpl;
	public:
		TcpSessionS();
		virtual ~TcpSessionS();
		void init(TcpServer& server);
		void init2(TcpServer& server);
	protected:
		TcpServer* m_TcpServer = nullptr;
		void removeSelf();
		void removeSelfImpl();
	};
	class TcpSessionC
		:public TcpSession,public Server
	{
	public:
		TcpSessionC();
		virtual ~TcpSessionC();
		
		bool connect(const std::string& ip, ui16 port);
		void init(bool isAsync = false);
		void startRead();
		std::atomic<bool> m_isInit;
	protected:
		void removeSelf();
	private:
		int checkServerSign(ui8* data_);//return 0 ok 1 error 2 warning
	};
	class TcpServer
		:public Server
	{
		friend class TcpSession;
		friend class TcpSessionS;
	public:
		TcpServer(const std::string& ip,ui16 port );
		virtual ~TcpServer();

		std::unique_ptr<TcpServerImpl> m_impl;

		bool m_Started = false;

		virtual TcpSessionS* createSession();

		virtual void addSession(const tcpuid& uid, TcpSessionPtr session);
		virtual void removeSession(const tcpuid& uid,ui64 serialID);
		virtual void accept();
		std::unordered_map<tcpuid, TcpSessionPtr > m_sessions;
	};
	TcpMessageParser& getTcpMessagerParse(const std::type_info& typeInfo);
	inline TcpSessionCommand::TcpSessionCommand(const std::type_info & info,  ui16 command, tcpcommand func )
	{
		TcpMessageParser& msgParser = getTcpMessagerParse(info);
		msgParser.m_commands[command] = func;
	}

}

#define TCP_SESSION_COMMAND(CLASS,COMMAND) \
static nicehero::TcpTask _##CLASS##_##COMMAND##FUNC(nicehero::TcpSessionPtr session,nicehero::MessagePtr msg);\
static nicehero::TcpSessionCommand _##CLASS##_##COMMAND(typeid(CLASS), COMMAND, _##CLASS##_##COMMAND##FUNC);\
static nicehero::TcpTask _##CLASS##_##COMMAND##FUNC(nicehero::TcpSessionPtr session,nicehero::MessagePtr msg)

#ifndef SESSION_COMMAND
#define SESSION_COMMAND TCP_SESSION_COMMAND
#endif

#endif
