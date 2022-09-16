#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace tri {
using uchar = unsigned char;
// Buffers are always allocated
enum struct InstructionType : uchar {
// this is easily the most cursed shit I've ever done
#define X(a) a,
#include "tri/detail/InstructionMacros"
#undef X
};

std::unordered_map<std::string_view, InstructionType> const instruction_table =
    {
#define X(a) {#a, InstructionType::a},
#include "tri/detail/InstructionMacros"
#undef X
};

std::unordered_map<InstructionType, std::string_view> const instruction_names =
    {
#define X(a) {InstructionType::a, #a},
#include "tri/detail/InstructionMacros"
#undef X
};

enum struct Register : uchar {
#define X(a) a,
#include "tri/detail/RegisterMacros"
#undef X
};

std::unordered_map<std::string_view, Register> const register_table = {
#define X(a) {#a, Register::a},
#include "tri/detail/RegisterMacros"
#undef X
};

enum struct Type : uchar { reg, lit };
struct RegisterOperand {
  Type type : 1 = Type::reg;
  Register operand : 7;
  operator Register() const noexcept { return operand; }
};
struct Literal {
  Type type : 1 = Type::lit;
  uchar rotate : 3;
  uchar value : 4;
  constexpr Literal() = default;
  constexpr Literal(uint32_t val) { convert(val); }
  constexpr Literal &operator=(uint32_t val) {
    convert(val);
    return *this;
  }
  operator uint32_t() const noexcept { return std::rotr(value, rotate); }

private:
  void convert(uint32_t val) {
    if (val < (1 << 4)) {
      value = val;
      rotate = 0;
    } else
      throw std::logic_error("have not implemented figuring out rotations yet");
  }
};
struct [[gnu::packed]] WideLiteral {
  Type type : 1 = Type::lit;
  uint16_t value : 15;
  uint8_t rotate;
  WideLiteral() = default;
  constexpr WideLiteral(Literal l) noexcept
      : value(l.value), rotate(l.rotate) {}
  constexpr WideLiteral(uint32_t val) { convert(val); }
  operator uint32_t() const noexcept { return std::rotr(value, rotate); }

private:
  void convert(uint32_t val) {
    if (val < std::numeric_limits<decltype(value)>::max()) {
      value = val;
      rotate = 0;
    } else
      throw std::logic_error("have not implemented figuring out rotations yet");
  }
};
static_assert(sizeof(WideLiteral) == 3, "WideLiteral is not packed");
struct [[gnu::packed]] SemiLiteral {
  Type type : 1 = Type::lit;
  uint16_t value : 15;
  SemiLiteral() = default;
  constexpr SemiLiteral(Literal l) noexcept : value(l) {}
  constexpr SemiLiteral(uint32_t val) { convert(val); }
  operator uint32_t() const noexcept { return value; }

private:
  void convert(uint32_t val) {
    if (val < (1 << 15)) {
      value = val;
    } else
      throw std::logic_error("literal is out of bounds");
  }
};
union Operand {
  Literal lit;
  RegisterOperand reg;
};

union WideOperand {
  WideLiteral lit;
  RegisterOperand reg;
  WideOperand() : lit() {}
  constexpr WideOperand(Operand o) {
    if (o.lit.type == Type::lit) {
      lit = o.lit;
    } else {
      reg = o.reg;
    }
  }
};

union SemiwideOperand {
  SemiLiteral lit;
  RegisterOperand reg;
  SemiwideOperand() : lit(0) {}
  constexpr SemiwideOperand(Operand o) {
    if (o.lit.type == Type::lit) {
      lit = o.lit;
    } else {
      reg = o.reg;
    }
  }
};

struct TriOps {
  Operand a, b;
  Register out : 4;
};

struct [[gnu::packed]] BiOps {
  Operand a;
  SemiwideOperand b;
};

union Op {
  WideOperand unary;
  BiOps binary;
  TriOps ternary;
};

