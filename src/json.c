#include "json.h"
#include "rmutil/rm_assert.h"

static RSLanguage SchemaRule_JsonLanguage(RedisModuleCtx *ctx, const SchemaRule *rule,
                                          RedisModuleString *keyName) {
  RSLanguage lang = rule->lang_default;
  const char *keyNameC = RedisModule_StringPtrLen(keyName, NULL);
  if (!rule->lang_field) {
    goto done;
  }

  const RedisJSON *jsonData;
  const char *lang_s;
  if (JSON_GetSinglePath(ctx, keyName, rule->lang_field, &jsonData) != REDISMODULE_OK ||
      JSON_Type(jsonData) != JSON_Str ||
      JSON_GetString(jsonData, &lang_s, NULL)) {
    RedisModule_Log(NULL, "warning", "invalid field %s for key %s", rule->lang_field, keyNameC);
    goto done;
  }
  lang = RSLanguage_Find(lang_s);
  if (lang == RS_LANG_UNSUPPORTED) {
    RedisModule_Log(NULL, "warning", "invalid language for key %s", keyNameC);
    lang = rule->lang_default;
  }
done:
  return lang;
}

static RSLanguage SchemaRule_JsonScore(RedisModuleCtx *ctx, const SchemaRule *rule,
                                          RedisModuleString *keyName) {
  double score = rule->score_default;
  if (!rule->score_field) {
    goto done;
  }

  const RedisJSON *jsonData;
  if (JSON_GetSinglePath(ctx, keyName, rule->score_field, &jsonData) != REDISMODULE_OK ||
      JSON_Type(jsonData) != JSON_Number || // TODO: Can this a number as well
      JSON_GetFloat(jsonData, &score)) {
    RedisModule_Log(NULL, "warning", "invalid field %s for key %s", rule->score_field,
                                              RedisModule_StringPtrLen(keyName, NULL));
    goto done;
  }
done:
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

  const RedisJSON *jsonData;
  doc->fields = rm_calloc(nitems, sizeof(*doc->fields));
  for (size_t ii = 0; ii < spec->numFields; ++ii) {
    const char *fname = spec->fields[ii].name;
    RedisModuleString *v = NULL;
    if (JSON_GetSinglePath(ctx, keyNameR, fname, &jsonData) != REDISMODULE_OK) {
      continue;
    }
    size_t oix = doc->numFields++;
    doc->fields[oix].name = rm_strdup(fname);

    // on crdt the return value might be the underline value, we must copy it!!!
    // TODO: change `fs->text` to char * ??
    const char *fieldText;
    JSON_GetString(jsonData, &fieldText, NULL);
    doc->fields[oix].text = RedisModule_CreateString(ctx, fieldText, strlen(fieldText));
    // RedisModule_FreeString(ctx, fieldText);
  }
  return REDISMODULE_OK;
}