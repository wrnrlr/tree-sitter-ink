#include "tree_sitter/parser.h"

enum TokenType { MINUS, BOOL, BOOLS, INTS, FLOATS, SYMBOLS, STRING, DATE, DATES, TIME, TIMES };

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

/* Scan symbol content after the leading backtick. */
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

/* Try to scan a date value: exactly DDDD.DD.DD */
static int scan_date_val(TSLexer *l) {
  int d = 0;
  while (dig(l->lookahead)) { adv(l); d++; }
  if (d != 4 || l->lookahead != '.') return 0;
  adv(l);
  d = 0;
  while (dig(l->lookahead)) { adv(l); d++; }
  if (d != 2 || l->lookahead != '.') return 0;
  adv(l);
  d = 0;
  while (dig(l->lookahead)) { adv(l); d++; }
  return (d == 2 && !dig(l->lookahead));
}

/* Try to scan a time value: DD:DD:DD[.D+] */
static int scan_time_val(TSLexer *l) {
  int d = 0;
  while (dig(l->lookahead)) { adv(l); d++; }
  if (d != 2 || l->lookahead != ':') return 0;
  adv(l);
  d = 0;
  while (dig(l->lookahead)) { adv(l); d++; }
  if (d != 2 || l->lookahead != ':') return 0;
  adv(l);
  d = 0;
  while (dig(l->lookahead)) { adv(l); d++; }
  if (d != 2) return 0;
  if (l->lookahead == '.') {
    adv(l);
    if (!dig(l->lookahead)) return 0;
    while (dig(l->lookahead)) adv(l);
  }
  return 1;
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

  /* Skip leading whitespace for strand/symbol/bool/date/time detection */
  int need_skip = valid_symbols[STRING]  || valid_symbols[SYMBOLS] ||
                  valid_symbols[BOOL]    || valid_symbols[BOOLS]   ||
                  valid_symbols[INTS]    || valid_symbols[FLOATS]  ||
                  valid_symbols[DATE]    || valid_symbols[DATES]   ||
                  valid_symbols[TIME]    || valid_symbols[TIMES];
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
  int need_date = (valid_symbols[DATE] || valid_symbols[DATES]);
  int need_time = (valid_symbols[TIME] || valid_symbols[TIMES]);

  /* Unified handler for digit-led tokens:
     BOOL, BOOLS, INTS, FLOATS, DATE, DATES, TIME, TIMES */
  if (dig(lexer->lookahead) && (need_bool || need_num || need_date || need_time)) {
    int first_digit = lexer->lookahead;
    int ndigits = 0;
    int all_01 = 1;
    while (dig(lexer->lookahead)) {
      if (lexer->lookahead > '1') all_01 = 0;
      adv(lexer); ndigits++;
    }

    /* TIME: DD:DD:DD[.DDD] — only when exactly 2 digits before first ':' */
    if (ndigits == 2 && lexer->lookahead == ':' && need_time) {
      adv(lexer); /* consume first ':' */
      int m = 0;
      while (dig(lexer->lookahead)) { adv(lexer); m++; }
      if (m == 2 && lexer->lookahead == ':') {
        adv(lexer); /* consume second ':' */
        int s = 0;
        while (dig(lexer->lookahead)) { adv(lexer); s++; }
        if (s == 2) {
          if (lexer->lookahead == '.') {
            adv(lexer);
            while (dig(lexer->lookahead)) adv(lexer);
          }
          lexer->mark_end(lexer);
          int count = 1;
          while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
            while (lexer->lookahead == ' ' || lexer->lookahead == '\t') adv(lexer);
            if (!scan_time_val(lexer)) break;
            count++;
            lexer->mark_end(lexer);
          }
          if (count >= 2 && valid_symbols[TIMES]) { lexer->result_symbol = TIMES; return 1; }
          if (valid_symbols[TIME]) { lexer->result_symbol = TIME; return 1; }
        }
      }
      return 0; /* failed time match, let internal rules handle digits */
    }

    /* BOOL/BOOLS: [01]+b */
    if (need_bool && all_01 && lexer->lookahead == 'b') {
      adv(lexer);
      lexer->mark_end(lexer);
      if (ndigits == 1 && valid_symbols[BOOL]) { lexer->result_symbol = BOOL; return 1; }
      if (ndigits >= 2 && valid_symbols[BOOLS]) { lexer->result_symbol = BOOLS; return 1; }
      return 0;
    }

    if (!need_num && !need_date) return 0;

    /* Special 0-prefix: 0N, 0W (int), 0n, 0w (float), 0x (hex) */
    if (ndigits == 1 && first_digit == '0') {
      if (lexer->lookahead == 'N' || lexer->lookahead == 'W') {
        adv(lexer);
        if (!need_num) return 0;
        return strand_loop(lexer, 0, valid_symbols[INTS], valid_symbols[FLOATS]);
      }
      if (lexer->lookahead == 'n' || lexer->lookahead == 'w') {
        adv(lexer);
        if (!need_num) return 0;
        return strand_loop(lexer, 1, valid_symbols[INTS], valid_symbols[FLOATS]);
      }
      if (lexer->lookahead == 'x' || lexer->lookahead == 'X') {
        adv(lexer);
        while (xdig(lexer->lookahead)) adv(lexer);
        if (!need_num) return 0;
        return strand_loop(lexer, 0, valid_symbols[INTS], valid_symbols[FLOATS]);
      }
      /* fall through to general suffix handling */
    }

    /* Float or Date: digits followed by '.' */
    if (lexer->lookahead == '.') {
      adv(lexer); /* consume '.' */
      int frac = 0;
      while (dig(lexer->lookahead)) { adv(lexer); frac++; }

      /* DATE check: DDDD.DD.DD */
      if (need_date && ndigits == 4 && frac == 2 && lexer->lookahead == '.') {
        adv(lexer); /* consume second '.' */
        int day = 0;
        while (dig(lexer->lookahead)) { adv(lexer); day++; }
        if (day == 2 && !dig(lexer->lookahead) && lexer->lookahead != '.') {
          lexer->mark_end(lexer);
          int count = 1;
          while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
            while (lexer->lookahead == ' ' || lexer->lookahead == '\t') adv(lexer);
            if (!scan_date_val(lexer)) break;
            count++;
            lexer->mark_end(lexer);
          }
          if (count >= 2 && valid_symbols[DATES]) { lexer->result_symbol = DATES; return 1; }
          if (valid_symbols[DATE]) { lexer->result_symbol = DATE; return 1; }
          return 0;
        }
        /* Not a valid date after consuming second dot — return 0, let internal rules handle */
        return 0;
      }

      /* 'e' exponent for float */
      if (lexer->lookahead == 'e' || lexer->lookahead == 'E') {
        adv(lexer);
        if (lexer->lookahead == '+' || lexer->lookahead == '-') adv(lexer);
        while (dig(lexer->lookahead)) adv(lexer);
      }
      if (!need_num) return 0;
      return strand_loop(lexer, 1, valid_symbols[INTS], valid_symbols[FLOATS]);
    }

    /* Scientific notation: digits followed by 'e' or 'E' */
    if (lexer->lookahead == 'e' || lexer->lookahead == 'E') {
      adv(lexer);
      if (lexer->lookahead == '+' || lexer->lookahead == '-') adv(lexer);
      while (dig(lexer->lookahead)) adv(lexer);
      if (!need_num) return 0;
      return strand_loop(lexer, 1, valid_symbols[INTS], valid_symbols[FLOATS]);
    }

    /* Plain integer — try stranding */
    if (!need_num) return 0;
    return strand_loop(lexer, 0, valid_symbols[INTS], valid_symbols[FLOATS]);
  }

  /* Numeric strand starting with '-' or '.' (leading-dot float or negative number) */
  if (need_num && (dig(lexer->lookahead) || lexer->lookahead == '-' || lexer->lookahead == '.')) {
    int first = scan_num(lexer);
    if (first == 0) return 0;
    return strand_loop(lexer, first==2,
                       valid_symbols[INTS], valid_symbols[FLOATS]);
  }

  return 0;
}
