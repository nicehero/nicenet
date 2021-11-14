#ifndef ___EASY__MONGO_ASYNC_HPP__
#define ___EASY__MONGO_ASYNC_HPP__

#include "Type.h"
#include "Mongo.hpp"
#include "Service.h"
#include <memory>
#include "CoroAwaitable.hpp"
#include <string_view>

namespace nicehero
{
	struct MongoPoolFindAsync
	{
		using RetType = MongoCursorPtr;
		MongoPoolFindAsync(std::shared_ptr<MongoConnectionPool> pool
			, const std::string& collection
			, BsonPtr query
			, BsonPtr opt
			, mongoc_read_mode_t readMode = MONGOC_READ_PRIMARY) {
			m_pool = pool;
			m_collection = collection;
			m_query = std::move(query);
			m_opt = std::move(opt);
			m_readMode = readMode;
		}
		std::shared_ptr<MongoConnectionPool> m_pool;
		std::string m_collection;
		BsonPtr m_query;
		BsonPtr m_opt;
		mongoc_read_mode_t m_readMode;
		IMPL_AWAITABLE(TO_DB)
	};

	inline MongoCursorPtr MongoPoolFindAsync::execute()
	{
		if (!m_pool){
			return MongoCursorPtr(new MongoCursor(2));
		}
		return m_pool->find(m_collection, *m_query, *m_opt, m_readMode);
	}
	
	CORO_AWAITABLE(MongoPoolFindAsync2,TO_DB,MongoCursorPtr,MongoPoolPtr,std::string,BsonPtr,BsonPtr,mongoc_read_mode_t)
	{
		if (!std::get<0>(args)){
			return MongoCursorPtr(new MongoCursor(2));
		}
		return std::get<0>(args)->find(std::get<1>(args),*std::get<2>(args),*std::get<3>(args),std::get<4>(args));
	}
	CORO_AWAITABLE_EX(MongoPoolFindAsync3,TO_DB,MongoCursorPtr,MongoPoolPtr,std::string,BsonPtr,BsonPtr,mongoc_read_mode_t)
	(MongoPoolPtr pool,std::string collection,BsonPtr query,BsonPtr opt,mongoc_read_mode_t readMode)
	{
		if (!pool){
			return MongoCursorPtr(new MongoCursor(2));
		}
		return pool->find(collection, *query, *opt, readMode);
	}
}

#endif

