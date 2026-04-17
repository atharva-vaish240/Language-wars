#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Ensure the generator binary exists
if [ ! -f "./generator" ]; then
    echo "Error: './generator' binary not found in the current directory."
    echo "Please compile it first: rustc -O generator.rs"
    exit 1
fi

echo "🚀 Starting test generation..."

# Create a dedicated directory for the test files
mkdir -p tests

# Generate complex test cases
echo "Generating 10MB..."
./generator 10MB tests/test_01_10mb.txt

echo "Generating 20MB..."
./generator 20MB tests/test_02_20mb.txt

echo "Generating 40MB..."
./generator 40MB tests/test_03_40mb.txt

echo "Generating 500MB..."
./generator 500MB tests/test_04_500mb.txt

# Generate a generic test case for variation
echo "Generating 10MB (Generic)..."
./generator 10MB tests/test_05_10mb_generic.txt --generic

echo "✅ All test cases generated successfully in the ./tests/ directory."
