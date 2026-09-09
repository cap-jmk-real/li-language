#!/usr/bin/env bash
# Layer 4 self-host parity: the Li `check` subcommand (built from
# bootstrap/lic/main.li) must produce the same accept/reject verdicts
# as the C++ `lic check` on the typecheck corpus.
#
# `lic check` on both sides runs parse + source-policy/effects/borrow/
# encapsulation checks over the current li-tests/typecheck layout (the old
# let_bindings/closures/records files were removed in the compiler squash).
#
# Usage:
#   scripts/check_li_check_parity.sh
#   LI_CHECK_BIN=/custom/lic scripts/check_li_check_parity.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIC="${LIC:-$("$ROOT/scripts/resolve-lic.sh")}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
LI="${LI_CHECK_BIN:-$TMP/lic-from-li}"

# Build the Li check binary from bootstrap/lic/main.li with the C++ host
# (the self-host milestone gate), unless an external binary is supplied.
if [[ -z "${LI_CHECK_BIN:-}" ]]; then
  "$LIC" build "$ROOT/bootstrap/lic/main.li" -o "$LI" --allow-open-vc \
    --no-lean-verify >/dev/null 2>&1 \
    || { echo "check_li_check_parity: could not build bootstrap/lic/main.li" >&2; exit 1; }
fi

# Corpus: files both sides must accept (exercises the parse + source/effects/
# borrow/encapsulation layers of the Li `check` subcommand).
CORPUS_OK=(
  "li-tests/typecheck/fib.li"
  "li-tests/typecheck/probe_ifdecl.li"
  "li-tests/typecheck/probe_move.li"
  "li-tests/typecheck/probe_parse.li"
  "li-tests/typecheck/probe_ptr.li"
  "li-tests/typecheck/probe_ref.li"
  "examples/hello.li"
  "examples/arrays.li"
  "li-tests/effects/io_ok.li"
  "li-tests/collections/typedict_ok.li"
  "li-tests/collections/enum_ok.li"
  "li-tests/collections/tuple_pair.li"
  "li-tests/lexer_parser/parser_accept_elif.li"
  "li-tests/typecheck/scalar_width_ok.li"
  "li-tests/typecheck/literal_suffix_ok.li"
  "li-tests/typecheck/binary_literal_ok.li"
  "li-tests/generics/precision_real_alias.li"
  "li-tests/encapsulation/def_method_parse.li"
  "li-tests/encapsulation/def_method_call.li"
  "li-tests/encapsulation/object_method_mutate.li"
  "li-tests/encapsulation/inheritance_subtype.li"
  "li-tests/encapsulation/private_method_lib.li"
  "li-tests/contracts_verify/method_call_requires_ok.li"
  "li-tests/contracts_verify/method_call_requires_fail.li"
  "li-tests/contracts_verify/method_ensures_return_ok.li"
)

# Corpus: files both sides must reject. Entries are `file:EXXXX` when a
# specific error code must appear in both diagnostics, or `file:-` when both
# sides only need to reject (no shared code surface yet).
CORPUS_FAIL=(
  "li-tests/typecheck/bad_array_index.li:E0303"
  "li-tests/typecheck/bad_int_float_add.li:E0303"
  "li-tests/typecheck/bad_numeric_mix.li:E0303"
  "li-tests/typecheck/probe_alias.li:E0303"
  "li-tests/typecheck/probe_arrret.li:E0303"
  "li-tests/typecheck/scalar_width_mix_fail.li:-"
  "li-tests/encapsulation/def_method_missing.li:-"
  "li-tests/encapsulation/private_method_use.li:-"
)

checked=0
pass=0
fail=0

for f in "${CORPUS_OK[@]}"; do
  fp="$ROOT/$f"
  if [[ ! -f "$fp" ]]; then
    echo "  SKIP  $f (not found)"
    continue
  fi
  cpp_rc=0; li_rc=0
  "$LIC" check "$fp" >/dev/null 2>&1 || cpp_rc=$?
  "$LI" check "$fp" >/dev/null 2>&1 || li_rc=$?
  checked=$((checked + 1))
  if [[ "$cpp_rc" == "0" && "$li_rc" == "0" ]]; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    echo "  FAIL  $f  C++=$cpp_rc Li=$li_rc (expected both 0)"
  fi
done

for entry in "${CORPUS_FAIL[@]}"; do
  f="${entry%%:*}"
  expected_code="${entry##*:}"
  fp="$ROOT/$f"
  if [[ ! -f "$fp" ]]; then
    echo "  SKIP  $f (not found)"
    continue
  fi
  cpp_out=$( "$LIC" check "$fp" 2>&1 ) && cpp_rc=0 || cpp_rc=$?
  li_out=$( "$LI" check "$fp" 2>&1 ) && li_rc=0 || li_rc=$?
  checked=$((checked + 1))
  if [[ "$expected_code" == "-" ]]; then
    if [[ "$cpp_rc" != "0" && "$li_rc" != "0" ]]; then
      pass=$((pass + 1))
    else
      fail=$((fail + 1))
      echo "  FAIL  $f  expected reject cpp=$cpp_rc li=$li_rc"
    fi
    continue
  fi
  cpp_has=$(echo "$cpp_out" | grep -c "$expected_code" || true)
  li_has=$(echo "$li_out" | grep -c "$expected_code" || true)
  if [[ "$cpp_has" != "0" && "$li_has" != "0" && "$cpp_rc" != "0" && "$li_rc" != "0" ]]; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    echo "  FAIL  $f  expected $expected_code cpp=$cpp_rc/$cpp_has li=$li_rc/$li_has"
  fi
done

echo ""
echo "check_li_check_parity: $pass/$checked passed, $fail failed"
if [[ "$fail" -gt 0 ]]; then
  exit 1
fi
