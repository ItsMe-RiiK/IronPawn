#!/bin/bash
# run.sh - A helper script to safely compile and run IronPawn

# 1. Kill any existing instances of IronPawn running in the background
echo "🧹 Cleaning up old instances of IronPawn..."
pkill -f IronPawn || true

# 2. Recompile the project
echo "⚙️ Compiling..."
cd build
make -j$(nproc)
cd ..

# 3. Run the newly compiled binary
echo "🚀 Starting IronPawn..."
./build/IronPawn
