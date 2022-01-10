#ifndef ___EASY__BSON_HPP__
#define ___EASY__BSON_HPP__
#include <bson/bson.h>
#include "Type.h"
#include <string>
#include <memory>
#include "Log.h"
#include "CopyablePtr.hpp"

namespace nicehero
{
	class Bson;
	class BsonView;
	typedef CopyablePtr<Bson> BsonPtr;
	template <bool isView = false>
	class BsonT
	{
	public:
		BsonT() {

		}
		BsonT(bson_t* t)
			:m_bson(t)
		{

		}
		~BsonT()
		{
			if (isView && m_bson)
			{
				bson_destroy(m_bson);
			}
		}
		operator const bson_t*() const
		{
			return m_bson;
		}
		operator bson_t*() const
		{
			return m_bson;
		}

		bool isObject(const char* dotkey) const
		{
			bson_iter_t iter, iter2;
			if (!m_bson
				|| !bson_iter_init(&iter, m_bson)
				|| !bson_iter_find_descendant(&iter, dotkey, &iter2))
			{
				return false;
			}
			auto t = bson_iter_type(&iter2);
			return  t == BSON_TYPE_DOCUMENT || t == BSON_TYPE_NULL || t == BSON_TYPE_UNDEFINED;
		}
		BsonView asObject(const char* dotkey);
		bool isString(const char* dotkey) const 
		{
			bson_iter_t iter, iter2;
			if (!m_bson
				|| !bson_iter_init(&iter, m_bson)
				|| !bson_iter_find_descendant(&iter, dotkey, &iter2))
			{
				return false;
			}
			return bson_iter_type(&iter2) == BSON_TYPE_UTF8;
		}
		const char* asString(const char* dotkey) const
		{
			bson_iter_t iter, iter2;
			if (bson_iter_init(&iter, m_bson)
				&& bson_iter_find_descendant(&iter, dotkey, &iter2)
				&& BSON_ITER_HOLDS_UTF8(&iter2))
			{
				return bson_iter_utf8(&iter2, nullptr);
			}
			return "";
		}

		bool isInt64(const char* dotkey) const
		{
			bson_iter_t iter, iter2;
			if (!m_bson
				|| !bson_iter_init(&iter, m_bson)
				|| !bson_iter_find_descendant(&iter, dotkey, &iter2))
			{
				return false;
			}
			return bson_iter_type(&iter2) == BSON_TYPE_INT64;
		}

		i64 asInt64(const char* dotkey) const
		{
			bson_iter_t iter,iter2;
			if (bson_iter_init(&iter, m_bson)
				&& bson_iter_find_descendant(&iter, dotkey, &iter2) 
				&& BSON_ITER_HOLDS_INT64(&iter2)) 
			{
				return bson_iter_as_int64(&iter2);
			}
			return 0;
		}

		bool isNumber(const char* dotkey) const
		{
			bson_iter_t iter,iter2;
			if (!m_bson 
				|| !bson_iter_init(&iter, m_bson) 
				|| !bson_iter_find_descendant(&iter, dotkey, &iter2))
			{
				return false;
			}
			
			if (iter2.value.value_type == BSON_TYPE_INT64
				|| iter2.value.value_type == BSON_TYPE_INT32
				|| iter2.value.value_type == BSON_TYPE_DECIMAL128
				|| iter2.value.value_type == BSON_TYPE_DOUBLE
				)
			{
				return true;
			}
			return false;
		}

		double asNumber(const char* dotkey) const
		{
			bson_iter_t iter, iter2;
			if (bson_iter_init(&iter, m_bson)
				&& bson_iter_find_descendant(&iter, dotkey, &iter2)
				&& BSON_ITER_HOLDS_NUMBER(&iter2))
			{
				return bson_iter_as_double(&iter2);
			}
			return 0.0;
		}

