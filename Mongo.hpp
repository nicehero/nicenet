#ifndef ___EASY__MONGO_HPP__
#define ___EASY__MONGO_HPP__

#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "Type.h"
#include <string>
#include <memory>
#include "Bson.hpp"
#include <list>
#include "NoCopy.h"
#include "Log.h"
#include "CopyablePtr.hpp"

namespace nicehero
{
	class MongoCursor
		:public NoCopy
	{
		friend class MongoClient;
		friend class MongoConnectionPool;
	protected:
		MongoCursor(int err, mongoc_cursor_t* cursor = nullptr);
	public:
		virtual ~MongoCursor();
		BsonPtr fetch();
		int m_err = 0;
		std::list<_bson_t*> m_cursorImpl;
	};


	using MongoCursorPtr = CopyablePtr<MongoCursor>;
	class MongoConnectionPool;
	class MongoUpdater
	{
	public:
		MongoUpdater();
		MongoUpdater(const Bson& doc);
		void set(BsonPtr&& doc);
		void unset(BsonPtr&& doc);
		bool available() const;

		operator BsonPtr() const;
		std::vector<BsonPtr> m_sets;
		std::vector<BsonPtr> m_unsets;
	};
	class MongoClient
		:public NoCopy
	{
		friend class MongoConnectionPool;
	protected:
		MongoClient(MongoConnectionPool& pool, mongoc_client_t* c);
	public:
		~MongoClient();
		bson_error_t insert(const std::string& collection
			, const Bson& doc
			, mongoc_insert_flags_t flags = MONGOC_INSERT_NONE
			, bool specialWriteConcern = false
			, int writeConcern = 1
			);

		bson_error_t update(const std::string& collection
			, const Bson& query
			, const MongoUpdater& doc
			, mongoc_update_flags_t flags = MONGOC_UPDATE_NONE
			, bool specialWriteConcern = false
			, int writeConcern = 1
			);

		MongoCursorPtr find(const std::string& collection
			, const Bson& query
			, const Bson& opt
			, mongoc_read_mode_t readMode = MONGOC_READ_PRIMARY
			);

		std::string m_dbname;
	protected:

	private:
		MongoConnectionPool& m_pool;
		mongoc_client_t* m_client;

	};

	using MongoClientPtr = std::shared_ptr<MongoClient>;

	class MongoConnectionPool
		:public NoCopy
	{
	public:
		virtual ~MongoConnectionPool();
		bool init(const std::string& mongoUrl
			, const std::string& dbname
			, int writeConcern = 1
			, const std::string& appName = std::string()
			);

		mongoc_uri_t* m_url = nullptr;
		mongoc_client_pool_t * m_poolImpl = nullptr;

		bson_error_t insert(const std::string& collection
			, const Bson& doc
			, mongoc_insert_flags_t flags = MONGOC_INSERT_NONE
			, bool specialWriteConcern = false
			, int writeConcern = 1
			);

		bson_error_t update(const std::string& collection
			, const Bson& query
			, const MongoUpdater& doc
			, mongoc_update_flags_t flags = MONGOC_UPDATE_NONE
			, bool specialWriteConcern = false
			, int writeConcern = 1
			);

		MongoCursorPtr find(const std::string& collection
			, const Bson& query
			, const Bson& opt
			, mongoc_read_mode_t readMode = MONGOC_READ_PRIMARY
			);

		MongoClientPtr popClient();

		const std::string& getDBName()const;
	private:
		std::string m_dbname;//NotSet because thread safe
	};

	inline MongoConnectionPool::~MongoConnectionPool()
	{
		if (m_poolImpl)
		{
			mongoc_client_pool_destroy(m_poolImpl);
		}
		if (m_url)
		{
			mongoc_uri_destroy(m_url);
		}
	}

