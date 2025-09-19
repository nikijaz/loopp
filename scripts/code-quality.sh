#!/bin/bash

# This script performs code quality checks using clang-tidy, cppcheck, and clang-format.

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NO_COLOR='\033[0m'

# Project directories
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build}"

# Source directories to check
SOURCE_DIRS=(
    "${PROJECT_ROOT}/src"
    "${PROJECT_ROOT}/include"
    "${PROJECT_ROOT}/examples"
)
SOURCE_FILES=$(find "${SOURCE_DIRS[@]}" -type f \( -name "*.cpp" -o -name "*.hpp" \) 2>/dev/null)

# Logging functions
pinfo() { echo -e "${GREEN}[INFO]${NO_COLOR} $1"; }
pwarn() { echo -e "${YELLOW}[WARN]${NO_COLOR} $1"; }
perr() { echo -e "${RED}[ERROR]${NO_COLOR} $1"; }

# clang-tidy
run_clang_tidy() {
    pinfo "Running clang-tidy static analysis..."

    if ! command -v clang-tidy &>/dev/null; then
        pwarn "clang-tidy not found, skipping..."
        return 0
    fi

    if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
        pwarn "compile_commands.json not found in ${BUILD_DIR}, skipping..."
        return 0
    fi

    if echo "$SOURCE_FILES" | xargs clang-tidy -p "${BUILD_DIR}"; then
        pinfo "clang-tidy completed successfully"
    else
        perr "clang-tidy found issues"
        return 1
    fi
}

# cppcheck
run_cppcheck() {
    pinfo "Running cppcheck static analysis..."

    if ! command -v cppcheck &>/dev/null; then
        pwarn "cppcheck not found, skipping..."
        return 0
    fi

    if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
        pwarn "compile_commands.json not found in ${BUILD_DIR}, skipping..."
        return 0
    fi

    if cppcheck \
        --enable=all \
        --std=c++23 \
        --force \
        --quiet \
        --suppress=missingIncludeSystem \
        --error-exitcode=1 \
        --project="${BUILD_DIR}/compile_commands.json"; then
        pinfo "cppcheck completed successfully"
    else
        perr "cppcheck found issues"
        return 1
    fi
}

# clang-format
run_clang_format() {
    pinfo "Running clang-format check..."

    if ! command -v clang-format &>/dev/null; then
        pwarn "clang-format not found, skipping..."
        return 0
    fi

    if echo "$SOURCE_FILES" | xargs clang-format --dry-run --Werror; then
        pinfo "clang-format check completed successfully"
    else
        perr "clang-format found formatting issues"
        return 1
    fi
}

# Show usage information
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "By default, runs all code quality checks (clang-tidy, cppcheck, clang-format)."
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -t, --tidy          Run only clang-tidy"
    echo "  -c, --cppcheck      Run only cppcheck"
    echo "  -f, --format        Run only clang-format"
    echo ""
    echo "Environment variables:"
    echo "  BUILD_DIR          Override build directory (default: PROJECT_ROOT/build)"
}

# Entry point
main() {
    local run_tidy=false
    local run_cppcheck=false
    local run_format=false

    while [[ $# -gt 0 ]]; do
        case $1 in
        -h | --help)
            show_usage
            exit 0
            ;;
        -t | --tidy)
            run_tidy=true
            ;;
        -c | --cppcheck)
            run_cppcheck=true
            ;;
        -f | --format)
            run_format=true
            ;;
        *)
            perr "Unknown option: $1"
            show_usage
            exit 1
            ;;
        esac
        shift
    done

    # If no specific tool is selected, run all
    if ! $run_tidy && ! $run_cppcheck && ! $run_format; then
        run_tidy=true
        run_cppcheck=true
        run_format=true
    fi

    pinfo "Starting code quality checks..."
    pinfo "Project root: ${PROJECT_ROOT}"
    pinfo "Build directory: ${BUILD_DIR}"

    if [[ -z "$SOURCE_FILES" ]]; then
        perr "No source files found."
        exit 1
    fi

    local exit_code=0

    [[ $run_tidy == true ]] && { run_clang_tidy || exit_code=1; }
    [[ $run_cppcheck == true ]] && { run_cppcheck || exit_code=1; }
    [[ $run_format == true ]] && { run_clang_format || exit_code=1; }

    if [[ $exit_code -eq 0 ]]; then
        pinfo "All code quality checks passed!"
    else
        perr "Some code quality checks failed."
    fi

    exit $exit_code
}

main "$@"
