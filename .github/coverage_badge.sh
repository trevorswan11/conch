#!/bin/bash
LLVM_COV="$1"
CONCH_BINARY="$2"
COV_DIR="$3"
PROJECT_ROOT="$4"

SUMMARY=$COV_DIR/coverage_summary.txt
OUT=$COV_DIR/coverage.svg

IGNORES="$PROJECT_ROOT/tests/.*"

$LLVM_COV report $CONCH_BINARY \
    -instr-profile=$COV_DIR/default.profdata \
    -ignore-filename-regex="($IGNORES)" \
    > $SUMMARY

PERCENT=$(awk '/TOTAL/ {print int($10)}' "$SUMMARY")
echo "Total Coverage: $PERCENT%"
curl -o "$OUT" "https://img.shields.io/badge/Coverage-${PERCENT}%25-pink"
