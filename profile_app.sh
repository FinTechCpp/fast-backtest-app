#!/bin/bash

# Detect the operating system
OS=$(uname)

if [ "$OS" = "Linux" ]; then
    echo "Running profiler on Linux..."
    sudo perf record -g --call-graph=dwarf -F 997 ./build/cpp_backtestApp/backtestapp
    sudo hotspot perf.data
elif [ "$OS" = "Darwin" ]; then
    echo "Running profiler on macOS..."
    xcrun xctrace record --template 'Time Profiler' --launch ./build/cpp_backtestApp/backtestapp
    open -a Instruments *.trace
else
    echo "Unsupported operating system: $OS"
    exit 1
fi