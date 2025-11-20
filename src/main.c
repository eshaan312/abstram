#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token_type;
struct token_struct;
char *symbols[] = { // sort by len of symbol 
    "bitcast", "sizeof", "cast", "<<=", ">>=", "..", "+=", "-=", "*=",
    "=>",      "/=",     "&=",   "^=",  "|=",  "%=", "||", "&&", "==",
    "!=",      ">=",     "<=",   "<<",  ">>",  "++", "--", "->", "!!"};
#define LOCAL_LEN(ARR) (sizeof(ARR) / sizeof(ARR[0]))
enum node_type;
struct node_struct;

enum token_type {
  WORD = 128 + LOCAL_LEN(symbols), // 128 because that's where ASCII ends
  INT,
  FLOAT
};

enum node_type {
  PROGRAM = 128 + LOCAL_LEN(symbols),
  FUNCTION_CALL,
  LITERAL,
  END,
  BRACK_PAREN,
  BRACE_PAREN
};

typedef struct token_struct {
  enum token_type type;
  char *string_argument;

  struct token_struct *token_argument;
  struct token_struct *colon_argument;
  struct token_struct *brace_argument;
  struct token_struct *brack_argument;

  size_t token_length;
  size_t colon_length;
  size_t brace_length;
  size_t brack_length;
} token;

typedef struct node_struct {
  enum node_type type;
  struct node_struct *back;
  struct node_struct *left;
  struct node_struct *right;
  struct token_struct *token_argument;
  char *name; // needed for variables with something like
              // brackets attached to it
} node;

typedef struct variable_struct {
  char *name;
  char *type;
  int memtype; // stack=0, heap=1, SSA=2
  int scope;
  int* scope_to_destroy;
} variable;

variable **variable_list;
size_t variable_list_length;

typedef struct instruction_struct {
	int id;
	size_t args_len;
	variable** args;
} instruction;

instruction **program;
size_t program_length;
size_t scope;

void append_token_c(token **code_lex, size_t *code_lex_size,
                    size_t *code_lex_index, enum token_type type,
                    char *string_argument, token *token_argument) {
  while ((*code_lex_size) < ((*code_lex_index) + 1) * sizeof(token)) {
    (*code_lex_size) *= 2;
    *code_lex = realloc(*code_lex, (*code_lex_size));
  }

  (*code_lex)[(*code_lex_index)].type = type;
  (*code_lex)[(*code_lex_index)].string_argument = string_argument;
  (*code_lex)[(*code_lex_index)].colon_argument = token_argument;
  (*code_lex)[(*code_lex_index)].colon_length = 0;
  (*code_lex_index)++;
}

void append_token_b(token **code_lex, size_t *code_lex_size,
                    size_t *code_lex_index, enum token_type type,
                    char *string_argument, token *token_argument) {
  while ((*code_lex_size) < ((*code_lex_index) + 1) * sizeof(token)) {
    (*code_lex_size) *= 2;
    *code_lex = realloc(*code_lex, (*code_lex_size));
  }

  (*code_lex)[(*code_lex_index)].type = type;
  (*code_lex)[(*code_lex_index)].string_argument = string_argument;
  (*code_lex)[(*code_lex_index)].brace_argument = token_argument;
  (*code_lex)[(*code_lex_index)].brace_length = 0;
  (*code_lex_index)++;
}

void append_token_bk(token **code_lex, size_t *code_lex_size,
                     size_t *code_lex_index, enum token_type type,
                     char *string_argument, token *token_argument) {
  while ((*code_lex_size) < ((*code_lex_index) + 1) * sizeof(token)) {
    (*code_lex_size) *= 2;
    *code_lex = realloc(*code_lex, (*code_lex_size));
  }

  (*code_lex)[(*code_lex_index)].type = type;
  (*code_lex)[(*code_lex_index)].string_argument = string_argument;
  (*code_lex)[(*code_lex_index)].brack_argument = token_argument;
  (*code_lex)[(*code_lex_index)].brack_length = 0;
  (*code_lex_index)++;
}

void append_token(token **code_lex, size_t *code_lex_size,
                  size_t *code_lex_index, enum token_type type,
                  char *string_argument, token *token_argument) {
  while ((*code_lex_size) < ((*code_lex_index) + 1) * sizeof(token)) {
    (*code_lex_size) *= 2;
    *code_lex = realloc(*code_lex, (*code_lex_size));
  }

  (*code_lex)[(*code_lex_index)].type = type;
  (*code_lex)[(*code_lex_index)].string_argument = string_argument;
  (*code_lex)[(*code_lex_index)].token_argument = token_argument;
  (*code_lex)[(*code_lex_index)].token_length = 0;

  if (type == WORD) {
    (*code_lex)[(*code_lex_index)].brace_length = 0;
    (*code_lex)[(*code_lex_index)].brack_length = 0;
    (*code_lex)[(*code_lex_index)].colon_length = 0;
  }

  (*code_lex_index)++;
}

int get_symbol(char *symbol) {
  for (int i = 0; i < LOCAL_LEN(symbols); i++) {
    if (strcmp(symbol, symbols[i]) == 0) {
      return 128 + i;
    }
  }
  return -1;
}

int number_or_dot(char symbol) {
  return (symbol >= '0' && symbol <= '9') || (symbol == '.');
}

int letter_number_or_underscore(char symbol) {
  return (symbol >= '0' && symbol <= '9') || (symbol >= 'A' && symbol <= 'Z') ||
         (symbol >= 'a' && symbol <= 'z') || (symbol == '_');
}

int letter_number_underscore_or_space(char symbol) {
  return (symbol >= '0' && symbol <= '9') || (symbol >= 'A' && symbol <= 'Z') ||
         (symbol >= 'a' && symbol <= 'z') || (symbol == '_') || (symbol == ' ');
}

