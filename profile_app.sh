#!/bin/bash

# Detect the operating system
OS=$(uname)

if [ "$OS" = "Linux" ]; then
    echo "Running profiler on Linux..."
    APP_PATH="./build-debug/backtestApp/backtestapp"
    PERF_DATA_FILE="perf.data"

    # Prefer running perf as current user when allowed by kernel settings.
    # Fallback to sudo while preserving user session/config environment.
    if perf record -g --call-graph=dwarf -F 1999 -o "$PERF_DATA_FILE" "$APP_PATH"; then
        echo "perf record completed without sudo"
    else
        echo "perf record requires elevated permissions, retrying with sudo..."
        sudo --preserve-env=HOME,DISPLAY,XAUTHORITY,DBUS_SESSION_BUS_ADDRESS,XDG_RUNTIME_DIR,XDG_CONFIG_HOME,XDG_DATA_HOME \
            perf record -g --call-graph=dwarf -F 1999 -o "$PERF_DATA_FILE" "$APP_PATH"
    fi

    # Keep Hotspot in user context so GUI/session integration stays functional.
    hotspot "$PERF_DATA_FILE"
elif [ "$OS" = "Darwin" ]; then
    echo "Running profiler on macOS..."
    xcrun xctrace record --template 'Time Profiler' --launch ./build/backtestApp/backtestapp
    open -a Instruments *.trace
else
    echo "Unsupported operating system: $OS"
    exit 1
fi