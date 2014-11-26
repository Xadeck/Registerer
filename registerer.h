#ifndef REGISTERER_H
#define REGISTERER_H

#include <mutex>
#include <string>
#include <map>
#include <functional>

class Registry {
  public:
    using std::function<T*()> function_t;
  private:
    static std::mutex function_map_mutex_;
    std::map<std::string, function_t> function_map_;
};

#undef REGISTERER_H
