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
    ["input_rectangle_with_two_holes.csv"]=11
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

    if [[ "${SKIP_ORIGINALS:-0}" == "1" && "$INPUT" == input_original_* ]]; then
        echo -e "${YELLOW}SKIP${NC}  $INPUT (SKIP_ORIGINALS=1)"
        continue
    fi

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
    echo -e "${YELLOW}RUN${NC}   $INPUT  (target=$TARGET)"
    START_S=$SECONDS
    TIMEOUT_S="${TIMEOUT_S:-0}"
    ERR_FILE="$(mktemp)"
    if [[ "$TIMEOUT_S" != "0" ]]; then
        ACTUAL=$(timeout "${TIMEOUT_S}s" ./simplify "$INPUT_FILE" "$TARGET" 2>"$ERR_FILE") || {
            rm -f "$ERR_FILE"
            echo -e "${RED}FAIL${NC}  $INPUT -> crashed/timeout"
            FAIL=$((FAIL + 1))
            continue
        }
    else
        ACTUAL=$(./simplify "$INPUT_FILE" "$TARGET" 2>"$ERR_FILE") || {
            rm -f "$ERR_FILE"
            echo -e "${RED}FAIL${NC}  $INPUT -> crashed or returned error"
            FAIL=$((FAIL + 1))
            continue
        }
    fi
    rm -f "$ERR_FILE"
    ELAPSED_S=$((SECONDS - START_S))

    STRICT_COMPARE="${STRICT_COMPARE:-0}"
    if [[ "$STRICT_COMPARE" == "1" ]]; then
        EXPECTED=$(cat "$EXPECTED_FILE")
        if [[ "$ACTUAL" == "$EXPECTED" ]]; then
            echo -e "${GREEN}PASS${NC}  $INPUT  (target=$TARGET, ${ELAPSED_S}s)"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}FAIL${NC}  $INPUT  (target=$TARGET, ${ELAPSED_S}s)"
            echo "      --- expected ---"
            echo "$EXPECTED" | head -6 | sed 's/^/      /'
            echo "      --- got ---"
            echo "$ACTUAL"   | head -6 | sed 's/^/      /'
            FAIL=$((FAIL + 1))
        fi
    else
        ACTUAL_FILE="$(mktemp)"
        printf '%s\n' "$ACTUAL" > "$ACTUAL_FILE"
        if python3 - "$TARGET" "$ACTUAL_FILE" <<'PY'
import re
import sys

target = int(sys.argv[1])
with open(sys.argv[2], "r", encoding="utf-8", errors="replace") as f:
    lines = f.read().splitlines()
if not lines or lines[0].strip() != "ring_id,vertex_id,x,y":
    raise SystemExit(1)

vertex_lines = []
ring_counts = {}
area_in = area_out = None
for line in lines[1:]:
    line = line.strip()
    if not line:
        continue
    if line.startswith("Total signed area in input:"):
        m = re.search(r"([-+0-9.eE]+)$", line)
        area_in = float(m.group(1)) if m else None
        continue
    if line.startswith("Total signed area in output:"):
        m = re.search(r"([-+0-9.eE]+)$", line)
        area_out = float(m.group(1)) if m else None
        continue
    if line.startswith("Total areal displacement:"):
        continue
    try:
        rid_s, _vid_s, _x_s, _y_s = line.split(",", 3)
        rid = int(rid_s)
        ring_counts[rid] = ring_counts.get(rid, 0) + 1
    except Exception:
        raise SystemExit(1)
    vertex_lines.append(line)

if not vertex_lines:
    raise SystemExit(1)
ring_count = len(ring_counts)
min_total = 3 * ring_count
effective_target = max(target, min_total)
if len(vertex_lines) > effective_target:
    raise SystemExit(1)
if any(c < 3 for c in ring_counts.values()):
    raise SystemExit(1)
if area_in is None or area_out is None:
    raise SystemExit(1)

tol = 1e-6 * max(1.0, abs(area_in))
if abs(area_in - area_out) > tol:
    raise SystemExit(1)
PY
        then
            rm -f "$ACTUAL_FILE"
            echo -e "${GREEN}PASS${NC}  $INPUT  (target=$TARGET, ${ELAPSED_S}s)"
            PASS=$((PASS + 1))
        else
            rm -f "$ACTUAL_FILE"
            echo -e "${RED}FAIL${NC}  $INPUT  (target=$TARGET, ${ELAPSED_S}s)"
            echo "      --- got ---"
            echo "$ACTUAL" | head -6 | sed 's/^/      /'
            FAIL=$((FAIL + 1))
        fi
    fi
done

# ---- Summary ----------------------------------------------------------------
echo ""
echo "==============================="
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC} / $TOTAL total"
echo "==============================="
[[ $FAIL -eq 0 ]] && exit 0 || exit 1
