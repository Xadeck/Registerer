#include "registerer.h"
#include "gtest/gtest.h"

class Vehicle {
public:
  virtual ~Vehicle() {}
  
  virtual int nb_wheels() const = 0;
};

// #define REGISTER(type) \
//   Registerer<type>
//
// #define REGISTER_VEHICLE REGISTER(Vehicle)

template <typename Trait, typename derived_type>
struct AutoRegisterer {
  static const Registerer<typename Trait::base_type> registerer;
};

template <typename Trait, typename derived_type>
Registerer<typename Trait::base_type> const AutoRegisterer<Trait, derived_type>::registerer(__FILE__, __LINE__,
  []() -> typename Trait::base_type* { return new derived_type; },
  Trait::key());

class Car : public Vehicle {
public:
  struct VehicleTrait {
    typedef Vehicle base_type;
    static const char* key() { return "Car"; }
  };
  const void* unused_Vehicle_pointer() {
    return &AutoRegisterer<VehicleTrait, Car>::registerer;
  }
  
  int nb_wheels() const override { return 4; }  
};



TEST(Point, creation) {
  auto vehicle = Registerer<Vehicle>::CreateByName("Car");
  ASSERT_TRUE(vehicle.get());
  EXPECT_EQ(4, vehicle->nb_wheels());
}
