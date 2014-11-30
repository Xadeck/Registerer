#include "registerer.h"
#include <iostream>

using factory::Registry;

class Shape {
public:
  virtual ~Shape() {}
  virtual void Draw() const = 0;
};

class Circle : public Shape {
public:
  REGISTER("Circle", Shape);
  void Draw() const override { std::cout << "Circle()\n"; }
};

class Rect : public Shape {
public:
  REGISTER("Rectangle", Shape);
  void Draw() const override { std::cout << "Rectangle()\n"; }
};

class Ellipsis : public Shape {
public:
  REGISTER("Ellipsis", Shape, const std::string &);
  explicit Ellipsis(const std::string &params) : params(params) {}
  void Draw() const override { std::cout << "Ellipsis(" << params << ")\n"; }

private:
  const std::string params;
};

int main(int argc, char **argv) {
  for (int i = 1; i + 1 < argc; i += 2) {
    const std::string key = argv[i];
    const std::string params = argv[i + 1];
    if (Registry<Shape, const std::string &>::CanNew(key, params)) {
      Registry<Shape, const std::string &>::New(key, params)->Draw();
    } else if (Registry<Shape>::CanNew(key)) {
      Registry<Shape>::New(key)->Draw();
    } else {
      // Unknown shape key, use Registry<>::GetKey() to log existing ones.
      return -1;
    }
  }
  return 0;
}
