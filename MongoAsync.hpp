#ifndef ___EASY__MONGO_ASYNC_HPP__
#define ___EASY__MONGO_ASYNC_HPP__

#include "Type.h"
#include "Mongo.hpp"
#include "Service.h"
#include <memory>
#include "Task.hpp"
#include <string_view>

namespace nicehero
{
	template<ToService executer = TO_DB, ToService return_context = TO_MAIN>
	Task<MongoCursorPtr, executer, return_context> MongoPoolFindAsync(
		MongoPoolPtr pool,std::string collection,BsonPtr query,BsonPtr opt,mongoc_read_mode_t readMode)
	{
		if (!pool){
			return MongoCursorPtr(new MongoCursor(2));
		}
		return pool->find(collection, *query, *opt, readMode);
	}
}

#endif

