#!/bin/bash
# filepath: /home/hugo/Repositories/Finance/ig-trading-bot/build_and_run.sh

# Définition des couleurs pour une meilleure lisibilité
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
RUN=1

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
        RUN=0
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

# Fonction pour afficher les avertissements
show_warning() {
    echo -e "${YELLOW}${BOLD}[ATTENTION]${NC} $1"
}

# Vérifier si CMake est installé
if ! command -v cmake &> /dev/null; then
    show_error "CMake n'est pas installé. Veuillez l'installer avant de continuer."
    exit 1
fi

# Nettoyer le dossier de build si demandé
if [ $CLEAN -eq 1 ] && [ -d "build" ]; then
    show_step "Nettoyage du dossier de build..."
    rm -rf build
    show_success "Dossier de build nettoyé."
fi

# Créer le dossier build s'il n'existe pas
if [ ! -d "build" ]; then
    show_step "Création du dossier build..."
    mkdir -p build
    show_success "Dossier build créé."
fi

# Accéder au dossier build
cd build

# Configurer le projet avec CMake
show_step "Configuration du projet avec CMake..."
if [ $DEBUG -eq 1 ]; then
    show_warning "Mode DEBUG activé."
    cmake -DBUILD_WITH_DEBUG=ON .. || { show_error "La configuration CMake a échoué."; exit 1; }
else
    cmake .. || { show_error "La configuration CMake a échoué."; exit 1; }
fi
show_success "Configuration CMake terminée."

# Compiler le projet
show_step "Compilation du projet..."
make -j$(nproc) || { show_error "La compilation a échoué."; exit 1; }
show_success "Compilation terminée."

# Exécuter le programme si demandé
if [ $RUN -eq 1 ]; then
    show_step "Exécution du programme..."
    echo -e "${BOLD}----------------------------------------${NC}"
    ./cpp_adaptator/main
    EXIT_CODE=$?
    echo -e "${BOLD}----------------------------------------${NC}"
    
    if [ $EXIT_CODE -eq 0 ]; then
        show_success "Programme exécuté avec succès (code de sortie: $EXIT_CODE)."
    else
        show_error "Programme terminé avec des erreurs (code de sortie: $EXIT_CODE)."
    fi
else
    show_warning "Exécution ignorée (--no-run spécifié)."
fi

# Retour au répertoire initial
cd "$SCRIPT_DIR"