#!/bin/bash
# filepath: /home/hugo/Repositories/Finance/ig-trading-bot/backtest_app_cpp/run.sh

# Obtenir le répertoire du script (pour fonctionner depuis n'importe où)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Obtenir les chemins des librairies Python Conda
PYTHON_CONFIG_PATH=$(python3-config --ldflags | grep -o '\-L[^ ]*' | head -1 | sed 's/-L//')
PYTHON_LIB_PATH=$(python3-config --ldflags | grep -o '\-L[^ ]*' | tail -1 | sed 's/-L//')

# Définir les chemins des librairies avec chemins absolus
export LD_LIBRARY_PATH="$SCRIPT_DIR/../ChartDirector/lib:$PYTHON_LIB_PATH:$PYTHON_CONFIG_PATH:$LD_LIBRARY_PATH"

echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

# Vérification des librairies avec chemins absolus
echo "Vérification des librairies:"
ls -la $PYTHON_LIB_PATH/libpython3.11.so* 2>/dev/null || echo "Librairie Python non trouvée"
ls -la "$SCRIPT_DIR/../ChartDirector/lib/libchartdir.so*" 2>/dev/null || echo "Librairie ChartDirector non trouvée"

# Vérifier que l'exécutable existe
if [ ! -f "$SCRIPT_DIR/build/backtest_app_cpp" ]; then
    echo "Erreur: L'exécutable n'existe pas. Compilez d'abord avec ./build.sh"
    exit 1
fi

# Lancer l'application avec le chemin absolu
"$SCRIPT_DIR/build/backtest_app_cpp" "$@"