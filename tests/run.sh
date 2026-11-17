#!/usr/bin/env bash
# Implementace překladače imperativního jazyka IFJ25
# Test runner.
#
# tests/lexer/NAME.wren     input, run with --tokens
#   NAME.out                expected token dump (optional)
#   NAME.code               expected exit code (optional, default 0)
# tests/programs/NAME.wren  whole program, compiled
#   NAME.code               expected compiler exit code (default 0)
#   NAME.in                 stdin for the compiled program (optional)
#   NAME.out                expected output of the compiled program (optional)
#   NAME.rc                 expected exit code of the compiled program (default 0)
# examples/*.wren           must compile with exit code 0
#
# Compiled programs are run by tests/ic25.py (a local stand-in for ic25int).
#
# Usage: tests/run.sh [path/to/ifj25]

BIN=${1:-./ifj25}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
INTERPRETER="python3 $ROOT/tests/ic25.py"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
pass=0
fail=0

ok() { pass=$((pass + 1)); }
bad() { echo "FAIL $1"; fail=$((fail + 1)); }

opt() { [ -f "$1" ] && cat "$1" || echo "$2"; }

for src in "$ROOT"/tests/lexer/*.wren; do
    base=${src%.wren}
    name="lexer/$(basename "$src")"
    out=$("$BIN" --tokens < "$src" 2>/dev/null)
    code=$?
    if [ "$code" != "$(opt "$base.code" 0)" ]; then
        bad "$name: exit code $code, expected $(opt "$base.code" 0)"
    elif [ -f "$base.out" ] && [ "$out" != "$(cat "$base.out")" ]; then
        bad "$name: token output differs"
        diff <(echo "$out") "$base.out" | head -10
    else
        ok
    fi
done

for src in "$ROOT"/tests/programs/*.wren "$ROOT"/examples/*.wren; do
    base=${src%.wren}
    name="$(basename "$(dirname "$src")")/$(basename "$src")"
    "$BIN" < "$src" > "$TMP/prog.code" 2> "$TMP/err"
    code=$?
    expected=$(opt "$base.code" 0)
    if [ "$code" != "$expected" ]; then
        bad "$name: compiler exit code $code, expected $expected ($(head -1 "$TMP/err"))"
        continue
    fi
    if [ "$code" != 0 ] || [ ! -f "$base.out" ]; then
        ok
        continue
    fi
    input=/dev/null
    [ -f "$base.in" ] && input="$base.in"
    $INTERPRETER "$TMP/prog.code" < "$input" > "$TMP/out" 2> "$TMP/err"
    rc=$?
    if [ "$rc" != "$(opt "$base.rc" 0)" ]; then
        bad "$name: program exit code $rc, expected $(opt "$base.rc" 0) ($(head -1 "$TMP/err"))"
    elif ! cmp -s "$TMP/out" "$base.out"; then
        bad "$name: program output differs"
        diff "$TMP/out" "$base.out" | head -10
    else
        ok
    fi
done

echo "passed: $pass, failed: $fail"
[ "$fail" -eq 0 ]
