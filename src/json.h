#pragma once

#include "rejson_api.h"
#include "redismodule.h"
#include "document.h"
#include "doc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern RedisJSONAPI_V1 *japi;
extern RedisModuleCtx *RSDummyContext;

#define JSON_ROOT "$"

int GetJSONAPIs(RedisModuleCtx *ctx, int subscribeToModuleChange);
int Document_LoadSchemaFieldJson(Document *doc, RedisSearchCtx *sctx);

const char *JSON_ToString(RedisModuleCtx *ctx, RedisJSON json, JSONType type, size_t *len);
RedisModuleString *JSON_ToStringR(RedisModuleCtx *ctx, RedisJSON json, JSONType type);
int JSON_GetStringR_POC(RedisModuleCtx *ctx, const char *keyName, const char *path, RedisModuleString **val);

static inline int RedisJSON_GetString(RedisJSONKey key, const char *path, const char **str, size_t *len) {
  size_t size;
  JSONType type;
  const char *tmpStr;
  RedisJSON json = japi->get(key, path, &type, &size);
  if (!json) {
    return REDISMODULE_ERR;
  }
  int rv = japi->getString(json, path, &tmpStr, len);
  if (rv == REDISMODULE_OK) {
    *str = rm_strndup(tmpStr, *len);
  }
  japi->close(json);
  return rv;
}

static inline int RedisJSON_GetRedisModuleString(RedisJSONKey key, const char *path, RedisModuleString **rstr) {
  size_t size;
  JSONType type;
  RedisJSON json = japi->get(key, path, &type, &size);
  if (!json) {
    return REDISMODULE_ERR;
  }
  int rv = japi->getRedisModuleString(RSDummyContext, json, path, rstr);
  japi->close(json);
  return rv;
}

static inline void RedisJSON_FreeRedisModuleString(RedisJSONKey key, RedisModuleString *rstr) {
  RedisModule_FreeString(RSDummyContext, rstr);
}

static inline int RedisJSON_GetNumeric(RedisJSONKey key, const char *path, double *dbl){
  size_t size;
  JSONType type;
  RedisJSON json = japi->get(key, path, &type, &size);
  if (!json) {
    return REDISMODULE_ERR;
  }
  int rv = japi->getDouble(json, dbl);
  japi->close(json);
  return rv;
}

#ifdef __cplusplus
}
#endif