enum struct opCount : unsigned char { zero, one, two, three };

inline opCount operandCount(InstructionType i) {
  using enum opCount;
  switch (i) {
  case InstructionType::subi:
  case InstructionType::muli:
  case InstructionType::divi:
  case InstructionType::addi:
    return three;
  case InstructionType::mov:
  case InstructionType::load:
  case InstructionType::store:
  case InstructionType::alloc:
  case InstructionType::jnz:
  case InstructionType::jez:
    return two;
  case InstructionType::out:
  case InstructionType::jmp:
  case InstructionType::call:
  case InstructionType::in:
    return one;
  case InstructionType::hlt:
  case InstructionType::noop:
    return zero;
  }
}
inline opCount count(unsigned i) {
  using enum opCount;
  if (i == 0)
    return zero;
  if (i == 1)
    return one;
  if (i == 2)
    return two;
  if (i == 3)
    return three;
  throw std::logic_error("invalid opcount");
}
struct Instruction {
  InstructionType instruct : 6;
  int reserved : 2;
  Op op;
  Instruction(InstructionType type, TriOps o)
      : instruct(type), op{.ternary = o} {
    if (operandCount(type) != opCount::three) {
      throw std::logic_error("non-unary operator is given too many operands.");
    }
  }
  Instruction(InstructionType type, BiOps o) : instruct(type), op{.binary = o} {
    if (operandCount(type) != opCount::two) {
      throw std::logic_error("non-unary operator is given too many operands.");
    }
  }
  Instruction(InstructionType type, WideOperand o)
      : instruct(type), op{.unary = o} {
    if (operandCount(type) != opCount::one) {
      throw std::logic_error("uninary operator is given too many operands.");
    }
  }
  Instruction(InstructionType type) : instruct(type), op{} {
    if (operandCount(type) != opCount::zero) {
      throw std::logic_error("uninary operator is given too many operands.");
    }
  }
};

struct Val {
  Val() = default;
  Val(uint32_t i) : data(i) {}
  uint32_t data : 31;
  bool is_alloc : 1 = false;
  Val &operator=(uint32_t d) {
    data = d;
    return *this;
  }
  operator uint32_t() const { return data; }
};
inline bool operator==(Val l, Val r) {
  return l.is_alloc == r.is_alloc && l.data == r.data;
}

struct Alloc {
  Alloc() = default;
  Alloc(uint16_t num, uint16_t offset) : number(num), offset(offset) {}
  uint16_t number : 16;
  uint16_t offset : 15;
  bool is_alloc : 1 = true;
};
struct Nullword_t {};
constexpr inline static Nullword_t nullw;
union Word {
  Alloc alloc;
  Val val;
  Word() : val(0) {}
  Word(Alloc a) : alloc(a) {}
  Word(Val v) : val(v) {}
  Word &operator=(Word other) {
    if (other.alloc.is_alloc)
      alloc = other.alloc;
    else
      val = other.val;
    return *this;
  }

  Word &operator=(Alloc other) {
    alloc = other;
    return *this;
  }
  Word &operator=(Val other) {
    val = other;
    return *this;
  }
  operator Alloc() {
    if (!alloc.is_alloc)
      throw std::runtime_error("invalid cast: val->alloc");
    return alloc;
  }
  operator Val() {
    if (alloc.is_alloc)
      throw std::runtime_error("invalid cast: val->alloc");
    return val;
  }
};

inline bool operator==(Word w, Nullword_t n) {
  if (w.alloc.is_alloc) {
    return false;
  } else {
    return w.val.data == 0;
  }
}
inline bool operator!=(Word w, Nullword_t n) { return !(w == n); }

