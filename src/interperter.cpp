#include "tri/asm.hpp"

#include <bit>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

#include "fmt/format.h"

using namespace tri;
namespace {
std::string_view log(Instruction i) {
  return tri::instruction_names.at(i.instruct);
}
void handleNoop(tri::Interpreter &state, tri::Instruction i) {
  switch (i.instruct) {
  case InstructionType::noop:
    return;
  default:
    throw std::runtime_error("Non-extern operator incorrectly handled");
  }
}
void handleUnary(tri::Interpreter &state, tri::Instruction i) {
  auto wide = i.op.wide;
  switch (i.instruct) {
  case InstructionType::out: {
    state.out(state.reg(wide.r).val);
    return;
  }
  case InstructionType::in: {
    state.reg(wide.r).val = state.in();
    return;
  }
  case InstructionType::call:
    state.rp().val = state.ip().val;
    [[fallthrough]];
  case InstructionType::jmp: {
    state.ip().val = wide.wide;
    return;
  }
  default: {
    throw std::logic_error("non-unary operator incorrectly handled");
  }
  }
}
void handleTertiary(tri::Interpreter &state, tri::Instruction i) {
  auto eval = [&](tri::Operand o) -> Word {
    if (o.literal.type == tri::Type::lit) {
      return Val(o.literal);
    } else {
      return state.reg(o.reg);
    }
  };
  auto a = eval(i.op.ops.a), b = eval(i.op.ops.b);
  auto out = [&]() -> Word & { return state.reg(i.op.ops.out); };
  switch (i.instruct) {
  case InstructionType::addi: {
    auto n = a + b;
    out() = n;
    return;
  }
  case InstructionType::subi:
    out() = a - b;
    return;
  case InstructionType::muli:
    out() = a * b;
    return;
  case InstructionType::divi:
    out() = a / b;
    return;
  case InstructionType::jn:
    if (a != b) {
      state.ip().val = out();
    }
    return;
  case InstructionType::je:
    if (a == b) {
      state.ip().val = out();
    }
    return;
  case InstructionType::alloc:
    state.heap.push_back(tri::Interpreter::Allocation(static_cast<Val>(a)));
    state.reg(i.op.ops.b).alloc = tri::Alloc(state.heap.size() - 1, 0);
    return;
  case InstructionType::mov:
    state.reg(i.op.ops.b) = a;
    return;
  case InstructionType::load:
    state.reg(i.op.ops.b) = state.deref(state.reg(i.op.ops.a));
    return;
  case InstructionType::store:
    state.deref(state.reg(i.op.ops.b)) = state.reg(i.op.ops.a);
    return;
  default:
    throw std::logic_error("non-tertiary operator incorrectly handled");
  }
}
} // namespace
tri::Interpreter::Interpreter(Executable &&e)
    : text(std::move(e.text)), stack(std::move(e.data)) {
  sp() = stack.size();
}

void tri::Interpreter::execute() {
  auto operand = [this](Operand o) -> tri::Word {
    auto type = o.literal.type;
    switch (type) {
    case Type::reg:
      return reg(o.reg.operand);
    case Type::lit:
      return tri::Word(std::rotl(o.literal.value, o.literal.rotate));
    }
  };
  while (true) {
    auto instruction = text[ip().val];
    ip().val = ip().val + 1;
    auto ops = instruction.count;
    switch (ops) {
    case opCount::zero:
      if (instruction.instruct == tri::InstructionType::hlt)
        return;
      break;
    case opCount::one:
      handleUnary(*this, instruction);
      break;
    case opCount::two:
      handleTertiary(*this, instruction);
      break;
    case opCount::three:
      handleTertiary(*this, instruction);
      break;
    default:
      if (ops == opCount::three) {
        std::cerr << "three\n";
      }
      throw std::logic_error("this really shouldn't happen");
    }
    if (debug)
      fmt::print("{0} callled: ip {1}\n", log(instruction), int(ip().val));
  }
}

Word tri::Interpreter::alloc(uint32_t size) {
  uint32_t begin = 0;
  if (!heap.empty()) {
    begin = heap.back().end();
  }
  if (begin > 1 << 31) {
    throw std::runtime_error("out of memory");
  }
  auto alloc = Allocation{begin};
  alloc.data.resize(size);
  heap.push_back(std::move(alloc));
  return Word(begin);
}

Word &tri::Interpreter::deref(Word ptr) {
  if (!ptr.val.is_alloc) {
    return stack.at(ptr.val);
  } else {
    if (heap.size() > ptr.alloc.number &&
        heap[ptr.alloc.number].inRange(ptr.alloc)) {
      return heap[ptr.alloc.number].data[ptr.alloc.offset];
    }
  }
  throw std::runtime_error("invalid derefence");
}