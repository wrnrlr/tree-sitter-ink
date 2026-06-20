; Document outline — top-level definitions (name: value / name:: value).
; Powers Zed's outline panel, breadcrumbs, and "go to symbol in editor".

(terse
  (bind
    v: (literal (var) @name)) @item)

; Partial-verb definitions parse the binding under an `adjunct` wrapper.
(terse
  (adjunct
    (pending
      v: (literal (var) @name)) @item))
