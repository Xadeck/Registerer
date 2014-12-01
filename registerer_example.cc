#include "registerer.h"
#include <iostream>

using factory::Registry;

class Shape {
public:
  virtual ~Shape() {}
  virtual void Draw() const = 0;
};

#define REGISTER_SHAPE(KEY, ARGS...) REGISTER(#KEY, Shape, ##ARGS)

class Circle : public Shape {
  REGISTER_SHAPE(Circle);

public:
  void Draw() const override { std::cout << "Circle\n"; }
};

class Rect : public Shape {
  REGISTER_SHAPE(Rectangle);

public:
  void Draw() const override { std::cout << "Rectangle\n"; }
};

class Ellipsis : public Shape {
  REGISTER_SHAPE(Ellipsis);
  REGISTER_SHAPE(Ellipsis, const std::string &);

public:
  explicit Ellipsis(const std::string &params = "") : params(params) {}
  void Draw() const override { std::cout << "Ellipsis:" << params << "\n"; }

private:
  const std::string params;
};

int main(int argc, char **argv) {
  REGISTER_ALIAS(Shape, "Rectangle", "Rect");

  for (int i = 1; i + 1 < argc; i += 2) {
    const std::string key = argv[i];
    const std::string params = argv[i + 1];
    if (Registry<Shape, const std::string &>::CanNew(key, params)) {
      Registry<Shape, const std::string &>::New(key, params)->Draw();
    } else if (Registry<Shape>::CanNew(key)) {
      Registry<Shape>::New(key)->Draw();
    } else {
      std::cerr << "No '" << key << "' shape registered. Registered are\n";
      for (const auto &k : Registry<Shape>::GetKeys()) {
        std::cerr << "  " << k << '\n';
      }
      for (const auto &k : Registry<Shape, const std::string &>::GetKeys()) {
        std::cerr << "  " << k << "(string)\n";
      }
      return -1;
    }
  }
  return 0;
}
