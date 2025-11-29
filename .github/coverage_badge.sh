#!/bin/sh
SUMMARY="$1"
OUT="$2"

PERCENT=$(awk '/TOTAL/ {print int($10)}' "$SUMMARY")
echo "Total Coverage: $PERCENT%"
curl -o "$OUT" "https://img.shields.io/badge/Coverage-${PERCENT}%25-pink"
