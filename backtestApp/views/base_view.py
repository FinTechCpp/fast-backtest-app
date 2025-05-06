from PyQt5.QtWidgets import QWidget
from abc import ABC, abstractmethod

class ResultView(ABC):
    """Classe de base pour toutes les vues de résultats du backtest."""
    
    def __init__(self, parent=None):
        """Initialise la vue avec une référence à l'application parent."""
        self.parent = parent
        self.widgets = {}
    
    @abstractmethod
    def create(self) -> QWidget:
        """Crée et retourne le widget principal de la vue."""
        raise NotImplementedError("Les classes dérivées doivent implémenter cette méthode")
    
    @abstractmethod
    def update(self, data=None, stats=None):
        """Met à jour la vue avec les nouvelles données."""
        raise NotImplementedError("Les classes dérivées doivent implémenter cette méthode")
    
    def clear(self):
        """Réinitialise la vue à son état initial."""
        pass