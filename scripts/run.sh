#!/bin/bash

echo "Starting LAN Chat application..."

# Check if executable exists
if [ ! -f "./lanchat" ]; then
    echo "Error: lanchat executable not found!"
    echo "Please run ./scripts/build.sh first to build the application."
    exit 1
fi

# Make sure the executable is executable
chmod +x ./lanchat

# Create data directory if it doesn't exist
mkdir -p data

# Check if port is already in use
PORT=8080
if [ "$1" = "--port" ] && [ -n "$2" ]; then
    PORT=$2
fi

# Check if port is in use
if command -v netstat >/dev/null 2>&1; then
    if netstat -tuln | grep ":$PORT " >/dev/null 2>&1; then
        echo "Warning: Port $PORT appears to be in use."
        echo "Use --port <number> to specify a different port."
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
fi

echo "Starting server on port $PORT..."
echo "Press Ctrl+C to stop the server."
echo ""
echo "Open your browser and go to:"
echo "  http://localhost:$PORT"
echo ""

# Start the application
./lanchat "$@"