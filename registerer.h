// Framework for performing registration of object factories.
//
// Limitations
// -----------
// The code requires a C++11 compliant compiler.
//
// The code relies on the a GNU preprocessor feature for variadic macros:
//
//    #define MACRO(x, opt...) call(x, ##opt)
//
// which extends to call(x) when opt is empty and call(x, ...) otherwise.
//
// None of the static methods of Registry<> can be called from
// a global static, as it would result in an initialization order fiasco.

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

namespace factory {
template <typename T, class... Args> class Registry {
public:
  static std::unique_ptr<T> New(const std::string &key, Args... args) {
    std::unique_ptr<T> result;
    registry_mutex_.lock();
    auto it = GetInjectors()->find(key);
    bool found = (it != GetInjectors()->end());
    if (!found) {
      it = GetRegistry()->find(key);
      found = (it != GetRegistry()->end());
    }
    if (found) {
      result.reset(it->second.function(args...));
    }
    registry_mutex_.unlock();
    return std::move(result);
  }

  template <typename C> static const char *GetKeyFor() {
    return C::__key(std::function<void(const T *, Args...)>());
  }

  static std::vector<std::string> GetKeys() {
    std::vector<std::string> keys;
    registry_mutex_.lock();
    for (const auto &iter : *GetRegistry()) {
      keys.emplace_back(iter.first);
    }
    registry_mutex_.unlock();
    return keys;
  }

  static std::vector<std::string> GetKeysWithLocations() {
    std::vector<std::string> keys;
    registry_mutex_.lock();
    for (const auto &iter : *GetRegistry()) {
      keys.emplace_back(std::string(iter.second.file) + ":" +
                        std::string(iter.second.line) + ": " + iter.first);
    }
    registry_mutex_.unlock();
    return keys;
  }

  struct Injector {
    const std::string &key;
    Injector(const std::string &key,
             const std::function<T *(Args...)> &function,
             const char *file = "undefined", const char *line = "undefined")
        : key(key) {
      registry_mutex_.lock();
      const Entry entry = {file, line, function};
      GetInjectors()->emplace(key, entry);
      registry_mutex_.unlock();
    }
    ~Injector() {
      registry_mutex_.lock();
      GetInjectors()->erase(key);
      registry_mutex_.unlock();
    }
  };
  //***************************************************************************
  // Implementation details that can't be made private because used in macros
  //***************************************************************************
  typedef std::function<T *(Args...)> __function_t;

  struct __Registerer {
    __Registerer(__function_t function, const std::string &key,
                 const char *file, const char *line) {
      const Entry entry = {file, line, function};
      registry_mutex_.lock();
      GetRegistry()->emplace(key, entry);
      registry_mutex_.unlock();
    }
  };

private:
  struct Entry {
    const char *const file;
    const char *const line;
    const __function_t function;
  };
  // The registry is created on demand using a static variable inside a static
  // method so that there is no order initialization fiasco.
  static std::map<std::string, Entry> *GetRegistry() {
    static std::map<std::string, Entry> registry;
    return &registry;
  }
  static std::mutex registry_mutex_;
  static std::map<std::string, Entry> *GetInjectors() {
    static std::map<std::string, Entry> injectors;
    return &injectors;
  };
};

template <typename T, class... Args>
std::mutex Registry<T, Args...>::registry_mutex_;

//*****************************************************************************
// Implementation details of REGISTER() macro.
//
// Creates uniquely named traits class and functions which forces the
// instantiation of a TypeRegisterer class with a static member doing the
// actual registration in the registry. Note that TypeRegisterer being a
// template, there is no violation of the One Definition Rule. The use of
// Trait class is way to pass back information from the class where the
// macro is called, to the definition of TypeRegisterer static member.
// This works only because the Trait functions do not reference any other
// static variable, or it would create an initialization order fiasco.
//*****************************************************************************
template <typename Trait, typename base_type, typename derived_type,
          class... Args>
struct TypeRegisterer {
  static const typename Registry<base_type, Args...>::__Registerer instance;
};

template <typename Trait, typename base_type, typename derived_type,
          class... Args>
typename Registry<base_type, Args...>::__Registerer const
    TypeRegisterer<Trait, base_type, derived_type,
                   Args...>::instance([](Args... args) -> base_type *
                                      { return new derived_type(args...); },
                                      Trait::key(), Trait::file(),
                                      Trait::line());

#define CONCAT_TOKENS(x, y) x##y
#define STRINGIFY(x) #x

#define REGISTER_AT(LINE, KEY, TYPE, ARGS...)                                  \
  struct CONCAT_TOKENS(__Trait, LINE) {                                        \
    static const char *key() { return KEY; }                                   \
    static const char *file() { return __FILE__; }                             \
    static const char *line() { return STRINGIFY(LINE); }                      \
  };                                                                           \
  const void *CONCAT_TOKENS(__unused, LINE)() const {                          \
    return &::factory::TypeRegisterer<CONCAT_TOKENS(__Trait, LINE), TYPE,      \
                                      std::decay<decltype(*this)>::type,       \
                                      ##ARGS>::instance;                       \
  }                                                                            \
  static const char *__key(std::function<void(TYPE *, ##ARGS)>) { return KEY; }

} // namespace factory

#endif // REGISTERER_H
