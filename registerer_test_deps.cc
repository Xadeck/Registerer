#include "registerer_test_deps.h"

#include "registerer.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::factory::Registry;
using ::testing::ContainerEq;
using ::testing::UnorderedElementsAre;

namespace test {
Engine::~Engine() {}

class V4Engine : public Engine {
  REGISTER("V4", Engine);

public:
  virtual float consumption() const { return 5.0; }
};

class V8Engine : public Engine {
  REGISTER("V8", Engine);

public:
  virtual float consumption() const { return 15.0; }
};


class Car : public Vehicle {
  REGISTER("Car", Vehicle, Engine *);

public:
  explicit Car(Engine *engine) : engine_(engine) {}

  const Engine *engine() const override { return engine_; }
  int tank_size() const override { return 60; }

private:
  Engine *const engine_;
};

class Truck : public Vehicle {
  REGISTER("Truck", Vehicle, Engine *);

public:
  explicit Truck(Engine *engine) : engine_(engine) {}

  const Engine *engine() const override { return engine_; }
  int tank_size() const override { return 140; }

private:
  Engine *const engine_;
};

class Bicycle : public Vehicle {
  REGISTER("Bicycle", Vehicle);
  REGISTER("Motorbike", Vehicle, Engine *);

public:
  explicit Bicycle(Engine *engine = nullptr) : engine_(engine) {}

  const Engine *engine() const override { return engine_; }
  int tank_size() const override { return engine_ ? 10 : 0; }

public:
  Engine *const engine_;
};


namespace {
TEST(Engine, GetKeysWorks) {
  EXPECT_EQ("V4", Registry<Engine>::GetKeyFor<V4Engine>());
  EXPECT_EQ("V8", Registry<Engine>::GetKeyFor<V8Engine>());
}

TEST(Vehicle, GetKeysWorks) {
  EXPECT_EQ("Car", (Registry<Vehicle, Engine *>::GetKeyFor<Car>()));
  EXPECT_EQ("Truck", (Registry<Vehicle, Engine *>::GetKeyFor<Truck>()));
  EXPECT_THAT((Registry<Vehicle, Engine *>::GetKeys()),
              UnorderedElementsAre("Car", "Truck", "Motorbike"));
  EXPECT_THAT(Registry<Vehicle>::GetKeys(), UnorderedElementsAre("Bicycle"));
}
TEST(Vehicle, MixedRegistrationAreSupported) {
  EXPECT_EQ("Bicycle", (Registry<Vehicle>::GetKeyFor<Bicycle>()));
  EXPECT_EQ("Motorbike", (Registry<Vehicle, Engine *>::GetKeyFor<Bicycle>()));
}

} // namespace
} // namespace test
