; Comments
(comment) @comment

; Strings
(string) @string

; Escape sequences inside strings (`\n \t \\ \" …`) — grey like comments.
; Must come after (string) so it overrides the green for its sub-range.
(escape_sequence) @comment

; Symbols (atoms) — colour them like strings (green); both are literal text.
(symbol) @string
(symbols) @string

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

; I/O verbs (0: 1: etc.) — generic fallback (standalone / partial use).
(verb_io) @function.builtin

; …but colour by valence like ordinary verbs when the AST knows it:
; monadic (read) reads like a monadic op, dyadic (write) like a dyadic op.
; These come after the generic rule so they win under Zed's last-match-wins.
(apposit f: (verb_io) @function)
(transit v: (verb_io) @keyword.operator)

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
(op "depth" @function.builtin)
(op "epoch" @function.builtin)

; Conditionals
(cond "$[" @keyword)
(cond "]" @keyword)

; Brackets and delimiters (de-emphasized)
(list "(" @punctuation)
(list ")" @punctuation)
(group "(" @punctuation.bracket)
(group ")" @punctuation.bracket)

; Structural brackets stay white (dict / table literals).
(dict "[" @punctuation.bracket)
(dict "]" @punctuation.bracket)
(table "[[]" @punctuation.bracket)
(table "]" @punctuation.bracket)
(utable "[[" @punctuation.bracket)
(utable "]" @punctuation.bracket)
(cond "$[" @punctuation.bracket)
(cond "]" @punctuation.bracket)

; Syntactic brackets — colour the brackets grey like comments while leaving the
; content (parameters, indices) its own colour. Covers lambda args `[x;y]`,
; call/index `f[…]`, amend `@[…]` and drill `.[…]`.
(args "[" @comment)
(args "]" @comment)
(apply "[" @comment)
(apply "]" @comment)
; Amend `@[…]` and drill `.[…]` — only the `@`/`.` glyph is red like assignment;
; the brackets are grey like the other syntactic brackets.  The glyph is its own
; external token (amend_op/drill_op) so it can be coloured apart from the `[`.
(amend (amend_op) @punctuation.special)
(amend "[" @comment)
(amend "]" @comment)
(dmend (drill_op) @punctuation.special)
(dmend "[" @comment)
(dmend "]" @comment)

; Separators (`;` between items / statements) — grey like comments.
(div) @comment
(sep) @comment
(cond ";" @comment)

; Assignment (local) and global both red — `:` and `::` read identically.
(bind ":" @punctuation.special)
(pending ":" @punctuation.special)
(bind "::" @punctuation.special)
(pending "::" @punctuation.special)

; Modify-assign operator (`x+:y`, `x*::y`): the op fused to the assignment is
; red like the `:`/`::` it binds with, not teal like a free operator.
(bind f: (op) @punctuation.special)
(pending f: (op) @punctuation.special)

; Monadic-force / partial verb colon (<:  #:  *:  :x return).
; Here ':' is NOT assignment — it belongs to the verb, so colour it like one.
(right ":" @function)
(defer ":" @function)
; Standalone partial/composed verb (e.g. `f: *:`) — the verb and its ':' both green.
(compose f: (op) @function)
(compose v: (op) @function)

; Fused reduce operators: +/ */ &/ |/  (see src/primitive/derived/fuse.zig).
; DISABLED — kept for reference. Re-enable by uncommenting AND adding a theme
; override with BOTH color and weight (Zed renders an unknown scope as default
; foreground, i.e. white, so a colourless override is not enough):
;   "theme_overrides": { "syntax": { "operator.fused": { "color": "#268bd2", "font_weight": 700 } } }
; #match? must hold for BOTH the op and the adverb, so only the four fused
; reductions match (not -/, +\, </, …). The optional leading space tolerates the
; whitespace a (var/op) node absorbs when preceded by a space.
; ((term f: (op) @operator.fused a: (adverb) @operator.fused)
;  (#match? @operator.fused "^[ \t]*([+*&|]|/)$"))

; Apply-form verb arity coloring: `f[x]` vs `f[x;y]`.
; When a primitive op or io-verb appears as the called function, colour it like
; the appropriate valence.  Default: monadic (@function).  Overridden to dyadic
; (@keyword.operator) when the argument list contains a `;` separator (div node).
; Trains (`*|[x]`), user vars (`f[x]`) and splices stay their default colour.
(apply f: (op) @function)
(apply f: (op) @keyword.operator
       a: (seq (div)))
(apply f: (verb_io) @function)
(apply f: (verb_io) @keyword.operator
       a: (seq (div)))
