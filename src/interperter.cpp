#include "fmt/format.h"
#define TRI_ENABLE_FMT_FORMATTING
#include "tri/asm.hpp"
#undef TRI_ENABLE_FMT_FORMATTING

#include <bit>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

using namespace tri;
namespace {
std::string log(Instruction i) {
  return std::string(tri::instruction_names.at(i.instruct));
}
} // namespace
tri::Interpreter::Interpreter(Executable &&e)
    : text(std::move(e.text)), stack(std::move(e.data)) {
  sp() = stack.size() - 1;
  bp() = sp();
}

void tri::Interpreter::execute() {
  // these are organized here bc I try to minimize stuff in headers
  // and it has to be in function bc of visibility rules
  // this is not ideal
  auto handleUnary = [this](tri::Instruction i) {
    auto wide = i.op.unary;
    auto eval = [&](auto o) -> Word {
      if (o.lit.type == tri::Type::lit) {
        return Val(o.lit);
      } else {
        return reg(o.reg);
      }
    };
    switch (i.instruct) {
    case InstructionType::out: {
      out(reg(wide.reg).val);
      return;
    }
    case InstructionType::in: {
      reg(wide.reg).val.is_alloc = false;
      reg(wide.reg).val = in();
      return;
    }
    case InstructionType::call:
      rp().val = ip().val;
      [[fallthrough]];
    case InstructionType::jmp: {
      ip().val = eval(wide);
      return;
    }
    default: {
      throw std::logic_error("non-unary operator incorrectly handled");
    }
    }
  };

  auto handleBinary = [this](tri::Instruction i) {
    auto eval = [&](auto o) -> Word {
      if (o.lit.type == tri::Type::lit) {
        return Val(o.lit);
      } else {
        return reg(o.reg);
      }
    };
    auto a = eval(i.op.binary.a), b = eval(i.op.binary.b);
    switch (i.instruct) {
    case InstructionType::jnz:
      if (a != tri::nullw) {
        ip().val = b;
      }
      return;
    case InstructionType::jez:
      if (a == tri::nullw) {
        ip().val = b;
      }
      return;
    case InstructionType::alloc:
      heap.push_back(tri::Interpreter::Allocation(static_cast<Val>(a)));
      reg(i.op.binary.b.reg).alloc = tri::Alloc(heap.size() - 1, 0);
      return;
    case InstructionType::mov:
      reg(i.op.binary.b.reg) = a;
      return;
    case InstructionType::load:
      reg(i.op.binary.b.reg) = deref(a);
      return;
    case InstructionType::store:
      deref(reg(i.op.binary.b.reg)) = a;
      return;
    default:
      throw std::logic_error("non-tertiary operator incorrectly handled");
    }
  };

  auto handleTertiary = [this](tri::Instruction i) {
    auto eval = [&](tri::Operand o) -> Word {
      if (o.lit.type == tri::Type::lit) {
        return Val(o.lit);
      } else {
        return reg(o.reg);
      }
    };
    auto a = eval(i.op.ternary.a), b = eval(i.op.ternary.b);
    auto out = [&]() -> Word & { return reg(i.op.ternary.out); };
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
    default:
      throw std::logic_error("non-tertiary operator incorrectly handled");
    }
  };

  auto operand = [this](Operand o) -> tri::Word {
    auto type = o.lit.type;
    switch (type) {
    case Type::reg:
      return reg(o.reg.operand);
    case Type::lit:
      return tri::Word(std::rotl(o.lit.value, o.lit.rotate));
    }
  };
  while (true) {
    auto instruction = text[ip().val];
    ip().val = ip().val + 1;
    switch (tri::operandCount(instruction.instruct)) {
    case opCount::zero:
      if (instruction.instruct == tri::InstructionType::hlt)
        return;
      break;
    case opCount::one:
      handleUnary(instruction);
      break;
    case opCount::two:
      handleBinary(instruction);
      break;
    case opCount::three:
      handleTertiary(instruction);
      break;
    default:
      throw std::logic_error("this really shouldn't happen");
    }
    if (debug) {
      fmt::print("{}: i: {} s: {} b: {} r: {} [{}]\n", log(instruction),
                 int(ip().val), int(sp().val), int(bp().val), int(rp().val),
                 fmt::join(std::span(registers).subspan(4), "|"));
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms);
    }
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
    try {
      if (ptr.val >= stack.size()) {
        if (debug) {
          std::cout << "resized stack to " << ptr.val.data << '\n';
        }
        stack.resize(ptr.val + 1);
      }
      return stack.at(ptr.val);
    } catch (const std::out_of_range &e) {
      std::cerr << "invalid stack dereference at " << ptr.val.data << " (max "
                << stack.size() << ")\n";
      std::exit(1);
    }
  } else {
    if (heap.size() > ptr.alloc.number &&
        heap[ptr.alloc.number].inRange(ptr.alloc)) {
      return heap[ptr.alloc.number].data[ptr.alloc.offset];
    }
  }
  throw std::runtime_error("invalid derefence");
}