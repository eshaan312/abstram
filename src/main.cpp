#include <cctype>
#include <iostream>
#include <string>
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
      switch (current_mode) {
      case lex_type::word:

        if (std::isdigit(source[i][j])) {
          eval_word(current_word, source_lex, current_mode);
          current_word = "";
          current_mode = lex_type::number;
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

int main() {
  std::vector<std::string> source = {"hello hi", "hello=56+75 hol", "a"};
  std::vector<token> source_lex;

  lex(source, source_lex);

  for (token t : source_lex) {
    std::cout << (int)(t.type) << '\n';
    std::cout << t.load << "\n\n";
  }
}
