#ifndef REGISTERER_H
#define REGISTERER_H

#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <functional>
#include <type_traits>
#include <vector>

template <typename T, class... Args> class Registry {
public:
  typedef std::function<T *(Args...)> function_t;

  struct Registerer {
    Registerer(const char *filename, int line, function_t function,
               const std::vector<std::string> &keys) {
      const Entry entry = {filename, line, function};
      registry_mutex_.lock();
      for (const std::string &key : keys) {
        GetRegistry()->emplace(key, entry);
      }
      registry_mutex_.unlock();
    }
  };

  static std::unique_ptr<T> CreateByName(const std::string &key, Args... args) {
    std::unique_ptr<T> result;
    registry_mutex_.lock();
    auto entry_it = GetRegistry()->find(key);
    if (entry_it != GetRegistry()->end()) {
      result.reset(entry_it->second.function(args...));
    }
    registry_mutex_.unlock();
    return std::move(result);
  }

private:
  struct Entry {
    const char *const filename;
    const int line;
    const function_t function;
  };
  static std::mutex registry_mutex_;
  static std::map<std::string, Entry> *GetRegistry() {
    static std::map<std::string, Entry> registry;
    return &registry;
  }
};

template <typename T, class... Args>
std::mutex Registry<T, Args...>::registry_mutex_;

template <typename Trait, typename derived_type, class... Args>
struct TypeRegisterer {
  static const typename Registry<typename Trait::base_type, Args...>::Registerer
      registerer;
};

template <typename Trait, typename derived_type, class... Args>
typename Registry<typename Trait::base_type, Args...>::Registerer const
TypeRegisterer<Trait, derived_type, Args...>::registerer(
    __FILE__, __LINE__, [](Args... args) -> typename Trait::base_type *{
  return new derived_type(args...);
}, Trait::keys());

#define REGISTER(KEYS, TYPE, ARGS...)                                          \
  struct TYPE##Trait {                                                         \
    typedef TYPE base_type;                                                    \
    static std::vector<std::string> keys() { return {KEYS}; }                  \
  };                                                                           \
  const void *unused_##TYPE##_pointer() {                                      \
    return &TypeRegisterer<TYPE##Trait, std::decay<decltype(*this)>::type,     \
                           ##ARGS>::registerer;                                \
  }

#endif // REGISTERER_H
