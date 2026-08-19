#include <cctype>
#include <cmath>
#include <expected>
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class lex_type { symbol, word, number, newline };

int label_counter = 0;

struct token {
  lex_type type;
  std::string load;
};

bool ispunct(char c) {
  if (std::ispunct(c) && c != '_')
    return true;

  return false;
}

void eval_word(std::string &current_word, std::vector<token> &source_lex,
               lex_type type) {
  if (current_word == "") {
    return;
  }

  source_lex.push_back({type, current_word});
}

std::unordered_set<std::string> two_character_symbols = {
    "+=", "-=", "*=", "/=", "&&", "||", "==", "!=", "->", "<-"};

void lex(std::vector<std::string> &source, std::vector<token> &source_lex) {
  for (int i = 0; i < source.size(); ++i) {
    std::string current_word = "";
    lex_type current_mode = lex_type::word;

    for (int j = 0; j < source[i].size(); ++j) {
      source[i][j] = std::tolower(source[i][j]);
      switch (current_mode) {
      case lex_type::word:

        if (ispunct(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";
          current_mode = lex_type::symbol;
          --j;

        } else if (std::isspace(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";

        } else if (j == source[i].size() - 1) {
          current_word += source[i][j];
          eval_word(current_word, source_lex, current_mode);
          source_lex.push_back({lex_type::newline, ""});
          current_word = "";

        } else {
          current_word += source[i][j];
        }

        break;

      case lex_type::symbol:

        if (std::isdigit(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";
          current_mode = lex_type::number;
          --j;

        } else if (std::isalpha(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";
          current_mode = lex_type::word;
          --j;

        } else if (std::isspace(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";

        } else if (j == source[i].size() - 1) {
          current_word += source[i][j];
          std::string temp_word = "";

          if (current_word.size() == 2 &&
              two_character_symbols.contains(current_word)) {
            eval_word(current_word, source_lex, current_mode);
          } else if (two_character_symbols.size() == 3 &&
                     two_character_symbols.contains(
                         current_word.substr(0, 2))) {
            std::string temp_word = current_word.substr(0, 2);
            eval_word(temp_word, source_lex, current_mode);
            temp_word = current_word[2];
            eval_word(temp_word, source_lex, current_mode);
          }

          else {
            for (char c : current_word) {
              temp_word = c;
              eval_word(temp_word, source_lex, current_mode);
            }
          }
          source_lex.push_back({lex_type::newline, ""});
          current_word = "";

        } else {
          current_word += source[i][j];
          if (current_word.size() == 2 &&
              two_character_symbols.contains(current_word)) {
            eval_word(current_word, source_lex, current_mode);
            current_word = "";
          }

          else if (current_word.size() == 2) {
            std::string temp_word = "";
            for (char c : current_word) {
              temp_word = c;
              eval_word(temp_word, source_lex, current_mode);
            }
            current_word = "";
          }
        }

        break;

      case lex_type::number:

        if (std::isalpha(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";
          current_mode = lex_type::word;
          --j;

        } else if (ispunct(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";
          current_mode = lex_type::symbol;
          --j;

        } else if (std::isspace(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";

        } else if (j == source[i].size() - 1) {
          current_word += source[i][j];
          eval_word(current_word, source_lex, current_mode);
          source_lex.push_back({lex_type::newline, ""});
          current_word = "";

        } else {
          current_word += source[i][j];
        }

        break;

      default:
        break;
      }
    }
  }
}

// all the registers i plan to use currently. might add YMMs later too
std::unordered_set<std::string> registers = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
    "xmm7", "eax",  "ebx",  "ecx",  "edx",  "edi",  "esi"};
std::unordered_set<std::string> expression_registers = {
    "xmm0_s", "xmm1_s", "xmm2_s", "xmm3_s", "xmm4_s", "xmm5_s", "xmm6_s",
    "xmm7_s", "eax",    "ebx",    "ecx",    "edx",    "edi",    "esi"};

// capitalizing those bc idk if itll get confused by the c++ type names

std::unordered_map<std::string, std::string> register_types = {
    {"xmm0", "[register_types]"},      {"xmm1", "[register_types + 4]"},
    {"xmm2", "[register_types + 8]"},  {"xmm3", "[register_types + 12]"},
    {"xmm4", "[register_types + 16]"}, {"xmm5", "[register_types + 20]"},
    {"xmm6", "[register_types + 24]"}, {"xmm7", "[register_types + 28]"},
    {"eax", "[register_types + 32]"},  {"ebx", "[register_types + 36]"},
    {"ecx", "[register_types + 40]"},  {"edx", "[register_types + 44]"},
    {"edi", "[register_types + 48]"},  {"esi", "[register_types + 52]"},

};

// standard shunting yard

int levels(const std::string &op) {
  if (op == "u+" || op == "u-")
    return 3;
  if (op == "*" || op == "/" || op == "%")
    return 2;
  if (op == "+" || op == "-")
    return 1;
  return 0;
}

std::vector<token> to_rpn(const std::span<const token> infix) {
  std::vector<token> output;
  std::stack<token> operators;

  bool operand_incoming = true;

  for (const token &t : infix) {
    if (t.type == lex_type::number || t.type == lex_type::word) {
      output.push_back(t);
      operand_incoming = false;
    } else if (t.type == lex_type::symbol) {
      if (t.load == "(") {
        operators.push(t);
        operand_incoming = true;
      } else if (t.load == ")") {
        while (!operators.empty() && operators.top().load != "(") {
          output.push_back(operators.top());
          operators.pop();
        }
        if (!operators.empty())
          operators.pop();
        operand_incoming = false;
      } else {
        std::string op_str = t.load;

        if (operand_incoming && (op_str == "+" || op_str == "-")) {
          op_str = "u" + op_str;
        }

        token current_op = {lex_type::symbol, op_str};

        while (!operators.empty() && operators.top().load != "(") {
          std::string top_op = operators.top().load;

          if ((!(op_str == "u+" || op_str == "u-") &&
               levels(op_str) <= levels(top_op)) ||
              ((op_str == "u+" || op_str == "u-") &&
               levels(op_str) < levels(top_op))) {
            output.push_back(operators.top());
            operators.pop();
          } else {
            break;
          }
        }
        operators.push(current_op);
        operand_incoming = true;
      }
    }
  }

  while (!operators.empty()) {
    output.push_back(operators.top());
    operators.pop();
  }

  return output;
}
std::expected<std::string, std::string>
expected_number(const std::vector<token> &line_tokens, int index_of_number,
                std::vector<std::string> &assembly,
                std::vector<std::string> &cleanup, std::string &type,
                int line_number) {
  // type has the string of the access of the number that has the type
  // this solves for someplace a number should be
  // it puts instructions into the assembly to solve for the number
  // or uses a calculator to get the number
  //
  // if its a runtime calculation it adds things to the assembly
  // and then it will return a register
  // it will also fill up the cleanup vector in this case,
  // to set whatever register or memory it used back to normal

  if (line_tokens[index_of_number].load == "compiler" &&
      line_tokens[index_of_number + 1].load == "." &&
      line_tokens[index_of_number + 2].load == "int_calculator") {

    if (line_tokens[index_of_number + 3].load != "{")
      return std::unexpected(
          "failed to find curly brace after compiler.int_calculator");

    int start_math = index_of_number + 4;
    int end_math = index_of_number + 4;
    while (line_tokens[end_math].load != "}") {
      ++end_math;
    }
    // end_math is now equal to the index of the ending curly brace

    std::span<const token> to_do_math_on{&line_tokens[start_math],
                                         &line_tokens[end_math]};

    std::vector<token> rpn = to_rpn(to_do_math_on);
    std::stack<int> eval_stack;
    for (const token t : rpn) {
      if (t.type == lex_type::number) {
        eval_stack.push(std::stoi(t.load));
      } else if (t.type == lex_type::word) {
        return std::unexpected(
            "why is there a variable in your compiler.calculator call");
      } else if (t.type == lex_type::symbol) {

        if (t.load == "u+" || t.load == "u-") {
          if (eval_stack.empty())
            return std::unexpected("need more operands");
          int val = eval_stack.top();
          eval_stack.pop();
          eval_stack.push(t.load == "u-" ? -val : val);
        } else {
          if (eval_stack.size() < 2)
            return std::unexpected("need more operators");

          int right = eval_stack.top();
          eval_stack.pop();
          int left = eval_stack.top();
          eval_stack.pop();

          if (t.load == "+")
            eval_stack.push(left + right);
          else if (t.load == "-")
            eval_stack.push(left - right);
          else if (t.load == "*")
            eval_stack.push(left * right);
          else if (t.load == "/") {
            if (right == 0)
              return std::unexpected("can't divide by zero");
            eval_stack.push(left / right);
          } else if (t.load == "%") {
            if (right == 0)
              return std::unexpected("can't modulus by zero");
            eval_stack.push(left % right);
          } else {
            return std::unexpected("what is this: " + t.load);
          }
        }
      }
    }

    if (eval_stack.size() != 1)
      return std::unexpected("couldn't figure it out");
    return std::to_string(eval_stack.top());
  }
  if (line_tokens[index_of_number].load == "compiler" &&
      line_tokens[index_of_number + 1].load == "." &&
      line_tokens[index_of_number + 2].load == "float_calculator") {
    if (line_tokens[index_of_number + 3].load != "{")
      return std::unexpected(
          "failed to find curly brace after compiler.float_calculator");

    int start_math = index_of_number + 4;
    int end_math = index_of_number + 4;
    while (line_tokens[end_math].load != "}") {
      ++end_math;
    }
    // end_math is now equal to the index of the ending curly brace

    std::span<const token> to_do_math_on{&line_tokens[start_math],
                                         &line_tokens[end_math]};

    std::vector<token> rpn = to_rpn(to_do_math_on);
    // float logic
    std::stack<float> eval_stack;
    for (const token t : rpn) {
      if (t.type == lex_type::number) {
        eval_stack.push(std::stof(t.load));
      } else if (t.type == lex_type::word) {
        return std::unexpected(
            "why is there a variable in your compiler.calculator call");
      } else if (t.type == lex_type::symbol) {

        if (t.load == "u+" || t.load == "u-") {
          if (eval_stack.empty())
            return std::unexpected("need more operands");
          float val = eval_stack.top();
          eval_stack.pop();
          eval_stack.push(t.load == "u-" ? -val : val);
        } else {
          if (eval_stack.size() < 2)
            return std::unexpected("need more operators");

          float right = eval_stack.top();
          eval_stack.pop();
          float left = eval_stack.top();
          eval_stack.pop();

          if (t.load == "+")
            eval_stack.push(left + right);
          else if (t.load == "-")
            eval_stack.push(left - right);
          else if (t.load == "*")
            eval_stack.push(left * right);
          else if (t.load == "/") {
            if (right == 0.0f)
              return std::unexpected("can't divide by zero");
            eval_stack.push(left / right);
          } else if (t.load == "%") {
            if (right == 0.0f)
              return std::unexpected("can't modulus by zero");
            eval_stack.push(std::fmod(left, right));
          } else {
            return std::unexpected("what is this: " + t.load);
          }
        }
      }
    }

    if (eval_stack.size() != 1)
      return std::unexpected("couldn't figure it out");

    return std::to_string(eval_stack.top());
  }

  if (line_tokens[index_of_number].load == "runtime" &&
      line_tokens[index_of_number + 1].load == "." &&
      line_tokens[index_of_number + 2].load == "calculator") {

    // another rpn evaluator but outputting assembly
    if (line_tokens[index_of_number + 3].load != "{")
      return std::unexpected(
          "failed to find curly brace after runtime.calculator");

    int start_math = index_of_number + 4;
    int end_math = index_of_number + 4;
    while (line_tokens[end_math].load != "}") {
      ++end_math;
    }
    // end_math is now equal to the index of the ending curly brace

    std::span<const token> to_do_math_on{&line_tokens[start_math],
                                         &line_tokens[end_math]};

    std::vector<token> rpn = to_rpn(to_do_math_on);

    std::stack<std::string> eval_stack;
    for (const token t : rpn) {
      if (t.type == lex_type::number) {
        eval_stack.push(t.load);
      } else if (t.type == lex_type::word) {
        std::string first_four_or_3 = "";
        if (t.load[0] == 'x')
          first_four_or_3 = t.load.substr(0, 4);
        else
          first_four_or_3 = t.load;

        // WHAT IF THE TYPE OF THE EXPRESSION IS DYNAMIC
        if (!expression_registers.contains(t.load)) {
          return t.load + " isn't a register allowed in expressions";
        } else if ((register_types[first_four_or_3] != type) &&
                   register_types[first_four_or_3] != "dynamic" &&
                   type != "dynamic") {
          return t.load + " doesn't match the type of the expression";
        } else if (register_types[first_four_or_3] ==
                   "dynamic") { // only xmms can be dynamic
          if (type == "int") {
            // dynamic_type_index *= 4; not this bc its defined in bytes

            bool eax_is_taken = register_types.contains("eax");
            if (eax_is_taken)
              assembly.push_back("push eax");
            assembly.push_back("mov al, [register_types + ");
            assembly[assembly.size() - 1] += first_four_or_3[3];
            assembly[assembly.size() - 1] += ']';

            assembly.push_back(
                "cmp al, 1"); // 1 means int 2 means float 0 means uninit
            assembly.push_back("je true_type_" + std::to_string(label_counter));
            assembly.push_back("mov bl, 'T'");
            assembly.push_back("jne crash");
            assembly.push_back("true_type_" + std::to_string(label_counter) +
                               ":");
            ++label_counter;
            assembly.push_back("pop eax");
          } else if (type == "float") {
            assembly.push_back("push eax");
            assembly.push_back("mov al, [register_types + ");
            assembly[assembly.size() - 1] += first_four_or_3[3];
            assembly[assembly.size() - 1] += ']';

            assembly.push_back(
                "cmp al, 2"); // 1 means int 2 means float 0 means uninit
            assembly.push_back("je true_type_" + std::to_string(label_counter));
            assembly.push_back("mov bl, 'T'");
            assembly.push_back("jne crash");
            assembly.push_back("true_type_" + std::to_string(label_counter) +
                               ":");
            ++label_counter;
            bool eax_is_taken = register_types.contains("eax");
            if (eax_is_taken)
              assembly.push_back("pop eax");
          }
        }

      } else if (t.type == lex_type::symbol) {

        if (t.load == "u+" || t.load == "u-") {
          if (eval_stack.empty())
            return std::unexpected("need more operands");
          std::string val = eval_stack.top();
          if (val[0] == 'x' && t.load == "u-") {
            std::string first_four_or_3 = val.substr(0, 4);
            // simd regs
            bool eax_is_taken = register_types.contains("eax");
            if (eax_is_taken)
              assembly.push_back("push eax");

            // completely revamp the type system to be in runtime partially
            // actually
            //
            // also you fucked up the symbols should not be multiple symbols
            // because {-6 + 4} wont work
            // dumbass
            // ok fixed (badly but still)
            assembly.push_back("vmovd eax, " + first_four_or_3);
            assembly.push_back("xor eax, 0x80000000");
            assembly.push_back("vmovd xmm0, eax");
            if (eax_is_taken)
              assembly.push_back("pop eax");

            if (eax_is_taken)
              cleanup.push_back("push eax");
            cleanup.push_back("vmovd eax, " + first_four_or_3);
            cleanup.push_back("xor eax, 0x80000000");
            cleanup.push_back("vmovd xmm0, eax");
            if (eax_is_taken)
              cleanup.push_back("pop eax");
          } else if (val[0] == 'e' && t.load == "u-") {
            // gen regs
            assembly.push_back("xor " + val + ", 0x80000000");
            cleanup.push_back("xor " + val + ", 0x80000000");
          } else if (t.load == "u-") {
            // number
            eval_stack.pop();
            if (type == "int") {
              val = std::to_string(-std::stoi(val));
            } else {
              val = std::to_string(-std::stof(val));
            }
            eval_stack.push(val);
          }

        } else {
          if (eval_stack.size() < 2)
            return std::unexpected("need more operators");

          std::string right = eval_stack.top();
          eval_stack.pop();
          std::string left = eval_stack.top();
          eval_stack.pop();

          if (t.load == "+") {

            // foolproof operation functions handle when the inputs are
            // general register, xmm register, or a normal number
            // it also handles cleaning up based. so like if the left
            // is a register then it needs to do push the original value of the
            foolproof_add(assembly, cleanup, left, right)

          } else if (t.load == "-")
            eval_stack.push(left - right);
          else if (t.load == "*")
            eval_stack.push(left * right);
          else if (t.load == "/") {
            if (right == 0.0f)
              return std::unexpected("can't divide by zero");
            eval_stack.push(left / right);
          } else if (t.load == "%") {
            if (right == 0.0f)
              return std::unexpected("can't modulus by zero");
            eval_stack.push(std::fmod(left, right));
          } else {
            return std::unexpected("what is this: " + t.load);
          }
        }
      }
    }

    if (eval_stack.size() != 1)
      return std::unexpected("couldn't figure it out");

    return std::to_string(eval_stack.top());
  }

  return std::unexpected("returned at the end: couldn't figure it out");
}

std::optional<std::string> evaluate(std::vector<std::vector<token>> &source,
                                    std::vector<std::string> &assembly) {
  for (int line = 0; line < source.size(); ++line) {
    for (int t = 0; t < source[line].size(); ++t) {
      if (source[line][t].load[0] == '#')
        goto next_line;

      if (registers.contains(source[line][t].load) &&
          source[line][t + 1].load == ":") {
        if (source[line][t + 2].load == "int" ||
            source[line][t + 2].load == "float") {
          register_types[source[line][t].load] = source[line][t + 2].load;
          goto next_line;
        } else
          return source[line][t].load + " is marked as " +
                 source[line][t + 2].load +
                 " instead of int or float on line " + std::to_string(line + 1);
      }

      if (source[line][t].type == lex_type::word &&
          source[line][t + 1].load == ":") {
        if (source[line][t + 2].load != "alloc")
          return "on line " + std::to_string(line + 1) + " you put " +
                 source[line][t + 2].load + "instead of alloc";

        // expected number funcition
        std::vector<std::string> cleanup = {""};
        std::string type;
        // not providing a valid type because the expected number won't access
        // it since this is compiler not runtime
        // i shd probably add a check for that actually
        if (source[line][t + 3].load == "runtime" ||
            expression_registers.contains(source[line][t + 3].load)) {
          return "on line " + std::to_string(line + 1) +
                 "an alloc can't be decided at runtime, nothing dynamic is "
                 "allowed. this is os dev so alloc as much as you want";
        }

        auto expected_number_result = expected_number(
            source[line], t + 3, assembly, cleanup, type, line + 1);
        if (!expected_number_result.has_value())
          return expected_number_result.error() + " on line " +
                 std::to_string(line + 1);

        assembly.push_back("section .bss");
        assembly.push_back(source[line][t].load + ": resb " +
                           *expected_number_result);
        assembly.push_back("section .text");

        for (std::string l : cleanup) {
          assembly.push_back(l);
        }
      }
    }
  next_line:
  }

  return {};
}

int main() {
  // test negatives again
  // std::unordered_set<std::string> registers = {
  // "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
  // "xmm7", "eax",  "ebx",  "ecx",  "edx",  "edi",  "esi"};

  std::vector<std::string> source = {
      "field: alloc compiler.float_calculator {-1 + 3 / 2}"};
  std::vector<token> source_lex_1d;

  lex(source, source_lex_1d);
  std::vector<std::vector<token>> source_lex = {{}};

  // i probably shouldve done this in the lex function. oh well
  int current_line = 0;
  for (token t : source_lex_1d) {
    if (t.type == lex_type::newline) {
      ++current_line;
      source_lex.push_back({});
    } else
      source_lex[current_line].push_back(t);
  }

  std::vector<std::string> assembly;
  // type for each register, 0 is not typed, 1 is int, 2 is float
  assembly.push_back("section .data");
  assembly.push_back("register_types: db 0, 0, 0, 0, 0, 0, 0, 0");
  assembly.push_back("section .text");
  assembly.push_back("jmp skip_crash");
  assembly.push_back("crash: ");
  assembly.push_back("mov al, 'E'");
  assembly.push_back("out 0xe9, al");
  assembly.push_back("mov al, 'R'");
  assembly.push_back("out 0xe9, al");
  assembly.push_back("mov al, 'R'");
  assembly.push_back("out 0xe9, al");
  assembly.push_back("mov al, '-'");
  assembly.push_back("out 0xe9, al");
  assembly.push_back("out 0xe9, bl"); // load bl with error code
  assembly.push_back("cli");
  assembly.push_back("hlt");
  assembly.push_back("jmp $");
  assembly.push_back("skip_crash: ");

  auto err = evaluate(source_lex, assembly);

  if (err) {
    std::cout << "Compiler error: " << *err << '\n';
    return 1;
  }

  for (std::string line : assembly) {
    std::cout << line << '\n';
  }

  return 0;
}
