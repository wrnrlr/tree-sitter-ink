; Comments
(comment) @comment

; Strings
(string) @string

; Symbols (atoms) — use the symbol-family scope so themes can colour them
; distinctly from numbers (which are @number).
(symbol) @string.special.symbol
(symbols) @string.special.symbol

; Numbers
(int) @number
(float) @number
(ints) @number
(floats) @number

; Dates and times
(date) @number
(dates) @number
(time) @number
(times) @number

; Booleans
(bool) @constant.builtin
(bools) @constant.builtin

; Operators and adverbs (generic fallback — must come before specific)
(op) @operator
(adverb) @operator

; Monadic operators (inside apposit — overrides generic)
(apposit f: (op) @function)

; Dyadic operators (inside transit — overrides generic)
(transit v: (op) @keyword.operator)

; Variables (generic — later rules override per Zed's last-match-wins)
(var) @variable

; Shader / GLSL builtin functions (mathFns in lib/gpu/spirv.k) — overrides generic var.
; sin/cos/sqrt/abs/floor/sqr/mod are lexer keyword-ops (handled below); these are the rest.
; NB: a (var) node preceded by whitespace absorbs the leading space into its range,
; so we anchor with #match? allowing optional leading space rather than #any-of?.
((var) @function.builtin
 (#match? @function.builtin
  "^[ \t]*(dot|fract|mix|clamp|normalize|length|cross|step|smoothstep|sign|pow|min|max|floor)$"))

; Lambda arguments (parameters override both — keep last so a param named like a
; builtin still reads as a parameter)
(args (var) @variable.parameter)

; I/O verbs (0: 1: etc.)
(verb_io) @function.builtin

; Lambda braces
(lambda "{" @punctuation.bracket)
(lambda "}" @punctuation.bracket)

; Commands (\cmd)
(command) @keyword

; Keyword operators (builtins — overrides generic op)
(op "sqrt" @function.builtin)
(op "sqr" @function.builtin)
(op "exp" @function.builtin)
(op "log" @function.builtin)
(op "sin" @function.builtin)
(op "cos" @function.builtin)
(op "abs" @function.builtin)
(op "first" @function.builtin)
(op "last" @function.builtin)
(op "count" @function.builtin)
(op "in" @function.builtin)
(op "has" @function.builtin)
(op "mod" @function.builtin)
(op "div" @function.builtin)
(op "parse" @function.builtin)
(op "exec" @function.builtin)

; Conditionals
(cond "$[" @keyword)
(cond "]" @keyword)

; Brackets and delimiters (de-emphasized)
(list "(" @punctuation)
(list ")" @punctuation)
(group "(" @punctuation.bracket)
(group ")" @punctuation.bracket)
(apply "[" @punctuation.bracket)
(apply "]" @punctuation.bracket)
(dict "[" @punctuation.bracket)
(dict "]" @punctuation.bracket)
(table "[[]" @punctuation.bracket)
(table "]" @punctuation.bracket)
(amend "@[" @punctuation.bracket)
(amend "]" @punctuation.bracket)
(dmend ".[" @punctuation.bracket)
(dmend "]" @punctuation.bracket)
(cond "$[" @punctuation.bracket)
(cond "]" @punctuation.bracket)
(args "[" @punctuation.bracket)
(args "]" @punctuation.bracket)

; Separators (de-emphasized)
(div) @punctuation

; Assignment (local)
(bind ":" @punctuation.special)
(pending ":" @punctuation.special)

; Assignment (global)
(bind "::" @keyword)
(pending "::" @keyword)

; Monadic-force / partial verb colon (<:  #:  *:  :x return).
; Here ':' is NOT assignment — it belongs to the verb, so colour it like one.
(right ":" @function)
(defer ":" @function)
; Standalone partial/composed verb (e.g. `f: *:`) — the verb and its ':' both green.
(compose f: (op) @function)
(compose v: (op) @function)
