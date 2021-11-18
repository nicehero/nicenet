#include "Service.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include <asio/yield.hpp>
#ifdef WIN32
#include <windows.h>
#endif
#include <chrono>
#include <iomanip>
#include "TestProtocol.h"
#include <mongoc/mongoc.h>
#include "Mongo.hpp"
#include "Clock.h"

void benchmark_update(int threadNum, std::shared_ptr<nicehero::MongoConnectionPool> pool, std::string tablename)
{
	auto t1 = nicehero::Clock::getInstance()->getMilliSeconds();
	std::shared_ptr<int> xx = std::make_shared<int>(0);
	for (int j = 1; j <= threadNum; ++j)
	{
		nicehero::post([xx, j, pool, t1, threadNum, tablename] {
			for (int i = 1; i <= 100 * (1000 / threadNum); ++i)
			{
				auto obj = NBSON("ar.0.hello", BCON_INT64(101));
				pool->update(tablename, NBSON_T(
					"_id", BCON_INT64(j * 10000 + i))
					, *obj);
			}
			nicehero::post([pool, xx, t1, threadNum, tablename] {
				++(*xx);
				if (*xx >= threadNum)
				{
					auto t = nicehero::Clock::getInstance()->getMilliSeconds() - t1;
					double qps = 100000.0 / double(t)  * 1000.0;
					nlog("update qps:%.2lf", qps);
				}
			});
		}, nicehero::TO_DB);
	}
}

void benchmark_query(int threadNum, std::shared_ptr<nicehero::MongoConnectionPool> pool, std::string tablename)
{
	auto t1 = nicehero::Clock::getInstance()->getMilliSeconds();
	std::shared_ptr<int> xx = std::make_shared<int>(0);
	for (int j = 1; j <= threadNum; ++j)
	{
		nicehero::post([xx, j, pool, t1, threadNum, tablename] {
			for (int i = 1; i <= 100 * (1000 / threadNum); ++i)
			{
				auto cursor = pool->find(tablename, NBSON_T(
					"_id", BCON_INT64(j * 10000 + i))
					, nicehero::Bson(nullptr)
				);
			}
			nicehero::post([pool, xx, t1, threadNum, tablename] {
				++(*xx);
				if (*xx >= threadNum)
				{
					auto t = nicehero::Clock::getInstance()->getMilliSeconds() - t1;
					double qps = 100000.0 / double(t)  * 1000.0;
					nlog("query qps:%.2lf", qps);
					benchmark_update(threadNum, pool, tablename);
				}
			});
		}, nicehero::TO_DB);
	}
}

void benchmark_insert(int threadNum,std::shared_ptr<nicehero::MongoConnectionPool> pool, std::string tablename)
{
	auto t1 = nicehero::Clock::getInstance()->getMilliSeconds();
	std::shared_ptr<int> xx = std::make_shared<int>(0);
	for (int j = 1; j <= threadNum; ++j)
	{
		nicehero::post([xx, j, pool, t1, threadNum,tablename] {
			for (int i = 1; i <= 100 * (1000 / threadNum); ++i)
			{
				pool->insert(tablename,
					NBSON_T(
						"_id", BCON_INT64(j * 10000 + i)
						, "hello", BCON_UTF8("world")
						, "ar"
						, "["
						, "{"
						, "hello", BCON_INT64(666)
						, "}"
						, "world5"
						, BCON_DATE_TIME(nicehero::Clock::getInstance()->getTimeMS())
						, "]"
						, "oo"
						, "{"
						, "xhello", BCON_INT64(666)
						, "}"
					));
			}
			nicehero::post([pool,xx, t1, threadNum,tablename] {
				++(*xx);
				if (*xx >= threadNum)
				{
					auto t = nicehero::Clock::getInstance()->getMilliSeconds() - t1;
					double qps = 100000.0 / double(t)  * 1000.0;
					nlog("insert qps:%.2lf", qps);
					benchmark_query(threadNum,pool,tablename);
				}
			});
		}, nicehero::TO_DB);
	}
}
int benchmark(int threadNum, const char* db, const char* tablename)
{
	nicehero::start(true);
	std::shared_ptr<nicehero::MongoConnectionPool> pool = std::make_shared<nicehero::MongoConnectionPool>();
	pool->init(db, "benchmark");
	benchmark_insert(threadNum,pool,tablename);
	nicehero::joinMain();
	return 0;
}

