#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

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

std::unordered_map<InstructionType,std::string_view > const instruction_names =
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
  Register operand : 7;
  Type type : 1 = Type::reg;
};
struct Literal {
  uchar rotate : 3;
  uchar value : 4;
  Type type : 1 = Type::lit;
};

union Operand {
  Literal literal;
  RegisterOperand reg;
};

struct Instruction {
  InstructionType instruct;
  Operand a, b;
  Register out;
};

struct Word {
  uint32_t data : 30;
  bool is_alloc : 1 = false;
  bool mark : 1 = false;
  Word &operator=(uint32_t d) {
    data = d;
    return *this;
  }
  operator uint32_t() const { return data; }
};

struct Executable {
  std::vector<Word> data;
  std::vector<Instruction> text;
};

Executable assemble(const char *data, const char *assembly);

class Interpreter final {
  std::vector<Word> stack;
  std::vector<uint32_t> allocation_sizes;
  std::vector<std::vector<Word>> heap;
  std::vector<Instruction> text;
  std::array<Word, 16> registers{};
  bool debug = false;

  auto &ip() { return registers[0]; }
  auto &bp() { return registers[1]; }
  auto &sp() { return registers[2]; }
  auto &rp() { return registers[3]; }
  auto &reg(Register r) {
    if (r != Register::invalid)
      return registers[static_cast<uchar>(r)];
    else
      throw std::logic_error("invalid register accessed");
  }
  auto &reg(Operand o) {
    if (o.reg.type != Type::reg)
      throw std::runtime_error("wrong operand access type");
    return reg(o.reg.operand);
  }

public:
  Interpreter(Executable&&);
  void execute();
  size_t mem_consumption() const;
  void enable_debug() { debug = true; }
};
} // namespace tri
#undef TRI_ENUM_TABLE