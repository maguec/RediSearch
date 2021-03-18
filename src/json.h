#ifndef REJSON_H
#define REJSON_H

#include "rejson_api.h"
#include "redismodule.h"
#include "document.h"

#ifdef __cplusplus
extern "C" {
#endif

extern RedisJSONAPI_V1 *japi;

int GetJSONAPIs(RedisModuleCtx *ctx, int subscribeToModuleChange);
int Document_LoadSchemaFieldJson(Document *doc, RedisSearchCtx *sctx);

#ifdef __cplusplus
}
#endif
#endif /* REJSON_H */ 
