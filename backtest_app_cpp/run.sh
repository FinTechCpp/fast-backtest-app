#!/usr/bin/env bash
#
# fichier : /home/hugo/Repositories/Finance/ig-trading-bot/backtest_app_cpp/run.sh
#
# Ce script configure LD_LIBRARY_PATH pour :
#   1) charger en priorité Qt 6.8.3 (venant de $HOME/Qt/6.8.3/gcc_64/lib)
#   2) puis ChartDirector (../ChartDirector/lib)
#   3) puis la librairie Python de l'env Conda (libpython3.11)
#
# On évite ainsi que le Qt de Conda prévale sur Qt 6.8.3.

# 1) Obtenir le répertoire du script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 2) Répertoire lib de Qt 6.8.3
QT6_LIB_DIR="$HOME/Qt/6.8.3/gcc_64/lib"

# 3) Répertoire lib de ChartDirector
CHARTDIRECTOR_LIB_DIR="$SCRIPT_DIR/../ChartDirector/lib"

# 4) Répertoire lib de l'environnement Conda (où se trouve libpython3.11)
PYTHON_LIB_DIR="${CONDA_PREFIX}/lib"

# 5) Construire LD_LIBRARY_PATH dans l'ordre correct :
#    1. Qt 6.8.3
#    2. ChartDirector
#    3. Python (Conda)
#    4. tout le reste déjà en LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$QT6_LIB_DIR:$CHARTDIRECTOR_LIB_DIR:$PYTHON_LIB_DIR:$LD_LIBRARY_PATH"

echo "----- LD_LIBRARY_PATH configuré ----------------"
echo "  Qt 6.8.3 lib dir : $QT6_LIB_DIR"
echo "  ChartDirector lib dir : $CHARTDIRECTOR_LIB_DIR"
echo "  Python (Conda) lib dir : $PYTHON_LIB_DIR"
echo "-------------------------------------------------"
echo

# 6) Vérification des bibliothèques
echo "Vérification des bibliothèques dynamiques :"
if [ -f "$QT6_LIB_DIR/libQt6Core.so.6" ]; then
    echo "  ✓ Qt6Core trouvé : $QT6_LIB_DIR/libQt6Core.so.6"
else
    echo "  ✗ Qt6Core non trouvé dans : $QT6_LIB_DIR"
fi

if [ -f "$PYTHON_LIB_DIR/libpython3.11.so.1.0" ]; then
    echo "  ✓ libpython3.11 trouvé : $PYTHON_LIB_DIR/libpython3.11.so.1.0"
else
    echo "  ✗ libpython3.11 non trouvé dans : $PYTHON_LIB_DIR"
fi

if ls "$CHARTDIRECTOR_LIB_DIR"/libchartdir.so* 1> /dev/null 2>&1; then
    for f in "$CHARTDIRECTOR_LIB_DIR"/libchartdir.so*; do
        echo "  ✓ libchartdir trouvé : $f"
    done
else
    echo "  ✗ libchartdir non trouvé dans : $CHARTDIRECTOR_LIB_DIR"
fi
echo

# 7) Vérifier que l'exécutable existe et est exécutable
EXE="$SCRIPT_DIR/build/backtest_app_cpp"
if [ ! -x "$EXE" ]; then
    echo "Erreur : L'exécutable n'existe pas ou n'est pas exécutable :"
    echo "  $EXE"
    echo "Veuillez compiler d'abord avec buildqt6.sh ou make."
    exit 1
fi

# 8) Lancer l'application avec les arguments passés
echo "Démarrage de l'application : $EXE $*"
"$EXE" "$@"
