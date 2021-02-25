#define REDISMODULE_MAIN
#define REDISEARCH_API_EXTERN

#include "redisearch_api.h"
#include "redisearch.h"

#include <string.h>

#define DOCID1 "doc1"
#define DOCID2 "doc2"
#define FIELD_NAME_1 "text1"
#define FIELD_NAME_2 "text2"
#define NUMERIC_FIELD_NAME "num"
#define GEO_FIELD_NAME "geo"
#define TAG_FIELD_NAME1 "tag1"
#define TAG_FIELD_NAME2 "tag2"

#define ASSERT_COMMON(a, b, op)                                                                         \
  if (a op b) {                                                                                         \
    RedisModule_ReplyWithSimpleString(ctx, "OK");                                                       \
  } else {                                                                                              \
    RedisModule_ReplyWithPrintf(ctx, "Condition (##a ##op ##b) failed at %s:%d.", __FILE__, __LINE__);  \
  }                                                                                                     \
  arrlen++;

#define ASSERT_EQ(a, b) ASSERT_COMMON(a, b, ==)
#define ASSERT_NOT_EQ(a, b) ASSERT_COMMON(a, b, !=)
#define ASSERT_GT(a, b) ASSERT_COMMON(a, b, >)
#define ASSERT_GE(a, b) ASSERT_COMMON(a, b, >=)
#define ASSERT_LT(a, b) ASSERT_COMMON(a, b, <)
#define ASSERT_LE(a, b) ASSERT_COMMON(a, b, <=)
#define ASSERT_TRUE(a) ASSERT_COMMON((a), 0, >)

#define ASSERT_STREQ(a, b)                                                                         \
  if (strcmp(a, b) == 0) {                                                                                         \
    RedisModule_ReplyWithSimpleString(ctx, "OK");                                                       \
  } else {                                                                                              \
    RedisModule_ReplyWithPrintf(ctx, "Condition (##a ##b) failed at %s:%d.", __FILE__, __LINE__);  \
  }                                                                                                     \
  arrlen++;

#define INIT_TEST                                                     \
  int arrlen = 0;                                                     \
  REDISMODULE_NOT_USED(argc);                                         \
  REDISMODULE_NOT_USED(argv);                                         \
  RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN)

int RS_llapi_basic_check(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	REDISMODULE_NOT_USED(argv);

	if (argc > 1) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_OK;
	}

  RedisModule_ReplyWithSimpleString(ctx, "OK");
	return REDISMODULE_OK;
}

int RS_llapi_geo(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
/*  INIT_TEST;
  RSIndex* index = RediSearch_CreateIndex("index", NULL);

  // adding geo point field to the index
  RediSearch_CreateGeoField(index, GEO_FIELD_NAME);

  // adding document to the index
  RSDoc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  // check error on lat > GEO_LAT_MAX
  int res = RediSearch_DocumentAddFieldGeo(d, GEO_FIELD_NAME, 100, 0, RSFLDTYPE_DEFAULT);
  ASSERT_EQ(res, REDISMODULE_ERR);
  // check error on lon > GEO_LON_MAX
  res = RediSearch_DocumentAddFieldGeo(d, GEO_FIELD_NAME, 0, 200, RSFLDTYPE_DEFAULT);
  ASSERT_EQ(res, REDISMODULE_ERR);
  // valid geo point
  res = RediSearch_DocumentAddFieldGeo(d, GEO_FIELD_NAME, 20.654321, 0.123456, RSFLDTYPE_DEFAULT);
  ASSERT_EQ(res, REDISMODULE_OK);
  RediSearch_SpecAddDocument(index, d);

  // searching on the index
  RSQNode* qn = RediSearch_CreateGeoNode(index, GEO_FIELD_NAME, 20.6543222, 0.123455, 10, RS_GEO_DISTANCE_M);
  RSResultsIterator* iter = RediSearch_GetResultsIterator(qn, index);
  ASSERT_TRUE(iter != NULL);

  size_t len;
  const char* id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);
  RediSearch_ResultsIteratorFree(iter);

  // searching on the index and getting NULL result
  qn = RediSearch_CreateGeoNode(index, GEO_FIELD_NAME, 20.6543000, 0.123000, 10, RS_GEO_DISTANCE_M);
  iter = RediSearch_GetResultsIterator(qn, index);
  ASSERT_TRUE(iter != NULL);

  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);
  RediSearch_ResultsIteratorFree(iter);

  RediSearch_DropIndex(index);
  RedisModule_ReplySetArrayLength(ctx, arrlen);
  */
  return REDISMODULE_OK;
}

#define REGISTER_CMD(name, func)                                                  \
  if (RedisModule_CreateCommand(ctx, name, func, "", 0, 0, 0) == REDISMODULE_ERR) \
    return REDISMODULE_ERR;

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	REDISMODULE_NOT_USED(argv);
	REDISMODULE_NOT_USED(argc);

	if(RedisModule_Init(ctx, "RS_llapi", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if(RediSearch_Init(ctx, REDISEARCH_INIT_MODULE) != REDISMODULE_OK)
		RedisModule_Log(ctx, "warning",
		  "could not initialize RediSearch api, running without RS support.");

  REGISTER_CMD("RS_LLAPI.BASIC_CHECK", RS_llapi_basic_check);
	REGISTER_CMD("RS_LLAPI.GEO", RS_llapi_geo);

	return REDISMODULE_OK;
}