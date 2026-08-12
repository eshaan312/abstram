#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class lex_type { symbol, word, number, newline };

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

void lex(std::vector<std::string> &source, std::vector<token> &source_lex) {
  for (int i = 0; i < source.size(); ++i) {
    std::string current_word = "";
    lex_type current_mode = lex_type::word;

    for (int j = 0; j < source[i].size(); ++j) {
      source[i][j] = std::tolower(source[i][j]);
      switch (current_mode) {
      case lex_type::word:

        if (std::ispunct(source[i][j])) {
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
          eval_word(current_word, source_lex, current_mode);
          source_lex.push_back({lex_type::newline, ""});
          current_word = "";

        } else {
          current_word += source[i][j];
        }

        break;

      case lex_type::number:

        if (std::isalpha(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";
          current_mode = lex_type::word;
          --j;

        } else if (std::ispunct(source[i][j])) {
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

// capitalizing those bc idk if itll get confused by the c++ type names

std::unordered_map<std::string, std::string> register_types;

std::optional<std::string> evaluate(std::vector<std::vector<token>> &source,
                                    std::vector<std::string> &assembly) {
  for (int i = 0; i < source.size(); ++i) {
    for (int j = 0; j < source[i].size(); ++j) {
      if (registers.contains(source[i][j].load) &&
          source[i][j + 1].load == ":") {
        if ((source[i][j + 2].load == "int") &&)
      }
    }
  }

  return {};
}

int main() {
  std::vector<std::string> source = {"hello hi", "hello5=56+75 hol", "a"};
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
