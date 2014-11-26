#include <iostream>

struct Point {
  int x;
  int y;
};

int main(int,char**) {
  Point p = {1,2};
  std::cout << p.y << '\n';
  
  return 0;
}