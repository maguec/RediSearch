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
int JSON_GetSinglePath(struct RedisModuleCtx *ctx, RedisModuleString *keyName, const char *path, const RedisJSON **data);
int JSON_GetMultiPath (struct RedisModuleCtx *ctx, RedisModuleString *keyName, const char *path, size_t path_len, const RedisJSON **data);

// For all types
size_t JSON_Count(const RedisJSON *data);
JsonValueType JSON_Type(const RedisJSON *data);
int JSON_ReplyWith(struct RedisModuleCtx *ctx, const RedisJSON *data);
int JSON_ReplyWithMulti(struct RedisModuleCtx *ctx, const RedisJSON *data, size_t *len);

int JSON_GetInt(const RedisJSON *data, int *result);
int JSON_GetBool(const RedisJSON *data, int *result);
int JSON_GetFloat(const RedisJSON *data, double *result); //TODO: ensure double
int JSON_GetString(const RedisJSON *data, const char **result, size_t *len);

// For Object and Array
// (for Array - question if internal Vec<Value> can be returned (it does not have attribute #[repr(C)])
// (for Object - question if whether we should flat down the map into an array)
// Can use callbacks instead
int JSON_GetAllChildren(const RedisJSON *data, RedisJSON **dataResult, size_t *count);

// For Array
int JSON_GetAtPos(const RedisJSON *data, size_t indexStart, size_t indexEnd, const RedisJSON *data_result);
// For Object
int JSON_GetAtField(const RedisJSON *data, const char *fieldName, RedisJSON *dataResult);

// Free memory which was specifically allocated for API for multi
int JSON_FreeMulti(const RedisJSON *data);

// TODO: either isJSON or share pointer for RedisModuleType
int JSON_IsJSON(RedisModuleCtx *ctx, const char *key);
extern RedisModuleType *JSONType;
///////////////////////////////////////////////////////////////////////////////////////

// Open questions for phase 1:
// Should Array with size > 1 be supported in phase1?

//Path "...A" => Object '{.., "A": [ "C","D", {"E":"F"} ] , ...}'  => not supported since one of the elements is an Object
//Path "...A" => Object '{.., "A": [ "C","D", true, 10.12 ] , ...}' => Not supported since array size is > 1

// Ingest - get all values of fields in index
// Load - get all values of requested fields
// Printout - Reply With

#ifdef __cplusplus
}
#endif
#endif /* SRC_REJSON_API_H_ */
