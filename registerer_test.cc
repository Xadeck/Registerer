#include "registerer.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::ContainerEq;
using ::testing::UnorderedElementsAre;

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

class V8Engine : public Engine {
public:
  REGISTER("V8", Engine);

  virtual float consumption() const { return 15.0; }
};

TEST(Engine, RegistrationNameWorks) {
  auto engine = Registry<Engine>::CreateByName("V4");
  ASSERT_TRUE(engine.get());
  EXPECT_EQ(5, engine->consumption());
}

TEST(Engine, OtherRegistrationNameWorks) {
  auto engine = Registry<Engine>::CreateByName("V8");
  ASSERT_TRUE(engine.get());
  EXPECT_EQ(15, engine->consumption());
}

TEST(Engine, UnknownRegistrationNameYieldsNull) {
  auto engine = Registry<Engine>::CreateByName("V16");
  ASSERT_FALSE(engine.get());
}

TEST(Engine, RegistrationNames) {
  EXPECT_EQ("V4", Registry<Engine>::GetKeyFor<V4Engine>());
  EXPECT_EQ("V8", Registry<Engine>::GetKeyFor<V8Engine>());

  EXPECT_THAT(Registry<Engine>::GetKeys(), UnorderedElementsAre("V4", "V8"));
  static const std::string this_file(__FILE__);
  EXPECT_THAT(
      Registry<Engine>::GetKeysWithLocations(),
      UnorderedElementsAre(this_file + ":18: V4", this_file + ":24: V8"));
}

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

public:
  Engine *const engine_;
};

class Truck : public Vehicle {
public:
  REGISTER("Truck", Vehicle, Engine *);

  explicit Truck(Engine *engine) : engine_(engine) {}

  const Engine *engine() const override { return engine_; }
  int tank_size() const override { return 140; }

public:
  Engine *const engine_;
};

TEST(Vehicle, RegistrationNameWorks) {
  auto engine = Registry<Engine>::CreateByName("V4");
  auto vehicle = Registry<Vehicle, Engine *>::CreateByName("Car", engine.get());
  ASSERT_TRUE(vehicle.get());
  EXPECT_EQ(60, vehicle->tank_size());
}

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

TEST(Vehicle, MultipleRegistrationWorks) {
  auto vehicle1 = Registry<Vehicle>::CreateByName("Bicycle");
  ASSERT_TRUE(vehicle1.get());
  EXPECT_EQ(0, vehicle1->tank_size());

  auto engine = Registry<Engine>::CreateByName("V4");
  auto vehicle2 =
      Registry<Vehicle, Engine *>::CreateByName("Motorbike", engine.get());
  ASSERT_TRUE(vehicle2.get());
  EXPECT_EQ(10, vehicle2->tank_size());
}

TEST(Vehicle, MultipleRegistrationsWorks) {
  EXPECT_EQ("Bicycle", (Registry<Vehicle>::GetKeyFor<Bicycle>()));
  EXPECT_EQ("Motorbike", (Registry<Vehicle, Engine *>::GetKeyFor<Bicycle>()));
}

TEST(Vehicle, RegistrationNames) {
  EXPECT_EQ("Car", (Registry<Vehicle, Engine *>::GetKeyFor<Car>()));
  EXPECT_EQ("Truck", (Registry<Vehicle, Engine *>::GetKeyFor<Truck>()));
  EXPECT_THAT((Registry<Vehicle, Engine *>::GetKeys()),
              UnorderedElementsAre("Car", "Truck", "Motorbike"));
  EXPECT_THAT(Registry<Vehicle>::GetKeys(), UnorderedElementsAre("Bicycle"));

  static const std::string this_file(__FILE__);
  EXPECT_THAT((Registry<Vehicle, Engine *>::GetKeysWithLocations()),
              UnorderedElementsAre(this_file + ":72: Car",
                                   this_file + ":85: Truck",
                                   this_file + ":106: Motorbike"));
  EXPECT_THAT(Registry<Vehicle>::GetKeysWithLocations(),
              UnorderedElementsAre(this_file + ":105: Bicycle"));
}

} // namespace
