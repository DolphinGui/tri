#include "fmt/format.h"
#include "tri/asm.hpp"
#include <iostream>
#include <sstream>
#include <utility>

int main() {
  auto data = "";
  auto read_string = "in r0\n"         // size of string
                     "alloc r0 r1\n"   // malloc(strsize) = begin
                     "mov r1 r2\n"     // end
                     "addi r2 r0 r2\n" // end = begin + strsize
                     "@read\n"
                     "in r3\n"         // getchar
                     "store r3 r1\n"   // store getchar
                     "addi r1 1 r1\n"  // increment begin
                     "subi r2 r1 r4\n" // compare
                     "jnz r4 @read\n"  // jump if begin != end

                     "subi r1 r0 r1\n" // begin = begin - strsize
                     "@output\n"
                     "load r1 r3\n"
                     "out r3\n"        // print char
                     "addi r1 1 r1\n"  // increment begin
                     "subi r2 r1 r4\n" // compare
                     "jnz r4 @output";
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
  fmt::print("words: {}\n", run.mem_consumption());
}