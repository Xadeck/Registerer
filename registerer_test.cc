#include "gtest/gtest.h"

#include <iostream>

struct Point {
  int x;
  int y;
};

TEST(Point, creation) {
  const Point p = {1,2};
  
  EXPECT_EQ(2, p.y);
}
