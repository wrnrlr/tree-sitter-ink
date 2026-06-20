#include "tree_sitter/parser.h"

enum TokenType { MINUS, BOOL, BOOLS, INTS, FLOATS, SYMBOLS, STRING };

void *tree_sitter_ink_external_scanner_create() { return 0; }
void tree_sitter_ink_external_scanner_destroy(void *p) {}
unsigned tree_sitter_ink_external_scanner_serialize(void *payload, const char *buffer) { return 0; }
void tree_sitter_ink_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {}

static void adv(TSLexer *l) { l->advance(l, 0); }
static int dig(int c) { return c >= '0' && c <= '9'; }
static int xdig(int c) { return dig(c) || (c>='a'&&c<='f') || (c>='A'&&c<='F'); }

/* A '"' inside a string closes it only when the next char is a "close char".
   This implements the K string-doubling convention. */
static int is_string_close_char(int c) {
  if (c == 0 || c == '\n' || c == '\t' || c == ' ' || c == ';') return 1;
  if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}') return 1;
  if (c == '`' || c == '\\') return 1;
  if (c >= '0' && c <= '9') return 1;
  const char *ops = "%!&+*|<>=~,^#_$?@./-:";
  for (const char *p = ops; *p; p++) if (c == (int)*p) return 1;
  return 0;
}

static int scan_string(TSLexer *l) {
  adv(l);  /* consume opening '"' */
  while (1) {
    if (l->lookahead == 0 || l->lookahead == '\n') break;
    if (l->lookahead == '"') {
      adv(l);
      if (is_string_close_char(l->lookahead)) break;
    } else if (l->lookahead == '\\') {
      adv(l);
      if (l->lookahead != 0) adv(l);
    } else {
      adv(l);
    }
  }
  l->mark_end(l);
  l->result_symbol = STRING;
  return 1;
}

/* After initial digit(s), scan suffix to determine type.
   Returns: 0=error, 1=int, 2=float. */
static int finish_suffix(TSLexer *l) {
  while (dig(l->lookahead)) adv(l);

  if (l->lookahead == '.') {
    adv(l);
    if (!dig(l->lookahead)) return 2; /* trailing dot → float */
    while (dig(l->lookahead)) adv(l);
    if (l->lookahead == 'e' || l->lookahead == 'E') {
      adv(l);
      if (l->lookahead == '+' || l->lookahead == '-') adv(l);
      if (!dig(l->lookahead)) return 2;
      while (dig(l->lookahead)) adv(l);
    }
    return 2;
  }
  if (l->lookahead == 'e' || l->lookahead == 'E') {
    adv(l);
    if (l->lookahead == '+' || l->lookahead == '-') adv(l);
    if (!dig(l->lookahead)) return 2;
    while (dig(l->lookahead)) adv(l);
    return 2;
  }
  return 1;
}

/* Scan a number from current position.
   Returns: 0=not a number, 1=int, 2=float. */
static int scan_num(TSLexer *l) {
  int neg = (l->lookahead == '-');
  if (neg) adv(l);

  if (l->lookahead == '.') {
    adv(l);
    if (!dig(l->lookahead)) return 0;
    while (dig(l->lookahead)) adv(l);
    if (l->lookahead == 'e' || l->lookahead == 'E') {
      adv(l);
      if (l->lookahead == '+' || l->lookahead == '-') adv(l);
      if (!dig(l->lookahead)) return 0;
      while (dig(l->lookahead)) adv(l);
    }
    return 2;
  }

  if (!dig(l->lookahead)) return 0;

  if (l->lookahead == '0') {
    adv(l);
    if (l->lookahead == 'x' || l->lookahead == 'X') {
      adv(l);
      if (!xdig(l->lookahead)) return 0;
      while (xdig(l->lookahead)) adv(l);
      return 1;
    }
    if (l->lookahead == 'N' || l->lookahead == 'W') { adv(l); return 1; }
    if (l->lookahead == 'n' || l->lookahead == 'w') { adv(l); return 2; }
  } else {
    adv(l);
  }

  return finish_suffix(l);
}

/* Scan symbol content after the leading backtick.
   Follows Zig lexer rules:
   1. '"' → quoted symbol
   2. op char → consume all consecutive op chars (greedy)
   3. else → consume all consecutive alnum+dot chars */
static void scan_sym_content(TSLexer *l) {
  if (l->lookahead == '"') {
    adv(l);
    while (l->lookahead && l->lookahead != '"' && l->lookahead != '\n') adv(l);
    if (l->lookahead == '"') adv(l);
    return;
  }
  const char *ops = "%!&+*|<>=~,^#_$?@/-";
  int is_op = 0;
  for (const char *p = ops; *p; p++) {
    if (l->lookahead == (int)*p) { is_op = 1; break; }
  }
  if (is_op) {
    while (1) {
      int found = 0;
      for (const char *p = ops; *p; p++) {
        if (l->lookahead == (int)*p) { found = 1; break; }
      }
      if (!found) break;
      adv(l);
    }
    return;
  }
  while (
    (l->lookahead >= 'a' && l->lookahead <= 'z') ||
    (l->lookahead >= 'A' && l->lookahead <= 'Z') ||
    (l->lookahead >= '0' && l->lookahead <= '9') ||
    l->lookahead == '.'
  ) adv(l);
}

