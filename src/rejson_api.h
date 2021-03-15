#ifndef SRC_REJSON_API_H_
#define SRC_REJSON_API_H_

#include "redismodule.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JSON_Null,
    JSON_Bool,
    JSON_Number,
    JSON_Str,
    JSON_Arr,
    JSON_Object,
} JsonValueType;

union JsonValueUnion {
    int err;
    const char *str;
    int num_int;
    float num_float;
    bool boolean;
    //nil: std::mem::ManuallyDrop<null>,
    //arr: Vec<JsonValueUnion>,
    //obj: HashMap<String,JsonValueUnion>,
};

typedef struct {
    JsonValueType type;
    union JsonValueUnion value;
} JsonValue;


//typedef int (*get_json_path)(RedisModuleKey *key, const char *path);
typedef RedisModuleString *(*get_json_path)(struct RedisModuleCtx *ctx, RedisModuleString *key_name, const char *path);
//Demo use case 1
// FT.CREATE idx on json...
// FT.SEARCH * .. ==> 0 results
// JSON.SET searchToken '{}'...
// FT.SEARCH * ==> 1 result

///////////////////////////////////////////////////////////////////////////////////////
//API
typedef void *RedisJSON;

// Return RM_OK or RM_Err
// Return result in out parameter data
int RedisJSON_GetSinglePath(struct RedisModuleCtx *ctx, RedisModuleString *keyName, const char *path, const RedisJSON **data);
int RedisJSON_GetMultiPath (struct RedisModuleCtx *ctx, RedisModuleString *keyName, const char *path, size_t path_len, const RedisJSON **data);

// For all types
size_t RedisJSON_Count(const RedisJSON *data);
JsonValueType RedisJSON_Type(const RedisJSON *data);
int RedisJSON_ReplyWith(struct RedisModuleCtx *ctx, const RedisJSON *data);
int RedisJSON_ReplyWithMulti(struct RedisModuleCtx *ctx, const RedisJSON *data, size_t *len);

int RedisJSON_GetInt(const RedisJSON *data, int *result);
int RedisJSON_GetBool(const RedisJSON *data, int *result);
int RedisJSON_GetFloat(const RedisJSON *data, double *result); //TODO: ensure double
int RedisJSON_GetString(const RedisJSON *data, const char **result, size_t *len);

// For Object and Array
// (for Array - question if internal Vec<Value> can be returned (it does not have attribute #[repr(C)])
// (for Object - question if whether we should flat down the map into an array)
// Can use callbacks instead
int RedisJSON_GetAllChildren(const RedisJSON *data, RedisJSON **dataResult, size_t *count);

// For Array
int RedisJSON_GetAtPos(const RedisJSON *data, size_t indexStart, size_t indexEnd, const RedisJSON *data_result);
// For Object
int RedisJSON_GetAtField(const RedisJSON *data, const char *fieldName, RedisJSON *dataResult);

// Free memory which was specifically allocated for API for multi
int RedisJSON_FreeMulti(const RedisJSON *data);

// TODO: either isJSON or share pointer for RedisModuleType
int RedisJSON_IsJSON(RedisModuleCtx *ctx, const char *key);
extern RedisModuleType *JSONType;
///////////////////////////////////////////////////////////////////////////////////////

// Open questions for phase 1:
// Should Array with size > 1 be supported in phase1?

//Path "...A" => Object '{.., "A": [ "C","D", {"E":"F"} ] , ...}'  => not supported since one of the elements is an Object
//Path "...A" => Object '{.., "A": [ "C","D", true, 10.12 ] , ...}' => Not supported since array size is > 1

// Ingest - get all values of fields in index
// Load - get all values of requested fields
// Printout - Reply With

#define REDISJSON_CAPI_VERSION 1

int RedisJson_GetCApiVersion() {
  return REDISJSON_CAPI_VERSION;
}

#define RJSON_XAPIFUNC(X)             \
  X(GetSinglePath)                    \
  X(GetMultiPath)                     \
  X(Count)                            \
  X(Type)                             \
  X(ReplyWith)                        \
  X(ReplyWithMulti)                   \
  X(GetInt)                           \
  X(GetBool)                          \
  X(GetFloat)                         \
  X(GetInt)                           \
  X(GetString)

#define REDISJSON_MODULE_INIT_FUNCTION(name)                                    \
  if (RedisModule_GetApi("RedisJSON_" #name, ((void**)&RedisJson_##name))) {   \
    printf("could not initialize RedisJSON_" #name "\r\n");                     \
    rv__ = REDISMODULE_ERR;                                                     \
    goto rsfunc_init_end__;                                                     \
  }

// TODO: fix build in order to return to `ifdef`
#ifndef REDISJSON_API_EXTERN
/**
 * This is implemented as a macro rather than a function so that the inclusion of this
 * header file does not automatically require the symbols to be defined above.
 *
 * We are making use of special GCC statement-expressions `({...})`. This is also
 * supported by clang.
 *
 * This function should not be used if RediSearch is compiled as a
 * static library. In this case, the functions are actually properly
 * linked.
 */

// TODO: JSON version function
#define RedisJSONInitialize()                                    \
  ({                                                             \
    int rv__ = REDISMODULE_OK;                                   \
    RJSON_XAPIFUNC(REDISJSON_MODULE_INIT_FUNCTION);              \
    if (RedisJson_GetCApiVersion() > REDISJSON_CAPI_VERSION) {   \ 
      rv__ = REDISMODULE_ERR;                                    \
    }                                                            \
  rsfunc_init_end__:;                                            \
    rv__;                                                        \
  })

#define REDISJSON__API_INIT_NULL(s) __typeof__(RedisJson_##s) RedisJson_##s = NULL;
#define REDISJSON_API_INIT_SYMBOLS() RJSON_XAPIFUNC(REDISJSON__API_INIT_NULL)
#else
#define REDISJSON_API_INIT_SYMBOLS()
#define RedisJson_Initialize()
#endif




#ifdef __cplusplus
}
#endif
#endif /* SRC_REJSON_API_H_ */
