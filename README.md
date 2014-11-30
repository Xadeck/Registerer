Registerer
==========

C++ header-only library to create classe factories registered by name.

## Basic usage

Given an interface:

```cpp
    class Shape {
    public:
      virtual ~Shape() {}
      virtual void Draw() const = 0;
    };

The framework allows to annotate with `REGISTER` macro any subclass:

```cpp
    class Circle : public Shape {
      REGISTER("Circle", Shape);
     public:
       void Draw() const override { ... }
     };

The first parameter of the macro can be any string and does not have to
match the name of the class:

```cpp
    class Rect : public Shape {
      REGISTER("Rectangle", Shape);
    public:
      void Draw() const override { ... }
    };

Note that the annotation is done only inside the derived classes.
Nothing needs to be added to the Shape class, and no declaration
other than the Shape class need to be done. The annotation can be
put inside the private, protected or public section of the class.

With the annotation, a Shape instance can be created from a string:

```cpp
    std::unique_ptr<Shape> shape = Registry<Shape>::New("Rect");
    shape->Draw();  // will draw a rectangle!

The function returns a `unique_ptr<>` which makes ownership clear and simple.
If no class is registered, it will return a null `unique_ptr<>`.
The `CanNew()` predicate can be used to check if `New()` would succeed without
actually creating an instance.

## Advanced usage

The basic usage considered classes with parameter-less constructor.
The `REGISTER` macro can also be used with classes taking arbitrary
parameters. For example, shapes with injectable parameters
can be registered using `REGISTER` macro with extra parameters matching
the constructor signature:

```cpp
    class Ellipsis : public Shape {
      REGISTER("Ellipsis", Shape, const std::string&);
    public:
      explicit Ellipsis(const std::string& params) { ... }
      void Draw() const override { ... }
    };

The class can be instantiated using Registry

    auto shape = Registry<Shape, const string&>::New("Ellipsis");
    shape->Draw();  // will draw a circle!

One very interesting benefit of this approach is that client code can
extend the supported types without editing the base class or
any of the existing registered classes.

```cpp
    int main(int argc, char **argv) {
      for (int i = 1; i + 1 < argc; i += 2) {
        const std::string key = argv[i];
        const std::string params = argv[i + 1];
        if (Registry<Shape, const std::string &>::CanNew(key, params)) {
          Registry<Shape, const std::string &>::New(key, params)->Draw();
        } else if (Registry<Shape>::CanNew(key)) {
          Registry<Shape>::New(key)->Draw();
        }
      }
    }

Additionally, it is possible to register a class with different constructors
so it works with some old client code that only uses Registry<Shape> and
some new client code like the one above:

```cpp
   class Ellipsis : public Shape {
    REGISTER("Ellipsis", Shape);
    REGISTER("Ellipsis", Shape, const std::string&);

   public:
    explicit Ellipsis(const std::string& params = "") { ... }
    void Draw() const override { ... }
  };

## Testing

When testing client code using registered classes, it may be not desired to
actually instantiate real classes. If the Draw() implementations for example
require a graphic context to be active, calling those functions in an
offscreen automated test will fail. Injectors can be used to temporarily
replace registered classes by arbitrary factories:

```cpp
    class FakeShape : public Shape {
     public:
      void Draw() override {}
    };
    
    TEST(Draw, WorkingCase) {
      const Registry<Shape>::Injector injectors[] =
        Registry<Shape>::Injector("Circle",
                                  []->Shape*{ return new FakeShape});
        Registry<Shape>::Injector("Rectangle",
                                  []->Shape*{ return new FakeShape});
        Registry<Shape>::Injector("Circle",
                                  []->Shape*{ return new FakeShape});
        EXPECT_TRUE(Draw({...}));
      };
    }

Injectors can also be used as static global variables to perform
registration of a class *outside* of the class, in replacement for
the REGISTER macro. This is useful to register classes whose code
cannot be edited.

## Goodies

Classes registered with the REGISTER macro can know the key under
which they are registered. The following code:

```cpp
    Registry<Vehicle>::GetKeyFor<Circle>()

will return the string "Circle". This works on object classes,
passed as template parameter to GetKeyFor(), and not on object
instances. So there is no such functionality as:

```cpp
    std::unique_ptr<Shape> shape = new Circle();
    std::cout << Registry<Vehicle>::GetKeyFor(*shape); // Not implemented

as it would require runtime type identification. It is possible to
extend the framework to support it though, by storing `type_info`
objects in the registry.

The Registry<> class can also be used to list all keys that are
registered, along with the filename and line number at which the
registration is defined. It does not list though the type associated
to the key. Here again, it is pretty easy to extend the framework
to provide such functionality using type_info objects.

## Limitations

The code requires a C++11 compliant compiler.

The code relies on the a GNU preprocessor feature for variadic macros:

```cpp
    #define MACRO(x, opt...) call(x, ##opt)

which extends to call(x) when opt is empty and call(x, ...) otherwise.

None of the static methods of Registry<> can be called from
a global static, as it would result in an initialization order fiasco.

## Features summary
 
 - Neader-only library
 - No need to declare registration ahead on the base class,
   which means this file need to be included only in files
   declaring subclasses that register themselves.
 - Registration is done via a macro called inside the declaration
   of the class, which reduces boilerplate code, and incidentally makes
   the registration name available to the class.
 - Macro supports constructors with arbitrary number of arguments,
   all implemented in a single macro
 - Class can be registered for different constructors, making
   it easy to implement logic that try to instantiate an object
   from different parameters.
 - Builtin mechanism to override registered class, making dependency
   injection e.g. for tests very easy.
