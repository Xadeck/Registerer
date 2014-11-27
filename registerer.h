#ifndef REGISTERER_H
#define REGISTERER_H

#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <functional>

template <typename T>
class Registerer {
  public:
    typedef std::function<T*()> function_t;
    Registerer(const char* filename, int line, function_t function, const std::string& key) {
      const Entry entry = { filename, line, function};
      registry_mutex_.lock();
      registry_.emplace(key, entry);
      registry_mutex_.unlock();
    }
    
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
std::mutex Registerer<T>::registry_mutex_;

template <typename T>
std::map<std::string, typename Registerer<T>::Entry> Registerer<T>::registry_;

#endif // REGISTERER_H
