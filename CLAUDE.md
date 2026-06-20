# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Tree-sitter grammar for **ink**, a k-based array programming language. The grammar is defined in `grammar.js` and tree-sitter generates a C parser from it. Bindings are provided for C, Go, Node.js, Python, Rust, and Swift.

## Key Commands

```bash
# Generate the parser from grammar.js
tree-sitter generate

# Run corpus tests (test/corpus/*.txt)
tree-sitter test

# Run a single test by name filter
tree-sitter test -f "test_name"

# Parse a specific file for debugging
tree-sitter parse <file.k>

# Launch interactive playground (builds WASM first)
npm start

# Run Node.js binding tests
npm test
```

## Development Workflow

1. Edit `grammar.js` to define/modify grammar rules
2. Run `tree-sitter generate` to regenerate `src/parser.c` (and `src/tree_sitter/parser.h`)
3. Add test cases in `test/corpus/` as `.txt` files using tree-sitter's test format
4. Run `tree-sitter test` to validate

## Tree-sitter Test Format

Test files in `test/corpus/` use this format:
```
==================
Test Name
==================

source code here

---

(expected_syntax_tree)
```

## Architecture

- `grammar.js` — Single source of truth for the grammar definition
- `src/` — Auto-generated C parser (do not edit manually)
- `bindings/` — Language-specific bindings (c, go, node, python, rust, swift)
- `queries/` — Tree-sitter query files (highlights, locals, etc.) when added
- `tree-sitter.json` — Parser metadata; file extension is `.k`
