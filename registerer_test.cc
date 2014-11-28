#include "registerer.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::ContainerEq;
using ::testing::UnorderedElementsAre;

namespace {
//*****************************************************************************
// Test the simple case of objects with constructors taking no parameters
//*****************************************************************************
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

class V8Engine : public Engine {
public:
  REGISTER("V8", Engine);

  virtual float consumption() const { return 15.0; }
};

TEST(Engine, KnownKeysYieldTheRightObjects) {
  auto engine = Registry<Engine>::New("V4");
  ASSERT_TRUE(engine.get());
  EXPECT_EQ(5, engine->consumption());

  auto other_engine = Registry<Engine>::New("V8");
  ASSERT_TRUE(other_engine.get());
  EXPECT_EQ(15, other_engine->consumption());
}

TEST(Engine, UnknownKeyYieldsNull) {
  auto engine = Registry<Engine>::New("V16");
  ASSERT_FALSE(engine.get());
}

TEST(Engine, GetKeysWorks) {
  EXPECT_EQ("V4", Registry<Engine>::GetKeyFor<V4Engine>());
  EXPECT_EQ("V8", Registry<Engine>::GetKeyFor<V8Engine>());
}

TEST(Engine, GetKeysWithLocationsWorks) {
  EXPECT_THAT(Registry<Engine>::GetKeys(), UnorderedElementsAre("V4", "V8"));
  static const std::string this_file(__FILE__);
  EXPECT_THAT(
      Registry<Engine>::GetKeysWithLocations(),
      UnorderedElementsAre(this_file + ":21: V4", this_file + ":27: V8"));
}

//*****************************************************************************
// Test advanced cases:
//  - base class takes a parameter (a vehicle receives an engine)
//  - subclass registered via multiple signatures
//*****************************************************************************
class Vehicle {
public:
  virtual ~Vehicle() {}

  virtual const Engine *engine() const = 0;
  virtual int tank_size() const = 0;

  float autonomy() const {
    return engine() ? tank_size() / engine()->consumption() : -1;
  }
};

class Car : public Vehicle {
public:
  REGISTER("Car", Vehicle, Engine *);

  explicit Car(Engine *engine) : engine_(engine) {}

  const Engine *engine() const override { return engine_; }
  int tank_size() const override { return 60; }

private:
  Engine *const engine_;
};

class Truck : public Vehicle {
public:
  REGISTER("Truck", Vehicle, Engine *);

  explicit Truck(Engine *engine) : engine_(engine) {}

  const Engine *engine() const override { return engine_; }
  int tank_size() const override { return 140; }

private:
  Engine *const engine_;
};

class Bicycle : public Vehicle {
public:
  REGISTER("Bicycle", Vehicle);
  REGISTER("Motorbike", Vehicle, Engine *);

  explicit Bicycle(Engine *engine = nullptr) : engine_(engine) {}

  const Engine *engine() const override { return engine_; }
  int tank_size() const override { return engine_ ? 10 : 0; }

public:
  Engine *const engine_;
};

TEST(Vehicle, KnownKeysYieldTheRightObjects) {
  auto engine = Registry<Engine>::New("V4");
  auto vehicle = Registry<Vehicle, Engine *>::New("Car", engine.get());
  ASSERT_TRUE(vehicle.get());
  EXPECT_EQ(60, vehicle->tank_size());
}

TEST(Vehicle, ObjectsCanBeInstantiatedViaMultipleRegistration) {
  auto vehicle = Registry<Vehicle>::New("Bicycle");
  ASSERT_TRUE(vehicle.get());
  EXPECT_EQ(0, vehicle->tank_size());

  auto engine = Registry<Engine>::New("V4");
  auto other_vehicle =
      Registry<Vehicle, Engine *>::New("Motorbike", engine.get());
  ASSERT_TRUE(other_vehicle.get());
  EXPECT_EQ(10, other_vehicle->tank_size());
}

TEST(Vehicle, MixedRegistrationAreSupported) {
  EXPECT_EQ("Bicycle", (Registry<Vehicle>::GetKeyFor<Bicycle>()));
  EXPECT_EQ("Motorbike", (Registry<Vehicle, Engine *>::GetKeyFor<Bicycle>()));
}

TEST(Vehicle, GetKeysWorks) {
  EXPECT_EQ("Car", (Registry<Vehicle, Engine *>::GetKeyFor<Car>()));
  EXPECT_EQ("Truck", (Registry<Vehicle, Engine *>::GetKeyFor<Truck>()));
  EXPECT_THAT((Registry<Vehicle, Engine *>::GetKeys()),
              UnorderedElementsAre("Car", "Truck", "Motorbike"));
  EXPECT_THAT(Registry<Vehicle>::GetKeys(), UnorderedElementsAre("Bicycle"));
}

TEST(Vehicle, GetKeysWithLocationsWorks) {
  static const std::string this_file(__FILE__);
  EXPECT_THAT((Registry<Vehicle, Engine *>::GetKeysWithLocations()),
              UnorderedElementsAre(this_file + ":79: Car",
                                   this_file + ":92: Truck",
                                   this_file + ":106: Motorbike"));
  EXPECT_THAT(Registry<Vehicle>::GetKeysWithLocations(),
              UnorderedElementsAre(this_file + ":105: Bicycle"));
}

} // namespace
