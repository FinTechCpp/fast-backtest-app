#!/bin/bash

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/backtest_app_cpp"
BUILD_DIR="$PROJECT_DIR/build"
QT_VERSION="5"

echo "=== Build du projet Backtest App C++ ==="

# Vérifications préliminaires
echo "Vérification des dépendances..."

# Vérifier Qt
if ! command -v qmake-qt${QT_VERSION} &> /dev/null && ! command -v qmake &> /dev/null; then
    echo "Erreur: qmake non trouvé. Installez Qt${QT_VERSION}."
    exit 1
fi

# Vérifier Python et pybind11
if ! python3 -c "import pybind11" &> /dev/null; then
    echo "Erreur: pybind11 non trouvé. Installez avec: pip install pybind11"
    exit 1
fi

# Vérifier ChartDirector
if [ ! -f "$PROJECT_DIR/../ChartDirector/lib/libchartdir.so" ]; then
    echo "Erreur: Librairie ChartDirector non trouvée"
    exit 1
fi

# CORRECTION: Vérifier qchartviewer.h dans le bon répertoire
if [ ! -f "$PROJECT_DIR/../ChartDirector/qtdemo/qtdemo/qchartviewer.h" ]; then
    echo "Erreur: qchartviewer.h non trouvé dans ChartDirector/qtdemo/qtdemo/"
    exit 1
fi

# Créer le répertoire de build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Nettoyer le build précédent
echo "Nettoyage du build précédent..."
rm -f Makefile *.o moc_* backtest_app_cpp

# Générer le Makefile avec qmake
echo "Génération du Makefile..."
if command -v qmake-qt${QT_VERSION} &> /dev/null; then
    qmake-qt${QT_VERSION} "$PROJECT_DIR/backtest_app_cpp.pro"
elif command -v qmake &> /dev/null; then
    qmake "$PROJECT_DIR/backtest_app_cpp.pro"
else
    echo "Erreur: Aucune version de qmake trouvée"
    exit 1
fi

# Après qmake, vérifier les fichiers MOC générés
echo "Vérification des fichiers MOC..."
ls -la moc_*.cpp 2>/dev/null || echo "Aucun fichier MOC trouvé"

# Forcer la régénération des MOC si nécessaire
find .. -name "*.h" -exec grep -l "Q_OBJECT" {} \; | while read file; do
    echo "Fichier avec Q_OBJECT: $file"
done

if [ $? -ne 0 ]; then
    echo "Erreur lors de la génération du Makefile"
    exit 1
fi

# Compiler
echo "Compilation..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "=== Compilation réussie ==="
    echo "Exécutable créé: $BUILD_DIR/backtest_app_cpp"
    
    if [ "$1" = "--run" ]; then
        echo "Lancement de l'application..."
        export LD_LIBRARY_PATH="$PROJECT_DIR/../ChartDirector/lib:$LD_LIBRARY_PATH"
        ./backtest_app_cpp
    fi
else
    echo "=== Erreur de compilation ==="
    exit 1
fi