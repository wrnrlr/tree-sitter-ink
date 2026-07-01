const F = field, O = optional, R = repeat, R1 = repeat1, S = seq, P = prec, T = token, D = prec.dynamic,
      C = choice, A = alias, OF = (a, b) => O(F(a, b)), PR = prec.right, I = T.immediate
module.exports = grammar({
  name: 'ink',
  word: $ => $.var,
  extras: $ => [/[ \t]+/],
  externals: $ => [$._MINUS, $._BOOL, $._BOOLS, $._INTS, $._FLOATS, $._SYMBOLS, $.str_open, $._str_body, $.escape_sequence, $.str_close, $._DATE, $._DATES, $._TIME, $._TIMES, $.amend_op, $.drill_op],
  inline: $ => [$.phrase, $.items],
  supertypes: $ => [$.clause, $.phrase, $.noun, $.verb],
  conflicts: $ => [
    [$.group, $.seq],
    [$.clause, $.compose],
    [$.apposit, $.transit],
    [$.literal, $.name],
    [$.args, $.dict],
  ],
  rules: {
    terse: $ => S(O($._line), R(S($.sep, O($._line)))),
    _line: $ => C(S($._stmt, O($._inline_comment)), $._line_comment, $.command),
    command: _ => T(P(10, S('\\', /[a-zA-Z][^\n]*/))),
    _stmt: $ => P(1, C(D(2, $.verb), $.noun, D(-1, $.clause), D(-1, $.adjunct))),
    clause: $ => D(-1, C($.right, $.bind, $.transit, $.apposit, $.phrase, $.cond)),
    adjunct: $ => D(-1, C($.defer, $.pending, $.intrans, $.prefix, $.compose)),

    // Clauses
    right: $ => S(':', $.clause),
    defer: $ => S(':', $.adjunct),
    // Plain assignment allows an empty value (`x:` continued on the next line);
    // compound assignment (`x+:y`) requires the value, so that `1<:\y` is NOT
    // mis-read as "assign to 1" — it falls back to a transit with verb `<:\`.
    bind:    $ => D(1, P(1, C(
                     S(F('v', $.noun), C('::', ':'), F('a', O($.clause))),
                     S(F('v', $.noun), F('f', $.op), C('::', ':'), F('a', $.clause))))),
    pending: $ => P(1, S(F('v', $.noun), F('f', O($.op)), C('::', ':'), F('a', $.adjunct))),
    transit: $ => D(1, S(F('a', $.noun), F('v', C($.verb, A($._MINUS, $.op))), F('b', $.clause))),
    intrans: $ => P(1, S(F('a', $.noun), F('v', $.verb), OF('z', $.adjunct))),
    apposit: $ => D(-1, S(F('f', $.phrase), F('a', $.clause))),
    compose: $ => D(1, C(F('v', $.verb), S(F('f', $.phrase), F('z', $.adjunct)))),
    affix:   $ => D(2, S(F('a', $.noun), F('v', A($._MINUS, $.op)), F('b', $.clause))),
    prefix:  $ => D(2, S(F('a', $.noun), F('v', A($._MINUS, $.op)), O(F('z', $.adjunct)))),

    phrase: $ => C($.verb, $.noun),
    noun: $ => C($.apply, $.group, $.list, $.lambda, $.literal, $.amend, $.dmend, $.dict, $.table, $.utable),
    verb: $ => C($.term, $.op, $.verb_io),

    term: $ => S(F('f', $.phrase), F('a', $.adverb)),
    apply: $ => P(10, S(F('f', $.phrase), '[', F('a', O($.seq)), ']')),
    group: $ => S('(', $._line, ')'),
    list: $ => S('(', O($.seq), ')'),
    seq: $ => C(R1($.div), S(R($.div), S($._line, R(S($.div, O($._line)))))),
    lambda: $ => S('{', F('a', O($.args)), F('b', O($.seq)), '}'),
    args: $ => S('[', O($.var), R(S($.div, O($.var))), ']'),
    cond: $ => S('$[', $._stmt, R1(S(';', $._stmt)), ']'),
    // `@`/`.` are emitted as their own external token (amend_op/drill_op) with an
    // immediate `[`, so the glyph and bracket can be highlighted separately.
    amend: $ => P(11, S($.amend_op, I('['), $.seq, ']')),
    dmend: $ => P(11, S($.drill_op, I('['), $.seq, ']')),
    dict: $ => S('[', O($.items), ']'),
    table: $ => P(3, S('[[]', O($.items), ']')),
    utable: $ => P(2, S('[[', $.items, ']', O($.items), ']')),
    items: $ => S($.item, R(S($.div, $.item))),
    item: $ => P(2, S(F('k', $.name), ':', F('v', O($._stmt)))),
    name: $ => C($.int, $.string, $.symbol, $.var),

    op: $ => C(P(-1, ':'), /[%!&+*|<>=~,^#_$?@.\/-]/, $._keyword_op),
    verb_io: _ => T(S(/\d/, ':')),
    // A leading ':' forces the monadic valence of the verb being modified, e.g.
    // `<:\` (grade-up, scanned).  Gluing it to the adverb token keeps it from
    // being mistaken for an assignment ':'.
    adverb: _ => P(1, I(/:?[\/\\']:?/)),

    // Literals
    literal: $ => C($.bool, $.bools, $.int, $.ints, $.float, $.floats, $.string, $.symbols, $.symbol, $.var, $.date, $.dates, $.time, $.times),
    bool: $ => $._BOOL,
    bools: $ => $._BOOLS,
    int: _ => C(/-?0[NW]/, /-?\d+/, /-?0x[\da-f]+/i),
    ints: $ => $._INTS,
    float: _ => C(/-?0[nw]/, /-?(?:\d+\.\d*|\.\d+|\d+)(?:e[+-]?\d+)?/),
    floats: $ => $._FLOATS,
    // A string is open-quote, a run of escape sequences / literal body, then a
    // closing quote.  The three structural tokens come from the external scanner
    // (the close convention needs lookahead); `escape_sequence` is visible so
    // editors can colour `\n \t \\ …` apart from the body.  When a newline
    // immediately follows the opening quote the string is multi-line: the body
    // (`_str_body`) then spans newlines until the closing quote.
    string: $ => S($.str_open, R(C($.escape_sequence, $._str_body)), $.str_close),
    symbol: $ => T(S('`', O(C(
      /[a-zA-Z0-9.]+/,
      /[%!&+*|<>=~,^#_$?@\/-]+/,
      S('"', /[^"]*/, '"'),
    )))),
    symbols: $ => $._SYMBOLS,
    date: $ => $._DATE,
    dates: $ => $._DATES,
    time: $ => $._TIME,
    times: $ => $._TIMES,
    var: _ => /[a-zA-Z][a-zA-Z\d]*(?:\.[a-zA-Z][a-zA-Z\d]*)*/,

    _keyword_op: $ => C('sqrt', 'sqr', 'exp', 'log', 'sin', 'cos', 'abs',
                         'first', 'last', 'count', 'in', 'has',
                         'mod', 'div', 'parse', 'exec', 'depth', 'epoch'),

    sep: _ => C(/;\s*/, /\n/),
    div: _ => C(/;\s*/, /\n\s*/),
    _line_comment: $ => A(T(P(10, S('/', /[^\n]*/))), $.comment),
    _inline_comment: $ => A(T(P(10, S(/[ \t]+/, '/', /[^\n]*/))), $.comment),
  }
});
