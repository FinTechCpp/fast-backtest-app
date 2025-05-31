# Guide de compilation et d'exécution du projet ig-trading-bot

Ce guide explique comment compiler, exécuter et déboguer le projet ig-trading-bot de manière progressive, en partant des méthodes manuelles jusqu'aux configurations plus avancées.

## Prérequis

- CMake 3.14 ou supérieur
- GCC/G++ avec support C++17
- Qt6 (Core, Widgets, Charts)
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

### Étape 1: Configuration d'IntelliSense

Pour que VS Code reconnaisse correctement les fichiers d'en-tête:

```bash
# Générer le fichier compile_commands.json
mkdir -p build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cd ..
ln -sf build/compile_commands.json .
```

### Étape 2: Créer les fichiers de configuration VS Code

Créez un dossier .vscode à la racine du projet et ajoutez les fichiers suivants:

**launch.json** pour la configuration du débogueur:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Déboguer ig-trading-bot",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/cpp_backtestApp/backtestapp",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Activer l'affichage amélioré pour gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "build-debug",
            "miDebuggerPath": "/usr/bin/gdb"
        }
    ]
}
```

**tasks.json** pour automatiser les tâches de compilation:
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build-debug",
            "type": "shell",
            "command": "${workspaceFolder}/build_and_run.sh",
            "args": [
                "--debug",
                "--no-run"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": {
                "owner": "cpp",
                "fileLocation": ["relative", "${workspaceFolder}"],
                "pattern": {
                    "regexp": "^(.*):(\\d+):(\\d+):\\s+(warning|error):\\s+(.*)$",
                    "file": 1,
                    "line": 2,
                    "column": 3,
                    "severity": 4,
                    "message": 5
                }
            }
        },
        {
            "label": "build-release",
            "type": "shell",
            "command": "${workspaceFolder}/build_and_run.sh",
            "args": ["--no-run"],
            "group": "build"
        }
    ]
}
```

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
   make
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
