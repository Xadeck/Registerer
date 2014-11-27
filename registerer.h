#ifndef REGISTERER_H
#define REGISTERER_H

#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <functional>
#include <type_traits>

template <typename T>
class Registry {
  public:
    typedef std::function<T*()> function_t;
    
    struct Registerer {
      Registerer(const char* filename, int line, function_t function, const std::string& key) {
        const Entry entry = { filename, line, function};
        registry_mutex_.lock();
        registry_.emplace(key, entry);
        registry_mutex_.unlock();
      }
    };
    
    static std::unique_ptr<T> CreateByName(const std::string& key) {
      std::unique_ptr<T> result;
      registry_mutex_.lock();
      auto entry_it = registry_.find(key);
      if (entry_it != registry_.end()) {
        result.reset(entry_it->second.function());
      }
      registry_mutex_.unlock();      
      return std::move(result);
    }
  private:    
    struct Entry {
      const char* const filename;
      const int line;
      const function_t function;
    };
    static std::mutex registry_mutex_;
    static std::map<std::string, Entry> registry_;
};

template <typename T>
std::mutex Registry<T>::registry_mutex_;

template <typename T>
std::map<std::string, typename Registry<T>::Entry> Registry<T>::registry_;

template <typename Trait, typename derived_type>
struct TypeRegisterer {
  static const typename Registry<typename Trait::base_type>::Registerer registerer;
};

template <typename Trait, typename derived_type>
typename Registry<typename Trait::base_type>::Registerer const TypeRegisterer<Trait, derived_type>::registerer(__FILE__, __LINE__,
  []() -> typename Trait::base_type* { return new derived_type; },
  Trait::key());

#define REGISTER(TYPE, KEY) \
  struct TYPE##Trait { \
    typedef TYPE base_type; \
    static const char* key() { return KEY; } \
  }; \
  const void* unused_##TYPE##_pointer() { \
    return &TypeRegisterer<TYPE##Trait, std::decay<decltype(*this)>::type>::registerer;\
  } \
  static const char* TYPE##RegistrationName() { return KEY; }

#endif // REGISTERER_H
