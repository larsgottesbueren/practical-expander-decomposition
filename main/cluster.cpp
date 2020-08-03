#include <iostream>
#include "lib/unit_flow.hpp"

int main() {
  UnitFlow flow(5,3);
  flow.addEdge(0,1,5);

  std::cout << "Hello world" << std::endl;
}