	inline bool MongoConnectionPool::init(const std::string& mongoUrl
		, const std::string& dbname
		, int writeConcern
		, const std::string& appName
		)
	{
		static bool firstInit = true;
		if (firstInit)
		{
			firstInit = false;
			mongoc_init();
		}
		m_url = mongoc_uri_new(mongoUrl.c_str());
		if (!m_url)
		{
			nlogerr("create mongo url err:%s", mongoUrl.c_str());
			return false;
		}
		if (appName != "")
		{
			if (!mongoc_uri_set_appname(m_url, appName.c_str()))
			{
				nlogerr("mongoc_uri_set_appname err:%s", appName.c_str());
				return false;
			}
		}
		if (!mongoc_uri_set_option_as_int32(m_url, MONGOC_URI_MAXPOOLSIZE, 100))
		{
			nlogerr("mongoc_uri_set_option_as_int32 err");
			return false;
		}
		auto write_concern = mongoc_write_concern_new();
		if (!write_concern)
		{
			nlogerr("mongoc_write_concern_new err");
			return false;
		}
		mongoc_write_concern_set_w(write_concern, writeConcern);
		mongoc_uri_set_write_concern(m_url, write_concern);
		mongoc_write_concern_destroy(write_concern);
		m_poolImpl = mongoc_client_pool_new(m_url);
		if (!m_poolImpl)
		{
			nlogerr("mongoc_client_pool_new err");
			return false;
		}
		auto client = mongoc_client_pool_pop(m_poolImpl);
		if (!client)
		{
			nlogerr("mongoc_client_pool_new2 err");
			return false;
		}
		bson_t ping = BSON_INITIALIZER;
		bson_error_t error;
		bool r;
		BSON_APPEND_INT32(&ping, "ping", 1);

		r = mongoc_client_command_simple(
			client, "admin", &ping, NULL, NULL, &error);

		if (!r) 
		{
			nlogerr("mongoc_client_pool_new3 %s", error.message);
			mongoc_client_pool_push(m_poolImpl,client);
			return false;
		}
		m_dbname = dbname;
		return true;
	}

	inline MongoCursor::MongoCursor(int err, mongoc_cursor_t* cursor /*= nullptr*/)
	{
		m_err = err;

		if (!cursor)
		{
			return;
		}
		const bson_t *doc = nullptr;
		while (mongoc_cursor_next(cursor, &doc))
		{
			if (doc)
			{
				bson_t * doc_ = bson_copy(doc);
				m_cursorImpl.push_back(doc_);
			}
		}
		mongoc_cursor_destroy(cursor);
	}

	inline MongoCursor::~MongoCursor()
	{
		for (auto v:m_cursorImpl)
		{
			if (v)
			{
				bson_destroy(v);
			}
		}
		m_cursorImpl.clear();
	}

	inline BsonPtr MongoCursor::fetch()
	{
		if (m_cursorImpl.size() < 1)
		{
			return BsonPtr();
		}
		auto ret = BsonPtr(new Bson(m_cursorImpl.front()));
		m_cursorImpl.pop_front();
		return ret;
	}

	inline bson_error_t MongoConnectionPool::insert(const std::string& collection, const Bson& doc, mongoc_insert_flags_t flags /*= MONGOC_INSERT_NONE */, bool specialWriteConcern /*= false */, int writeConcern /*= 1 */)
	{
		bson_error_t error;
		bson_set_error(&error, 0, 3, "pool empty");
		auto c = popClient();
		if (!c)
		{
			return error;
		}
		return c->insert(collection, doc, flags, specialWriteConcern, writeConcern);
	}


	inline bson_error_t MongoConnectionPool::update(const std::string& collection, const Bson& query, const MongoUpdater& doc, mongoc_update_flags_t flags /*= MONGOC_UPDATE_NONE */, bool specialWriteConcern /*= false */, int writeConcern /*= 1 */)
	{
		bson_error_t error;
		bson_set_error(&error, 0, 3, "pool empty");
		auto c = popClient();
		if (!c)
		{
			return error;
		}
		return c->update(collection,query, doc, flags, specialWriteConcern, writeConcern);
	}
	inline const std::string& nicehero::MongoConnectionPool::getDBName()const
	{
		return m_dbname;
	}

	inline MongoCursorPtr MongoConnectionPool::find(const std::string& collection, const Bson& query, const Bson& opt, mongoc_read_mode_t readMode /*= MONGOC_READ_PRIMARY */)
	{
		auto c = popClient();
		if (!c)
		{
			return MongoCursorPtr(new MongoCursor(2));
		}
		return c->find(collection, query,  opt, readMode);
	}

	inline nicehero::MongoClient::MongoClient(MongoConnectionPool& pool,mongoc_client_t* c)
		:m_pool(pool),m_client(c)
	{
		m_dbname = m_pool.getDBName();
	}

	inline nicehero::MongoClient::~MongoClient()
	{
		mongoc_client_pool_push(m_pool.m_poolImpl, m_client);
	}

