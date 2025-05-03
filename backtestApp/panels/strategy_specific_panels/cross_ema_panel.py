from PyQt5.QtWidgets import (QGroupBox, QVBoxLayout, QGridLayout, QSpinBox, QLabel)
from ..base_panel import BasePanel

class CrossEMAPanel(BasePanel):
    """Panel spécifique à la stratégie CrossEMA."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
    
    def create(self):
        """Crée le panel des paramètres spécifiques à CrossEMA."""
        strategy_group = QGroupBox("Paramètres CrossEMA")
        strategy_layout = QVBoxLayout()
        
        # Configuration des indicateurs EMA
        ema_group = QGroupBox("Configuration des EMA")
        ema_layout = QGridLayout()
        
        # EMA court
        ema_layout.addWidget(QLabel("Période EMA court:"), 0, 0)
        self.widgets['ema_short_period'] = QSpinBox()
        self.widgets['ema_short_period'].setRange(5, 500)
        self.widgets['ema_short_period'].setSingleStep(1)
        self.widgets['ema_short_period'].setValue(50)  # Valeur par défaut
        self.widgets['ema_short_period'].setToolTip("Période de l'EMA court")
        ema_layout.addWidget(self.widgets['ema_short_period'], 0, 1)
        
        # EMA long
        ema_layout.addWidget(QLabel("Période EMA long:"), 1, 0)
        self.widgets['ema_long_period'] = QSpinBox()
        self.widgets['ema_long_period'].setRange(20, 1000)
        self.widgets['ema_long_period'].setSingleStep(5)
        self.widgets['ema_long_period'].setValue(200)  # Valeur par défaut
        self.widgets['ema_long_period'].setToolTip("Période de l'EMA long")
        ema_layout.addWidget(self.widgets['ema_long_period'], 1, 1)
        
        # Ajout d'une validation pour s'assurer que EMA long > EMA court
        self.widgets['ema_short_period'].valueChanged.connect(self.validate_ema_periods)
        self.widgets['ema_long_period'].valueChanged.connect(self.validate_ema_periods)
        
        ema_group.setLayout(ema_layout)
        strategy_layout.addWidget(ema_group)
        
        # Section d'aide/information sur la stratégie
        help_group = QGroupBox("Informations sur la stratégie")
        help_layout = QVBoxLayout()
        help_text = QLabel("""La stratégie CrossEMA génère un signal d'achat lorsque l'EMA court 
croise l'EMA long à la hausse, et un signal de vente lorsque l'EMA court 
croise l'EMA long à la baisse.

Pour un meilleur fonctionnement:
- L'EMA court doit être significativement plus petit que l'EMA long
- Les périodes typiques sont 50/200 pour les tendances longues
- Ou 9/21 pour les mouvements à court terme""")
        help_text.setWordWrap(True)
        help_layout.addWidget(help_text)
        help_group.setLayout(help_layout)
        strategy_layout.addWidget(help_group)
        
        # Ajouter un espace extensible en bas
        strategy_layout.addStretch()
        
        strategy_group.setLayout(strategy_layout)
        return strategy_group
    
    def validate_ema_periods(self):
        """Vérifie que l'EMA long est toujours supérieur à l'EMA court."""
        if self.widgets['ema_short_period'].value() >= self.widgets['ema_long_period'].value():
            # Ajuster l'EMA long pour qu'il soit au moins supérieur de 5 à l'EMA court
            self.widgets['ema_long_period'].setValue(self.widgets['ema_short_period'].value() + 5)
    
    def get_values(self):
        """Récupère les valeurs des widgets du panel."""
        return {
            'ema_short_period': self.widgets['ema_short_period'].value(),
            'ema_long_period': self.widgets['ema_long_period'].value()
        }
    
    def set_values(self, values):
        """Définit les valeurs des widgets du panel."""
        if 'ema_short_period' in values:
            self.widgets['ema_short_period'].setValue(values['ema_short_period'])
        
        if 'ema_long_period' in values:
            self.widgets['ema_long_period'].setValue(values['ema_long_period'])