inline bool operator==(Word l, Word r) {
  return l.alloc.is_alloc == r.alloc.is_alloc && l.val == r.val;
}
template <typename Op> Word WordOps(Op op, Word left, Word right) {
  if (left.alloc.is_alloc != right.val.is_alloc) {
    if (left.alloc.is_alloc)
      return Word(Alloc(left.alloc.number, op(left.alloc.offset, right.val)));
    else
      return Word(Alloc(right.alloc.number, op(right.alloc.offset, left.val)));
  } else if (!left.val.is_alloc && !right.val.is_alloc) {
    return Word(Val(op(left.val, right.val)));
  }
  throw std::runtime_error("tried operating on two alloc operands");
}
// the int cast exists because of ambiguous addition operators
inline auto operator+(Word l, Word r) {
  return WordOps([](auto l, auto r) { return int(l) + r; }, l, r);
}
inline auto operator-(Word left, Word right) {
  if (left.alloc.is_alloc != right.val.is_alloc) {
    if (left.alloc.is_alloc)
      return Word(Alloc(left.alloc.number, left.alloc.offset - right.val));
    else
      return Word(Alloc(right.alloc.number, right.alloc.offset - left.val));
  } else if (!left.val.is_alloc && !right.val.is_alloc) {
    return Word(Val(unsigned(left.val) - right.val));
  }
  return Word(Val(left.alloc.offset - right.alloc.offset));
}
template <typename Op> Word WordValOps(Op op, Word left, Word right) {
  if (!left.val.is_alloc && !right.val.is_alloc) {
    return Word(Val(op(left.val, right.val)));
  }
  throw std::runtime_error("illegal operation with alloc operand");
}
inline auto operator*(Word l, Word r) {
  return WordValOps([](auto l, auto r) { return int(l) + r; }, l, r);
}
inline auto operator/(Word l, Word r) {
  return WordValOps([](auto l, auto r) { return int(l) - r; }, l, r);
}

struct Executable {
  std::vector<Word> data;
  std::vector<Instruction> text;
};

Executable assemble(const char *data, const char *assembly);

struct Interpreter final {
  struct Allocation {
    uint32_t begin() const noexcept { return 0; }
    uint32_t end() const noexcept { return data.size(); }
    bool inRange(Alloc ptr) const noexcept { return ptr.offset < end(); }
    Word &deref(uint32_t ptr) { return data.at(ptr); }
    std::vector<Word> data;
    Allocation(uint32_t size) { data.resize(size); }
  };

  std::vector<Word> stack;
  std::vector<Allocation> heap;
  std::vector<Instruction> text;
  std::array<Word, 16> registers{};
  bool debug = false;

  auto &ip() { return registers[0]; }
  auto &bp() { return registers[1]; }
  auto &sp() { return registers[2]; }
  auto &rp() { return registers[3]; }
  auto &reg(Register r) {
    if (r != Register::invalid)
      return registers[static_cast<uchar>(r) - 1];
    else
      throw std::logic_error("invalid register accessed");
  }

  auto &reg(Operand o) {
    if (o.reg.type != Type::reg)
      throw std::runtime_error("wrong operand access type");
    return reg(o.reg.operand);
  }

  Word alloc(uint32_t size);
  Word &deref(Word ptr);

public:
  std::function<uint32_t()> in;
  std::function<void(uint32_t)> out = [](uint32_t i) { std::cout << char(i); };

  Interpreter(Executable &&);
  void execute();
  size_t mem_consumption() const;
  void enable_debug() { debug = true; }
};
} // namespace tri

#ifdef TRI_ENABLE_FMT_FORMATTING

template <> struct fmt::formatter<tri::Word> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(tri::Word w, FormatContext &ctx) const -> decltype(ctx.out()) {
    // I have to int cast this stuff bc fmt for some reason likes taking
    // const refs of stuff
    if (w.alloc.is_alloc)
      return fmt::format_to(ctx.out(), "({}, {})", int(w.alloc.number),
                            int(w.alloc.offset));
    return fmt::format_to(ctx.out(), "{0:x}", int(w.val.data));
  }
};
#endif