token *lex(char *raw_code, size_t strlen_argv_1, size_t *code_lex_index_ptr) {
  size_t code_lex_size = 1;
  size_t code_lex_index = 0;
  token *code_lex = malloc(code_lex_size * sizeof(token *));

  int quote_mode = 0;
  size_t quote_buf_start;

  int paren_mode = 0;
  int brace_mode = 0;
  int colon_mode = 0;
  int brack_mode = 0;
  size_t paren_buf_start;
  size_t brace_buf_start;
  size_t colon_buf_start;
  size_t brack_buf_start;

  int comment_mode = 0;
  int multi_comment_mode = 0;

  for (int i = 0; i < strlen_argv_1; i++) {
    if (raw_code[i] == '\\') { // for escaping
      i++;
      continue;
    }

    if (comment_mode && raw_code[i] != '\n')
      continue;
    else if (comment_mode)
      comment_mode--;

    if (multi_comment_mode && strncmp(&raw_code[i], "*/", 2) != 0)
      continue;
    else if (multi_comment_mode)
      multi_comment_mode--;

    if (quote_mode && raw_code[i] == '"') {
      quote_mode--;

      int buf_size = i - quote_buf_start;
      char *quote_arg =
          malloc(1 + buf_size); // free every string argument in quotes
      strncpy(quote_arg, &raw_code[quote_buf_start], buf_size);
      quote_arg[buf_size] = '\0';
      append_token(&code_lex, &code_lex_size, &code_lex_index, '"', quote_arg,
                   NULL);

      continue;
    } else if (quote_mode)
      continue;

    if (raw_code[i] == '"') {
      quote_mode++;
      quote_buf_start = i + 1;

      continue;
    }

    if (paren_mode && raw_code[i] == ')') {
      paren_mode--;

      int buf_size = i - paren_buf_start;
      char *paren_arg =
          malloc(1 + buf_size); // free every string argument in parens
      strncpy(paren_arg, &raw_code[paren_buf_start], buf_size);
      paren_arg[buf_size] = '\0';

      size_t lexed_paren_index;
      token *lexed_paren =
          lex(paren_arg, strlen(paren_arg), &lexed_paren_index);
      if (code_lex_index > 0 && (code_lex[code_lex_index - 1].type == WORD ||
                                 code_lex[code_lex_index - 1].type == '(' ||
                                 code_lex[code_lex_index - 1].type == ':' ||
                                 code_lex[code_lex_index - 1].type == '[')) {
        code_lex_index--;
        append_token(&code_lex, &code_lex_size, &code_lex_index, WORD,
                     code_lex[code_lex_index].string_argument, lexed_paren);
      } else
        append_token(&code_lex, &code_lex_size, &code_lex_index, '(', NULL,
                     lexed_paren);
      code_lex[code_lex_index - 1].token_length = lexed_paren_index;
      free(paren_arg);

      continue;
    } else if (paren_mode)
      continue;

    if (!paren_mode && raw_code[i] == '(') {
      paren_mode++;
      paren_buf_start = i + 1;

      continue;
    }

    if (brace_mode && raw_code[i] == '}') {
      brace_mode--;

      int buf_size = i - brace_buf_start;
      char *paren_arg =
          malloc(1 + buf_size); // free every string argument in parens
      strncpy(paren_arg, &raw_code[brace_buf_start], buf_size);
      paren_arg[buf_size] = '\0';

      size_t lexed_paren_index;
      token *lexed_paren =
          lex(paren_arg, strlen(paren_arg), &lexed_paren_index);
      if (code_lex_index > 0 && (code_lex[code_lex_index - 1].type == WORD ||
                                 code_lex[code_lex_index - 1].type == '(' ||
                                 code_lex[code_lex_index - 1].type == ':' ||
                                 code_lex[code_lex_index - 1].type == '[')) {
        code_lex_index--;
        append_token_b(&code_lex, &code_lex_size, &code_lex_index, WORD,
                       code_lex[code_lex_index].string_argument, lexed_paren);
      } else
        append_token_b(&code_lex, &code_lex_size, &code_lex_index, '{', NULL,
                       lexed_paren);
      code_lex[code_lex_index - 1].brace_length = lexed_paren_index;
      free(paren_arg);

      continue;
    } else if (brace_mode)
      continue;

    if (!brace_mode && raw_code[i] == '{') {
      brace_mode++;
      brace_buf_start = i + 1;

      continue;
    }

    if (brack_mode && raw_code[i] == ']') {
      brack_mode--;

      int buf_size = i - brack_buf_start;
      char *paren_arg =
          malloc(1 + buf_size); // free every string argument in parens
      strncpy(paren_arg, &raw_code[brack_buf_start], buf_size);
      paren_arg[buf_size] = '\0';

      size_t lexed_paren_index;
      token *lexed_paren =
          lex(paren_arg, strlen(paren_arg), &lexed_paren_index);
      if (code_lex_index > 0 && (code_lex[code_lex_index - 1].type == WORD ||
                                 code_lex[code_lex_index - 1].type == '(' ||
                                 code_lex[code_lex_index - 1].type == ':' ||
                                 code_lex[code_lex_index - 1].type == '[')) {
        code_lex_index--;
        append_token_bk(&code_lex, &code_lex_size, &code_lex_index, WORD,
                        code_lex[code_lex_index].string_argument, lexed_paren);
      } else
        append_token_bk(&code_lex, &code_lex_size, &code_lex_index, '[', NULL,
                        lexed_paren);
      code_lex[code_lex_index - 1].brack_length = lexed_paren_index;
      free(paren_arg);

      continue;
    } else if (brack_mode)
      continue;

    if (!brack_mode && raw_code[i] == '[') { // give everything different buffer
      brack_mode++;
      brack_buf_start = i + 1;

      continue;
    }

    if (colon_mode && !letter_number_underscore_or_space(raw_code[i])) {
      colon_mode--;

      int buf_size = i - colon_buf_start;
      char *paren_arg =
          malloc(1 + buf_size); // free every string argument in parens
      strncpy(paren_arg, &raw_code[colon_buf_start], buf_size);
      paren_arg[buf_size] = '\0';

      size_t lexed_paren_index;
      token *lexed_paren =
          lex(paren_arg, strlen(paren_arg), &lexed_paren_index);
      if (code_lex_index > 0 && (code_lex[code_lex_index - 1].type == WORD ||
                                 code_lex[code_lex_index - 1].type == '(' ||
                                 code_lex[code_lex_index - 1].type == ':' ||
                                 code_lex[code_lex_index - 1].type == '[')) {
        code_lex_index--;
        append_token_c(&code_lex, &code_lex_size, &code_lex_index, WORD,
                       code_lex[code_lex_index].string_argument, lexed_paren);
      } else
        continue;

      code_lex[code_lex_index - 1].colon_length = lexed_paren_index;
      free(paren_arg);

      continue;
    } else if (colon_mode)
      continue;

    if (!colon_mode && raw_code[i] == ':'){
      colon_mode++;
      colon_buf_start = i + 1;

      continue;
    }

    if (strncmp(&raw_code[i], "//", 2) == 0) {
      comment_mode++;
      continue;
    }

    if (strncmp(&raw_code[i], "/*", 2) == 0) {
      multi_comment_mode++;
      continue;
    }

    enum token_type type = INT;
    size_t num_start = i;

    while (number_or_dot(raw_code[i])) {
      if (raw_code[i] == '.')
        type = FLOAT;
      i++;
    }

    if (i - num_start == 1 && type == FLOAT) {
      type = '.';
      num_start = i; // to avoid next conditional
    }

    if (num_start != i) { // if it's a number
      char *string_argument =
          malloc(i - num_start +
                 1); // free every string argument in ints and floats too
      strncpy(string_argument, &raw_code[num_start], i - num_start);
      string_argument[i - num_start] = '\0';

      append_token(&code_lex, &code_lex_size, &code_lex_index, type,
                   string_argument, NULL);
      i--;
      continue;
    }

    type = WORD;
    size_t word_start = i;

    while (letter_number_or_underscore(raw_code[i])) {
      i++;
    }

    if (word_start != i) { // if it's a word
      char *string_argument =
          malloc(i - word_start + 1); // free every string argument in words
      strncpy(string_argument, &raw_code[word_start], i - word_start);
      string_argument[i - word_start] = '\0';

      append_token(&code_lex, &code_lex_size, &code_lex_index, type,
                   string_argument, NULL);
      i--;
      continue;
    }

    if (raw_code[i] == ' ' || raw_code[i] == '\t')
      continue;

    int multi_char_symbol = 0;

    for (int j = 0; j < LOCAL_LEN(symbols); j++) {
      size_t strlen_symbols_j = strlen(symbols[j]);

      if (strncmp(&raw_code[i], symbols[j], strlen_symbols_j) == 0) {
        append_token(&code_lex, &code_lex_size, &code_lex_index, 128 + j, NULL,
                     NULL);
        i += strlen_symbols_j - 1;
        multi_char_symbol++;
        break;
      }
    }

    if (multi_char_symbol)
      continue;
    append_token(&code_lex, &code_lex_size, &code_lex_index, raw_code[i], NULL,
                 NULL);
  }

  (*code_lex_index_ptr) = code_lex_index;
  return code_lex;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
void tree(node *code_tree_ptr, token *code_lex, size_t code_lex_index) {
  if (code_lex_index == 0) {
    code_tree_ptr->type = END;
    return;
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id = code_lex[i].type;

    switch (id) {
    case ',':
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;

    if (code_lex[i].type == get_symbol("+="))
      id = '=';
    else if (code_lex[i].type == get_symbol("-="))
      id = '=';
    else if (code_lex[i].type == get_symbol("=>"))
      id = '=';
    else if (code_lex[i].type == get_symbol("*="))
      id = '=';
    else if (code_lex[i].type == get_symbol("/="))
      id = '=';
    else if (code_lex[i].type == get_symbol("%="))
      id = '=';
    else if (code_lex[i].type == get_symbol("<<="))
      id = '=';
    else if (code_lex[i].type == get_symbol(">>="))
      id = '=';
    else if (code_lex[i].type == get_symbol("&="))
      id = '=';
    else if (code_lex[i].type == get_symbol("^="))
      id = '=';
    else if (code_lex[i].type == get_symbol("|="))
      id = '=';
    else
      id = code_lex[i].type;

    switch (id) {
    case '=': { // USES NEWLINES FOR ASSIGNMENTS
      int restore_i = i;
      while (i != -1 && code_lex[i].type != '\n') {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      while (i != code_lex_index && code_lex[i].type != '\n') {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
    tree(code_tree_ptr->right, &code_lex[i], code_lex_index - i);
      return;
    }
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;
    if (code_lex[i].type ==
        get_symbol("||")) // redo this, just put the entire thing in the if
                          // instead of this process
      id = 'T';
    else
      id = code_lex[i].type;

    switch (id) {
    case 'T': { // PRIORITY THING
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != get_symbol("||")) {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != get_symbol("||")) {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
            if (code_lex[i].type == get_symbol("||")){
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    }
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;
    if (code_lex[i].type ==
        get_symbol("&&")) // redo this, just put the entire thing in the if
                          // instead of this process
      id = 'T';
    else
      id = code_lex[i].type;

    switch (id) {
    case 'T':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != get_symbol("&&")) {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != get_symbol("&&")) {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
                  if (code_lex[i].type == get_symbol("&&")){
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id = code_lex[i].type;

    switch (id) {
    case '|':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != '|') {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != '|') {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
                  if (code_lex[i].type == get_symbol("|")){
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id = code_lex[i].type;

    switch (id) {
    case '^':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != '^') {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != '^') {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
                  if (code_lex[i].type == get_symbol("^")){
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id = code_lex[i].type;

    switch (id) {
    case '&':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != '&') {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != '&') {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
                  if (code_lex[i].type == get_symbol("&")){
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;
    if (code_lex[i].type == get_symbol("=="))
      id = 'T';
    else if (code_lex[i].type == get_symbol("!="))
      id = 'T';
    else
      id = code_lex[i].type;

    switch (id) {
    case 'T':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != get_symbol("==") &&
             code_lex[i].type != get_symbol("!=")) {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != get_symbol("==") &&
             code_lex[i].type != get_symbol("!=")) {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
                  if (code_lex[i].type == get_symbol("==") ||
             code_lex[i].type == get_symbol("!=")){
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;
    if (code_lex[i].type == get_symbol(">="))
      id = 'T';
    else if (code_lex[i].type == get_symbol("<="))
      id = 'T';
    else if (code_lex[i].type == get_symbol(".."))
      id = 'T';
    else
      id = code_lex[i].type;

    switch (id) {
    case '>':
    case '<':
    case 'T':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != get_symbol(">=") &&
             code_lex[i].type != get_symbol("<=") && code_lex[i].type != '>' &&
             code_lex[i].type != '<' && code_lex[i].type != get_symbol("..")) {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != get_symbol(">=") &&
             code_lex[i].type != get_symbol("<=") && code_lex[i].type != '>' &&
             code_lex[i].type != '<' && code_lex[i].type != get_symbol("..")) {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
                        if (code_lex[i].type == get_symbol(">=") ||
             code_lex[i].type == get_symbol("<=") || code_lex[i].type == '>' ||
             code_lex[i].type == '<' || code_lex[i].type == get_symbol("..")) {
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;
    if (code_lex[i].type == get_symbol("<<"))
      id = 'T';
    else if (code_lex[i].type == get_symbol(">>"))
      id = 'T';
    else
      id = code_lex[i].type;

    switch (id) {
    case 'T':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != get_symbol("<<") &&
             code_lex[i].type != get_symbol(">>")) {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != get_symbol("<<") &&
             code_lex[i].type != get_symbol(">>")) {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
                              if (code_lex[i].type == get_symbol("<<") ||
             code_lex[i].type == get_symbol(">>")) {
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id = code_lex[i].type;

    switch (id) {
    case '-':
    case '+':;
      if (i == 0 ||
          (i > 0 && code_lex[i - 1].type != WORD &&
           code_lex[i - 1].type != INT && code_lex[i - 1].type != FLOAT && code_lex[i - 1].type != '(' && code_lex[i - 1].type != '[' && code_lex[i - 1].type != '{')) {
        break;
      }

      int restore_i = i;
      i--;
      // an interrupting operator would have a word, int, or float immediately
      // behind it
      while (
          i != -1 &&
          !(/*following is defining for interrupting operator*/ (
                code_lex[i].type == '+' || code_lex[i].type == '-') &&
            (i > 0 &&
             (code_lex[i - 1].type == WORD || code_lex[i - 1].type == INT ||
              code_lex[i - 1].type == FLOAT || code_lex[i - 1].type == '(' ||
              code_lex[i - 1].type == '[' || code_lex[i - 1].type == '{')))) {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (
          i != code_lex_index &&
          !(/*following is defining for interrupting operator*/ (
                code_lex[i].type == '+' || code_lex[i].type == '-') &&
            (i > 0 &&
             (code_lex[i - 1].type == WORD || code_lex[i - 1].type == INT ||
              code_lex[i - 1].type == FLOAT || code_lex[i - 1].type == '(' ||
              code_lex[i - 1].type == '[' || code_lex[i - 1].type == '{')))) {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));

      if (code_lex[i].type == '+' || code_lex[i].type == '-'){
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id = code_lex[i].type;

    switch (id) {
    case '/':
    case '*':
    case '%':;
      int restore_i = i;
      i--;
      while (i != -1 && code_lex[i].type != '*' && code_lex[i].type != '%' &&
             code_lex[i].type != '/') {
        i--;
      }

      i++;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;
      while (i != code_lex_index && code_lex[i].type != '*' &&
             code_lex[i].type != '%' && code_lex[i].type != '/') {
        i++;
      }

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      token* to_return = malloc(sizeof(token) * (1 + code_lex_index - i));
            if (code_lex[i].type == '*' ||
             code_lex[i].type == '%' || code_lex[i].type == '/') {
	      memcpy(to_return + 1, &code_lex[i], sizeof(token) * (code_lex_index - i));
	      to_return[0].type = WORD;
	      to_return[0].string_argument = malloc(4);
	      to_return[0].string_argument[3] = '\0';
	      strcpy(to_return[0].string_argument, "SSA");
      tree(code_tree_ptr->right, to_return, code_lex_index - i + 1);
      } else {
	      to_return = &code_lex[i];
      tree(code_tree_ptr->right, to_return, code_lex_index - i);
      }
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;
    if (code_lex[i].type == get_symbol("!!"))
      id = 'U';
    else if (code_lex[i].type ==
             get_symbol("cast")) // syntax will be "var_name cast: i32"
                                 // so type will be inside the cast symbol token
      id = 'T';
    else if (code_lex[i].type == get_symbol("bitcast"))
      id = 'T';
    else if (code_lex[i].type == get_symbol("sizeof"))
      id = 'U';
    else
      id = code_lex[i].type;

    switch (id) {
    case '-':
    case '+': {
      int restore_i = i + 1;
      i += 2;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = END;
      code_tree_ptr->left->right->token_argument = NULL;
      code_tree_ptr->left->right->back = code_tree_ptr->left;

      int restore_i_minus_i = 1;
      token *left_token_argument = malloc(sizeof(token));
      memcpy(left_token_argument, &code_lex[restore_i], sizeof(token));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i - 1].type + (int)10000; // means unary operator for + and -
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i], code_lex_index - i);
      return;
    }
    case '!':
    case '@':
    case 'U': {
      int restore_i = i + 1;
      i += 2;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = END;
      code_tree_ptr->left->right->token_argument = NULL;
      code_tree_ptr->left->right->back = code_tree_ptr->left;

      int restore_i_minus_i = 1;
      token *left_token_argument = malloc(sizeof(token));
      memcpy(left_token_argument, &code_lex[restore_i], sizeof(token));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i - 1].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i], code_lex_index - i);
      return;
    }

    case 'T':;
      int restore_i = i; // left
      i--;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = END;
      code_tree_ptr->left->right->token_argument = NULL;
      code_tree_ptr->left->right->back = code_tree_ptr->left;

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i], code_lex_index - i);
      return;
    default:
      break;
    }
  }

  for (int i = 0; i < code_lex_index; i++) {
    int id;
    if (code_lex[i].type == get_symbol("++"))
      id = 'T';

    else if (code_lex[i].type == get_symbol("--"))
      id = 'T';

    else
      id = code_lex[i].type;

    switch (id) {
    case '(': {
      code_tree_ptr->left->name = malloc(1);
      code_tree_ptr->left->name[1] = '\0';

      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = END;
      code_tree_ptr->left->right->token_argument = NULL;
      code_tree_ptr->left->right->back = code_tree_ptr->left;

      code_tree_ptr->left->type = (enum node_type)'(';
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, code_lex[i].token_argument,
           code_lex[i].token_length);
      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
      return;
    }

    case '{': {
      code_tree_ptr->left->name = malloc(1);
      code_tree_ptr->left->name[1] = '\0';

      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = END;
      code_tree_ptr->left->right->token_argument = NULL;
      code_tree_ptr->left->right->back = code_tree_ptr->left;

      code_tree_ptr->left->type = (enum node_type)'{';
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, code_lex[i].brace_argument,
           code_lex[i].brace_length);
      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
      return;
    }

    case '[': {
      code_tree_ptr->left->name = malloc(1);
      code_tree_ptr->left->name[1] = '\0';

      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = END;
      code_tree_ptr->left->right->token_argument = NULL;
      code_tree_ptr->left->right->back = code_tree_ptr->left;

      code_tree_ptr->left->type = (enum node_type)'[';
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, code_lex[i].brack_argument,
           code_lex[i].brack_length);
      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
      return;
    }

    case WORD:
      // ok so this is gonna handle attached parens, brackets, and braces.
      // something can have parenthesis && brackets || parenthesis && braces
      // || all indivduals by themselves
      // the evaluator can handle colons itself.
      {
        if (code_lex[i].token_length != 0 && code_lex[i].brack_length != 0) {
          code_tree_ptr->left->name =
              malloc(1 + strlen(code_lex[i].string_argument));

          strcpy(code_tree_ptr->left->name, code_lex[i].string_argument);
          code_tree_ptr->left->name[strlen(code_lex[i].string_argument)] = '\0';

          code_tree_ptr->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->type = PROGRAM;
          code_tree_ptr->left->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->right = malloc(sizeof(node));
          code_tree_ptr->left->left->back = code_tree_ptr->left;
          code_tree_ptr->left->right = malloc(sizeof(node));
          code_tree_ptr->left->right->type = PROGRAM;
          code_tree_ptr->left->right->back = code_tree_ptr->left;
          code_tree_ptr->left->right->right = malloc(sizeof(node));
          code_tree_ptr->left->right->left = malloc(sizeof(node));

          code_tree_ptr->left->type = (enum node_type)BRACK_PAREN;
          code_tree_ptr->left->back = code_tree_ptr;

          tree(code_tree_ptr->left->left, code_lex[i].token_argument,
               code_lex[i].token_length);
          tree(code_tree_ptr->left->right, code_lex[i].brack_argument,
               code_lex[i].brack_length);

          code_tree_ptr->right->back = code_tree_ptr;
          code_tree_ptr->right->left = malloc(sizeof(node));
          code_tree_ptr->right->right = malloc(sizeof(node));
          code_tree_ptr->right->type = PROGRAM;
          tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
          return;
        }

        else if (code_lex[i].token_length != 0 &&
                 code_lex[i].brace_length != 0) {
          code_tree_ptr->left->name =
              malloc(1 + strlen(code_lex[i].string_argument));

          strcpy(code_tree_ptr->left->name, code_lex[i].string_argument);
          code_tree_ptr->left->name[strlen(code_lex[i].string_argument)] = '\0';

          code_tree_ptr->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->type = PROGRAM;
          code_tree_ptr->left->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->right = malloc(sizeof(node));
          code_tree_ptr->left->left->back = code_tree_ptr->left;
          code_tree_ptr->left->right = malloc(sizeof(node));
          code_tree_ptr->left->right->type = PROGRAM;
          code_tree_ptr->left->right->back = code_tree_ptr->left;
          code_tree_ptr->left->right->right = malloc(sizeof(node));
          code_tree_ptr->left->right->left = malloc(sizeof(node));

          code_tree_ptr->left->type = (enum node_type)BRACE_PAREN;
          code_tree_ptr->left->back = code_tree_ptr;

          tree(code_tree_ptr->left->left, code_lex[i].token_argument,
               code_lex[i].token_length);
          tree(code_tree_ptr->left->right, code_lex[i].brace_argument,
               code_lex[i].brace_length);

          code_tree_ptr->right->back = code_tree_ptr;
          code_tree_ptr->right->left = malloc(sizeof(node));
          code_tree_ptr->right->right = malloc(sizeof(node));
          code_tree_ptr->right->type = PROGRAM;
          tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
          return;

        } else if (code_lex[i].token_length != 0) {
          code_tree_ptr->left->name =
              malloc(1 + strlen(code_lex[i].string_argument));

          strcpy(code_tree_ptr->left->name, code_lex[i].string_argument);
          code_tree_ptr->left->name[strlen(code_lex[i].string_argument)] = '\0';

          code_tree_ptr->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->type = PROGRAM;
          code_tree_ptr->left->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->right = malloc(sizeof(node));
          code_tree_ptr->left->left->back = code_tree_ptr->left;
          code_tree_ptr->left->right = malloc(sizeof(node));
          code_tree_ptr->left->right->type = END;
          code_tree_ptr->left->right->token_argument = NULL;
          code_tree_ptr->left->right->back = code_tree_ptr->left;

          code_tree_ptr->left->type = (enum node_type)'(';
          code_tree_ptr->left->back = code_tree_ptr;

          tree(code_tree_ptr->left->left, code_lex[i].token_argument,
               code_lex[i].token_length);
          code_tree_ptr->right->back = code_tree_ptr;
          code_tree_ptr->right->left = malloc(sizeof(node));
          code_tree_ptr->right->right = malloc(sizeof(node));
          code_tree_ptr->right->type = PROGRAM;
          tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
          return;
        } else if (code_lex[i].brack_length != 0) {
          code_tree_ptr->left->name =
              malloc(1 + strlen(code_lex[i].string_argument));

          strcpy(code_tree_ptr->left->name, code_lex[i].string_argument);
          code_tree_ptr->left->name[strlen(code_lex[i].string_argument)] = '\0';

          code_tree_ptr->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->type = PROGRAM;
          code_tree_ptr->left->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->right = malloc(sizeof(node));
          code_tree_ptr->left->left->back = code_tree_ptr->left;
          code_tree_ptr->left->right = malloc(sizeof(node));
          code_tree_ptr->left->right->type = END;
          code_tree_ptr->left->right->token_argument = NULL;
          code_tree_ptr->left->right->back = code_tree_ptr->left;

          code_tree_ptr->left->type = (enum node_type)'[';
          code_tree_ptr->left->back = code_tree_ptr;

          tree(code_tree_ptr->left->left, code_lex[i].brack_argument,
               code_lex[i].brack_length);
          code_tree_ptr->right->back = code_tree_ptr;
          code_tree_ptr->right->left = malloc(sizeof(node));
          code_tree_ptr->right->right = malloc(sizeof(node));
          code_tree_ptr->right->type = PROGRAM;
          tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
          return;
        } else if (code_lex[i].brace_length != 0) {
          code_tree_ptr->left->name =
              malloc(1 + strlen(code_lex[i].string_argument));

          strcpy(code_tree_ptr->left->name, code_lex[i].string_argument);
          code_tree_ptr->left->name[strlen(code_lex[i].string_argument)] = '\0';

          code_tree_ptr->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->type = PROGRAM;
          code_tree_ptr->left->left->left = malloc(sizeof(node));
          code_tree_ptr->left->left->right = malloc(sizeof(node));
          code_tree_ptr->left->left->back = code_tree_ptr->left;
          code_tree_ptr->left->right = malloc(sizeof(node));
          code_tree_ptr->left->right->type = END;
          code_tree_ptr->left->right->token_argument = NULL;
          code_tree_ptr->left->right->back = code_tree_ptr->left;

          code_tree_ptr->left->type = (enum node_type)'{';
          code_tree_ptr->left->back = code_tree_ptr;

          tree(code_tree_ptr->left->left, code_lex[i].brace_argument,
               code_lex[i].brace_length);
          code_tree_ptr->right->back = code_tree_ptr;
          code_tree_ptr->right->left = malloc(sizeof(node));
          code_tree_ptr->right->right = malloc(sizeof(node));
          code_tree_ptr->right->type = PROGRAM;
          tree(code_tree_ptr->right, &code_lex[i + 1], code_lex_index - i - 1);
          return;
        } else
          break;
      }

    case '.': {
      int restore_i = i;
      i--;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = PROGRAM;
      code_tree_ptr->left->right->back = code_tree_ptr->left;
      code_tree_ptr->left->right->right = malloc(sizeof(node));
      code_tree_ptr->left->right->left = malloc(sizeof(node));

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i += 2;

      int i_minus_restore_i = i - restore_i - 1;
      token *right_token_argument = malloc(sizeof(token) * (i_minus_restore_i));
      memcpy(right_token_argument, &code_lex[restore_i + 1],
             sizeof(token) * (i_minus_restore_i));

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      tree(code_tree_ptr->left->right, right_token_argument, i_minus_restore_i);

      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i], code_lex_index - i);
      return;
    }
    case 'T':;
      int restore_i = i; // left
      i--;
      code_tree_ptr->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->type = PROGRAM;
      code_tree_ptr->left->left->left = malloc(sizeof(node));
      code_tree_ptr->left->left->right = malloc(sizeof(node));
      code_tree_ptr->left->left->back = code_tree_ptr->left;
      code_tree_ptr->left->right = malloc(sizeof(node));
      code_tree_ptr->left->right->type = END;
      code_tree_ptr->left->right->token_argument = NULL;
      code_tree_ptr->left->right->back = code_tree_ptr->left;

      int restore_i_minus_i = restore_i - i;
      token *left_token_argument = malloc(sizeof(token) * restore_i_minus_i);
      memcpy(left_token_argument, &code_lex[i],
             sizeof(token) * restore_i_minus_i);

      i = restore_i;
      i++;

      code_tree_ptr->left->type = (enum node_type)code_lex[restore_i].type;
      code_tree_ptr->left->back = code_tree_ptr;

      tree(code_tree_ptr->left->left, left_token_argument, restore_i_minus_i);
      code_tree_ptr->right->back = code_tree_ptr;
      code_tree_ptr->right->left = malloc(sizeof(node));
      code_tree_ptr->right->right = malloc(sizeof(node));
      code_tree_ptr->right->type = PROGRAM;
      tree(code_tree_ptr->right, &code_lex[i], code_lex_index - i);
      return;
    default:
      break;
    }
  }

  code_tree_ptr->type = LITERAL;
  code_tree_ptr->token_argument = code_lex;
}

void print_tree(node *root, size_t tabs) {
  for (int i = 0; i < tabs; i++)
    printf("   ");
  printf("left type: %d", root->left->type);
  if (root->left->type == LITERAL){
	  printf("  -- %s\n", root->left->token_argument->string_argument);
  } else printf("\n");

  if (root->left->type != LITERAL && root->left->type != END)
    print_tree(root->left, tabs + 1);

  for (int i = 0; i < tabs; i++)
    printf("   ");
  printf("right type: %d", root->right->type);
  if (root->right->type == LITERAL){
	  printf("  -- %s\n", root->right->token_argument->string_argument);
  } else printf("\n");

  if (root->right->type != LITERAL && root->right->type != END)
    print_tree(root->right, tabs + 1);
}

void init_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '=';
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("INIT: %s = %s\n", left->name, right->name);
}

void assign_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '=';
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s\n", left->name, right->name);
}

void mul_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '*';
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: %s * %s\n", left->name, right->name);
	else 
  printf("SSA: %s * %s\n", left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void iter_mul_dynIR(variable *left) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '*' + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA: SSA * %s\n", left->name);
}

void sub_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '-';
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: %s - %s\n", left->name, right->name);
	else 
  printf("SSA: %s - %s\n", left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void iter_sub_dynIR(variable *left) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '-' + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA: SSA - %s\n", left->name);
}

void div_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '/';
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: %s / %s\n", left->name, right->name);
	else 
  printf("SSA: %s / %s\n", left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void iter_div_dynIR(variable *left) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '/' + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA: SSA / %s\n", left->name);
}

void mod_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '%';
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: %s %c %s\n", left->name, '%', right->name);
	else 
  printf("SSA: %s %c %s\n", left->name, '%', right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void iter_mod_dynIR(variable *left) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '%' + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA: SSA %c %s\n", '%', left->name);
}

void add_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '+';
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: %s + %s\n", left->name, right->name);
	else 
  printf("SSA: %s + %s\n", left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void iter_add_dynIR(variable *left) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = '+' + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA: SSA + %s\n", left->name);
}

void minus_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("-="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s - %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void mul_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("*="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s * %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void div_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("/="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s / %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void mod_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("%="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s %c %s\n", left->name, left->name, '%', right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}
void shleft_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("<<="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s << %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void shright_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol(">>="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s >> %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void or_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("|="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s | %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}


void xor_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("^="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s ^ %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void and_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("&="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s & %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void plus_equal_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("+="); 
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  printf("ASSIGN: %s = %s + %s\n", left->name, left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void logor_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("||");
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: (BOOL) %s || %s\n", left->name, right->name);
	else 
  printf("SSA: (BOOL) %s || %s\n", left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void iter_logor_dynIR(variable *left) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("||") + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA: (BOOL) SSA || %s\n",  left->name);
}


void logand_dynIR(variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("&&");
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: (BOOL) %s && %s\n", left->name, right->name);
	else 
  printf("SSA: (BOOL) %s && %s\n", left->name, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void general_dynIR(char* symbol, variable *left, variable *right) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol(symbol) ? strlen(symbol) > 1 : symbol[0];
	new_assignment->args = malloc(sizeof(variable*) * 2);
	new_assignment->args[0] = left;
	new_assignment->args[1] = right;
	new_assignment->args_len = 2;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
	if (strcmp(left->name, "SSA") != 0 && strcmp(right->name, "SSA") != 0)
  printf("NEW SSA: (BOOL) %s %s %s\n", left->name, symbol, right->name);
	else 
  printf("SSA: (BOOL) %s %s %s\n", left->name, symbol, right->name);
 /* int i = program_length - 2;
  while (i >= 0 && program[i]->id >= 1000){
  	printf("SSA: SSA + %s\n", left->name);
	i--;
  }*/
}

void iter_general_dynIR(char* symbol, variable *left, char* paren) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol(symbol) + 1000 ? strlen(symbol) > 1 : symbol[0] + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA:%s SSA %s %s\n", paren, symbol, left->name);
}

void iter_logand_dynIR(variable *left) {
	instruction* new_assignment = malloc(sizeof(instruction));
	new_assignment->id = get_symbol("&&") + 1000; // +1000 means iterative
	new_assignment->args = malloc(sizeof(variable*) * 1);
	new_assignment->args[0] = left;
	new_assignment->args_len = 1;
	program_length++;
	program = realloc(program, sizeof(instruction*) * (program_length));
	program[program_length - 1] = new_assignment;
  	printf("SSA: (BOOL) SSA && %s\n",  left->name);
}

variable *evaluate(node *root, variable* high_var, int id) {
	if (root->type == PROGRAM){
		variable* to_return = evaluate(root->left, high_var, id);
		evaluate(root->right, high_var, id);
		return to_return;
	} else if (root->type == LITERAL){
		variable* new_var = malloc(sizeof(variable));
		new_var->name = root->token_argument->string_argument;
		if (root->token_argument->type == WORD){
			for (int i = 0; i < variable_list_length; i++){
				if (high_var != NULL && strcmp(variable_list[i]->name, new_var->name) == 0 && variable_list[i]->memtype == 1 && *(variable_list[i]->scope_to_destroy) < *(high_var->scope_to_destroy)){
					variable_list[i]->scope_to_destroy = (high_var->scope_to_destroy);
				} // basically, when inserting frees at the end, check the scope_to_destroy to see when to actually free the variable. however, the actual scope is the actual scope its defined in so errors can work.
				else if (strcmp(variable_list[i]->name, new_var->name) == 0) return (variable_list[i]);
			}

			if (id == '='){
				new_var->scope = -1;
				new_var->scope_to_destroy = malloc(sizeof(int));
				*(new_var->scope_to_destroy) = scope; 
			// ADD TYPE DEFINITION AND MEMTYPE DEFINITION HERE PLEASEEEEEE
			}
		}

		return new_var;
	} else if (root->type == '='){
		variable* left = evaluate(root->left, NULL, '=');
		variable* right = evaluate(root->right, left, '='); 
		
		if (left->scope != -1){
			assign_dynIR(left, right);
		} else {
			left->scope = *(left->scope_to_destroy);
			init_dynIR(left, right);

			variable_list_length++;
			variable_list = realloc(variable_list, variable_list_length * sizeof(variable*));
			variable_list[variable_list_length - 1] = left;
		}

		return left;
	} else if (root->type == '+'){
		variable* left = evaluate(root->left, high_var, '+');
		variable* right = evaluate(root->right, high_var, '+');
		if (right == NULL) iter_add_dynIR(left);
		else add_dynIR(left, right);
		
		variable* new_var = malloc(sizeof(variable));
		new_var->name = malloc(4);
		new_var->name[3] = '\0';
		strcpy(new_var->name, "SSA");
		return new_var;
	} else if (root->type == END){
		return NULL;
	} else if (root->type == '-'){
		variable* left = evaluate(root->left, high_var, '-');
		variable* right = evaluate(root->right, high_var, '-');
		if (right == NULL) iter_sub_dynIR(left);
		else sub_dynIR(left, right);
		
		variable* new_var = malloc(sizeof(variable));
		new_var->name = malloc(4);
		new_var->name[3] = '\0';
		strcpy(new_var->name, "SSA");
		return new_var;
	} else if (root->type == '*'){
		variable* left = evaluate(root->left, high_var, '*');
		variable* right = evaluate(root->right, high_var, '*');
		if (right == NULL) iter_mul_dynIR(left);
		else mul_dynIR(left, right);
		
		variable* new_var = malloc(sizeof(variable));
		new_var->name = malloc(4);
		new_var->name[3] = '\0';
		strcpy(new_var->name, "SSA");
		return new_var;
	} else if (root->type == '/'){
		variable* left = evaluate(root->left, high_var, '/');
		variable* right = evaluate(root->right, high_var, '/');
		if (right == NULL) iter_div_dynIR(left);
		else div_dynIR(left, right);
		
		variable* new_var = malloc(sizeof(variable));
		new_var->name = malloc(4);
		new_var->name[3] = '\0';
		strcpy(new_var->name, "SSA");
		return new_var;
	} else if (root->type == '%'){
                variable* left = evaluate(root->left, high_var, '%');
                variable* right = evaluate(root->right, high_var, '%');
                if (right == NULL) iter_mod_dynIR(left);
                else mod_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("+=")){
                variable* left = evaluate(root->left, high_var, get_symbol("+="));
                variable* right = evaluate(root->right, high_var, get_symbol("+="));
                plus_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol("-=")){
                variable* left = evaluate(root->left, high_var, get_symbol("-="));
                variable* right = evaluate(root->right, high_var, get_symbol("-="));
                minus_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol("*=")){
                variable* left = evaluate(root->left, high_var, get_symbol("*="));
                variable* right = evaluate(root->right, high_var, get_symbol("*="));
                mul_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("/=")){
                variable* left = evaluate(root->left, high_var, get_symbol("/="));
                variable* right = evaluate(root->right, high_var, get_symbol("/="));
                div_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol("%=")){
                variable* left = evaluate(root->left, high_var, get_symbol("%="));
                variable* right = evaluate(root->right, high_var, get_symbol("%="));
                mod_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol("<<=")){
                variable* left = evaluate(root->left, high_var, get_symbol("<<="));
                variable* right = evaluate(root->right, high_var, get_symbol("<<="));
                shleft_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol(">>=")){
                variable* left = evaluate(root->left, high_var, get_symbol(">>="));
                variable* right = evaluate(root->right, high_var, get_symbol(">>="));
                shright_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol("&=")){
                variable* left = evaluate(root->left, high_var, get_symbol("&="));
                variable* right = evaluate(root->right, high_var, get_symbol("&="));
                and_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol("^=")){
                variable* left = evaluate(root->left, high_var, get_symbol("^="));
                variable* right = evaluate(root->right, high_var, get_symbol("^="));
                xor_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol("|=")){
                variable* left = evaluate(root->left, high_var, get_symbol("|="));
                variable* right = evaluate(root->right, high_var, get_symbol("|="));
                or_equal_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("||")){
                variable* left = evaluate(root->left, high_var, get_symbol("||"));
                variable* right = evaluate(root->right, high_var, get_symbol("||"));
                if (right == NULL) iter_logor_dynIR(left);
                else logor_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("&&")){
                variable* left = evaluate(root->left, high_var, get_symbol("&&"));
                variable* right = evaluate(root->right, high_var, get_symbol("&&"));
                if (right == NULL) iter_logand_dynIR(left);
                else logand_dynIR(left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == '|'){
                variable* left = evaluate(root->left, high_var, '|');
                variable* right = evaluate(root->right, high_var, '|');
                if (right == NULL) iter_general_dynIR("|", left, "");
                else general_dynIR("|", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == '^'){
                variable* left = evaluate(root->left, high_var, '^');
                variable* right = evaluate(root->right, high_var, '^');
                if (right == NULL) iter_general_dynIR("^", left, "");
                else general_dynIR("^", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == '&'){
                variable* left = evaluate(root->left, high_var, '&');
                variable* right = evaluate(root->right, high_var, '&');
                if (right == NULL) iter_general_dynIR("&", left, "");
                else general_dynIR("&", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == '&'){
                variable* left = evaluate(root->left, high_var, '&');
                variable* right = evaluate(root->right, high_var, '&');
                if (right == NULL) iter_general_dynIR("&", left, "");
                else general_dynIR("&", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("==")){
                variable* left = evaluate(root->left, high_var, get_symbol("=="));
                variable* right = evaluate(root->right, high_var, get_symbol("=="));
                if (right == NULL) iter_general_dynIR("==", left, " (BOOL)");
                else general_dynIR("==", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("!=")){
                variable* left = evaluate(root->left, high_var, get_symbol("!="));
                variable* right = evaluate(root->right, high_var, get_symbol("!="));
                if (right == NULL) iter_general_dynIR("!=", left, " (BOOL)");
                else general_dynIR("!=", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == get_symbol(">=")){
                variable* left = evaluate(root->left, high_var, get_symbol(">="));
                variable* right = evaluate(root->right, high_var, get_symbol(">="));
                if (right == NULL) iter_general_dynIR(">=", left, " (BOOL)");
                else general_dynIR(">=", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("<=")){
                variable* left = evaluate(root->left, high_var, get_symbol("<="));
                variable* right = evaluate(root->right, high_var, get_symbol("<="));
                if (right == NULL) iter_general_dynIR("<=", left, " (BOOL)");
                else general_dynIR("<=", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }  else if (root->type == get_symbol("..")){
                variable* left = evaluate(root->left, high_var, get_symbol(".."));
                variable* right = evaluate(root->right, high_var, get_symbol(".."));
                if (right == NULL) iter_general_dynIR("..", left, " (BOOL)");
                else general_dynIR("..", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == '>'){
                variable* left = evaluate(root->left, high_var, '>');
                variable* right = evaluate(root->right, high_var, '>');
                if (right == NULL) iter_general_dynIR(">", left, " (BOOL)");
                else general_dynIR(">", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }else if (root->type == '<'){
                variable* left = evaluate(root->left, high_var, '<');
                variable* right = evaluate(root->right, high_var, '<');
                if (right == NULL) iter_general_dynIR("<", left, " (BOOL)");
                else general_dynIR("<", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("<<")){
                variable* left = evaluate(root->left, high_var, get_symbol("<<"));
                variable* right = evaluate(root->right, high_var, get_symbol("<<"));
                if (right == NULL) iter_general_dynIR("<<", left, "");
                else general_dynIR("<<", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        }  else if (root->type == get_symbol(">>")){
                variable* left = evaluate(root->left, high_var, get_symbol(">>"));
                variable* right = evaluate(root->right, high_var, get_symbol(">>"));
                if (right == NULL) iter_general_dynIR(">>", left, "");
                else general_dynIR(">>", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } else if (root->type == get_symbol("!!")){
                variable* left = evaluate(root->left, high_var, get_symbol("!!"));
                variable* right = evaluate(root->right, high_var, get_symbol("!!"));
                if (right == NULL) iter_general_dynIR("!!", left, "");
                else general_dynIR("!!", left, right);
                     
                variable* new_var = malloc(sizeof(variable));
                new_var->name = malloc(4);
                new_var->name[3] = '\0';
                strcpy(new_var->name, "SSA");
                return new_var;
        } 







}

int main(int argc, char **argv) {
  variable_list = malloc(sizeof(variable*));
  variable_list_length = 0;

	program = malloc(sizeof(instruction*));
	program_length = 0;

  scope = 0;

  size_t strlen_argv_1 =
      strlen(argv[1]); // "argv[1]" because I don't want to have to deal with
                       // file management until I need to
  char *raw_code = argv[1];
  size_t code_lex_index;
  token *code_lex = lex(raw_code, strlen_argv_1, &code_lex_index);

  // FOR PRINTING LEX:
  printf("PRINTED LEX:\n");
  for (int i = 0; i < code_lex_index; i++) {
    printf("type: %d\n", code_lex[i].type);
    printf("string argument: %s\n", code_lex[i].string_argument);
  }

  // MOVE THIS WHOLE SETUP TO ITS OWN FUNCTION SO THAT IT CAN RECURSE
  node code_tree;
  node *root = &code_tree;
  code_tree.type = PROGRAM;
  code_tree.back = NULL;
  code_tree.left = malloc(sizeof(node));
  code_tree.right = malloc(sizeof(node));

  tree(root, code_lex, code_lex_index);

  printf("\nPRINTED TREE:\n");
  print_tree(root, 0);

  printf("\nPRINTED COMPILER IR:\n");
evaluate(root, NULL, PROGRAM);

  //    printf("%d\n%d\n", PROGRAM, code_tree_ptr->type);
  return 0;
}