/* Try to form a strand of 2+ numbers separated by whitespace. */
static int strand_loop(TSLexer *l, int has_float,
                       int want_ints, int want_floats) {
  l->mark_end(l);
  int count = 1;
  while (1) {
    if (l->lookahead != ' ' && l->lookahead != '\t') break;
    while (l->lookahead == ' ' || l->lookahead == '\t') adv(l);
    int nxt = scan_num(l);
    if (nxt == 0) break;
    if (nxt == 2) has_float = 1;
    count++;
    l->mark_end(l);
  }
  if (count < 2) return 0;
  if (has_float && want_floats) { l->result_symbol = FLOATS; return 1; }
  if (!has_float && want_ints)  { l->result_symbol = INTS;   return 1; }
  return 0;
}

int tree_sitter_ink_external_scanner_scan(void *payload, TSLexer *lexer, const char *valid_symbols) {

  /* STRING: direct match when at '"' */
  if (valid_symbols[STRING] && lexer->lookahead == '"') {
    return scan_string(lexer);
  }

  /* MINUS: '-' that starts a negative number in transit position */
  if (valid_symbols[MINUS] && lexer->lookahead == '-') {
    lexer->advance(lexer, 0);
    lexer->mark_end(lexer);
    if (lexer->lookahead == '.') lexer->advance(lexer, 0);
    if ('0' > lexer->lookahead || lexer->lookahead > '9') return 0;
    lexer->result_symbol = MINUS;
    return 1;
  }

  /* Skip leading whitespace for strand/symbol/bool detection */
  int need_skip = valid_symbols[STRING]  || valid_symbols[SYMBOLS] ||
                  valid_symbols[BOOL]    || valid_symbols[BOOLS]   ||
                  valid_symbols[INTS]    || valid_symbols[FLOATS];
  if (need_skip && (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t')
      lexer->advance(lexer, 1);
  }

  /* STRING: match after whitespace skip */
  if (valid_symbols[STRING] && lexer->lookahead == '"') {
    return scan_string(lexer);
  }

  /* SYMBOLS: two or more symbols */
  if (valid_symbols[SYMBOLS] && lexer->lookahead == '`') {
    adv(lexer);
    scan_sym_content(lexer);
    lexer->mark_end(lexer);
    int count = 1;
    while (1) {
      while (lexer->lookahead == ' ' || lexer->lookahead == '\t') adv(lexer);
      if (lexer->lookahead != '`') break;
      adv(lexer);
      scan_sym_content(lexer);
      count++;
      lexer->mark_end(lexer);
    }
    if (count < 2) return 0;
    lexer->result_symbol = SYMBOLS;
    return 1;
  }

  int need_bool = (valid_symbols[BOOL] || valid_symbols[BOOLS]);
  int need_num  = (valid_symbols[INTS] || valid_symbols[FLOATS]);

  /* Unified handler for BOOL, BOOLS, INTS, FLOATS.
     '0' and '1' can start both bools ([01]+b) and ints. */
  if ((lexer->lookahead == '0' || lexer->lookahead == '1') && (need_bool || need_num)) {
    int first_digit = lexer->lookahead;
    int nbits = 0;
    while (lexer->lookahead == '0' || lexer->lookahead == '1') {
      adv(lexer);
      nbits++;
    }
    if (need_bool && lexer->lookahead == 'b') {
      adv(lexer);
      lexer->mark_end(lexer);
      if (nbits == 1 && valid_symbols[BOOL]) { lexer->result_symbol = BOOL; return 1; }
      if (nbits >= 2 && valid_symbols[BOOLS]) { lexer->result_symbol = BOOLS; return 1; }
      return 0;
    }
    if (!need_num) return 0;
    int first = 1;
    if (nbits == 1 && first_digit == '0') {
      if (lexer->lookahead == 'N' || lexer->lookahead == 'W') { adv(lexer); first = 1; }
      else if (lexer->lookahead == 'n' || lexer->lookahead == 'w') { adv(lexer); first = 2; }
      else if (lexer->lookahead == 'x' || lexer->lookahead == 'X') {
        adv(lexer);
        while (xdig(lexer->lookahead)) adv(lexer);
        first = 1;
      } else {
        first = finish_suffix(lexer);
      }
    } else {
      first = finish_suffix(lexer);
    }
    if (first == 0) return 0;
    return strand_loop(lexer, first==2,
                       valid_symbols[INTS], valid_symbols[FLOATS]);
  }

  /* Numeric strand starting with 2-9, '-', or '.' */
  if (need_num && (dig(lexer->lookahead) || lexer->lookahead == '-' || lexer->lookahead == '.')) {
    int first = scan_num(lexer);
    if (first == 0) return 0;
    return strand_loop(lexer, first==2,
                       valid_symbols[INTS], valid_symbols[FLOATS]);
  }

  return 0;
}
