#!/bin/bash
LLVM_PROFDATA="$1"
LLVM_COV="$2"
CONCH_BINARY="$3"
COV_DIR="$4"
PROJECT_ROOT="$5"

$LLVM_PROFDATA merge -sparse $COV_DIR/default.profraw -o $COV_DIR/default.profdata

IGNORES="$PROJECT_ROOT/tests/.*|status\.h|status\.c|hash\.h"

$LLVM_COV show $CONCH_BINARY \
    -instr-profile=$COV_DIR/default.profdata \
    -format=html \
    -output-dir=$COV_DIR/coverage_report \
    -ignore-filename-regex="($IGNORES)"
