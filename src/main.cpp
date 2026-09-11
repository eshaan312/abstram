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

std::unordered_map<std::string, std::string> lower_register_half = {
    {"eax", "ax"}, {"ebx", "bx"}, {"ecx", "cx"},
    {"edx", "dx"}, {"edi", "di"}, {"esi", "si"},
};

std::unordered_map<std::string, std::string> lower_lower_register_half = {
    {"eax", "al"}, {"ebx", "bl"}, {"ecx", "cl"}, {"edx", "dl"}};
std::unordered_map<std::string, std::string> lower_upper_register_half = {
    {"eax", "ax"}, {"ebx", "bx"}, {"ecx", "cx"}, {"edx", "dx"}};

// all the registers i plan to use currently. might add YMMs later too
std::unordered_set<std::string> registers = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
    "xmm7", "eax",  "ebx",  "ecx",  "edx",  "edi",  "esi"};
std::unordered_set<std::string> expression_registers = {
    "xmm0_s", "xmm1_s", "xmm2_s", "xmm3_s", "xmm4_s", "xmm5_s", "xmm6_s",
    "xmm7_s", "eax",    "ebx",    "ecx",    "edx",    "edi",    "esi"};

// capitalizing those bc idk if itll get confused by the c++ type names

std::unordered_map<std::string, std::string> register_types = {};

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

// kinda unnecessary but you know what i like it
bool check_if_register_is_taken(std::string &register_to_check) {
  if (register_types.contains(register_to_check)) {
    return true;
  }

  return false;
}

std::optional<std::string>
find_next_avaliable_general_register(bool requires_8_bit_reg) {
  size_t sz = requires_8_bit_reg ? registers.size() - 2 : registers.size();
  for (auto r : registers) {
    if (!check_if_register_is_taken(r)) {
      return r;
    }
  }

  return std::nullopt;
}

std::expected<std::string, std::string>

// type_location input if provided should look like "[register_types + X]" or
// somethign

