#include "tri/asm.hpp"
#include <iostream>
#include <utility>

int main() {
  auto data = ".ascii str 'hello world!\\n'\n"
              ".int strlen 13";
  auto code = "mov str r0\n" // begin
              "mov str r1\n" // end
              "addi r1 strlen r1\n" // end = begin + strlen
              "mov ip r2\n" // r2 is mark
              "out r0\n" // print char
              "addi r0 1 r0\n" // 
              "jn r0 r1 r2";
  auto e = tri::assemble(data, code);
  auto run = tri::Interpreter(std::move(e));
  // run.enable_debug();
  run.execute();
  int i = 123;
}