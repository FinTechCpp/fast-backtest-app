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
FRONTEND=1
RUN_APP=1  # Nouvelle option pour lancer l'application

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
        --no-frontend)
        FRONTEND=0
        shift
        ;;
        --no-run)
        RUN_APP=0
        shift
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
CMAKE_ARGS=""

if [ $DEBUG -eq 1 ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_WITH_DEBUG=ON"
fi

if [ $FRONTEND -eq 0 ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_FRONTEND=OFF"
fi

cmake $CMAKE_ARGS .. || { show_error "Échec de la configuration CMake"; exit 1; }

# Compiler le projet
show_step "Compilation du projet..."
make -j$(nproc) || { show_error "Échec de la compilation"; exit 1; }

show_success "Compilation terminée!"

# Créer le chemin vers l'exécutable
APP_PATH="./cpp_backtestApp/backtestapp"

# Afficher un message sur comment lancer l'application
if [ $FRONTEND -eq 1 ]; then
    echo -e "${YELLOW}${BOLD}[INFO]${NC} Chemin de l'application: ${BOLD}$APP_PATH${NC}"
    
    # Lancer l'application si demandé
    if [ $RUN_APP -eq 1 ]; then
        show_step "Lancement de l'application..."
        if [ -f "$APP_PATH" ]; then
            echo -e "${GREEN}${BOLD}[EXÉCUTION]${NC} $APP_PATH"
            $APP_PATH
        else
            show_error "L'exécutable n'existe pas: $APP_PATH"
            echo "Vérifiez que le chemin est correct et que la compilation a réussi."
            exit 1
        fi
    fi
fi

# Retour au répertoire initial
cd ..