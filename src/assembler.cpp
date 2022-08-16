#include "re2/stringpiece.h"
#include "tri/asm.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>

#include <re2/re2.h>
#include <unordered_map>

using namespace tri;

namespace {

void trim(std::string &s) {
  std::string substr;
  RE2::PartialMatch(s, R"(\s*(.+)\s*)", &substr);
  s = substr;
}

unsigned operandCount(InstructionType i) {
  switch (i) {
  case InstructionType::subi:
  case InstructionType::muli:
  case InstructionType::divi:
  case InstructionType::addi:
  case InstructionType::jn:
  case InstructionType::je:
    return 3;
  case InstructionType::mov:
  case InstructionType::load:
  case InstructionType::store:
  case InstructionType::alloc:
    return 2;
  case InstructionType::out:
  case InstructionType::jmp:
  case InstructionType::call:
  case InstructionType::in:
    return 1;
  case InstructionType::hlt:
  case InstructionType::noop:
    return 0;
  }
}

bool isJump(InstructionType i) {
  switch (i) {
  case InstructionType::subi:
  case InstructionType::muli:
  case InstructionType::divi:
  case InstructionType::addi:
  case InstructionType::mov:
  case InstructionType::out:
  case InstructionType::alloc:
  case InstructionType::hlt:
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
  auto operand_count = operandCount(type);
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

Literal toLit(uchar val) {
  if (val < (1 << 4)) {
    return Literal{.rotate = 0, .value = val};
  }
  throw std::logic_error("have not implemented figuring out rotations yet");
  for (auto i = 1; i < 3; ++i) {
    uchar n = val << i;
  }
}

Instruction finishInstruction(const UnfinishedInstruction &s,
                              const MappedData &data) {
  auto reg = [&](const std::string &s) {
    if (s.empty()) {
      return tri::Register::invalid;
    }
    auto reg = tri::register_table.find(s);
    if (reg != tri::register_table.end()) {
      return reg->second;
    }
    throw std::runtime_error("register not found");
  };
  auto process = [&](const std::string &s) -> Operand {
    auto reg = tri::register_table.find(s);
    if (reg != tri::register_table.end()) {
      return Operand{.reg = RegisterOperand{.operand = reg->second}};
    }
    auto global = data.map.find(s);
    if (global != data.map.end()) {
      return Operand{.literal = toLit(global->second)};
    }
    char *c = nullptr;
    auto i = std::strtol(s.c_str(), &c, 0);
    if (c != nullptr) {
      return Operand{.literal = toLit(i)};
    }
    throw std::runtime_error("Unfinished instruction has not been found");
  };
  auto maybe = [&](std::string_view s) {
    auto n = std::string(s);
    if (!s.empty())
      return process(n);
    return Operand{.literal = Literal{}};
  };
  return Instruction{.instruct = s.instruct,
                     .a = maybe(s.a),
                     .b = maybe(s.b),
                     .out = reg(s.out)};
}

std::vector<Instruction> processAsm(const char *a, MappedData &data) {
  std::string assembly = a;
  auto n = std::istringstream(assembly);
  std::vector<UnfinishedInstruction> results;
  for (std::string ln; std::getline(n, ln);) {
    trim(ln);
    auto line = re2::StringPiece(ln);
    std::string instruction;
    RE2::FindAndConsume(&line, R"((.+?)\s+)", &instruction);
    if (!instruction.empty())
      results.push_back(processInstruction(instruction, line));
  }
  results.shrink_to_fit();
  std::vector<Instruction> done;
  done.reserve(results.size() + 1);
  std::ranges::for_each(results, [&](const UnfinishedInstruction &i) {
    done.push_back(finishInstruction(i, data));
  });
  done.push_back(Instruction{.instruct = InstructionType::hlt});
  return done;
}

MappedData processData(const char *d) {
  std::string data = d;
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
        results.push_back(Word{c});
      }
      substitutions.insert({identifier, start});
    } else if (type == ".int") {
      RE2::FindAndConsume(&line, R"((.+)\s*)", &data);
      auto i = std::strtol(data.c_str(), nullptr, 0);
      results.push_back(Word{static_cast<uint32_t>(i)});
      substitutions.insert({identifier, i});
    }
  }
  return {results, substitutions};
}

} // namespace
Executable tri::assemble(const char *d, const char *a) {
  auto m = processData(d);
  auto instructions = processAsm(a, m);
  return {std::move(m.data), std::move(instructions)};
}