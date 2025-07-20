#!/bin/bash

echo "Building LAN Chat application..."

# Create build directory
mkdir -p build

# Run CMake
if ! cmake -S . -B build -DCMAKE_BUILD_TYPE=Release; then
    echo "Error: CMake configuration failed"
    exit 1
fi

# Build the project
if ! cmake --build build --config Release; then
    echo "Error: Build failed"
    exit 1
fi

# Copy executable to root directory for easy access
if [ -f "build/lanchat" ]; then
    cp build/lanchat ./
    echo "✓ Build successful!"
    echo "✓ Executable created: ./lanchat"
    echo ""
    echo "To run the application:"
    echo "  ./lanchat"
    echo "  (or ./lanchat --port 8888 to use a different port)"
    echo ""
    echo "Then open your browser and go to:"
    echo "  http://localhost:8080"
else
    echo "Error: Executable not found after build"
    exit 1
fi