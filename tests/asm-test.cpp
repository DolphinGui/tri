#include "tri/asm.hpp"
#include <iostream>
#include <utility>

int main() {
  auto data = ".ascii str 'hello world'\n"
              ".int strlen 11\n"
              ".int c1 1";
  auto code = "mov str r0\n"
              "mov str r1\n"
              "addi r1 strlen r1\n"
              "mov ip r2\n"
              "out r0\n"
              "subi r0 c1 r0\n"
              "jn r0 r2";
  auto e = tri::assemble(data, code);
  auto run = tri::Interpreter(std::move(e));
  run.enable_debug();
  run.execute();
  int i = 123;
}