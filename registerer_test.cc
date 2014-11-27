#include "registerer.h"
#include "gtest/gtest.h"

class Vehicle {
public:
  virtual ~Vehicle() {}
  
  virtual int nb_wheels() const = 0;
};

#define REGISTER_VEHICLE(KEY) REGISTER(Vehicle, KEY)

class Car : public Vehicle {
public:
  REGISTER_VEHICLE("Car");
  
  int nb_wheels() const override { return 4; }  
};

class Moto : public Vehicle {
public:
  REGISTER_VEHICLE("Motorcycle");
  
  int nb_wheels() const override { return 2; }  
};

TEST(Vehicle, RegistrationNames) {
  EXPECT_EQ("Car", Car::VehicleRegistrationName());
  EXPECT_EQ("Motorcycle", Moto::VehicleRegistrationName());
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
