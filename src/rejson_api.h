#pragma once

#include "redismodule.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum JSONType {
    JSONType_String = 0,
    JSONType_Int = 1,
    JSONType_Double = 2,
    JSONType_Bool = 3,
    JSONType_Object = 4,
    JSONType_Array = 5,
    JSONType_Null = 6,
    JSONType__EOF
} JSONType;

typedef const void *RedisJSONKey;
typedef const void *RedisJSON;

typedef struct RedisJSONAPI_V1 {
    /* RedisJSONKey functions */
    RedisJSONKey (*openKey)(RedisModuleCtx *ctx, RedisModuleString *key_name);
    RedisJSONKey (*openKeyFromStr)(RedisModuleCtx *ctx, const char *path);

    void (*closeKey)(RedisJSONKey key);

    /* RedisJSON functions
     * Return NULL if path does not exist
     * `count` can be NULL and return 0 for non array/object
     **/
    RedisJSON (*get)(RedisJSONKey key, const char *path, JSONType *type, size_t *count);

    RedisJSON (*getAt)(RedisJSON jsonIn, size_t index, JSONType *type, size_t *count);

    void (*close)(RedisJSON json);

    /* RedisJSON value functions
     * Return REDISMODULE_OK if RedisJSON is of the correct JSONType,
     * else REDISMODULE_ERR is returned
     **/
    int (*getInt)(RedisJSON json, long long *integer);

    int (*getDouble)(RedisJSON json, double *dbl);

    int (*getBoolean)(RedisJSON json, int *boolean);

    // Callee has ownership of `str` - valid until api `close` is called
    int (*getString)(RedisJSON json, const char *path, const char **str, size_t *len);

    // Caller gains ownership of `str`
    int (*getRedisModuleString)(RedisModuleCtx *ctx, RedisJSON json, const char *path, RedisModuleString **str);

    int (*replyWith)(RedisModuleCtx *ctx, RedisJSON json);
} RedisJSONAPI_V1;

#ifdef __cplusplus
}
#endif
