#include "registerer.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace {
class Engine {
public:
  virtual ~Engine() {}

  virtual float consumption() const = 0;
};

class V4Engine : public Engine {
public:
  REGISTER("V4", Engine);
  virtual float consumption() const { return 5.0; }
};

#define NAMES(...) __VA_ARGS__

class V8Engine : public Engine {
public:
  REGISTER(NAMES("V8", "Truck"), Engine);

  virtual float consumption() const { return 15.0; }
};

using testing::UnorderedElementsAre;

TEST(Vehicle, RegistrationNames) {
  EXPECT_THAT(V4Engine::EngineTrait::keys(), UnorderedElementsAre("V4"));
  EXPECT_THAT(V8Engine::EngineTrait::keys(),
              UnorderedElementsAre("V8", "Truck"));
}

TEST(Vehicle, RegistrationNameWorks) {
  auto engine = Registry<Engine>::CreateByName("V4");
  ASSERT_TRUE(engine.get());
  EXPECT_EQ(5, engine->consumption());
}

TEST(Vehicle, OtherRegistrationNameWorks) {
  auto engine = Registry<Engine>::CreateByName("Truck");
  ASSERT_TRUE(engine.get());
  EXPECT_EQ(15, engine->consumption());
}

TEST(Vehicle, UnknownRegistrationNameYieldsNull) {
  auto engine = Registry<Engine>::CreateByName("V16");
  ASSERT_FALSE(engine.get());
}

class Vehicle {
public:
  virtual ~Vehicle() {}

  virtual const Engine &engine() const = 0;

  virtual int tank_size() const = 0;

  float autonomy() const { return tank_size() / engine().consumption(); }
};

class Car : public Vehicle {
public:
  REGISTER_1("Car", Vehicle, Engine *);

  explicit Car(Engine *engine) : engine_(engine) {}

  const Engine &engine() const override { return *engine_; }
  int tank_size() const override { return 60; }

public:
  Engine *const engine_;
};

class Truck : public Vehicle {
public:
  REGISTER_1("Truck", Vehicle, Engine*);

  explicit Truck(Engine *engine) : engine_(engine) {}

  const Engine &engine() const override { return *engine_; }
  int tank_size() const override { return 140; }

public:
  Engine *const engine_;
};

} // namespace
