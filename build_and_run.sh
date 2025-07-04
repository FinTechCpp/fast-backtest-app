#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

# Process command line arguments
DEBUG=0
CLEAN=0
RUN_APP=1
APP_NAME="backtestapp"  # Default application

for arg in "$@"
do
    case $arg in
        --debug)
        DEBUG=1
        shift
        ;;
        --clean)
        CLEAN=1
        shift
        ;;
        --no-run)
        RUN_APP=0
        shift
        ;;
        --backtest-app)
        APP_NAME="backtestapp"
        shift
        ;;
        --help|-h)
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --debug         Compiler en mode debug"
        echo "  --clean         Nettoyer le dossier build avant compilation"
        echo "  --no-run        Ne pas exécuter l'application après compilation"
        echo "  --backtest-app  Compiler et exécuter l'application de backtest (défaut)"
        echo "  --help, -h      Afficher cette aide"
        echo ""
        echo "Exemples:"
        echo "  $0                     # Compile et lance l'app de backtest"
        echo "  $0 --clean --no-run   # Nettoie et compile sans lancer"
        exit 0
        ;;
    esac
done

# Function to display step messages
show_step() {
    echo -e "${BLUE}${BOLD}[ÉTAPE]${NC} $1"
}

# Function to display success messages
show_success() {
    echo -e "${GREEN}${BOLD}[SUCCÈS]${NC} $1"
}

# Function to display error messages
show_error() {
    echo -e "${RED}${BOLD}[ERREUR]${NC} $1"
}

# Clean the build directory if requested
if [ $CLEAN -eq 1 ] && [ -d "build" ]; then
    show_step "Cleaning build directory..."
    rm -rf build
    show_success "Build directory cleaned."
fi

# Create the build directory
show_step "Creating build directory..."
mkdir -p build
cd build

# Configure the project with CMake
show_step "Configuring project with CMake..."
CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

if [ $DEBUG -eq 1 ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_WITH_DEBUG=ON"
fi

cmake $CMAKE_ARGS .. || { show_error "Échec de la configuration CMake"; exit 1; }

# Compile the project
show_step "Compiling project with all available cores..."
if command -v nproc &> /dev/null; then
    CORES=$(nproc)
else
    CORES=$(sysctl -n hw.ncpu)
fi
make -j"$CORES" || { show_error "Compilation failed"; exit 1; }

show_success "Compilation completed successfully!"

APP_PATH="./backtestApp/backtestapp"
APP_DISPLAY_NAME="Backtest App"

# Display a message on how to launch the application
echo -e "${YELLOW}${BOLD}[INFO]${NC} Application path: ${BOLD}./build${APP_PATH:1}${NC}"
echo -e "${YELLOW}${BOLD}[INFO]${NC} Selected application: ${BOLD}$APP_DISPLAY_NAME${NC}"

# Launch the application if requested
if [ $RUN_APP -eq 1 ]; then
    show_step "Launching $APP_DISPLAY_NAME..."
    if [ -f "$APP_PATH" ]; then
        echo -e "${GREEN}${BOLD}[EXECUTION]${NC} $APP_PATH"
        $APP_PATH
    else
        show_error "Executable not found: $APP_PATH"
        echo "Check that the path is correct and that the compilation was successful."
        echo "vailable applications:"
        echo "  - Backtest App: ./build/backtestApp/backtestapp"
        echo "  - IG Command App: ./build/bin/ig_cmd_app"
        exit 1
    fi
fi

# Back to the initial directory
cd ..