#!/bin/bash
# Script de compilation et d'exécution de la démo
# filepath: /home/hugo/Repositories/Finance/ig-trading-bot/cpp_backtestEngine/build_and_run_demo.sh

# Couleurs pour les messages
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Obtenir le répertoire du script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

# Créer le dossier build s'il n'existe pas
if [ ! -d "build" ]; then
    echo -e "${GREEN}Création du dossier build...${NC}"
    mkdir -p build
fi

# Naviguer vers le dossier build
cd build

# Exécuter CMake pour générer les fichiers de build
echo -e "${GREEN}Génération des fichiers de build avec CMake...${NC}"
cmake .. || { echo -e "${RED}Erreur lors de la génération des fichiers de build${NC}"; exit 1; }

# Compiler la bibliothèque et la démo
echo -e "${GREEN}Compilation de la bibliothèque et de la démo...${NC}"
cmake --build . || { echo -e "${RED}Erreur lors de la compilation${NC}"; exit 1; }

# Exécuter la démo
echo -e "${YELLOW}Exécution de la démo de backtest...${NC}"
./backtest_demo

echo -e "${GREEN}Démonstration terminée!${NC}"