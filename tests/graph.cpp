#include "fmt/format.h"
#include "tri/asm.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

int main() {
  auto text = "alloc 1 r0\n"
              "alloc 1 r1\n"
              "alloc 1 r2\n"
              "store r1 r0\n"
              "store r2 r1\n"
              "store r2 r0\n" // this is a cyclical graph
              "mov 0 r2\n"
              "mov 0 r1\n"
              "hlt\n"
              "mov 0 r0\n";
  auto run = tri::Interpreter(tri::assemble("", text));
  run.execute();
  fmt::print("memory: {}\n", run.mem_consumption());
  run.execute();
  run.clean();
  fmt::print("memory: {}\n", run.mem_consumption());
}