// Framework for performing registration of object factories.
//
// Limitations
// -----------
//  The code requires a C++11 compliant compiler.
//
//  The code relies on the a GNU preprocessor feature for variadic macros:
//
//    #define MACRO(x, opt...) call(x, ##opt)
//
// which extends to call(x) when opt is empty and call(x, ...) otherwise.
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
      keys.emplace_back(std::string(iter.second.location) + ": " + iter.first);
    }
    registry_mutex_.unlock();
    return keys;
  }

  //***************************************************************************
  // Implementation details that can't be made private because used in macros
  //***************************************************************************
  typedef std::function<T *(Args...)> __function_t;

  struct __Registerer {
    __Registerer(const std::string &key, __function_t function,
                 const char *location) {
      const Entry entry = {location, function};
      registry_mutex_.lock();
      GetRegistry()->emplace(key, entry);
      registry_mutex_.unlock();
    }
  };

private:
  struct Entry {
    const char *const location;
    const __function_t function;
  };
  // The registry is created on demand using a static variable inside a static
  // method so that there is no order initialization fiasco.
  static std::map<std::string, Entry> *GetRegistry() {
    static std::map<std::string, Entry> registry;
    return &registry;
  }
  static std::mutex registry_mutex_;
};

template <typename T, class... Args>
std::mutex Registry<T, Args...>::registry_mutex_;

//*****************************************************************************
// Implementation details of REGISTER() macro.
//
// Creates uniquely named traits class and functions which forces the
// instantiation of a class with a static member doing the actual registration
// in the registry. Note that this class being a template, there is no violation
// of the One Definition Rule.
//*****************************************************************************
template <typename Trait, typename derived_type, class... Args>
struct TypeRegisterer {
  static const typename Registry<typename Trait::base_type,
                                 Args...>::__Registerer instance;
};

template <typename Trait, typename derived_type, class... Args>
typename Registry<typename Trait::base_type, Args...>::__Registerer const
    TypeRegisterer<Trait, derived_type, Args...>::instance(
        Trait::key(), [](Args... args) -> typename Trait::base_type *
                           { return new derived_type(args...); },
        Trait::location());

#define CONCAT_STRINGS(x, y) x##y
#define STRINGIFY(x) #x

#define REGISTER_AT(LINE, KEY, TYPE, ARGS...)                                  \
  struct CONCAT_STRINGS(__Trait, LINE) {                                       \
    typedef TYPE base_type;                                                    \
    static const char *location() {                                            \
      static const std::string l =                                             \
          std::string(__FILE__) + ":" STRINGIFY(LINE);                         \
      return l.c_str();                                                        \
    }                                                                          \
    static const char *key() { return KEY; }                                   \
  };                                                                           \
  const void *CONCAT_STRINGS(__unused, LINE)() const {                         \
    return &::factory::TypeRegisterer<CONCAT_STRINGS(__Trait, LINE),           \
                                      std::decay<decltype(*this)>::type,       \
                                      ##ARGS>::instance;                       \
  }                                                                            \
  static const char *__key(std::function<void(TYPE *, ##ARGS)>) { return KEY; }

} // namespace factory

#endif // REGISTERER_H
