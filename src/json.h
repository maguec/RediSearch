#pragma once

#include "redismodule.h"
#include "document.h"
#include "rejson_api.h"

int Document_LoadSchemaFieldJson(Document *doc, RedisSearchCtx *sctx);
