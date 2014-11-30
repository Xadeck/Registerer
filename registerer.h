// Framework for performing registration of object factories.
//
// The major points of the framework:
//  - header-only library
//  - no need to declare registration ahead on the base class,
//    which means header need to be included only in files
//    declaring subclasses that register themselves.
//  - registration is done via a macro called inside the declaration
//    of the class, which reduces boilerplate code, and incidentally makes
//    the registration name available to the class.
//  - ingle macro supports constructors with arbitrary number of arguments.
//  - class can be registered for different constructors, making
//    it easy to implement logic that try to instantiate an object
//    from different parameters.
//  - builtin mechanism to override registered classes, making dependency
//    injection e.g. for tests very easy.
//
// Basic usage
// -----------
// Given an interface:
//
//    class Shape {
//      public:
//       virtual ~Shape() {}
//       virtual void Draw() const = 0;
//    };
//
// The framework allows to annotate with REGISTER macro any subclass:
//
//    class Circle : public Shape {
//       REGISTER("Circle", Shape);
//      public:
//       void Draw() const override { ... }
//    };
//
// The first parameter of the macro can be any string and does not have to
// match the name of the class:
//
//  class Rect : public Shape {
//     REGISTER("Rectangle", Shape);
//   public:
//     void Draw() const override { ... }
//  };
//
// Note that the annotation is done only inside the derived classes.
// Nothing needs to be added to the Shape class, and no declaration
// other than the Shape class need to be done. The annotation can be
// put inside the private, protected or public section of the class.
//
// With the annotation, a Shape instance can be created from a string:
//
//    std::unique_ptr<Shape> shape = Registry<Shape>::New("Rect");
//    shape->Draw();  // will draw a rectangle!
//
// The function returns a unique_ptr<> which makes ownership clear and simple.
// If no class is registered, it will return a null unique_ptr<>.
// The CanNew() predicate can be used to check if New() would succeed without
// actually creating an instance.
//
// Advanced usage
// --------------
//
// The basic usage considered classes with parameter-less constructor.
// The REGISTER macro can also be used with classes taking arbitrary
// parameters. For example, shapes with injectable parameters
// can be registered using REGISTER() macro with extra parameters matching
// the constructor signature:
//
//  class Ellipsis : public Shape {
//    REGISTER("Ellipsis", Shape, const std::string&);
//   public:
//    explicit Ellipsis(const std::string& params) { ... }
//      void Draw() const override { ... }
//  };
//
// The class can be instantiated using Registry<> with extra parameters 
// matching the constructor signature:
//
//  auto shape = Registry<Shape, const string&>::New("Ellipsis");
//  shape->Draw();  // will draw a circle!
//
// One very interesting benefit of this approach is that client code can
// extend the supported types without editing the base class or
// any of the existing registered classes.
//
//   int main(int argc, char **argv) {
//     for (int i = 1; i + 1 < argc; i += 2) {
//       const std::string key = argv[i];
//       const std::string params = argv[i + 1];
//       if (Registry<Shape, const std::string &>::CanNew(key, params)) {
//         Registry<Shape, const std::string &>::New(key, params)->Draw();
//       } else if (Registry<Shape>::CanNew(key)) {
//         Registry<Shape>::New(key)->Draw();
//       }
//     }
//   }
//
// Additionally, it is possible to register a class with different constructors
// so it works with some old client code that only uses Registry<Shape> and
// some new client code like the one above:
//
//   class Ellipsis : public Shape {
//     REGISTER("Ellipsis", Shape);
//     REGISTER("Ellipsis", Shape, const std::string&);
//    public:
//     explicit Ellipsis(const std::string& params = "") { ... }
//     void Draw() const override { ... }
//   };
//
// Testing
// -------
//
// When testing client code using registered classes, it may be not desired to
// actually instantiate real classes. If the Draw() implementations for example
// require a graphic context to be active, calling those functions in an
// offscreen automated test will fail. Injectors can be used to temporarily
// replace registered classes by arbitrary factories:
//
//   class FakeShape : public Shape {
//    public:
//     void Draw() override {}
//   };
//
//   TEST(Draw, WorkingCase) {
//     const Registry<Shape>::Injector injectors[] =
//       Registry<Shape>::Injector("Circle",
//                                 []->Shape*{ return new FakeShape});
//       Registry<Shape>::Injector("Rectangle",
//                                 []->Shape*{ return new FakeShape});
//       Registry<Shape>::Injector("Circle",
//                                 []->Shape*{ return new FakeShape});
//       EXPECT_TRUE(FunctionUsingRegistryForShape());
//     };
//   }
//
// Injectors can also be used as static global variables to perform
// registration of a class *outside* of the class, in replacement for
// the REGISTER macro. This is useful to register classes whose code
// cannot be edited.
//
// Goodies
// -------
//
// Classes registered with the REGISTER macro can know the key under
// which they are registered. The following code:
//
//   Registry<Vehicle>::GetKeyFor<Circle>()
//
// will return the string "Circle". This works on object classes,
// passed as template parameter to GetKeyFor(), and not on object
// instances. So there is no such functionality as:
//
//   std::unique_ptr<Shape> shape = new Circle();
//   std::cout << Registry<Vehicle>::GetKeyFor(*shape); // Not implemented
//
// as it would require runtime type identification. It is possible to
// extend the framework to support it though, by storing `type_info`
// objects in the registry.
//
// The Registry<> class can also be used to list all keys that are
// registered, along with the filename and line number at which the
// registration is defined. It does not list though the type associated
// to the key. Here again, it is pretty easy to extend the framework
// to provide such functionality using type_info objects.
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
  // Return 'true' if there is a class registered for `key` for
  // a constructor with signature (Args... args).
  //
  // This function can not be called from any static initializer
  // or it creates initializer order fiasco.
  static bool CanNew(const std::string &key, Args... args) {
    return GetEntry(key, args...).first;
  }

  // If there is a class registered for `key` for a constructor 
  // with signature (Args... args), instantiate an object of that class
  // passing args to the constructor. Returns a null pointer otherwise.
  //
  // This function can not be called from any static initializer
  // or it creates initializer order fiasco.
  static std::unique_ptr<T> New(const std::string &key, Args... args) {
    std::unique_ptr<T> result;
    auto entry = GetEntry(key, args...);
    if (entry.first) {
      result.reset(entry.second->function(args...));
    }
    registry_mutex_.unlock();
    return std::move(result);
  }

  // Return the key under which class `C` is registered. The header
  // defining that class must be included by code calling this
  // function. If class is not registered, there will be a compile-
  // time failure.
  template <typename C> static const char *GetKeyFor() {
    return C::__key(std::function<void(const T *, Args...)>());
  }

  // Returns the list of keys registered for the registry.
  //
  // This function can not be called from any static initializer
  // or it creates initializer order fiasco.
  static std::vector<std::string> GetKeys() {
    std::vector<std::string> keys;
    registry_mutex_.lock();
    for (const auto &iter : *GetRegistry()) {
      keys.emplace_back(iter.first);
    }
    registry_mutex_.unlock();
    return keys;
  }

  // Like GetKeys() function, but also returns the filename
  // and line number of the corresponding REGISTER() macros.
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

  // Helper class which uses RAII to inject a factory which will be used
  // instead of any class registered with the same key, for any call 
  // within the scope of the variable.
  // If there are two injectors in the same scope for the same key,
  // the last one takes precedence and cancels the first one, which will
  // never be active, even if the second gets out of scope.
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
  typedef std::map<std::string, Entry> EntryMap;
  // The registry and injectors are created on demand using static variables
  // inside a static method so that there is no order initialization fiasco.
  static EntryMap *GetRegistry() {
    static EntryMap registry;
    return &registry;
  }
  static EntryMap *GetInjectors() {
    static EntryMap injectors;
    return &injectors;
  };
  static std::mutex registry_mutex_;

  static std::pair<bool, const Entry *> GetEntry(const std::string &key,
                                                 Args... args) {
    registry_mutex_.lock();
    auto it = GetInjectors()->find(key);
    bool found = (it != GetInjectors()->end());
    if (!found) {
      it = GetRegistry()->find(key);
      found = (it != GetRegistry()->end());
    }
    registry_mutex_.unlock();
    return std::make_pair(found, &(it->second));
  }
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
  friend class ::factory::Registry<TYPE, ##ARGS>;                              \
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
