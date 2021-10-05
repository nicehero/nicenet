#ifndef __PROTO_TEST_PROTOCOL___
#define __PROTO_TEST_PROTOCOL___

#include "Message.h"

namespace Proto
{
	struct XDataBase :
		public nicehero::Serializable
	{
	public:
		ui64 bn = 1;
		std::string bs;
		std::vector<std::string> bs2 = {"55","66","77"};
		ui32 getSize() const;
		void serializeTo(nicehero::Message& msg) const;
		void unserializeFrom(nicehero::Message& msg);
	};

	extern const ui16 XDataID;
	struct XData :
		public nicehero::Serializable
	{
	public:
		ui64 n1 = 1;
		XDataBase d1;
		std::string s1;
		nicehero::Binary s2;

		ui32 getSize() const;
		ui16 getID() const;
		void serializeTo(nicehero::Message& msg) const;
		void unserializeFrom(nicehero::Message& msg);

	};


}

namespace Proto
{

	inline ui32 XDataBase::getSize() const
	{
		ui32 s = 0;
		s += nicehero::Serializable::getSize(bn);
		s += nicehero::Serializable::getSize(bs);
		s += nicehero::Serializable::getSize(bs2);
		return s;
	}

	inline ui32 XData::getSize() const
	{
		ui32 s = 0;
		s += nicehero::Serializable::getSize(n1);
		s += nicehero::Serializable::getSize(d1);
		s += nicehero::Serializable::getSize(s1);
		s += nicehero::Serializable::getSize(s2);
		return s;
	}

	inline nicehero::Message & operator << (nicehero::Message &m, const XDataBase& p)
	{
		m << p.bn;
		m << p.bs;
		m << p.bs2;
		return m;
	}

	inline nicehero::Message & operator >> (nicehero::Message &m, XDataBase& p)
	{
		m >> p.bn;
		m >> p.bs;
		m >> p.bs2;
		return m;
	}

	inline nicehero::Message & operator << (nicehero::Message &m, const XData& p)
	{
		m << p.n1;
		m << p.d1;
		m << p.s1;
		m << p.s2;
		return m;
	}

	inline nicehero::Message & operator >> (nicehero::Message &m, XData& p)
	{
		m >> p.n1;
		m >> p.d1;
		m >> p.s1;
		m >> p.s2;
		return m;
	}

	inline void XDataBase::serializeTo(nicehero::Message& msg) const
	{
		msg << (*this);
	}
	inline void XDataBase::unserializeFrom(nicehero::Message& msg)
	{
		msg >> (*this);
	}

	inline void XData::serializeTo(nicehero::Message& msg) const
	{
		msg << (*this);
	}

	inline void XData::unserializeFrom(nicehero::Message& msg)
	{
		msg >> (*this);
	}

	inline ui16 XData::getID() const
	{
		return XDataID;
	}

}

#endif
