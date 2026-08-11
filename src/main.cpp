#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

struct token {
  int id;
};

enum class lex_mode { symbol_mode, letter_mode, number_mode };

std::unordered_map<std::string, token> token_types{
    {"=", token{static_cast<int>('=')}},
};

void eval_word(std::string &current_word, std::vector<token> &source_lex) {
  if (current_word == "") {
  }
}

void lex(std::vector<std::string> &source, std::vector<token> &source_lex) {
  for (int i = 0; i < source.size(); ++i) {
    std::string current_word = "";
    lex_mode current_mode = lex_mode::letter_mode;

    for (int j = 0; j < source[i].size(); ++j) {
      switch (current_mode) {
      case lex_mode::letter_mode:

        if (std::isdigit(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";
          current_mode = lex_mode::number_mode;
          --j;

        } else if (std::ispunct(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";
          current_mode = lex_mode::symbol_mode;
          --j;

        } else if (std::isspace(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";

        } else if (j == source[i].size() - 1) {
          eval_word(current_word, source_lex);
          current_word = "";

        } else {
          current_word += source[i][j];
        }

        break;

      case lex_mode::symbol_mode:

        if (std::isdigit(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";
          current_mode = lex_mode::number_mode;
          --j;

        } else if (std::isalpha(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";
          current_mode = lex_mode::letter_mode;
          --j;

        } else if (std::isspace(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";

        } else if (j == source[i].size() - 1) {
          eval_word(current_word, source_lex);
          current_word = "";

        } else {
          current_word += source[i][j];
        }

        break;

      case lex_mode::number_mode:

        if (std::isalpha(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";
          current_mode = lex_mode::number_mode;
          --j;

        } else if (std::ispunct(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";
          current_mode = lex_mode::symbol_mode;
          --j;

        } else if (std::isspace(source[i][j])) {
          eval_word(current_word, source_lex);
          current_word = "";

        } else if (j == source[i].size() - 1) {
          eval_word(current_word, source_lex);
          current_word = "";

        } else {
          current_word += source[i][j];
        }

        break;
      }
    }
  }
}

int main() {}
