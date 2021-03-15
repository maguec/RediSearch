#include "json.h"
#include "rmutil/rm_assert.h"

/************************************************************************************************/

void ModuleChangeHandler(struct RedisModuleCtx *ctx, RedisModuleEvent e, uint64_t sub, void *data) {

  REDISMODULE_NOT_USED(e);
  RedisModuleModuleChange *ei = data;
  if (sub == REDISMODULE_SUBEVENT_MODULE_LOADED) {    
    // If RedisJSON module is loaded after RediSearch
    // Need to get the API exported by RedisJSON
    if (strcmp(ei->module_name, "ReJSON") == 0) {
        printf("detected %p loading %s\n", ctx, ei->module_name);
        if (!jsonAPI && GetJSONAPIs(ctx, 0)) {
            //TODO: Once registered we can unsubscribe from ServerEvent RedisModuleEvent_ModuleChange
            // Unless we want to hanle ReJSON module unload
        }
    }
  }
}

int GetJSONAPIs(RedisModuleCtx *ctx, int subscribeToModuleChange) {
    jsonAPI = NULL;
    jsonAPI = RedisModule_GetSharedAPI(ctx, "RedisJSON_V1");
    if (jsonAPI) {
        return 1;
    } else if (subscribeToModuleChange) {
        RedisModule_SubscribeToServerEvent(ctx,
            RedisModuleEvent_ModuleChange, ModuleChangeHandler);
    }
    return 0;
}

/******************************************************************************************************/

static RSLanguage SchemaRule_JsonLanguage(RedisModuleCtx *ctx, const SchemaRule *rule,
                                          RedisModuleString *keyName) {
  int rv = REDISMODULE_ERR;
  RSLanguage lang = rule->lang_default;
  const char *keyNameC = RedisModule_StringPtrLen(keyName, NULL);
  if (!rule->lang_field) {
    goto done;
  }

  size_t items;
  JSONType lang_t;
  size_t lang_len;
  const char *lang_s;
  const RedisJSON *jsonData = jsonAPI->getPath(ctx, keyName, rule->lang_field);
  if (jsonData == NULL) {
    goto done;
  }

  rv = jsonAPI->getInfo(jsonData, &lang_s, &lang_t, &items);
  if (rv == REDISMODULE_ERR || lang_t != JSONType_String) {
    goto done;
  }

  rv = (jsonAPI->getString(jsonData, &lang_s, &lang_len));
  if (rv == REDISMODULE_ERR || lang_len == 0) {
    goto done;
  }

  
  lang = RSLanguage_Find(lang_s);
  if (lang == RS_LANG_UNSUPPORTED) {
    rv = REDISMODULE_ERR;
    lang = rule->lang_default;
  }
done:
  if (rv == REDISMODULE_ERR) {
        RedisModule_Log(NULL, "warning", "invalid language for key %s", keyNameC);
  }
  return lang;
}

static RSLanguage SchemaRule_JsonScore(RedisModuleCtx *ctx, const SchemaRule *rule,
                                          RedisModuleString *keyName) {
  int rv = REDISMODULE_ERR;
  double score = rule->score_default;
  if (!rule->score_field) {
    goto done;
  }

  size_t items;
  JSONType score_t;
  const char *score_s = NULL;
  char *score_s_end = NULL;
  const RedisJSON *jsonData = jsonAPI->getPath(ctx, keyName, rule->score_field);
  if (jsonData == NULL) {
    goto done;
  }

  rv = jsonAPI->getInfo(jsonData, &score_s, &score_t, &items);
  if (rv || items != 1) {
    goto done;
  }

  switch (score_t) {
  case JSONType_Double:
    rv = jsonAPI->getDouble(jsonData, &score);
    break;
//  case JSONType_Int:
//    rv = jsonAPI->getInt(jsonData, &score);
//    break;
  case JSONType_String:
    rv = jsonAPI->getString(jsonData, &score_s, NULL);
    if (score_s) {
      score = strtod(score_s, &score_s_end);
      rv = (*score_s_end == '\0') ? REDISMODULE_OK : REDISMODULE_ERR;
    }
    break;  
  default:
    break;
  }

done:
  if (rv == REDISMODULE_ERR) {
    RedisModule_Log(NULL, "warning", "invalid field %s for key %s", rule->score_field,
                                              RedisModule_StringPtrLen(keyName, NULL));
  }
  return score;
}

int Document_LoadSchemaFieldJson(Document *doc, RedisSearchCtx *sctx) {
  IndexSpec *spec = sctx->spec;
  SchemaRule *rule = spec->rule;
  RedisModuleCtx *ctx = sctx->redisCtx;
  size_t nitems = sctx->spec->numFields;

  RedisModuleString *payload_rms = NULL;
  Document_MakeStringsOwner(doc); // TODO: necessary??
  RedisModuleString *keyNameR = doc->docKey;

  doc->language = SchemaRule_JsonLanguage(sctx->redisCtx, rule, keyNameR);
  doc->score = SchemaRule_JsonScore(sctx->redisCtx, rule, keyNameR);
  // No payload on JSON as RedisJSON does not support binary fields

  size_t items;
  JSONType jsonType;
  const char *jsonVal;
  const RedisJSON *jsonData;
  doc->fields = rm_calloc(nitems, sizeof(*doc->fields));
  for (size_t ii = 0; ii < spec->numFields; ++ii) {
    const char *fname = spec->fields[ii].name;

    // retrive json pointer
    jsonData = jsonAPI->getPath(ctx, keyNameR, fname);
    if (!jsonData) {
      continue;
    }

    // pull json data
    if (jsonAPI->getInfo(jsonData, &jsonVal, &jsonType, &items) == REDISMODULE_ERR ||
                                                                        items != 1) {
      continue;
    }

    size_t oix = doc->numFields++;
    doc->fields[oix].name = rm_strdup(fname);

    // on crdt the return value might be the underline value, we must copy it!!!
    // TODO: change `fs->text` to char * ??
    const char *fieldText;
    jsonAPI->getString(jsonData, &fieldText, NULL);
    doc->fields[oix].text = RedisModule_CreateString(ctx, fieldText, strlen(fieldText));
    // RedisModule_FreeString(ctx, fieldText);
  }
  return REDISMODULE_OK;
}
