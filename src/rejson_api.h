#ifndef SRC_REJSON_API_H_
#define SRC_REJSON_API_H_

#include "redismodule.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum JSONType {
    JSONType_String = 0,
    JSONType_Int = 1,
    JSONType_Float = 2,
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
  const RedisJSONKey *(*openKey)(struct RedisModuleCtx* ctx, RedisModuleString* key);
  void (*closeKey)(const RedisJSONKey *key);
  
  /* RedisJSON functions
   * Return NULL if path does not exist
   * `count` can be NULL and return 0 for for non array/object
   **/
  const RedisJSON *(*get)(const RedisJSONKey *key, const char *path, 
                          JSONType *type, size_t *count);
  const RedisJSON *(*getAt)(const RedisJSON *jsonIn, size_t index, 
                            JSONType *type, size_t *count);
  void (*close)(const RedisJSON json);

  /* RedisJSON value functions
   * Return REDISMODULE_OK if RedisJSON is of the correct JSONType,
   * else REDISMODULE_ERR is returned
   **/
  int (*getInt)(const RedisJSON *json, int *integer);
  int (*getFloat)(const RedisJSON *json, double *dbl);
  int (*getBoolean)(const RedisJSON *json, int *boolean);
  int (*getString)(const RedisJSON *json, char **str, size_t *len);

  int (*setInt)(const RedisJSON *json, int integer);
  int (*setFloat)(const RedisJSON *json, double dbl);
  int (*setBoolean)(const RedisJSON *json, int boolean);
  int (*setString)(const RedisJSON *json, const char *str, size_t len);

  void (*replyWith)(struct RedisModuleCtx* ctx, const RedisJSON *json);
} RedisJSONAPI_V1;

// TODO: remove
// RedisJSONAPI_V1 *jsonAPI;

#ifdef __cplusplus
}
#endif
#endif /* SRC_REJSON_API_H_ */
