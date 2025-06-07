## Installation

### 1. Cloner le dépôt

Clonez le dépôt et placez-vous à la racine du projet :

```bash
git clone https://github.com/hugoMiCode/ig-trading-bot
cd ig-trading-bot
```

### 3. Installer les dépendances

Installez les dépendances nécessaires à partir du fichier `requirements.txt` :

```bash
pip install -r requirements.txt
```

L’installation en mode développement vous permet de modifier le code source et de voir immédiatement les modifications sans avoir à réinstaller le package.

### 5. Configurer vos identifiants IG

Copiez le fichier `.env-example` en `.env` et renseignez-y vos identifiants et configurations :

```bash
cp .env-example .env
```

Ouvrez ensuite `.env` dans votre éditeur et ajoutez vos informations.

---

## Structure du projet

La structure du projet a été réorganisée pour centraliser le code source dans un package unique et séparer les points d'entrée. Par exemple :

```
ig-trading-bot/
├── setup.py                  # Script d'installation en mode développement
├── README.md
├── requirements.txt
├── igtradingbot/             # Package principal
│   ├── __init__.py
│   ├── Strategies/         # Sous-package pour vos stratégies
│   │   ├── __init__.py
│   │   ├── Strategy.py
│   │   ├── Helpers.py
│   │   ├── BuyTrendFollowingStrategy/
│   │   │   ├── __init__.py
│   │   │   ├── BuyTrendFollowingStrategy.py
│   │   │   └── BacktestAdapter.py
│   │   └── SellTrendFollowingStrategy/
│   │       ├── __init__.py
│   │       ├── SellTrendFollowingStrategy.py
│   │       └── BacktestAdapter.py
│   └── trading_ig/         # Sous-package pour l'intégration avec IG (Broker, API, etc.)
│       ├── __init__.py
│       └── Broker.py
└── scripts/                  # Points d'entrée et scripts divers
    ├── liveIG.py
    ├── backtest_runner.py
    └── main.py
```

Dans vos scripts (dans le dossier `scripts/`), vous utiliserez des imports absolus. Par exemple, dans `liveIG.py` :

```python
from igtradingbot.trading_ig.Broker import Broker
from igtradingbot.Strategies.BuyTrendFollowingStrategy.BuyTrendFollowingStrategy import BuyTrendFollowingStrategy

def main():
    broker = Broker(epic="IX.D.NASDAQ.IFE.IP", working_resolution='1Min')
    strategy = BuyTrendFollowingStrategy()
    # Logique de live trading...
    print("Live trading started")

if __name__ == "__main__":
    main()
```

---

## Utilisation

### En mode live

Pour exécuter le bot en mode live :

```bash
python scripts/liveIG.py
```

# Guide de compilation et d'exécution du projet ig-trading-bot

Ce guide explique comment compiler, exécuter et déboguer le projet ig-trading-bot de manière progressive, en partant des méthodes manuelles jusqu'aux configurations plus avancées.

## Prérequis

- CMake 3.14 ou supérieur
- GCC/G++ avec support C++17
- Qt6.8.3 (Core, Widgets, Charts)
- VS Code (pour le débogage)
- Extensions VS Code: C/C++, CMake Tools

## Méthode 1: Compilation manuelle avec CMake

Cette méthode est la plus basique et fonctionne sur tout système compatible:

```bash
# Créer le dossier de build
mkdir -p build
cd build

# Configurer le projet - mode Release (par défaut)
cmake ..

# Compiler le projet
make -j$(nproc)  # Utilise tous les cœurs disponibles

# Exécuter l'application
./cpp_backtestApp/backtestapp
```

### Compilation en mode Debug

```bash
# Dans le dossier build
cmake -DBUILD_WITH_DEBUG=ON ..
make -j$(nproc)
```

## Méthode 2: Utilisation du script build_and_run.sh

Le script automatise le processus de compilation et d'exécution:

```bash
# Compilation standard et exécution
./build_and_run.sh

# Compilation en mode debug et exécution
./build_and_run.sh --debug

# Nettoyage du dossier build avant compilation
./build_and_run.sh --clean

# Compilation sans exécution
./build_and_run.sh --no-run
```

## Méthode 3: Configuration VS Code pour le débogage

Utiliser les fichiers `.vscode/launch.json` et `./vscode/tasks.json` 

Choisir son système d'exploitation et le mode (Debug/Release) dans l'onglet run/debug de VSCode

**launch.json** 

**tasks.json** 

### Comment utiliser le débogueur

1. Placez des points d'arrêt en cliquant dans la marge à gauche des numéros de ligne
2. Appuyez sur F5 pour lancer le débogueur
3. Utilisez les contrôles de débogage:
   - F10: Pas à pas principal (step over)
   - F11: Pas à pas détaillé (step into)
   - Shift+F11: Sortir de la fonction (step out)
   - F5: Continuer l'exécution

## Points d'entrée du projet

Voici un résumé des différentes façons de compiler et exécuter le projet:

1. **Compilation et exécution manuelles**:
   ```bash
   cmake ..
   make (Linux / macOS)
   cmake --build . --config Release --parallel (Windows)
   ./cpp_backtestApp/backtestapp
   ```

2. **Script automatisé**:
   ```bash
   ./build_and_run.sh
   ```

3. **Compilation avec VS Code**:
   - Ctrl+Shift+B: Lance la tâche de compilation par défaut (build-debug)
   - Terminal > Run Task > build-release: Pour une version optimisée

4. **Débogage avec VS Code**:
   - F5: Lance le débogueur avec les points d'arrêt définis

## Structure du projet

- **CMakeLists.txt**: Fichier de configuration principal du projet
- **build_and_run.sh**: Script d'automatisation de compilation/exécution
- **cpp_backtestApp/**: Application principale avec interface Qt6
- **cpp_backtestEngine/**: Moteur de backtest
- **cpp_strategies/**: Implémentations des stratégies
- **ChartDirector/**: Bibliothèque pour les graphiques

Cette structure permet une séparation claire des composants et facilite la maintenance du code.

## Remarques importantes

- En mode Debug (--debug), l'application est compilée avec les symboles de débogage (-g) et sans optimisations (-O0)
- En mode Release (par défaut), les optimisations (-O3) sont activées pour de meilleures performances
- Le fichier compile_commands.json aide VS Code à comprendre la structure du projet mais n'est pas essentiel à la compilation

