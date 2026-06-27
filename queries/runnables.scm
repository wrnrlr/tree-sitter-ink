; Run the whole ink script. Mirrors Zed's built-in bash "run a script file"
; runnable: capture the file node (@_ink-script) and anchor the ▶ indicator to
; the first top-level node (@run). The `ink-file` tag binds it to tasks.json.
((terse
  .
  (_) @run) @_ink-script
 (#set! tag ink-file))
