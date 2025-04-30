from PyQt5.QtWidgets import QGroupBox, QVBoxLayout
from abc import ABC, abstractmethod

class BasePanel(ABC):
    """Classe de base pour tous les panels de l'application."""
    
    def __init__(self, parent=None):
        self.parent = parent
        self.widgets = {}
    
    @abstractmethod
    def create(self) -> QGroupBox:
        """Méthode à implémenter par les classes dérivées pour créer le panel."""
        raise NotImplementedError("Les classes dérivées doivent implémenter cette méthode")
    
    def get_values(self):
        """Récupère les valeurs des widgets du panel sous forme de dictionnaire."""
        return {}
    
    def set_values(self, values):
        """Définit les valeurs des widgets du panel à partir d'un dictionnaire."""
        pass