#include "fmt/format.h"
#include "tri/asm.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
/*
this is the btree that will be formed
   l  l
   \ /
    e  o
    \/
    h
sequence:
(root)h,
  true, e,
    true, l, false, false,
    true, l, false, false,
  true, o,
    false,
    false

node structure
left: 1
right: 1
value: 1
@printtree stack
ret
leftptr - bp
rightptr
@createchildren (ptr)
in val
*(ptr + 2) = val
in is_left
if(is_left) createchildren(*ptr)
in is_right
if(is_right) createchildren(*(ptr+1))
*/
int main() {
  auto data = ".ascii str 'hello world!\\n'\n"
              ".int strlen 13";
  auto text = "@main\n"
              "alloc 3 r0\n"
              "call @createchildren\n"
              "call @printtree\n"
              "hlt\n"
              "mov 0 r0\n"
              "hlt\n"
              "@printtree\n" // r0 = rootptr
              "addi sp 1 sp\n"
              "store rp sp\n" // store ret onto stack
              "addi sp 1 sp\n"
              "store r0 sp\n"
              "addi r0 2 r0\n" // access value
              "load r0 r1\n"
              "out r1\n" // print val
              "load sp r0\n"
              "load r0 r1\n"
              "jez r1 @leftprint\n"
              "load r0 r0\n"
              "call @printtree\n"
              "@leftprint\n"
              "load sp r0\n"
              "addi r0 1 r0\n" // load rightval
              "load r0 r1\n"
              "jez r1 @rightprint\n"
              "load r0 r0\n"
              "call @printtree\n"
              "@rightprint\n"
              "subi sp 1 sp\n"
              "load sp rp\n"
              "subi sp 1 sp\n"
              "jmp rp\n"
              "@createchildren\n" // r0 = parent
              "addi sp 1 sp\n"
              "store rp sp\n"
              "addi sp 1 sp\n"
              "store r0 sp\n" // var r0 = root
              "in r1\n"       // in val
              "addi r0 2 r0\n"
              "store r1 r0\n" // ptr+2 = val
              "in r1\n"
              "jez r1 @skipleft\n"
              "alloc 3 r1\n" // leftptr
              "load sp r0\n"
              "store r1 r0\n" // allocate left
              "mov r1 r0\n"   // leftptr
              "call @createchildren\n"
              "@skipleft\n"
              "load sp r0\n" // restore r0 = root
              "in r1\n"
              "jez r1 @skipright\n"
              "addi r0 1 r0\n" // ptr + 1
              "alloc 3 r1\n"   // leftptr
              "store r1 r0\n"
              "load r0 r0\n" // leftptr
              "call @createchildren\n"
              "@skipright\n"
              "load sp r0\n" // restore r0 = root
              "subi sp 1 sp\n"
              "load sp rp\n"
              "subi sp 1 sp\n"
              "jmp rp\n";
  auto run = tri::Interpreter(tri::assemble(data, text));
  std::vector<int> sequence = {'h',   true,  'e',  true,  'l',   false,
                               false, true,  'l',  false, false, true,
                               'o',   false, true, '\n',  false, false};
  std::ranges::reverse(sequence);
  run.in = [&]() {
    if (sequence.empty())
      throw std::runtime_error("sequence overquery");
    auto val = sequence.back();
    sequence.pop_back();
    return val;
  };
  // run.enable_debug();
  run.execute();
  fmt::print("words: {}\n", run.mem_consumption());
  run.execute();
  run.clean();
  fmt::print("words: {}\n", run.mem_consumption());
}