expected_number(const std::vector<token> &line_tokens, int index_of_number,
                std::vector<std::string> &assembly,
                std::vector<std::string> &cleanup, std::string &type,
                int line_number, std::string type_location) {
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
        if (!expression_registers.contains(t.load)) { // handled
          return t.load + " isn't a register allowed in expressions";
        } else if ((register_types[first_four_or_3] != type) &&
                   register_types[first_four_or_3] != "dynamic" &&
                   type != "dynamic") { // handled
          return t.load + " doesn't match the type of the expression";
        } else if (type == "dynamic" &&
                   register_types[first_four_or_3] != "dynamic") { // handled
          // test if the type is the static type of the input register
          if (register_types[first_four_or_3] == "int") {
            // dynamic_type_index *= 4; not this bc its defined in bytes

            auto register_to_use_for_things =
                find_next_avaliable_general_register(true);

            std::string genreg_for_things = "";
            if (!register_to_use_for_things) {
              assembly.push_back("push eax");
              genreg_for_things = "eax";
            } else {
              genreg_for_things = *register_to_use_for_things;
            }

            assembly.push_back(
                "mov " + lower_lower_register_half[genreg_for_things] + ", ");
            assembly[assembly.size() - 1] += type_location;

            assembly.push_back(
                "cmp " + lower_lower_register_half[genreg_for_things] +
                ", 1"); // 1 means int 2 means float 0 means uninit
            assembly.push_back("je true_type_" + std::to_string(label_counter));
            assembly.push_back("mov bl, 'T'");
            assembly.push_back("jne crash");
            assembly.push_back("true_type_" + std::to_string(label_counter) +
                               ":");
            ++label_counter;
            if (!register_to_use_for_things)
              assembly.push_back("pop " + genreg_for_things);
          } else if (register_types[first_four_or_3] == "float") {
            auto register_to_use_for_things =
                find_next_avaliable_general_register(true);

            std::string genreg_for_things = "";
            if (!register_to_use_for_things) {
              assembly.push_back("push eax");
              genreg_for_things = "eax";
            } else {
              genreg_for_things = *register_to_use_for_things;
            }
            assembly.push_back(
                "mov " + lower_lower_register_half[genreg_for_things] + ", ");
            assembly[assembly.size() - 1] += type_location;

            assembly.push_back(
                "cmp " + lower_lower_register_half[genreg_for_things] +
                ", 2"); // 1 means int 2 means float 0 means uninit
            assembly.push_back("je true_type_" + std::to_string(label_counter));
            assembly.push_back("mov bl, 'T'");
            assembly.push_back("jne crash");
            assembly.push_back("true_type_" + std::to_string(label_counter) +
                               ":");
            ++label_counter;
            if (!register_to_use_for_things)
              assembly.push_back("pop eax");
          }
        } else if (type == "dynamic" &&
                   register_types[first_four_or_3] == "dynamic") {
          // test if the types are the same

          auto register_to_use_for_things =
              find_next_avaliable_general_register(true);

          std::string genreg_for_things = "";
          if (!register_to_use_for_things) {
            assembly.push_back("push eax");
            genreg_for_things = "eax";
          } else {
            genreg_for_things = *register_to_use_for_things;
            register_types[genreg_for_things] =
                ""; // so that genreg_for_things2 wont be the same
          }
          auto register_to_use_for_things2 =
              find_next_avaliable_general_register(true);

          std::string genreg_for_things2 = "";
          if (!register_to_use_for_things2) {
            assembly.push_back("push ebx");
            genreg_for_things2 = "ebx";
          } else {
            genreg_for_things2 = *register_to_use_for_things2;
          }
          if (register_to_use_for_things) {
            register_types.erase(genreg_for_things); // bc this is just a temp
          }

          assembly.push_back(
              "mov " + lower_lower_register_half[genreg_for_things] + ", ");
          assembly[assembly.size() - 1] += type_location;

          assembly.push_back("mov " +
                             lower_lower_register_half[genreg_for_things2] +
                             ", [register_types + ");
          assembly[assembly.size() - 1] += first_four_or_3[3];
          assembly[assembly.size() - 1] += ']';

          assembly.push_back(
              "cmp " + lower_lower_register_half[genreg_for_things] + ", " +
              lower_lower_register_half[genreg_for_things2]); // 1 means int 2
                                                              // means float 0
                                                              // means uninit
          assembly.push_back("je true_type_" + std::to_string(label_counter));
          assembly.push_back("mov bl, 'T'");
          assembly.push_back("jne crash");
          assembly.push_back("true_type_" + std::to_string(label_counter) +
                             ":");
          ++label_counter;
          if (!register_to_use_for_things)
            assembly.push_back("pop eax");
          if (!register_to_use_for_things2)
            assembly.push_back("pop ebx");
        } else if (register_types[first_four_or_3] == "dynamic") { // handled
          // test if the input register is the same as the static type of the
          // expression

          // only xmms can be dynamic
          if (type == "int") {
            // dynamic_type_index *= 4; not this bc its defined in bytes

            auto register_to_use_for_things =
                find_next_avaliable_general_register(true);

            std::string genreg_for_things = "";
            if (!register_to_use_for_things) {
              assembly.push_back("push eax");
              genreg_for_things = "eax";
            } else {
              genreg_for_things = *register_to_use_for_things;
            }

            assembly.push_back("mov " +
                               lower_lower_register_half[genreg_for_things] +
                               ", [register_types + ");
            assembly[assembly.size() - 1] += first_four_or_3[3];
            assembly[assembly.size() - 1] += ']';

            assembly.push_back(
                "cmp " + lower_lower_register_half[genreg_for_things] +
                ", 1"); // 1 means int 2 means float 0 means uninit
            assembly.push_back("je true_type_" + std::to_string(label_counter));
            assembly.push_back("mov bl, 'T'");
            assembly.push_back("jne crash");
            assembly.push_back("true_type_" + std::to_string(label_counter) +
                               ":");
            ++label_counter;
            if (!register_to_use_for_things)
              assembly.push_back("pop eax");
          } else if (type == "float") {
            auto register_to_use_for_things =
                find_next_avaliable_general_register(true);

            std::string genreg_for_things = "";
            if (!register_to_use_for_things) {
              assembly.push_back("push eax");
              genreg_for_things = "eax";
            } else {
              genreg_for_things = *register_to_use_for_things;
            }
            assembly.push_back("mov " +
                               lower_lower_register_half[genreg_for_things] +
                               ", [register_types + ");
            assembly[assembly.size() - 1] += first_four_or_3[3];
            assembly[assembly.size() - 1] += ']';

            assembly.push_back(
                "cmp " + lower_lower_register_half[genreg_for_things] +
                ", 2"); // 1 means int 2 means float 0 means uninit
            assembly.push_back("je true_type_" + std::to_string(label_counter));
            assembly.push_back("mov bl, 'T'");
            assembly.push_back("jne crash");
            assembly.push_back("true_type_" + std::to_string(label_counter) +
                               ":");
            ++label_counter;
            bool eax_is_taken = register_types.contains("eax");
            if (!register_to_use_for_things)
              assembly.push_back("pop eax");
          }
        }
      } else if (t.type == lex_type::symbol) {

        // ok so what we have to do here is really annoying
        // so basically there are a couple things
        // the first is we check to see if the type of the expression is dynamic
        // if it is then we need two new std::vectors, one for the integer
        // operations and one for the float operations
        //
        // afterwards, we make an integer label and a float label and have it
        // jump to whatever the type of the expression is
        //
        // ok so the next thing we need to do is we need to have the main
        // register and we need to have a secondary register the main register
        // is the one the operation is being done on the second register is the
        // one that the layered operations happen in
        //
        // so for example if you have an exp like
        // xmm0_s = 56 + eax * (eax + 7) * (eax + 9)
        // then the rpn would be 56 eax eax 7 + * eax 9 + * +
        // then you would do
        // ebx = eax
        // ebx += 7
        // ebx += eax
        // push ebx
        // ebx = eax
        // ebx += 9
        // ebx *= [esp]
        // ebx += 56
        // xmm0_s = ebx
        //
        // eventually we can do the optimization to use every avaliable register
        // before spilling to the stack, but for now just using one register is
        // probably good enough

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

            // what the fuck why are we moving to xmm0???
            // also wheres the eval_stack push
            assembly.push_back("vmovd xmm0, eax");
            if (eax_is_taken)
              assembly.push_back("pop eax");

            // ok so lets do it so instead of fucking reversing the change
            // we just store the value and bring it back
            // not a big priority here tho since its not that bad
            // but in other ops do that REMEMBER ESHAAN
            if (eax_is_taken)
              cleanup.push_back("push eax");
            cleanup.push_back("vmovd eax, " + first_four_or_3);
            cleanup.push_back("xor eax, 0x80000000");

            // replace xmm0 with whatever is supposed to be here too
            // wait um if we're changing the actual value in the xmm0 register
            // how will math work if you use xmm0_s again in the same expression
            // i think we'll need a search to see what registers are in use
            // and another search to see what registers are already in use for
            // this expression wow thisll be so fun
            // also i think we need to start using stack more freely atp
            //
            //
            // ok so new goal: apply each expression to the actual input
            // register in order why rhe fuck did that take me so long to think
            // of am i dumb
            //
            //
            // wait no im even more dumb thats now how math works
            // ok so i need a temporary register to handle things in parenthesis
            // then for the temporary register if theres anything in parenthesis
            // i push it to the stack
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
            //
            //
            //
            // actually no im not making this
            foolproof_add(assembly, cleanup, left, right);

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
            source[line], t + 3, assembly, cleanup, type, line + 1, "");
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
  assembly.push_back(
      "register_types: db 0, 0, 0, 0, 0, 0, 0, 0"); // one type space for each
                                                    // xmm
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
