import sys
import os

# Add the parent directory of scripts to the path
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(project_root)

# Add the cpp_strategies directory specifically
cpp_strategies_dir = os.path.join(project_root, 'cpp_strategies')
sys.path.append(cpp_strategies_dir)

from backtestApp.app import BacktestApp
from PyQt5.QtWidgets import QApplication
import argparse
import logging
import datetime

import matplotlib as mpl
mpl.set_loglevel('warning')  # Ignore les messages DEBUG (niveau plus bas que WARNING)


# Create backtest logs directory with date
current_date = datetime.datetime.now().strftime("%Y-%m-%d")
log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'logs', 'backtest', current_date)
os.makedirs(log_dir, exist_ok=True)

# Créer un nom de fichier avec horodatage
log_time = datetime.datetime.now().strftime("%H-%M-%S")
log_file = os.path.join(log_dir, f'backtest_{log_time}.log')

# Parser les arguments de ligne de commande
parser = argparse.ArgumentParser(description="Set logging level for the application.")
parser.add_argument("--log-level", type=str, default="WARNING", 
                    help="Set the logging level (e.g., DEBUG, INFO, WARNING, ERROR, CRITICAL).")
parser.add_argument("--console", action="store_true", 
                   help="Also display logs in console.")
args = parser.parse_args()

# Configurer le niveau de log
log_level = getattr(logging, args.log_level.upper(), logging.INFO)

# Configurer les handlers pour le logging
handlers = [logging.FileHandler(log_file)]
if args.console:
    handlers.append(logging.StreamHandler())


# Configuration du système de logs
logging.basicConfig(
    level=log_level,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=handlers
)

# Confirmer la configuration du logging
logging.info(f"Logs configurés dans le fichier: {log_file}")
logging.info(f"Niveau de log: {args.log_level}")

# Configurer le callback pour les logs C++
def cpp_log_callback(message, level):
    """Fonction de callback pour les logs provenant du code C++"""
    if level == 0:  # ERROR
        logging.error(f"C++: {message}")
    elif level == 1:  # INFO
        logging.info(f"C++: {message}")
    elif level == 2:  # WARNING
        logging.warning(f"C++: {message}")
    elif level == 3:  # DEBUG
        logging.debug(f"C++: {message}")
    else:
        logging.info(f"C++: {message}")  # Fallback pour les autres niveaux

# Essayer d'importer et de configurer le callback C++ si disponible
try:
    from cpp_strategies import set_log_callback
    set_log_callback(cpp_log_callback)
    logging.info("Callback C++ configuré avec succès")
except ImportError:
    logging.warning("Module cpp_strategies non disponible, les logs C++ ne seront pas redirigés")
except Exception as e:
    logging.error(f"Erreur lors de la configuration du callback C++: {e}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    logging.info("Démarrage de l'application de backtest")
    window = BacktestApp()
    window.show()
    sys.exit(app.exec_())