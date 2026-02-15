#!/usr/bin/env bash
# ----------------------------------------------------------------------
# Verify that the project builds and passes tests with GCC and Clang.
# Run this before committing changes.
#
# Usage:
#   ./scripts/verify-all.sh              # Debug builds (gcc + clang), stop on first failure
#   ./scripts/verify-all.sh --all        # All 4 variants (debug/release x gcc/clang)
#   ./scripts/verify-all.sh --continue   # Continue on failure, report all at end
#   ./scripts/verify-all.sh debug-gcc    # Specific preset(s)
#   ./scripts/verify-all.sh debug-gcc release-clang
# ----------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}==>${NC} $1"
}

log_error() {
    echo -e "${RED}==>${NC} $1"
}

# Default presets (debug builds with both compilers)
DEFAULT_PRESETS=("debug-gcc" "debug-clang")
ALL_PRESETS=("debug-gcc" "debug-clang" "release-gcc" "release-clang")

# Parse arguments
PRESETS=()
FAIL_FAST=true
while [ $# -gt 0 ]; do
    case "$1" in
        --continue)
            FAIL_FAST=false
            shift
            ;;
        --all)
            PRESETS=("${ALL_PRESETS[@]}")
            shift
            ;;
        *)
            PRESETS+=("$1")
            shift
            ;;
    esac
done

if [ ${#PRESETS[@]} -eq 0 ]; then
    PRESETS=("${DEFAULT_PRESETS[@]}")
fi

# Track results
declare -a PASSED_PRESETS=()
declare -a FAILED_PRESETS=()

for preset in "${PRESETS[@]}"; do
    echo ""
    log_info "Building and testing: $preset"
    echo "========================================"

    if cmake --preset "$preset" && \
       cmake --build --preset "$preset" && \
       ctest --preset "$preset"; then
        PASSED_PRESETS+=("$preset")
        log_info "$preset: PASSED"
    else
        FAILED_PRESETS+=("$preset")
        log_error "$preset: FAILED"
        if [ "$FAIL_FAST" = true ]; then
            echo ""
            log_error "Stopping on first failure (use --continue to run all presets)"
            exit 1
        fi
    fi
done

# Summary
echo ""
echo "========================================"
echo "SUMMARY"
echo "========================================"

if [ ${#PASSED_PRESETS[@]} -gt 0 ]; then
    log_info "Passed: ${PASSED_PRESETS[*]}"
fi

if [ ${#FAILED_PRESETS[@]} -gt 0 ]; then
    log_error "Failed: ${FAILED_PRESETS[*]}"
    echo ""
    log_error "Verification FAILED - fix errors before committing"
    exit 1
fi

echo ""
log_info "All builds passed - safe to commit"
exit 0
