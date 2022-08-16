#include "tri/asm.hpp"

#include <bit>
#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

#include "fmt/format.h"

using namespace tri;
namespace {
std::string_view log(Instruction i) {
  return tri::instruction_names.at(i.instruct);
}
} // namespace
tri::Interpreter::Interpreter(Executable &&e)
    : text(std::move(e.text)), stack(std::move(e.data)) {
  sp() = stack.size();
}

void tri::Interpreter::execute() {
  auto operand = [this](Operand o) {
    auto type = o.literal.type;
    switch (type) {
    case Type::reg:
      return reg(o.reg.operand);
    case Type::lit:
      return tri::Word{.data = std::rotl(o.literal.value, o.literal.rotate)};
    }
  };
  while (ip() < text.size()) {
    auto instruction = text[ip()];
    ip() = ip().data + 1;
    auto a = operand(instruction.a), b = operand(instruction.b);
    if (debug)
      fmt::print("{0} callled: ip {1}\n", log(instruction), int(ip().data));

    switch (instruction.instruct) {
    case InstructionType::hlt:
      return;
    case InstructionType::addi: {
      auto &out = reg(instruction.out);
      out = a + b;
      break;
    }
    case InstructionType::subi: {
      auto &out = reg(instruction.out);
      out = a - b;
      break;
    }
    case InstructionType::muli: {
      auto &out = reg(instruction.out);
      out = a * b;
      break;
    }
    case InstructionType::divi: {
      auto &out = reg(instruction.out);
      out = a / b;
      break;
    }
    case InstructionType::mov: {
      reg(instruction.b) = a;
      break;
    }
    case InstructionType::out: {
      std::cout << char(stack.at(a));
      break;
    }
    case InstructionType::jmp: {
      ip() = a;
      break;
    }
    case InstructionType::jn: {
      auto &label = reg(instruction.out);
      if (a != b)
        ip() = label;
      break;
    }
    case InstructionType::je: {
      auto &label = reg(instruction.out);
      if (a == b)
        ip() = label;
      break;
    }
    case InstructionType::call: {
      rp() = ip();
      ip() = a;
      break;
    }
    case InstructionType::alloc: {
      return; // todo implement later
    }
    case InstructionType::noop:
      break;
    }
    using namespace std::chrono_literals;
  }
}