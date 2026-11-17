#!/usr/bin/env bash
# Implementace překladače imperativního jazyka IFJ25
# Test runner.
#
# tests/lexer/NAME.wren   input, run with --tokens
#   NAME.out              expected token dump (optional)
#   NAME.code             expected exit code (optional, default 0)
# examples/*.wren         must compile with exit code 0
#
# Usage: tests/run.sh [path/to/ifj25]

BIN=${1:-./ifj25}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
pass=0
fail=0

check() { # name expected_code actual_code expected_out_file actual_out
    local name=$1 exp_code=$2 code=$3 exp_out=$4 out=$5
    if [ "$code" != "$exp_code" ]; then
        echo "FAIL $name: exit code $code, expected $exp_code"
        fail=$((fail + 1))
    elif [ -n "$exp_out" ] && [ "$out" != "$(cat "$exp_out")" ]; then
        echo "FAIL $name: output differs"
        diff <(echo "$out") "$exp_out" | head -10
        fail=$((fail + 1))
    else
        pass=$((pass + 1))
    fi
}

for src in "$ROOT"/tests/lexer/*.wren; do
    base=${src%.wren}
    exp_code=0
    [ -f "$base.code" ] && exp_code=$(cat "$base.code")
    exp_out=""
    [ -f "$base.out" ] && exp_out="$base.out"
    out=$("$BIN" --tokens < "$src" 2>/dev/null)
    code=$?
    check "lexer/$(basename "$src")" "$exp_code" "$code" "$exp_out" "$out"
done

for src in "$ROOT"/examples/*.wren; do
    out=$("$BIN" < "$src" 2>&1)
    code=$?
    check "examples/$(basename "$src")" 0 "$code" "" "$out"
done

echo "passed: $pass, failed: $fail"
[ "$fail" -eq 0 ]
