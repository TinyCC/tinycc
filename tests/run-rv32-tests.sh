#!/bin/bash
# run-rv32-tests.sh — Run TCC tests2 and pp suites for riscv32 via qemu-user
#
# Usage: cd ~/tinycc && bash tests/run-rv32-tests.sh [test-number...]
#   With no args, runs all tests. With args, runs only those numbered tests.
#   Example: bash tests/run-rv32-tests.sh 22 31 46

set -u

# ── Paths ──────────────────────────────────────────────────────────────────
TCC_BUILD="$HOME/sonata-linux/buildroot/output/build/tcc-riscv32"
SYSROOT="$HOME/sonata-linux/buildroot/output/host/riscv32-buildroot-linux-gnu/sysroot"
TESTS2_DIR="$(cd "$(dirname "$0")/tests2" && pwd)"
PP_DIR="$(cd "$(dirname "$0")/pp" && pwd)"

TCC="$TCC_BUILD/tcc"
TCC_FLAGS="-B $TCC_BUILD -I $SYSROOT/usr/include -L $SYSROOT/usr/lib"

export QEMU_LD_PREFIX="$SYSROOT"

TMPDIR=$(mktemp -d /tmp/tcc-rv32-test.XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

# ── Skip lists ─────────────────────────────────────────────────────────────
# x86 asm tests
SKIP_X86="85 98 99 127"
# Bound-checking tests (no bcheck support on riscv32)
SKIP_BCHECK="112 113 114 115 116 117 126 132"
# Non-standard C
SKIP_NONSTD="34"
# 32-bit non-Windows bitfields_ms
SKIP_32BIT="95_bitfields_ms"
# ARM64-specific
SKIP_ARM64="73"

SKIP_SET=" $SKIP_X86 $SKIP_BCHECK $SKIP_NONSTD $SKIP_ARM64 "

is_skipped() {
    local num="$1" name="$2"
    [[ "$SKIP_SET" == *" $num "* ]] && return 0
    [[ "$name" == "95_bitfields_ms" ]] && return 0
    return 1
}

# ── Per-test flags and args ────────────────────────────────────────────────
get_flags() {
    local name="$1"
    case "$name" in
        22_floating_point|24_math_library) echo "-lm" ;;
        76_dollars_in_identifiers)         echo "-fdollars-in-identifiers" ;;
        60_errors_and_warnings|96_nodata_wanted|125_atomic_misc|128_run_atexit)
                                           echo "-dt" ;;
        106_versym)                        echo "-pthread" ;;
        124_atomic_counter)                echo "-pthread -latomic" ;;
        136_atomic_gcc_style)              echo "-latomic" ;;
        *) echo "" ;;
    esac
}

get_args() {
    local name="$1"
    case "$name" in
        31_args) echo "arg1 arg2 arg3 arg4 arg5" ;;
        46_grep) echo "'[^* ]*[:a:d: ]+\:\*-/: \$\$' $TESTS2_DIR/46_grep.c" ;;
        *) echo "" ;;
    esac
}

# Tests that must be compiled to exe (not -run)
needs_norun() {
    local name="$1"
    case "$name" in
        42_function_pointer|106_versym|108_constructor|120_alias|126_bound_global)
            return 0 ;;
        *) return 1 ;;
    esac
}

# Tests with extra source files
get_extra_sources() {
    local name="$1"
    case "$name" in
        104_inline) echo "$TESTS2_DIR/104+_inline.c" ;;
        120_alias)  echo "$TESTS2_DIR/120+_alias.c" ;;
        *) echo "" ;;
    esac
}

# Tests needing address scrubbing in output
needs_addr_scrub() {
    local name="$1"
    case "$name" in
        112_backtrace|113_btdll|126_bound_global) return 0 ;;
        *) return 1 ;;
    esac
}

# ── Color output ───────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

