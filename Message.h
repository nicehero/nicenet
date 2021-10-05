#ifndef __NICEMESSAGE____
#define __NICEMESSAGE____
#include <memory>
#include <memory.h>
#include "Type.h"
#include "NoCopy.h"
#include <vector>

#define MSG_SIZE 32 * 1024
namespace nicehero
{

	class Message
		:public NoCopy
	{
	public:
		Message()
		{
			m_buff = nullptr;
		}

		Message(const void* buff, size_t size_)
		{
			m_buff = nullptr;
			if (size_ < 6)
			{
				return;
			}
			buildBuff(size_);
			memcpy(m_buff, buff, size_);
		}
		Message(Message&& other)
		{
			swap(other);
		}
		Message& ID(ui16 msgID);
		void write_data(const void* buff, size_t size_)
		{
			if (getSize() < 1)
			{
				return;
			}
			memcpy(m_buff + m_writePoint, buff, size_);
			m_writePoint += (ui32)size_;
		}
		const void* read_data(size_t size_)
		{
			if (m_readPoint + size_ > getSize())
			{
				//todo throw
				return nullptr;
			}
			ui8* ret = m_buff + m_readPoint;
			m_readPoint += (ui32)size_;
			return ret;
		}
		~Message()
		{
			clear();
		}

		ui16 getMsgID()
		{
			if (!m_buff || m_buff == (unsigned char*)&m_writePoint)
			{
				return 0;
			}
			return *(unsigned short*)(m_buff + 4);
		}

		ui32 getSize()
		{
			if (!m_buff || m_buff == (unsigned char*)&m_writePoint)
			{
				return 0;
			}
			return *(unsigned long*)(m_buff);
		}

		virtual void serialize()
		{

		}

		void swap(Message& msg)
		{
			serialize();
			unsigned char* buff = msg.m_buff;
			msg.m_buff = m_buff;
			m_buff = buff;
			m_writePoint = 6;
			m_readPoint = 6;
			msg.m_writePoint = 6;
			msg.m_readPoint = 6;
		}

		void clear()
		{
			if (m_buff == (unsigned char*)&m_writePoint)
			{
				m_buff = nullptr;
			}
			if (m_buff)
			{
				delete m_buff;
				m_buff = nullptr;
			}
			m_writePoint = 6;
			m_readPoint = 6;
		}

		void buildBuff(size_t size_)
		{
			clear();
			m_buff = new unsigned char[size_];
			*(ui32*)m_buff = ui32(size_);
		}

		ui8* m_buff;
		ui32 m_writePoint = 6;//write offset
		ui32 m_readPoint = 6;//read offset

	};
	inline Message & Message::ID(ui16 msgID)
	{
		if (!m_buff)
		{
			buildBuff(6);
		}
		*((ui16*)(m_buff + 4)) = msgID;
		return *this;
	}

	struct Serializable
	{
	public:
		virtual ~Serializable() {

		}
		ui32 getSize(const std::string& s) const
		{
			return (ui32)s.size() + 4;
		}
		ui32 getSize(const Binary& s) const
		{
			return (ui32)s.m_Size + 4;
		}
		ui32 getSize(const ui64& s) const
		{
			return sizeof(s);
		}
		ui32 getSize(const OperUInt& s) const
		{
			return sizeof(s.impl);
		}
		ui32 getSize(const StoreUInt& s) const
		{
			return sizeof(s.impl);
		}

		template <typename T>
		ui32 getSize(const std::vector<T>& s) const
		{
			ui32 si = 4;
			for (size_t i = 0; i < s.size(); ++ i)
			{
				si += getSize(s[i]);
			}
			return si;
		}

		ui32 getSize(const Serializable& s) const
		{
			return s.getSize();
		}

		virtual ui32 getSize() const = 0;
		void toMsg(Message& msg) const;
		virtual ui16 getID() const 
		{
			return 0;
		}
		virtual void serializeTo(nicehero::Message& msg) const  = 0;
		virtual void unserializeFrom(nicehero::Message& msg) = 0;
	};

	inline void Serializable::toMsg(Message& msg) const
	{
		ui32 s = 6;
		s += getSize();
		msg.buildBuff(s);
		msg.ID(getID());
		serializeTo(msg);
	}

	inline Message & operator << (Message &m, const ui64 & p)
	{
		m.write_data(&p, sizeof(p));
		return m;
	}
	inline Message & operator << (Message &m, const ui32 & p)
	{
		m.write_data(&p, sizeof(p));
		return m;
	}
	inline Message & operator << (Message &m, const ui16 & p)
	{
		m.write_data(&p, sizeof(p));
		return m;
	}

	inline Message & operator << (Message &m, const StoreUInt & p)
	{
		m.write_data(&(p.impl), sizeof(p.impl));
		return m;
	}

	inline Message & operator << (Message &m, const OperUInt & p)
	{
		m.write_data(&(p.impl), sizeof(p.impl));
		return m;
	}

	inline Message & operator << (Message &m, const std::string& p)
	{
		ui32 s = ui32(p.size());
		m.write_data(&s, 4);
		m.write_data(p.data(), p.size());
		return m;
	}

	inline Message & operator << (Message &m, const Binary& p)
	{
		m.write_data(&p.m_Size, sizeof(p.m_Size));
		m.write_data(p.m_Data.get(),p.m_Size);
		return m;
	}

	template <typename T>
	Message & operator << (Message &m, const std::vector<T> & p)
	{
		ui32 s = ui32(p.size());
		m.write_data(&s, sizeof(ui32));
		for (size_t i = 0; i < p.size(); ++ i)
		{
			m << p[i];
		}
		return m;
	}

	inline Message & operator >> (Message &m, ui64 & p)
	{
		p = *(ui64*)m.read_data(sizeof(p));
		return m;
	}
	inline Message & operator >> (Message &m, ui32 & p)
	{
		p = *(ui32*)m.read_data(sizeof(p));
		return m;
	}
	inline Message & operator >> (Message &m, ui16 & p)
	{
		p = *(ui16*)m.read_data(sizeof(p));
		return m;
	}

	inline Message & operator >> (Message &m, StoreUInt & p)
	{
		p.impl = *(store_uint_base*)m.read_data(sizeof(p.impl));
		return m;
	}

	inline Message & operator >> (Message &m, OperUInt & p)
	{
		p.impl = *(oper_uint_base*)m.read_data(sizeof(p.impl));
		return m;
	}

	inline Message & operator >> (Message &m, std::string& p)
	{
		ui32 s = 0;
		s = *(ui32*)m.read_data(4);
		p.assign((const char*)m.read_data(s), s);
		return m;
	}

	template <typename T>
	Message & operator >> (Message &m, std::vector<T> & p)
	{
		ui32 s = 0;
		s = *(ui32*)m.read_data(4);
		for (size_t i = 0; i < s; ++i)
		{
			T t;
			m >> t;
			p.push_back(std::move(t));
		}
		return m;
	}

	inline Message & operator >> (Message &m, Binary& p)
	{
		p.m_Size = *(ui32*)m.read_data(4);
		if (p.m_Size > 0)
		{
			p.m_Data = std::unique_ptr<char[]>(new char[p.m_Size]);
			memcpy(p.m_Data.get(), m.read_data(p.m_Size), p.m_Size);
		}
		return m;
	}


}

#endif
