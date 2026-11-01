module;
#include <string>
#include <vector>

export module generate;
import lex;

export namespace generate {
std::vector<std::string> generate(std::vector<lex::token>)
}
