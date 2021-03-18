#include "json.h"
#include "rmutil/rm_assert.h"

// REJSON APIs
RedisJSONAPI_V1 *japi;

/************************************************************************************************/

void ModuleChangeHandler(struct RedisModuleCtx *ctx, RedisModuleEvent e, uint64_t sub, void *data) {

  REDISMODULE_NOT_USED(e);
  RedisModuleModuleChange *ei = data;
  if (sub == REDISMODULE_SUBEVENT_MODULE_LOADED) {    
    // If RedisJSON module is loaded after RediSearch
    // Need to get the API exported by RedisJSON
    if (strcmp(ei->module_name, "ReJSON") == 0) {
        printf("detected %p loading %s\n", ctx, ei->module_name);
        if (!japi && GetJSONAPIs(ctx, 0)) {
            //TODO: Once registered we can unsubscribe from ServerEvent RedisModuleEvent_ModuleChange
            // Unless we want to hanle ReJSON module unload
        }
    }
  }
}

int GetJSONAPIs(RedisModuleCtx *ctx, int subscribeToModuleChange) {
    japi = NULL;
    japi = RedisModule_GetSharedAPI(ctx, "RedisJSON_V1");
    if (japi) {
        return 1;
    } else if (subscribeToModuleChange) {
        RedisModule_SubscribeToServerEvent(ctx,
            RedisModuleEvent_ModuleChange, ModuleChangeHandler);
    }
    return 0;
}

/******************************************************************************************************/

static RSLanguage SchemaRule_JsonLanguage(RedisModuleCtx *ctx, const SchemaRule *rule,
                                          const RedisJSONKey *jsonKey, const char *keyName) {
  int rv = REDISMODULE_ERR;
  RSLanguage lang = rule->lang_default;
  if (!rule->lang_field) {
    goto done;
  }

  JSONType type;
  size_t count;
  RedisJSON json = japi->get(jsonKey, rule->lang_field, &type, &count);
  if (!json || type != JSONType_String) {
    goto done;
  }

  char *langStr;
  if (japi->getString(json, &langStr, NULL) != REDISMODULE_OK) {
    goto done;
  }
  
  lang = RSLanguage_Find(langStr);
  if (lang == RS_LANG_UNSUPPORTED) {
    goto done;
  }

  rv = REDISMODULE_OK;
done:
  if (rv == REDISMODULE_ERR) {
        RedisModule_Log(NULL, "warning", "invalid language for key %s", keyName);
  }
  if (json) {
    japi->close(json);
  }
  return lang;
}

static RSLanguage SchemaRule_JsonScore(RedisModuleCtx *ctx, const SchemaRule *rule,
                                       const RedisJSONKey *jsonKey, const char *keyName) {
  int rv = REDISMODULE_ERR;
  double score = rule->score_default;
  if (!rule->score_field) {
    goto done;
  }

  JSONType type;
  size_t count;
  const RedisJSON *json = japi->get(jsonKey, rule->score_field, &type, &count);
  if (json == NULL || (type != JSONType_Float && type != JSONType_Int)) {
    goto done;
  }

  if (japi->getFloat(json, &score) != REDISMODULE_OK) {
    goto done;
  }

  rv = REDISMODULE_OK;
done:
  if (rv == REDISMODULE_ERR) {
    RedisModule_Log(NULL, "warning", "invalid field %s for key %s", rule->score_field,
                                              RedisModule_StringPtrLen(keyName, NULL));
  }
  if (json) {
    japi->close(json);
  }
  return score;
}

/* For POC only */
/* this function copies the string */
static RedisModuleString *JSON_ToStringR(RedisModuleCtx *ctx, const RedisJSON *json, JSONType type) {
  size_t len;
  const char *str = JSON_ToString(ctx, json, type, &len);
  return RedisModule_CreateString(ctx, str, len);
}

/* this function does not copies the string */
static const char *JSON_ToString(RedisModuleCtx *ctx, const RedisJSON *json, JSONType type, size_t *len) {
  // TODO: union
  char *str = NULL;
  double dbl;
  int integer;
  int boolean;

  switch (type) {
  case JSONType_String:
    japi->getString(json, &str, len);
    return str;
  /* TODO:
  case JSONType_Bool:
    japi->getBoolean(json, &boolean);
    return boolean ? "1" : "0";
  case JSONType_Int:
    japi->getBoolean(json, &integer);
    return integer;
  case JSONType_Float:
    japi->getFloat(json, &dbl);
    return dbl;
  */
  default:
    break;
  }
  return str;
}


int Document_LoadSchemaFieldJson(Document *doc, RedisSearchCtx *sctx) {
  int rv = REDISMODULE_ERR;
  IndexSpec *spec = sctx->spec;
  SchemaRule *rule = spec->rule;
  RedisModuleCtx *ctx = sctx->redisCtx;
  size_t nitems = sctx->spec->numFields;

  RedisModuleString *payload_rms = NULL;
  Document_MakeStringsOwner(doc); // TODO: necessary??
  
  const RedisJSONKey *jsonKey = japi->openKey(ctx, doc->docKey);
  if (!jsonKey) {
    goto done;
  }

  const char *keyName = RedisModule_StringPtrLen(doc->docKey, NULL);
  doc->language = SchemaRule_JsonLanguage(sctx->redisCtx, rule, jsonKey, keyName);
  doc->score = SchemaRule_JsonScore(sctx->redisCtx, rule, jsonKey, keyName);
  // No payload on JSON as RedisJSON does not support binary fields

  const char *jsonVal;

  size_t count;
  JSONType type;
  const RedisJSON *json;
  doc->fields = rm_calloc(nitems, sizeof(*doc->fields));
  for (size_t ii = 0; ii < spec->numFields; ++ii) {
    const char *fname = spec->fields[ii].name;

    // retrive json pointer
    json = japi->get(ctx, jsonKey, &type, &count);
    if (!json) {
      continue;
    }

    size_t oix = doc->numFields++;
    doc->fields[oix].name = rm_strdup(fname);

    // on crdt the return value might be the underline value, we must copy it!!!
    // TODO: change `fs->text` to support hash or json not RedisModuleString
    doc->fields[oix].text = JSON_ToStringR(ctx, json, type);
    japi->close(json);
    // doc->fields[oix].text = RedisModule_CreateString(ctx, str, strLen);
    // RedisModule_FreeString(ctx, fieldText);
  }

done:
  if (jsonKey) {
    japi->closeKey(jsonKey);
  }
  return rv;
}
