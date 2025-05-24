#!/bin/bash
# filepath: /home/hugo/Repositories/Finance/ig-trading-bot/backtest_app_cpp/run.sh

# Obtenir les chemins des librairies Python Conda
PYTHON_CONFIG_PATH=$(python3-config --ldflags | grep -o '\-L[^ ]*' | head -1 | sed 's/-L//')
PYTHON_LIB_PATH=$(python3-config --ldflags | grep -o '\-L[^ ]*' | tail -1 | sed 's/-L//')

# Définir les chemins des librairies
export LD_LIBRARY_PATH="$(dirname "$0")/../ChartDirector/lib:$PYTHON_LIB_PATH:$PYTHON_CONFIG_PATH:$LD_LIBRARY_PATH"

echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

# Vérifier que les librairies existent
echo "Vérification des librairies:"
ls -la $PYTHON_LIB_PATH/libpython3.11.so*
ls -la "$(dirname "$0")/../ChartDirector/lib/libchartdir.so*"

# Lancer l'application avec tous les arguments passés
./build/backtest_app_cpp "$@"