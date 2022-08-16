#include "tri/asm.hpp"
#include <iostream>
#include <sstream>
#include <utility>

int main() {
  auto data = ".ascii str 'hello world!\\n'\n"
              ".int strlen 13";
  auto print_string = "mov str r0\n"        // begin
                      "mov str r1\n"        // end
                      "addi r1 strlen r1\n" // end = begin + strlen
                      "mov ip r2\n"         // r2 is mark
                      "out r0\n"            // print char
                      "addi r0 1 r0\n"      // increment begin
                      "jn r0 r1 r2";        // jump if begin != end

  auto read_string = "in r0\n"         // size of string
                     "alloc r0 r1\n"   // malloc(strsize) = begin
                     "mov r1 r2\n"     // end
                     "addi r2 r0 r2\n" // end = begin + strsize
                     "mov ip rp\n"     // rp is mark
                     "in r3\n"         // getchar
                     "store r3 r1\n"   // store getchar
                     "addi r1 1 r1\n"  // increment begin
                     "jn r1 r2 rp\n"   // jump if begin != end

                     "subi r1 r0 r1\n" // begin = begin - strsize
                     "mov ip rp\n"     // rp is mark
                     "out r1\n"        // print char
                     "addi r1 1 r1\n"  // increment begin
                     "jn r1 r2 rp";
  auto e = tri::assemble(data, read_string);
  auto run = tri::Interpreter(std::move(e));
  std::string input = "hello world!\n";
  auto n = std::stringstream(input);
  bool length_given = false;
  run.in = [&]() -> uint32_t {
    if (length_given)
      return n.get();
    else {
      length_given = true;
      return 13;
    }
  };
  run.execute();
  int i = 123;
}