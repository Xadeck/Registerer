#include "registerer.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

class Vehicle {
public:
  virtual ~Vehicle() {}
  
  virtual int nb_wheels() const = 0;
};

class Car : public Vehicle {
public:
  REGISTER(Vehicle, "Car", "Auto");
  
  int nb_wheels() const override { return 4; }  
};

class Moto : public Vehicle {
public:
  REGISTER(Vehicle, "Motorcycle");
  
  int nb_wheels() const override { return 2; }  
};

using testing::UnorderedElementsAre;

TEST(Vehicle, RegistrationNames) {
  EXPECT_THAT(Car::VehicleTrait::keys(), UnorderedElementsAre("Car", "Auto"));
  EXPECT_THAT(Moto::VehicleTrait::keys(), UnorderedElementsAre("Motorcycle"));
}

TEST(Vehicle, CreatingCar) {
  auto vehicle = Registry<Vehicle>::CreateByName("Car");
  ASSERT_TRUE(vehicle.get());
  EXPECT_EQ(4, vehicle->nb_wheels());
}

TEST(Vehicle, CreatingMotorcycle) {
  auto vehicle = Registry<Vehicle>::CreateByName("Motorcycle");
  ASSERT_TRUE(vehicle.get());
  EXPECT_EQ(2, vehicle->nb_wheels());
}

TEST(Vehicle, CreatingBoat) {
  auto vehicle = Registry<Vehicle>::CreateByName("Boat");
  ASSERT_FALSE(vehicle.get());
}
