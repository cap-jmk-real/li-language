#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export LI_REPO_ROOT="$ROOT"
LIC="${LIC:-$("$ROOT/scripts/resolve-lic.sh")}"
[[ -x "$LIC" ]] || { echo "check-mir-parallel-decorator: lic not built" >&2; exit 1; }
DECOR="$ROOT/li-tests/decorators/parallel_with_disjoint.li"
out="$("$LIC" verify "$DECOR" 2>&1)"
echo "$out" | grep -q 'mir_parallel_disjoint=1'
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
"$LIC" build "$DECOR" -o "$tmp/par" --release >/dev/null
# Capture nm output before grepping: `nm | grep -q` under pipefail dies with
# SIGPIPE the moment grep finds the symbol and closes the pipe.
if command -v llvm-nm >/dev/null 2>&1; then NM_TOOL=llvm-nm
elif command -v nm >/dev/null 2>&1; then NM_TOOL=nm
else NM_TOOL=""; echo "check-mir-parallel-decorator: skip OpenMP symbol check" >&2
fi
if [[ -n "${NM_TOOL:-}" ]]; then
  syms="$("$NM_TOOL" "$tmp/par" 2>/dev/null || true)"
  [[ "$(echo "$syms" | grep -c 'li_omp_parallel_for_i64' || true)" != "0" ]] \
    || { echo "check-mir-parallel-decorator: missing li_omp_parallel_for_i64 symbol" >&2; exit 1; }
fi
"$ROOT/li-tests/run_all.sh" race_shared_memory >/dev/null
echo check-mir-parallel-decorator: ok
