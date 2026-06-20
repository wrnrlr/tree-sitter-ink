/**
 * @file k based array programming language
 * @author Werner Laurensse
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const F = field, O = optional, R = repeat, R1 = repeat1, S = seq, P = prec, T = token, D = prec.dynamic,
      C = choice, A = alias, OF = (a, b) => O(F(a, b)), PR = prec.right, I = T.immediate

module.exports = grammar({
  name: 'ink',

  word: $ => $.var,
  extras: $ => [/[ \t]+/],
  externals: $ => [$._MINUS, $._BOOL, $._BOOLS, $._INTS, $._FLOATS, $._SYMBOLS, $._STRING, $._DATE, $._DATES, $._TIME, $._TIMES],
  inline: $ => [$.phrase, $.items],
  supertypes: $ => [$.clause, $.phrase, $.noun, $.verb, $.query],
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
    clause: $ => D(-1, C($.right, $.bind, $.transit, $.apposit, $.phrase, $.cond, $.query)),
    adjunct: $ => D(-1, C($.defer, $.pending, $.intrans, $.prefix, $.compose)),
    query: $ => C($.select, $.update, $.delete),

    // SQL queries
    select: $ => S('select', O($.rows), O(S('by', $.rows)), $.from, O($.where)),
    update: $ => S('update', $.rows, $.from, O($.where)),
    delete: $ => S('delete', $.from, O($.where)),
    from: $ => S('from', $.var),
    where: $ => S('where', $._stmt),
    rows: $ => D(2, S($._stmt, R(S(/,\s*/, $._stmt)))),

    // Clauses
    right: $ => S(':', $.clause),
    defer: $ => S(':', $.adjunct),
    bind:    $ => D(1, P(1, S(F('v', $.noun), F('f', O($.op)), ':', F('a', O($.clause))))),
    pending: $ => P(1, S(F('v', $.noun), F('f', O($.op)), ':', F('a', $.adjunct))),
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
    amend: $ => P(11, S('@[', $.seq, ']')),
    dmend: $ => P(11, S('.[', $.seq, ']')),
    dict: $ => S('[', O($.items), ']'),
    table: $ => P(3, S('[[]', O($.items), ']')),
    utable: $ => P(2, S('[[', $.items, ']', O($.items), ']')),
    items: $ => S($.item, R(S($.div, $.item))),
    item: $ => P(2, S(F('k', $.name), ':', F('v', O($._stmt)))),
    name: $ => C($.int, $.string, $.symbol, $.var),

    op: $ => C(P(-1, ':'), /[%!&+*|<>=~,^#_$?@.\/-]/, $._keyword_op),
    verb_io: _ => T(S(/\d/, ':')),
    adverb: _ => P(1, I(/[\/\\']:?/)),

    // Literals
    literal: $ => C($.bool, $.bools, $.int, $.ints, $.float, $.floats, $.string, $.symbols, $.symbol, $.var, $.date, $.dates, $.time, $.times),
    bool: $ => $._BOOL,
    bools: $ => $._BOOLS,
    int: _ => C(/-?0[NW]/, /-?\d+/, /-?0x[\da-f]+/i),
    ints: $ => $._INTS,
    float: _ => C(/-?0[nw]/, /-?(?:\d+\.\d*|\.\d+|\d+)(?:e[+-]?\d+)?/),
    floats: $ => $._FLOATS,
    string: $ => $._STRING,
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
                         'mod', 'div', 'parse', 'exec'),

    sep: _ => C(/;\s*/, /\n/),
    div: _ => C(/;\s*/, /\n\s*/),
    _line_comment: $ => A(T(P(10, S('/', /[^\n]*/))), $.comment),
    _inline_comment: $ => A(T(P(10, S(/[ \t]+/, '/', /[^\n]*/))), $.comment),
  }
});
