#ifndef ___CODEPROTO___
#define ___CODEPROTO___
#include "../Message.h"
namespace Proto
{
	struct Code : public nicehero::Serializable
	{
		ui32 value = 1;
		std::string file;
		ui32 line;

		ui32 getSize() const;
		void serializeTo(nicehero::Message& msg) const;
		void unserializeFrom(nicehero::Message& msg);
	};

}

namespace Proto
{
	inline nicehero::Message & operator << (nicehero::Message &m, const Code& p)
	{
		m << p.value;
		m << p.file;
		m << p.line;
		return m;
	}
	inline nicehero::Message & operator >> (nicehero::Message &m, Code& p)
	{
		m >> p.value;
		m >> p.file;
		m >> p.line;
		return m;
	}

	inline void Code::serializeTo(nicehero::Message& msg) const
	{
		msg << (*this);
	}
	inline void Code::unserializeFrom(nicehero::Message& msg)
	{
		msg >> (*this);
	}
	inline ui32 Code::getSize() const
	{
		ui32 s = 0;
		s += nicehero::Serializable::getSize(value);
		s += nicehero::Serializable::getSize(file);
		s += nicehero::Serializable::getSize(line);
		return s;
	}
}
#endif
