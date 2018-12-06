#include "redismock.h"
#include <cstdarg>
#include <vector>
namespace RMCK {
std::vector<RedisModuleString *> CreateArgv(RedisModuleCtx *, const char *s, ...);
std::vector<RedisModuleString *> CreateArgv(RedisModuleCtx *, const char **s, size_t n);

class ArgvList {
  std::vector<RedisModuleString *> m_list;
  RedisModuleCtx *m_ctx;

 public:
  template <typename... Ts>
  ArgvList(RedisModuleCtx *ctx, Ts... args) : m_ctx(ctx) {
    m_list = CreateArgv(ctx, args..., (const char *)NULL);
  }
  ArgvList(ArgvList &) = delete;
  ~ArgvList() {
    for (auto ss : m_list) {
      RedisModule_FreeString(m_ctx, ss);
    }
  }

  operator RedisModuleString **() {
    return &m_list[0];
  }

  size_t size() const {
    return m_list.size();
  }
};

}  // namespace RMCK