; Scope-aware variable resolution.
; Lets the editor distinguish definitions from references, highlight all
; occurrences of a local, and scope lambda parameters to their lambda body.

; Scopes
(lambda) @local.scope

; Definitions
;   - lambda parameters:  {[x;y] ...}
(args (var) @local.definition)
;   - bindings:           name: value   /  name:: value
(bind v: (literal (var) @local.definition))
(pending v: (literal (var) @local.definition))

; References — every other variable use
(var) @local.reference