# ── Run a single tests2 test ──────────────────────────────────────────────
run_test2() {
    local src="$1"
    local name=$(basename "$src" .c)
    local num="${name%%_*}"
    local expect="$TESTS2_DIR/$name.expect"
    local output="$TMPDIR/$name.output"
    local exe="$TMPDIR/$name.exe"
    local flags=$(get_flags "$name")
    local extra=$(get_extra_sources "$name")

    if is_skipped "$num" "$name"; then
        echo -e "  ${YELLOW}SKIP${NC}  $name"
        return 2
    fi

    if [[ ! -f "$expect" ]]; then
        echo -e "  ${YELLOW}SKIP${NC}  $name (no .expect)"
        return 2
    fi

    local rc=0

    if [[ "$flags" == *"-dt"* ]]; then
        # -dt mode: TCC runs snippets internally
        $TCC $TCC_FLAGS $flags "$src" $extra 2>&1 \
            | sed -e "s|$TESTS2_DIR/||g" > "$output" || true
    elif needs_norun "$name"; then
        # Compile to exe, then run
        $TCC $TCC_FLAGS $flags -o "$exe" "$src" $extra 2>&1 && {
            local args
            args=$(get_args "$name")
            eval "$exe" $args 2>&1
        }
        rc=$?
        { if [[ $rc -ne 0 ]] && [[ -s "$output" ]]; then true; fi; } 2>/dev/null
        # Capture output
        {
            $TCC $TCC_FLAGS $flags -o "$exe" "$src" $extra 2>&1
            eval "$exe" $(get_args "$name") 2>&1
        } | sed -e "s|$TESTS2_DIR/||g" > "$output" || true
    else
        # Default: compile to exe and run (since -run is broken)
        local args
        args=$(get_args "$name")
        {
            $TCC $TCC_FLAGS $flags -o "$exe" "$src" $extra 2>&1 && \
            eval "$exe" $args 2>&1
        } | sed -e "s|$TESTS2_DIR/||g" > "$output" || true
    fi

    # For -dt tests, output was already captured above
    if [[ "$flags" != *"-dt"* ]] && ! needs_norun "$name"; then
        # Already captured above in the default path
        true
    fi

    # Address scrubbing for backtrace tests
    if needs_addr_scrub "$name"; then
        sed -i -e 's/[0-9A-Fa-fx]\{5,\}/......../g' \
               -e 's/0x[0-9A-Fa-f]\{1,\}/0x?/g' "$output"
    fi

    # Compare
    if diff -Nbu "$expect" "$output" > "$TMPDIR/$name.diff" 2>&1; then
        echo -e "  ${GREEN}PASS${NC}  $name"
        rm -f "$output" "$TMPDIR/$name.diff"
        return 0
    else
        echo -e "  ${RED}FAIL${NC}  $name"
        # Show first 20 lines of diff
        head -30 "$TMPDIR/$name.diff" | sed 's/^/        /'
        return 1
    fi
}

# ── Run a single pp test ──────────────────────────────────────────────────
run_pp_test() {
    local src="$1"
    local base=$(basename "$src")
    local name="${base%.*}"
    local expect="$PP_DIR/$name.expect"
    local output="$TMPDIR/pp_$name.output"

    if [[ ! -f "$expect" ]]; then
        echo -e "  ${YELLOW}SKIP${NC}  pp/$name (no .expect)"
        return 2
    fi

    $TCC $TCC_FLAGS -E -P "$src" 2>&1 \
        | sed -e "s|$PP_DIR/||g" > "$output" || true

    local diff_opts="-Nbu"
    # Test 02 needs -w (ignore all whitespace)
    [[ "$name" == "02" ]] && diff_opts="-Nbuw"

    if diff $diff_opts "$expect" "$output" > "$TMPDIR/pp_$name.diff" 2>&1; then
        echo -e "  ${GREEN}PASS${NC}  pp/$name"
        rm -f "$output" "$TMPDIR/pp_$name.diff"
        return 0
    else
        echo -e "  ${RED}FAIL${NC}  pp/$name"
        head -20 "$TMPDIR/pp_$name.diff" | sed 's/^/        /'
        return 1
    fi
}

# ── Main ───────────────────────────────────────────────────────────────────
echo "=== TCC riscv32 Test Suite ==="
echo "TCC:     $TCC"
echo "Sysroot: $SYSROOT"
echo "Temp:    $TMPDIR"
echo ""

# Verify TCC works
if ! $TCC $TCC_FLAGS -E -P - <<< "" > /dev/null 2>&1; then
    echo "ERROR: TCC cannot run. Check QEMU_LD_PREFIX and paths."
    exit 1
fi

pass=0 fail=0 skip=0

# Filter tests if args given
filter_nums=("$@")

# ── tests2 ──
echo "── tests2 ──────────────────────────────────────────────"
for src in "$TESTS2_DIR"/[0-9]*_*.c; do
    name=$(basename "$src" .c)
    # Skip the "+" companion files (104+_inline, 120+_alias)
    [[ "$name" == *+* ]] && continue
    num="${name%%_*}"

    # If filter specified, only run matching tests
    if [[ ${#filter_nums[@]} -gt 0 ]]; then
        match=0
        for f in "${filter_nums[@]}"; do
            [[ "$num" == "$f" ]] && match=1 && break
        done
        [[ $match -eq 0 ]] && continue
    fi

    run_test2 "$src"
    rc=$?
    case $rc in
        0) ((pass++)) ;;
        1) ((fail++)) ;;
        2) ((skip++)) ;;
    esac
done

# ── pp ──
if [[ ${#filter_nums[@]} -eq 0 ]]; then
    echo ""
    echo "── pp ──────────────────────────────────────────────────"
    for src in "$PP_DIR"/[0-9]*.[cS] "$PP_DIR"/pp-*.c; do
        [[ -f "$src" ]] || continue
        run_pp_test "$src"
        rc=$?
        case $rc in
            0) ((pass++)) ;;
            1) ((fail++)) ;;
            2) ((skip++)) ;;
        esac
    done
fi

# ── Summary ──
echo ""
echo "════════════════════════════════════════════════════════"
echo -e "  ${GREEN}PASS: $pass${NC}  ${RED}FAIL: $fail${NC}  ${YELLOW}SKIP: $skip${NC}  TOTAL: $((pass+fail+skip))"
echo "════════════════════════════════════════════════════════"

[[ $fail -eq 0 ]] && exit 0 || exit 1
