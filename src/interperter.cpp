#include "tri/asm.hpp"

using namespace tri;
namespace {}
tri::Interpreter::Interpreter(Executable e)
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
      // return o.literal.;
      break;
    }
  };
  while (true) {
    auto instruction = text[ip()];
    ++ip().data;

    switch (instruction.instruct) {
    case InstructionType::hlt:
      return;
    case InstructionType::addi:

    case InstructionType::subi:
    case InstructionType::muli:
    case InstructionType::divi:
    case InstructionType::mov:
    case InstructionType::out:
    case InstructionType::jmp:
    case InstructionType::jn:
    case InstructionType::je:
    case InstructionType::call:
    case InstructionType::alloc:
    case InstructionType::noop:
      break;
    }
  }
}