#ifndef ___EASY__BSON_HPP__
#define ___EASY__BSON_HPP__
#include <bson/bson.h>
#include "Type.h"
#include <string>
#include <memory>
#include "Log.h"

namespace nicehero
{
	class Bson;
	typedef std::unique_ptr<Bson> BsonPtr;
	class Bson
	{
	public:
		Bson(bson_t* t)
			:m_bson(t)
		{

		}
		~Bson()
		{
			if (m_bson)
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
		void appendBson(const std::string& k, const Bson& o)
		{
			bson_append_document(m_bson, k.c_str(),(int)k.length(), o.m_bson);
		}
		void merge(const Bson& o)
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

		bool isObject(const char* dotkey)
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
		BsonPtr asObject(const char* dotkey)
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
					return BsonPtr(new Bson(bson_new_from_data(buf, len)));
				}
				else if (t != BSON_TYPE_NULL && t != BSON_TYPE_UNDEFINED)
				{
					nlogerr("Bson::asObject error");
				}
			}
			return BsonPtr(new Bson(bson_new()));
		}
		bool isString(const char* dotkey)
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
		std::string asString(const char* dotkey)
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

		bool isInt64(const char* dotkey)
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

		i64 asInt64(const char* dotkey)
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

		bool isNumber(const char* dotkey)
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

		double asNumber(const char* dotkey)
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

		std::string toJson()
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

		static BsonPtr createBsonPtr()
		{
			return BsonPtr(new Bson(bson_new()));
		}
		static BsonPtr createBsonPtr(const Bson& doc)
		{
			return BsonPtr(new Bson(bson_copy(doc.m_bson)));
		}
	};
}
#ifndef NBSON
#define NBSON(...) nicehero::BsonPtr(new ::nicehero::Bson(bcon_new (NULL, __VA_ARGS__, (void *) NULL))) //BCON_NEW
#define NBSON_T(...) ::nicehero::Bson(bcon_new (NULL, __VA_ARGS__, (void *) NULL)) //BCON_NEW
#endif
#endif // !___EASY__BSON_HPP__
