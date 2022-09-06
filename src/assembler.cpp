#include "re2/stringpiece.h"
#include "tri/asm.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

#include <re2/re2.h>
#include <string_view>
#include <unordered_map>

using namespace tri;

namespace {

void trim(std::string &s) {
  std::string substr;
  RE2::PartialMatch(s, R"(\s*(.+)\s*)", &substr);
  s = substr;
}

bool isJump(InstructionType i) {
  switch (i) {
  case InstructionType::subi:
  case InstructionType::muli:
  case InstructionType::divi:
  case InstructionType::addi:
  case InstructionType::out:
  case InstructionType::alloc:
  case InstructionType::hlt:
  case InstructionType::mov:
  case InstructionType::noop:
  case InstructionType::load:
  case InstructionType::store:
  case InstructionType::in:
    return false;
  case InstructionType::jmp:
  case InstructionType::jn:
  case InstructionType::je:
  case InstructionType::call:
    return true;
  }
}
struct UnfinishedInstruction {
  InstructionType instruct;
  std::string a, b, out;
};

UnfinishedInstruction processInstruction(std::string_view i,
                                         re2::StringPiece &s) {
  auto type = tri::instruction_table.at(i);
  auto operand_count = static_cast<int>(operandCount(type));
  UnfinishedInstruction unfinished;
  unfinished.instruct = type;
  if (operand_count >= 1) {
    RE2::FindAndConsume(&s, R"((\S+))", &unfinished.a);
  }
  if (operand_count >= 2) {
    RE2::FindAndConsume(&s, R"((\S+))", &unfinished.b);
  }
  if (operand_count == 3) {
    RE2::FindAndConsume(&s, R"((\S+))", &unfinished.out);
  }
  return unfinished;
}

struct MappedData {
  std::vector<Word> data;
  std::unordered_map<std::string, size_t> map;
};
using FunctionMap = std::unordered_map<std::string, size_t>;

Literal toLit(uchar val) {
  if (val < (1 << 4)) {
    return Literal{.rotate = 0, .value = val};
  }
  throw std::logic_error("have not implemented figuring out rotations yet");
}
using Value = std::variant<Register, size_t>;
Instruction finishInstruction(const UnfinishedInstruction &s,
                              const MappedData &data) {
  auto reg = [&](std::string_view s) {
    auto reg = tri::register_table.find(s);
    if (reg != tri::register_table.end()) {
      return reg->second;
    }
    return tri::Register::invalid;
  };

  auto process = [&](const std::string &s) -> Value {
    auto reg = tri::register_table.find(s);
    if (reg != tri::register_table.end()) {
      return reg->second;
    }
    auto global = data.map.find(s);
    if (global != data.map.end()) {
      return global->second;
    }

    char *c = nullptr;
    auto i = std::strtol(s.c_str(), &c, 0);
    if (c != nullptr) {
      return size_t(i);
    }
    throw std::runtime_error("Unfinished instruction has not been found");
  };

  unsigned num = 0;
  std::array<Value, 2> vals{};
  Register r;
  if (!s.a.empty()) {
    vals[0] = process(s.a);
    ++num;
  }
  if (!s.b.empty()) {
    vals[1] = process(s.b);
    ++num;
  }
  if (!s.out.empty()) {
    r = reg(s.out);
    ++num;
  }
  auto get = [](Value v) -> Operand {
    Operand result{};
    std::visit(
        [&](auto &&val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, Register>) {
            result.reg.operand = val;
            result.reg.type = tri::Type::reg;
          } else if constexpr (std::is_same_v<T, size_t>) {
            result.literal = toLit(val);
            result.reg.type = tri::Type::lit;
          }
          static_assert(std::is_same_v<T, Register> ||
                            std::is_same_v<T, size_t>,
                        "invalid type");
        },
        v);
    return result;
  };
  auto c = operandCount(s.instruct);
  if (c == opCount::one) {
    return Instruction(s.instruct, WideOperand(get(vals[0])));
  }
  return Instruction(s.instruct,
                     Operands{.a = get(vals[0]), .b = get(vals[1]), .out = r});
}

using Code = std::vector<UnfinishedInstruction>;

std::vector<Instruction> processAsm(const char *a, MappedData &data) {
  std::string assembly = a;
  auto n = std::istringstream(assembly);
  Code results;
  for (std::string ln; std::getline(n, ln);) {
    trim(ln);
    auto line = re2::StringPiece(ln);
    std::string instruction;
    RE2::FindAndConsume(&line, R"((.+?)\s+)", &instruction);
    if (!instruction.empty())
      results.push_back(processInstruction(instruction, line));
  }
  results.push_back(UnfinishedInstruction{.instruct = InstructionType::hlt});
  results.shrink_to_fit();
  std::vector<Instruction> finished;
  finished.reserve(results.size());
  for (auto &n : results) {
    finished.push_back(finishInstruction(n, data));
  }
  return finished;
}

MappedData processData(std::string_view d) {
  auto data = std::string(d);
  auto n = std::istringstream(data);
  std::vector<Word> results;
  std::unordered_map<std::string, size_t> substitutions;
  for (std::string ln; std::getline(n, ln);) {
    trim(ln);
    auto line = re2::StringPiece(ln);
    std::string type;
    RE2::FindAndConsume(&line, R"((\..+?) )", &type);
    std::string identifier;
    RE2::FindAndConsume(&line, R"((.+?) )", &identifier);
    std::string data;
    auto start = results.size();
    if (start != 0) {
      --start;
    }
    if (type == ".ascii") {
      RE2::FindAndConsume(&line, R"('(.*)')", &data);
      for (auto it = data.begin(); it != data.end(); ++it) {
        if (it != data.end() && *it == '\\') {
          auto lookahead = it;
          ++lookahead;
          if (*lookahead == 'n') {
            *it = '\n';
            it = --data.erase(lookahead);
          }
        }
      }
      for (uint32_t c : data) {
        results.push_back(Word{Val{c}});
      }
      substitutions.insert({identifier, start});
    } else if (type == ".int") {
      RE2::FindAndConsume(&line, R"((.+)\s*)", &data);
      auto i = std::strtol(data.c_str(), nullptr, 0);
      results.push_back(Word{Val{static_cast<uint32_t>(i)}});
      substitutions.insert({identifier, i});
    }
  }
  return {results, substitutions};
}
struct UnlinkedFunction {
  std::string name;
  Code code;
};

} // namespace
namespace tri {
Executable assemble(const char *d, const char *a) {
  auto m = processData(d);
  auto instructions = processAsm(a, m);
  return {std::move(m.data), std::move(instructions)};
}
} // namespace tri