	inline bson_error_t nicehero::MongoClient::insert(const std::string& collection, const Bson& doc, mongoc_insert_flags_t flags /*= MONGOC_INSERT_NONE */, bool specialWriteConcern /*= false */, int writeConcern /*= 1 */)
	{
		bson_error_t error;
		bson_set_error(&error, 0, 2, "db error");
		auto collection_ = mongoc_client_get_collection(m_client, m_dbname.c_str(), collection.c_str());
		if (!collection_)
		{
			return error;
		}
		mongoc_write_concern_t* write_concern = nullptr;
		if (specialWriteConcern)
		{
			write_concern = mongoc_write_concern_new();
			if (!write_concern)
			{
				return error;
			}
			mongoc_write_concern_set_w(write_concern, writeConcern);
		}
		if (!mongoc_collection_insert(collection_, MONGOC_INSERT_NONE, doc.m_bson, write_concern, &error))
		{
			if (write_concern)
			{
				mongoc_write_concern_destroy(write_concern);
			}
			return error;
		}
		if (write_concern)
		{
			mongoc_write_concern_destroy(write_concern);
		}
		return error;
	}
	
	inline bson_error_t nicehero::MongoClient::update(const std::string& collection, const Bson& query, const MongoUpdater& doc, mongoc_update_flags_t flags /*= MONGOC_UPDATE_NONE */, bool specialWriteConcern /*= false */, int writeConcern /*= 1 */)
	{
		bson_error_t error;
		bson_set_error(&error, 0, 2, "db error");
		if (!doc.available())
		{
			bson_set_error(&error, 0, 2, "!doc.available()");
			return error;
		}
		auto collection_ = mongoc_client_get_collection(m_client, m_dbname.c_str(), collection.c_str());
		if (!collection_)
		{
			return error;
		}
		mongoc_write_concern_t* write_concern = nullptr;
		if (specialWriteConcern)
		{
			write_concern = mongoc_write_concern_new();
			if (!write_concern)
			{
				nlogerr("mongoc_write_concern_new err");
				return error;
			}
			mongoc_write_concern_set_w(write_concern, writeConcern);
		}
		if (!mongoc_collection_update(collection_, flags, query.m_bson, ((BsonPtr)doc)->m_bson, write_concern, &error))
		{
			if (write_concern)
			{
				mongoc_write_concern_destroy(write_concern);
			}
			return error;
		}
		if (write_concern)
		{
			mongoc_write_concern_destroy(write_concern);
		}
		return error;
	}

	inline MongoCursorPtr nicehero::MongoClient::find(const std::string& collection, const Bson& query, const Bson& opt, mongoc_read_mode_t readMode /*= MONGOC_READ_PRIMARY */)
	{
		auto c = m_client;
		if (!c)
		{
			return MongoCursorPtr(new MongoCursor(2));
		}
		auto collection_ = mongoc_client_get_collection(c, m_dbname.c_str(), collection.c_str());
		if (!collection_)
		{
			return MongoCursorPtr(new MongoCursor(2));
		}
		auto read_prefs = mongoc_read_prefs_new(readMode);
		auto cursor = mongoc_collection_find_with_opts(collection_, query.m_bson, opt.m_bson, read_prefs);
		mongoc_read_prefs_destroy(read_prefs);
		if (!cursor)
		{
			return MongoCursorPtr(new MongoCursor(2));
		}
		auto ret = MongoCursorPtr(new MongoCursor(0, cursor));
		return ret;
	}

	inline MongoClientPtr nicehero::MongoConnectionPool::popClient()
	{
		auto c = mongoc_client_pool_pop(m_poolImpl);
		if (!c)
		{
			return MongoClientPtr();
		}
		return MongoClientPtr(new MongoClient(*this,c));
	}

	inline void MongoUpdater::set(BsonPtr&& doc)
	{
		m_sets.push_back(std::move(doc));
	}

	inline void MongoUpdater::unset(BsonPtr&& doc)
	{
		m_unsets.push_back(std::move(doc));
	}

	inline MongoUpdater::operator BsonPtr() const
	{
		if (m_sets.empty() && m_unsets.empty())
		{
			return nullptr;
		}
		auto r = Bson::createBsonPtr();
		if (m_sets.size() > 0)
		{
			auto sets = Bson::createBsonPtr();
			for (auto& i : m_sets)
			{
				sets->merge(*i);
			}
			r->appendBson("$set", *sets);
		}
		if (m_unsets.size() > 0)
		{
			auto unsets = Bson::createBsonPtr();
			for (auto& i : m_unsets)
			{
				unsets->merge(*i);
			}
			r->appendBson("$unset", *unsets);
		}
		return r;
	}
	inline bool MongoUpdater::available() const
	{
		if (m_sets.empty() && m_unsets.empty())
		{
			return false;
		}
		return true;
	}
	inline MongoUpdater::MongoUpdater()
	{

	}
	inline MongoUpdater::MongoUpdater(const Bson& doc)
	{
		set(Bson::createBsonPtr(doc));
	}

}

#endif

