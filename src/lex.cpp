module;
#include <string>

export module lex;

export namespace lex {
enum class type { word, number, symbol };

struct token {
  type type;
  std::string value;
};
} // namespace lex
