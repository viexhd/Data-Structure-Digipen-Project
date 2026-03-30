#!/usr/bin/env bash
# run_tests.sh — build and run all reference test cases, report pass/fail
# Usage (from WSL): bash run_tests.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- Colours ----------------------------------------------------------------
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

# ---- Build ------------------------------------------------------------------
echo -e "${YELLOW}==> Building...${NC}"
if make; then
    echo -e "${GREEN}Build OK${NC}"
else
    echo -e "${RED}Build FAILED — fix compile errors before running tests${NC}"
    exit 1
fi
echo ""

# ---- Test cases: name -> target vertex count --------------------------------
declare -A TESTS=(
    ["input_rectangle_with_two_holes.csv"]=7
    ["input_cushion_with_hexagonal_hole.csv"]=13
    ["input_blob_with_two_holes.csv"]=17
    ["input_wavy_with_three_holes.csv"]=21
    ["input_lake_with_two_islands.csv"]=17
    ["input_original_01.csv"]=99
    ["input_original_02.csv"]=99
    ["input_original_03.csv"]=99
    ["input_original_04.csv"]=99
    ["input_original_05.csv"]=99
    ["input_original_06.csv"]=99
    ["input_original_07.csv"]=99
    ["input_original_08.csv"]=99
    ["input_original_09.csv"]=99
    ["input_original_10.csv"]=99
)

PASS=0; FAIL=0; TOTAL=0

# ---- Run each test ----------------------------------------------------------
for INPUT in "${!TESTS[@]}"; do
    TARGET="${TESTS[$INPUT]}"
    EXPECTED_BASE="${INPUT/input_/output_}"
    EXPECTED_FILE="test_cases/${EXPECTED_BASE%.csv}.txt"
    INPUT_FILE="test_cases/$INPUT"

    if [[ ! -f "$INPUT_FILE" ]]; then
        echo -e "${YELLOW}SKIP${NC}  $INPUT (input not found)"
        continue
    fi
    if [[ ! -f "$EXPECTED_FILE" ]]; then
        echo -e "${YELLOW}SKIP${NC}  $INPUT (expected output not found)"
        continue
    fi

    TOTAL=$((TOTAL + 1))

    # Run the program, capture output and any crash
    ACTUAL=$(./simplify "$INPUT_FILE" "$TARGET" 2>/dev/null) || {
        echo -e "${RED}FAIL${NC}  $INPUT -> crashed or returned error"
        FAIL=$((FAIL + 1))
        continue
    }

    EXPECTED=$(cat "$EXPECTED_FILE")

    if [[ "$ACTUAL" == "$EXPECTED" ]]; then
        echo -e "${GREEN}PASS${NC}  $INPUT  (target=$TARGET)"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}FAIL${NC}  $INPUT  (target=$TARGET)"
        echo "      --- expected ---"
        echo "$EXPECTED" | head -6 | sed 's/^/      /'
        echo "      --- got ---"
        echo "$ACTUAL"   | head -6 | sed 's/^/      /'
        FAIL=$((FAIL + 1))
    fi
done

# ---- Summary ----------------------------------------------------------------
echo ""
echo "==============================="
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC} / $TOTAL total"
echo "==============================="
[[ $FAIL -eq 0 ]] && exit 0 || exit 1
