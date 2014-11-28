#ifndef REGISTERER_H
#define REGISTERER_H

#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <functional>
#include <type_traits>
#include <vector>

#define REGISTER(KEY, TYPE, ARGS...) REGISTER_AT(__LINE__, KEY, TYPE, ##ARGS)

template <typename T, class... Args> class Registry {
public:
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

  template <typename C> static const char *GetKeyFor() {
    return C::__key(std::function<void(const T *, Args...)>());
  }

  typedef std::function<T *(Args...)> function_t;

  struct __Registerer {
    __Registerer(const char *filename, int line, function_t function,
               const std::string &key) {
      const Entry entry = {filename, line, function};
      registry_mutex_.lock();
      GetRegistry()->emplace(key, entry);
      registry_mutex_.unlock();
    }
  };

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
struct Type__Registerer {
  static const typename Registry<typename Trait::base_type, Args...>::__Registerer
      registerer;
};

template <typename Trait, typename derived_type, class... Args>
typename Registry<typename Trait::base_type, Args...>::__Registerer const
    Type__Registerer<Trait, derived_type, Args...>::registerer(
        __FILE__, __LINE__, [](Args... args) -> typename Trait::base_type *
                            { return new derived_type(args...); },
        Trait::key());

#define CONCAT_STRINGS(x, y) x##y

#define REGISTER_AT(LINE, KEY, TYPE, ARGS...)                                  \
  struct CONCAT_STRINGS(__Trait, LINE) {                                       \
    typedef TYPE base_type;                                                    \
    static const char* key() { return {KEY}; }                   \
  };                                                                           \
  const void *CONCAT_STRINGS(__unused, LINE)() const {                         \
    return &Type__Registerer<CONCAT_STRINGS(__Trait, LINE),                      \
                           std::decay<decltype(*this)>::type,                  \
                           ##ARGS>::registerer;                                \
  }                                                                            \
  static const char *__key(std::function<void(TYPE *, ##ARGS)>) { return KEY; }

#endif // REGISTERER_H
