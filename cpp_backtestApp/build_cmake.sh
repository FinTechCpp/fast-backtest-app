#!/bin/bash

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-cmake"

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Set paths to Qt, Python, and pybind11
QT_DIR="${HOME}/Qt/6.8.3/macos"
PYTHON_DIR="${HOME}/miniconda3/envs/trading"
PYBIND11_DIR="${PYTHON_DIR}/lib/python3.12/site-packages/pybind11"

echo "=== Configuring project with CMake ==="
cmake -DCMAKE_PREFIX_PATH="${QT_DIR}" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DPython3_ROOT_DIR="${PYTHON_DIR}" \
      -DPython3_EXECUTABLE="${PYTHON_DIR}/bin/python3" \
      -Dpybind11_DIR="${PYBIND11_DIR}/share/cmake/pybind11" \
      ..

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed"
    exit 1
fi

echo "=== Building project ==="
cmake --build . -j $(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo "=== Build successful ==="
    echo "Application created: ${BUILD_DIR}/backtest_app_cpp.app"
    
    if [ "$1" = "--run" ]; then
        echo "=== Running application ==="
        open "${BUILD_DIR}/backtest_app_cpp.app"
    fi
else
    echo "=== Build failed ==="
    exit 1
fi