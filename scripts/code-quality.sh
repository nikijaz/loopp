#!/bin/bash

# This script performs code quality checks using Clang-Format and Clang-Tidy.

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
    "${PROJECT_ROOT}/tests"
    "${PROJECT_ROOT}/examples"
)
SOURCE_FILES=$(find "${SOURCE_DIRS[@]}" -type f \( -name "*.cpp" -o -name "*.hpp" \) 2>/dev/null)

# Logging functions
pinfo() { echo -e "${GREEN}[INFO]${NO_COLOR} $1"; }
pwarn() {
    if [[ -n "$GITHUB_ACTIONS" ]]; then
        echo "::warning::$1"
    else
        echo -e "${YELLOW}[WARN]${NO_COLOR} $1"
    fi
}
perr() {
    if [[ -n "$GITHUB_ACTIONS" ]]; then
        echo "::error::$1"
    else
        echo -e "${RED}[ERROR]${NO_COLOR} $1"
    fi
}

# Clang-Tidy
run_clang_tidy() {
    pinfo "Running Clang-Tidy analysis..."

    if ! command -v run-clang-tidy-18 &>/dev/null; then
        pwarn "Clang-Tidy not found"
        return 0
    fi

    if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
        pwarn "No compile_commands.json found in ${BUILD_DIR}"
        return 0
    fi

    if echo "$SOURCE_FILES" | xargs run-clang-tidy-18 -p "${BUILD_DIR}" -quiet; then
        pinfo "Clang-Tidy check passed"
    else
        perr "Clang-Tidy found linting issues"
        return 1
    fi
}

# Clang-Format
run_clang_format() {
    pinfo "Running Clang-Format check..."

    if ! command -v clang-format-18 &>/dev/null; then
        pwarn "Clang-Format not found"
        return 0
    fi

    if echo "$SOURCE_FILES" | xargs clang-format-18 --dry-run --Werror; then
        pinfo "Clang-Format check passed"
    else
        pwarn "Clang-Format found formatting issues"
        return 0
    fi
}

# Show usage information
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "By default, runs all code quality checks (clang-tidy, clang-format)."
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -t, --tidy          Run only clang-tidy"
    echo "  -f, --format        Run only clang-format"
    echo ""
    echo "Environment variables:"
    echo "  BUILD_DIR          Override build directory (default: PROJECT_ROOT/build)"
}

# Entry point
main() {
    local run_tidy=false
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
    if ! $run_tidy && ! $run_format; then
        run_tidy=true
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
    [[ $run_format == true ]] && { run_clang_format || exit_code=1; }

    exit $exit_code
}

main "$@"
