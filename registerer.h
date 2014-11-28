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

  template <typename C> static const char *GetKeyFor() {
    return C::__key(std::function<void(const T *, Args...)>());
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
        __FILE__, __LINE__, [](Args... args) -> typename Trait::base_type *
                            { return new derived_type(args...); },
        Trait::keys());

#define CONCAT_STRINGS(x, y) x##y

#define REGISTER_AT(LINE, KEY, TYPE, ARGS...)                                  \
  struct CONCAT_STRINGS(__Trait, LINE) {                                       \
    typedef TYPE base_type;                                                    \
    static std::vector<std::string> keys() { return {KEY}; }                   \
  };                                                                           \
  const void *CONCAT_STRINGS(__unused, LINE)() const {                         \
    return &TypeRegisterer<CONCAT_STRINGS(__Trait, LINE),                      \
                           std::decay<decltype(*this)>::type,                  \
                           ##ARGS>::registerer;                                \
  }                                                                            \
  static const char *__key(std::function<void(TYPE *, ##ARGS)>) { return KEY; }

#define REGISTER(KEY, TYPE, ARGS...) REGISTER_AT(__LINE__, KEY, TYPE, ##ARGS)

#endif // REGISTERER_H
