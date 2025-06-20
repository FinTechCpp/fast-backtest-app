#!/bin/bash

# Couleurs pour les messages
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Obtenir le répertoire du script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

# Traitement des arguments
DEBUG=0
CLEAN=0
RUN_APP=1
APP_NAME="backtestapp"  # Application par défaut

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
        --cmd-app)
        APP_NAME="ig_cmd_app"
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
        echo "  --cmd-app       Compiler et exécuter l'application en ligne de commande IG"
        echo "  --backtest-app  Compiler et exécuter l'application de backtest (défaut)"
        echo "  --help, -h      Afficher cette aide"
        echo ""
        echo "Exemples:"
        echo "  $0                     # Compile et lance l'app de backtest"
        echo "  $0 --cmd-app          # Compile et lance l'app IG en ligne de commande"
        echo "  $0 --debug --cmd-app  # Compile en debug et lance l'app IG"
        echo "  $0 --clean --no-run   # Nettoie et compile sans lancer"
        exit 0
        ;;
    esac
done

# Fonction pour afficher les messages d'étape
show_step() {
    echo -e "${BLUE}${BOLD}[ÉTAPE]${NC} $1"
}

# Fonction pour afficher les messages de succès
show_success() {
    echo -e "${GREEN}${BOLD}[SUCCÈS]${NC} $1"
}

# Fonction pour afficher les erreurs
show_error() {
    echo -e "${RED}${BOLD}[ERREUR]${NC} $1"
}

# Nettoyer le dossier de build si demandé
if [ $CLEAN -eq 1 ] && [ -d "build" ]; then
    show_step "Nettoyage du dossier de build..."
    rm -rf build
    show_success "Dossier de build nettoyé."
fi

# Créer le dossier build
show_step "Création du dossier build..."
mkdir -p build
cd build

# Configurer le projet avec CMake
show_step "Configuration du projet avec CMake..."
CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

if [ $DEBUG -eq 1 ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_WITH_DEBUG=ON"
fi

cmake $CMAKE_ARGS .. || { show_error "Échec de la configuration CMake"; exit 1; }

# Compiler le projet
show_step "Compilation du projet avec tous les cœurs disponibles..."
make -j$(nproc) || { show_error "Échec de la compilation"; exit 1; }

show_success "Compilation terminée!"

# Créer le chemin vers l'exécutable selon l'application choisie
if [ "$APP_NAME" = "ig_cmd_app" ]; then
    APP_PATH="./bin/ig_cmd_app"
    APP_DISPLAY_NAME="IG Command Line App"
else
    APP_PATH="./backtestApp/backtestapp"
    APP_DISPLAY_NAME="Backtest App"
fi

# Afficher un message sur comment lancer l'application
echo -e "${YELLOW}${BOLD}[INFO]${NC} Chemin de l'application: ${BOLD}./build${APP_PATH:1}${NC}"
echo -e "${YELLOW}${BOLD}[INFO]${NC} Application sélectionnée: ${BOLD}$APP_DISPLAY_NAME${NC}"

# Lancer l'application si demandé
if [ $RUN_APP -eq 1 ]; then
    show_step "Lancement de $APP_DISPLAY_NAME..."
    if [ -f "$APP_PATH" ]; then
        echo -e "${GREEN}${BOLD}[EXÉCUTION]${NC} $APP_PATH"
        $APP_PATH
    else
        show_error "L'exécutable n'existe pas: $APP_PATH"
        echo "Vérifiez que le chemin est correct et que la compilation a réussi."
        echo "Applications disponibles:"
        echo "  - Backtest App: ./build/backtestApp/backtestapp"
        echo "  - IG Command App: ./build/bin/ig_cmd_app"
        exit 1
    fi
fi

# Retour au répertoire initial
cd ..