#include "registerer.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::ContainerEq;
using ::testing::UnorderedElementsAre;
using ::testing::Return;

using ::factory::Registry;

// Use a namespace to check that macros work inside another namespace.
namespace test {
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
  REGISTER("V4", Engine);

public:
  virtual float consumption() const { return 5.0; }
};

class V8Engine : public Engine {
  REGISTER("V8", Engine);

public:
  virtual float consumption() const { return 15.0; }
};

TEST(Engine, KnownKeysYieldTheRightObjects) {
  ASSERT_TRUE(Registry<Engine>::CanNew("V4"));
  auto engine = Registry<Engine>::New("V4");
  ASSERT_TRUE(engine.get());
  EXPECT_EQ(5, engine->consumption());

  ASSERT_TRUE(Registry<Engine>::CanNew("V8"));
  auto other_engine = Registry<Engine>::New("V8");
  ASSERT_TRUE(other_engine.get());
  EXPECT_EQ(15, other_engine->consumption());
}

TEST(Engine, UnknownKeyYieldsNull) {
  ASSERT_FALSE(Registry<Engine>::CanNew("V16"));
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
      UnorderedElementsAre(this_file + ":25: V4", this_file + ":31: V8"));
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
              UnorderedElementsAre(this_file + ":85: Car",
                                   this_file + ":97: Truck",
                                   this_file + ":110: Motorbike"));
  EXPECT_THAT(Registry<Vehicle>::GetKeysWithLocations(),
              UnorderedElementsAre(this_file + ":109: Bicycle"));
}

//*****************************************************************************
// Test ability to override registered class using an injector.
//*****************************************************************************
class MockEngine : public Engine {
public:
  MOCK_CONST_METHOD0(consumption, float());
};

TEST(Registry, Injector) {
  MockEngine *mock = new ::testing::NiceMock<MockEngine>();
  ON_CALL(*mock, consumption()).WillByDefault(Return(123));

  Registry<Engine>::Injector injector("V4", [mock]() { return mock; });

  // Because of the injector, the New() vall below will return the mock
  //  (taking ownership) instead of creating a new V4Engine instance.
  std::unique_ptr<Engine> engine = Registry<Engine>::New("V4");
  ASSERT_TRUE(engine.get());
  EXPECT_EQ(123, engine->consumption()); // It's the mock expectation.
}

} // namespace test
} // namespace
