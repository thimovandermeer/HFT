#!/usr/bin/env bash
set -euo pipefail

# Directories/files to check
paths=(
  "GatewayIn/include"
  "GatewayIn/src"
  "Parser/include"
  "Parser/src"
  "main.cpp"
  "tests" # include tests too so contributions stay consistent
)

# Build a list of C/C++ files
mapfile -t files < <(git ls-files -- \
  ${paths[@]} | grep -E '\\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$' || true)

if [ ${#files[@]} -eq 0 ]; then
  echo "No source files found for formatting check."
  exit 0
fi

# Show clang-format version for transparency
clang-format --version || { echo "clang-format not found"; exit 1; }

# Run clang-format in dry-run mode and make it error if changes would be needed
# Some versions use -Werror, others accept --Werror; we try both.
if clang-format -n -Werror "${files[@]}" 2>/dev/null; then
  echo "clang-format check passed."
  exit 0
fi

# Retry with alternative flag spelling (older LLVM versions)
clang-format -n --Werror "${files[@]}" && {
  echo "clang-format check passed."
  exit 0
}

echo "clang-format found issues. Please run clang-format on the codebase." >&2
exit 2
