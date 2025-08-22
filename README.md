# Fast Backtest App

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/c++-17-blue.svg)](https://isocpp.org)
[![Qt](https://img.shields.io/badge/qt-6.8.3-41CD52.svg)](https://qt.io)
[![CMake](https://img.shields.io/badge/cmake-3.14+-064F8C.svg)](https://cmake.org)

## Overview

**Fast Backtest App** is a Qt-based trading backtesting application designed to be as fast as possible while providing a rich backtesting results analysis experience. For that, it leverages a multi-threaded C++ engine for computations and a user-friendly Qt interface for visualizations. Thanks to Qt, this application is cross-platform and can run on Windows, macOS, and Linux.

### ✨ Project Highlights

- **Optimized performance**: Multi-threaded C++ backtest engine
- **Advanced GUI**: Native Qt application with ChartDirector visualizations
- **Modular strategies**: Extensible framework to develop your own algorithms
- **Comprehensive analysis**: Advanced metrics, interactive charts, and detailed histories

### 🎯 Main Features

#### High-performance backtesting
- C++ engine optimized for speed
- Qt GUI with advanced visualizations
- Complete metrics (Sharpe, Sortino, drawdown, etc.)
- Trade analysis and equity curves

#### Monitoring and analysis
- Detailed logging and histories
- Jupyter Notebooks for data fetching, processing, and analysis
- Export results and reports

### 🏗️ Technical Architecture

The project adopts a modular architecture for better code organization and performance:

- **Frontend**: Native Qt6 interface for a smooth user experience
- **Computation engine**: C++17 for backtests and intensive calculations
- **Visualization**: ChartDirector for professional and huge data quantity charts
- **Build system**: Cross-platform CMake with automation scripts

This approach ensures both fast execution for backtests and flexibility for strategy development.

### 📸 Interface Preview

![Application interface](images/app_example.png)

*Qt graphical interface of the backtesting application with ChartDirector visualizations*

---

## Installation

### 1. Clone the repository

Clone the repository and go to the project root:

```bash
git clone https://github.com/FinTechCpp/fast-backtest-app
cd fast-backtest-app
# initialize submodules
git submodule update --init --recursive
git lfs pull
```

## Usage

# Build and Run Guide for fast-backtest-app

This guide explains how to build, run, and debug the fast-backtest-app project step by step, from manual methods to more advanced configurations.

## Prerequisites

- CMake 3.14 or higher
- GCC/G++ with C++17 support
- Qt6.8.3 (Core, Widgets, Charts, Network)
- VS Code (for debugging)
- VS Code Extensions: C/C++, CMake Tools

## 🔧 Prerequisite Installation (Linux)

### Step 1: Install build tools

Install essential development tools:

```bash
# Update packages
sudo apt update

# Install build tools
sudo apt install -y build-essential git curl wget

# Install required graphics libraries for Qt
sudo apt install -y libgl1-mesa-dev libglu1-mesa-dev
sudo apt install -y libxcb-cursor0 libxcb-cursor-dev
```

### Step 2: Install CMake 4.0.3

Download and install CMake manually to get a recent version:

```bash
# Download CMake 4.0.3
cd ~
wget https://github.com/Kitware/CMake/releases/download/v4.0.3/cmake-4.0.3-linux-x86_64.tar.gz

# Extract the archive
tar -xzf cmake-4.0.3-linux-x86_64.tar.gz

# Add CMake to PATH (temporary)
export PATH=$HOME/cmake-4.0.3-linux-x86_64/bin:$PATH

# Check installation
cmake --version
```

### Step 3: Install Qt 6.8.3

Download and install Qt from the official website:

```bash
# Download Qt Online Installer
cd ~
wget https://d13lb3tujbc8s0.cloudfront.net/onlineinstallers/qt-unified-linux-x64-4.6.1-online.run

# Make the installer executable
chmod +x qt-unified-linux-x64-4.6.1-online.run

# Run the installer
./qt-unified-linux-x64-4.6.1-online.run
```

**Qt installer instructions:**
1. Create a Qt account (free for personal use)
2. Select **Custom installation**
3. Select **Qt 6.8.3**
4. Check the fields such as the screenshot following:
5. ![Qt Installer Selection](images/qt_installer_config.png)
6. Install in the default directory: `~/Qt/`

### Step 5: Permanent PATH configuration

Add CMake and Qt to your PATH permanently:

```bash
# Add to .bashrc
echo '# Add cmake and Qt to PATH' >> ~/.bashrc
echo 'export PATH=$HOME/cmake-4.0.3-linux-x86_64/bin:$PATH' >> ~/.bashrc
echo 'export PATH=$HOME/Qt/6.8.3/gcc_64/bin:$PATH' >> ~/.bashrc
echo 'export CMAKE_PREFIX_PATH=$HOME/Qt/6.8.3/gcc_64:$CMAKE_PREFIX_PATH' >> ~/.bashrc

# Reload configuration
source ~/.bashrc
```

### Step 6: Verify installation

Check that all tools are correctly installed:

```bash
# Check CMake
cmake --version

# Check Qt
qmake --version

# Check compilers
gcc --version
g++ --version
```

**Expected results:**
- CMake version 4.0.3
- Qt version 6.8.3
- GCC/G++ version 13.x or higher

### Step 7: Test build

Test building the project:

```bash
# Go to the project directory
cd ~/fast-backtest-app

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)
```

If everything works, you should see:
```
-- Qt6 automatically detected: /home/username/Qt/6.8.3/gcc_64
-- Configuration completed successfully
-- Build files have been written to: /path/to/build
[100%] Built target backtestapp
```

---

## Building and Running the Project

### Method 1: Manual build with CMake

This is the most basic method and works on any compatible system:

```bash
# Create build directory
mkdir -p build
cd build

# Configure project - Release mode (default)
cmake ..

# Build project
make -j$(nproc)  # Uses all available cores

# Run the application
./cpp_backtestApp/backtestapp
```

#### Debug build

```bash
# In the build directory
cmake -DBUILD_WITH_DEBUG=ON ..
make -j$(nproc)
```

### Method 2: Using the **build_and_run.sh** script

The script automates the build and run process:

```bash
# Standard build and run
./build_and_run.sh

# Debug build and run
./build_and_run.sh --debug

# Clean build directory before build
./build_and_run.sh --clean

# Build without running
./build_and_run.sh --no-run
```

### Method 3: VS Code configuration for debugging

Use the files `./vscode/launch.json` and `./vscode/tasks.json`

Choose your OS and mode (Debug/Release) in the VSCode run/debug tab

#### How to use the debugger

1. Set breakpoints by clicking in the margin to the left of the line numbers
2. Press F5 to start the debugger
3. Use the debug controls:
   - F10: Step over
   - F11: Step into
   - Shift+F11: Step out
   - F5: Continue execution

### Project entry points

Here is a summary of the different ways to build and run the project:
1. **Manual build and run**:
   
   **Linux / macOS:**
   ```bash
   cmake ..
   make -j$(nproc)
   ./cpp_backtestApp/backtestapp
   ```
   
   **Windows:**
   ```bash
   cmake ..
   cmake --build . --config Release --parallel
   .\cpp_backtestApp\backtestapp.exe
   ```

2. **Automated script** (Linux/macOS only):
   ```bash
   ./build_and_run.sh
   ```

3. **Build with VS Code**:
   - Ctrl+Shift+B: Launches the default build task (build-debug)
   - Terminal > Run Task > build-release: For an optimized version

4. **Debug with VS Code**:
   - F5: Launches the debugger with defined breakpoints

## Project Structure

```
fast-backtest-app/
├── backtestAdapter/
│   ├── CMakeLists.txt
│   ├── include/
│   └── src/
├── backtestApp/
│   ├── CMakeLists.txt
│   ├── icons/
│   ├── include/
│   ├── qresources.qrc
│   └── src/
├── backtestEngine/
│   ├── CMakeLists.txt
│   ├── include/
│   └── src/
├── build_and_run.sh*
├── CMakeLists.txt
├── Doxyfile
├── images/
├── marketData/
├── Notebooks/
│   ├── Backtest.ipynb
│   ├── Helpers.py
│   ├── IBKR_API.ipynb
│   └── PolygonAPI.ipynb
├── profile_app.sh*
├── README.md
├── ThirdParty/
│   ├── ChartDirector/
│   ├── spdlog/
│   └── Strategies/
└── VERSION
```

### Component Description

#### Python Components
- **`Notebooks/`**: Strategy analysis and development

#### C++ Components
- **`backtestEngine/`**: High-performance backtest engine
- **`backtestApp/`**: Qt GUI for backtests
- **`Strategies/`**: Trading strategies 
- **`backtestAdapter/`**: Specific connections to use strategies in backtest mode
- **`marketData/`**: Historical market data files for backtesting
- **`Notebooks/`**: Jupyter Notebooks for data fetching, processing, and analysis

#### External Dependencies
- **`ChartDirector/`**: Charting library (commercial license)
- **`spdlog/`**: C++ Logging library for ultra-fast and modern logging

---

## Important notes


### Contributors

- **[hugoMiCode](https://github.com/hugoMiCode)** - Co-creator and main maintainer / code architecture specialist
- **[maks7d](https://github.com/maks7d)** - Co-creator / Developer 
- **[alexandre5-0](https://github.com/alexandre5-0/alexandre5-0)** - Beta tester / trading expert and strategy designer
#### How to contribute

1. Fork the project
2. Create a branch for your feature (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

All types of contributions are welcome: bug fixes, new features, documentation improvements, tests, etc.

