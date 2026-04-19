#!/bin/bash
# Build the SqlEcho Java example into a JAR.
# Usage: ./build.sh
#
# Prerequisites: javac and jar on PATH (JDK 11+).
# Output: sqlecho.jar in the same directory as this script.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
OUT_DIR="$SCRIPT_DIR/out"
JAR_NAME="sqlecho.jar"

echo "Compiling SqlEcho..."
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

javac -d "$OUT_DIR" \
    "$SRC_DIR/com/xwhqsj/example/SqlEcho.java"

echo "Packaging $JAR_NAME..."
jar cf "$SCRIPT_DIR/$JAR_NAME" -C "$OUT_DIR" .

echo "Done: $SCRIPT_DIR/$JAR_NAME"
echo ""
echo "Test it:"
echo "  java -cp $SCRIPT_DIR/$JAR_NAME com.xwhqsj.example.SqlEcho"
echo ""
echo "Use with CallJvm:"
echo "  export CALLJVM_CLASSPATH=$SCRIPT_DIR/$JAR_NAME"