		std::string toJson() const
		{
			if (!m_bson) {
				return "";
			}
			auto r = bson_as_json(m_bson, nullptr);
			auto rr = std::string(r);
			bson_free(r);
			return rr;
		}

		bson_t* m_bson = nullptr;
		operator bool() const
		{
			if (!m_bson) {
				return false;
			}
			bson_iter_t iter;
			bson_iter_init(&iter, m_bson);
			auto t = bson_iter_type(&iter);
			if (t == BSON_TYPE_NULL || t == BSON_TYPE_UNDEFINED)
			{
				return false;
			}
			if (t == BSON_TYPE_DOCUMENT || t == BSON_TYPE_ARRAY)
			{
				return bson_count_keys(m_bson) > 0;
			}
			return true;
		}
		template <bool b>
		void appendBson(const std::string& k, const BsonT<b>& o)
		{
			bson_append_document(m_bson, k.c_str(), (int)k.length(), o.m_bson);
		}
		void append(const std::string& k, i32 numInt)
		{
			bson_append_int32(m_bson, k.c_str(), (int)k.length(), numInt);
		}
		void append(const std::string& k, i64 numLong)
		{
			bson_append_int64(m_bson, k.c_str(), (int)k.length(), numLong);
		}
		void append(const std::string& k, const std::string& str)
		{
			bson_append_utf8(m_bson, k.c_str(), (int)k.length(), str.c_str(), (int)str.length());
		}
		void append(const std::string& k, double number)
		{
			bson_append_double(m_bson, k.c_str(), (int)k.length(), number);
		}
		template<bool b>
		void merge(const BsonT<b>& o)
		{
			if (!o.m_bson)
			{
				return;
			}
			bson_iter_t iter;
			if (!bson_iter_init(&iter, o.m_bson))
			{
				return;
			}
			while (bson_iter_next(&iter))
			{
				bson_append_iter(m_bson, bson_iter_key(&iter), bson_iter_key_len(&iter), &iter);
			}
		}
	};

	class BsonView : public BsonT<true> {
	public:
		BsonView() {

		}
		BsonView(const uint8_t *buf, uint32_t len) {
			bson_init_static(&m_static_buff, buf, size_t(len));
			m_bson = &m_static_buff;
		}
		bson_t m_static_buff;
	};

	class Bson : public BsonT<false> {
	public:
		Bson() {
			m_bson = bson_new();
		}
		Bson(bson_t* t) : BsonT<false>(t) {
			if (!m_bson)
			{
				m_bson = bson_new();
			}
		}
		Bson(const BsonT<true>& view) {
			if (view) {
				m_bson = bson_copy(view.m_bson);
			}
			else {
				m_bson = bson_new();
			}
		}
		static BsonPtr createBsonPtr()
		{
			return BsonPtr(new Bson(bson_new()));
		}
		static BsonPtr createBsonPtr(const Bson& doc)
		{
			return BsonPtr(new Bson(bson_copy(doc.m_bson)));
		}
	};
	template <bool isView /*= false*/>
	BsonView nicehero::BsonT<isView>::asObject(const char* dotkey)
	{
		bson_iter_t iter, iter2;
		if (bson_iter_init(&iter, m_bson)
			&& bson_iter_find_descendant(&iter, dotkey, &iter2))
		{
			auto t = bson_iter_type(&iter2);
			if (t == BSON_TYPE_DOCUMENT)
			{
				const uint8_t *buf;
				uint32_t len;
				bson_iter_document(&iter, &len, &buf);
				return BsonView(buf, len);
			}
		}
		return BsonView();
	}

}
#ifndef NBSON
#define NBSON(...) nicehero::BsonPtr(new ::nicehero::Bson(bcon_new (NULL, __VA_ARGS__, (void *) NULL))) //BCON_NEW
#define NBSON_T(...) ::nicehero::Bson(bcon_new (NULL, __VA_ARGS__, (void *) NULL)) //BCON_NEW
#endif
#endif // !___EASY__BSON_HPP__
