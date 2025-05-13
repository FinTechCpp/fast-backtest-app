import sys
import logging
logging.basicConfig(level=logging.WARNING, format='%(asctime)s - %(levelname)s - %(message)s')

# from app import BacktestApp
from backtestApp.app import BacktestApp
from PyQt5.QtWidgets import QApplication
import argparse

parser = argparse.ArgumentParser(description="Set logging level for the application.")
parser.add_argument("--log-level", type=str, default="WARNING", 
                    help="Set the logging level (e.g., DEBUG, INFO, WARNING, ERROR, CRITICAL).")
args = parser.parse_args()

log_level = getattr(logging, args.log_level.upper(), logging.INFO)
logging.getLogger().setLevel(log_level)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = BacktestApp()
    window.show()
    sys.exit(app.exec_())