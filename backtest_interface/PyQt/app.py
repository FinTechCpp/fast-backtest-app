import sys
import logging
import os
from datetime import datetime
import argparse

from backtest_app import BacktestApp
from PyQt5.QtWidgets import QApplication

# Création du dossier logs s'il n'existe pas
os.makedirs("logs", exist_ok=True)

parser = argparse.ArgumentParser(description="Set logging level for the application.")
parser.add_argument("--log-level", type=str, default="WARNING", 
                    help="Set the logging level (e.g., DEBUG, INFO, WARNING, ERROR, CRITICAL).")
parser.add_argument("--log-file", type=str, 
                    help="Specify log file path. Default: logs/backtest_TIMESTAMP.log")
args = parser.parse_args()

# Définir le nom du fichier de log
if args.log_file:
    log_file = args.log_file
else:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = os.path.join("logs", f"backtest_{timestamp}.log")

# Configuration du niveau de log
log_level = getattr(logging, args.log_level.upper(), logging.INFO)

# Configuration du logging pour envoyer les logs à la fois vers la console et le fichier
logging.basicConfig(
    level=log_level,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(log_file),
        logging.StreamHandler()
    ]
)

logging.info(f"Logs enregistrés dans: {log_file}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = BacktestApp()
    window.show()
    sys.exit(app.exec_())