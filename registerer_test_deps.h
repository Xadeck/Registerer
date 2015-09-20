#ifndef REGISTERER_TEST_DEPS_H
#define REGISTERER_TEST_DEPS_H

namespace test {
class Engine {
public:
  virtual ~Engine();

  virtual float consumption() const = 0;
};

class Vehicle {
public:
  virtual ~Vehicle() {}

  virtual const Engine *engine() const = 0;
  virtual int tank_size() const = 0;

  float autonomy() const {
    return engine() ? tank_size() / engine()->consumption() : -1;
  }
};

} // namespace test

#endif // REGISTERER_TEST_DEPS_H
