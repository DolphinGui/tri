#include "fmt/core.h"
#include "fmt/format.h"
#include "tri/asm.hpp"
#include <iostream>
#include <sstream>
#include <utility>

int main() {
  auto data = "";
  auto read_string = "alloc 5 r1\n"
                     "alloc 5 r0\n"
                     "mov 0 r0\n"
                     "hlt\n"
                     "alloc 5 r0\n"
                     "mov 0 r0\n"
                     "hlt\n";
  auto e = tri::assemble(data, read_string);
  auto run = tri::Interpreter(std::move(e));
  run.execute();
  fmt::print("before: {}", run.mem_consumption());
  run.clean();
  fmt::print(" | after: {}\n", run.mem_consumption());
  run.execute();
  fmt::print("before: {}", run.mem_consumption());
  run.clean();
  fmt::print(" | after: {}\n", run.mem_consumption());
}