from PyQt5.QtWidgets import (QGroupBox, QVBoxLayout, QGridLayout, QCheckBox, 
                           QSpinBox, QLabel, QFrame, QHBoxLayout)
from ..base_panel import BasePanel

class BuyHeikinGreenPanel(BasePanel):
    """Panel spécifique à la stratégie BuyHeikinGreen."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
    
    def create(self):
        """Crée le panel des paramètres spécifiques à BuyHeikinGreen."""
        strategy_group = QGroupBox("Paramètres BuyHeikinGreen")
        strategy_layout = QVBoxLayout()
        
        # Section EMA Court
        ema_short_group = QGroupBox("EMA Court")
        ema_short_layout = QVBoxLayout()
        
        # Checkbox pour activer/désactiver le filtre EMA court
        self.widgets['ema_short_filter_check'] = QCheckBox("Activer le filtre EMA court")
        self.widgets['ema_short_filter_check'].setChecked(True)
        ema_short_layout.addWidget(self.widgets['ema_short_filter_check'])
        
        # Frame pour les paramètres de l'EMA court
        ema_short_params = QFrame()
        ema_short_params_layout = QHBoxLayout()
        ema_short_params_layout.addWidget(QLabel("Période:"))
        self.widgets['ema_short_spin'] = QSpinBox()
        self.widgets['ema_short_spin'].setRange(1, 500)
        self.widgets['ema_short_spin'].setValue(20)
        ema_short_params_layout.addWidget(self.widgets['ema_short_spin'])
        ema_short_params_layout.addStretch()
        ema_short_params.setLayout(ema_short_params_layout)
        ema_short_layout.addWidget(ema_short_params)
        
        ema_short_group.setLayout(ema_short_layout)
        strategy_layout.addWidget(ema_short_group)
        
        # Section EMA Long
        ema_long_group = QGroupBox("EMA Long")
        ema_long_layout = QVBoxLayout()
        
        # Checkbox pour activer/désactiver le filtre EMA long
        self.widgets['ema_long_filter_check'] = QCheckBox("Activer le filtre EMA long")
        self.widgets['ema_long_filter_check'].setChecked(True)
        ema_long_layout.addWidget(self.widgets['ema_long_filter_check'])
        
        # Frame pour les paramètres de l'EMA long
        ema_long_params = QFrame()
        ema_long_params_layout = QHBoxLayout()
        ema_long_params_layout.addWidget(QLabel("Période:"))
        self.widgets['ema_long_spin'] = QSpinBox()
        self.widgets['ema_long_spin'].setRange(1, 500)
        self.widgets['ema_long_spin'].setValue(200)
        ema_long_params_layout.addWidget(self.widgets['ema_long_spin'])
        ema_long_params_layout.addStretch()
        ema_long_params.setLayout(ema_long_params_layout)
        ema_long_layout.addWidget(ema_long_params)
        
        ema_long_group.setLayout(ema_long_layout)
        strategy_layout.addWidget(ema_long_group)
        
        # Section Stochastique
        stoch_group = QGroupBox("Stochastique")
        stoch_layout = QVBoxLayout()
        
        # Checkbox pour activer/désactiver le filtre Stochastique
        self.widgets['stoch_filter_check'] = QCheckBox("Activer le filtre Stochastique")
        self.widgets['stoch_filter_check'].setChecked(True)
        stoch_layout.addWidget(self.widgets['stoch_filter_check'])
        
        # Frame pour les paramètres du Stochastique
        stoch_params = QFrame()
        stoch_params_layout = QGridLayout()
        
        stoch_params_layout.addWidget(QLabel("Fast %K:"), 0, 0)
        self.widgets['fastk_spin'] = QSpinBox()
        self.widgets['fastk_spin'].setRange(1, 100)
        self.widgets['fastk_spin'].setValue(10)
        stoch_params_layout.addWidget(self.widgets['fastk_spin'], 0, 1)
        
        stoch_params_layout.addWidget(QLabel("Slow %K:"), 1, 0)
        self.widgets['slowk_spin'] = QSpinBox()
        self.widgets['slowk_spin'].setRange(1, 100)
        self.widgets['slowk_spin'].setValue(10)
        stoch_params_layout.addWidget(self.widgets['slowk_spin'], 1, 1)
        
        stoch_params_layout.addWidget(QLabel("Slow %D:"), 2, 0)
        self.widgets['slowd_spin'] = QSpinBox()
        self.widgets['slowd_spin'].setRange(1, 100)
        self.widgets['slowd_spin'].setValue(3)
        stoch_params_layout.addWidget(self.widgets['slowd_spin'], 2, 1)
        
        stoch_params_layout.addWidget(QLabel("Seuil filtre %K:"), 3, 0)
        self.widgets['stoch_threshold_spin'] = QSpinBox()
        self.widgets['stoch_threshold_spin'].setRange(1, 99)
        self.widgets['stoch_threshold_spin'].setValue(50)
        stoch_params_layout.addWidget(self.widgets['stoch_threshold_spin'], 3, 1)
        
        stoch_params.setLayout(stoch_params_layout)
        stoch_layout.addWidget(stoch_params)
        
        stoch_group.setLayout(stoch_layout)
        strategy_layout.addWidget(stoch_group)
        
        # Section pour le filtre de bougie Heikin Ashi précédente rouge
        ha_group = QGroupBox("Condition bougie Heikin Ashi")
        ha_layout = QVBoxLayout()
        
        self.widgets['previous_ha_candle_red_check'] = QCheckBox("Activer le filtre bougie précédente rouge")
        self.widgets['previous_ha_candle_red_check'].setChecked(True)
        ha_layout.addWidget(self.widgets['previous_ha_candle_red_check'])
        
        ha_group.setLayout(ha_layout)
        strategy_layout.addWidget(ha_group)
        
        # Connexion des signaux pour activer/désactiver les widgets en fonction des checkboxes
        self.widgets['ema_short_filter_check'].toggled.connect(
            lambda checked: self._toggle_widget_group([self.widgets['ema_short_spin']], checked))
        
        self.widgets['ema_long_filter_check'].toggled.connect(
            lambda checked: self._toggle_widget_group([self.widgets['ema_long_spin']], checked))
        
        self.widgets['stoch_filter_check'].toggled.connect(
            lambda checked: self._toggle_widget_group([
                self.widgets['fastk_spin'], 
                self.widgets['slowk_spin'], 
                self.widgets['slowd_spin'], 
                self.widgets['stoch_threshold_spin']
            ], checked))
        
        strategy_group.setLayout(strategy_layout)
        return strategy_group
    
    def _toggle_widget_group(self, widgets, enabled):
        """Active/désactive un groupe de widgets."""
        for widget in widgets:
            widget.setEnabled(enabled)
    
    def get_values(self):
        """Récupère les valeurs des widgets du panel."""
        #TODO : Il serait bien ici de return directement l'objet de la stratégie : le dataclass : BuyHeikinGreenConfig
        return {
            # Périodes des EMA
            'ema_short_period': self.widgets['ema_short_spin'].value(),
            'ema_long_period': self.widgets['ema_long_spin'].value(),
            
            # Paramètres Stochastique
            'stoch_fastk': self.widgets['fastk_spin'].value(),
            'stoch_slowk': self.widgets['slowk_spin'].value(),
            'stoch_slowd': self.widgets['slowd_spin'].value(),
            'stoch_threshold': self.widgets['stoch_threshold_spin'].value(),
            
            # Activation des filtres
            'use_ema_short_filter': self.widgets['ema_short_filter_check'].isChecked(),
            'use_ema_long_filter': self.widgets['ema_long_filter_check'].isChecked(),
            'use_stoch_filter': self.widgets['stoch_filter_check'].isChecked(),
            'use_previous_ha_candle_red_filter': self.widgets['previous_ha_candle_red_check'].isChecked(),
        }
    
    def get_required_indicators(self):
        """
        Retourne les indicateurs nécessaires pour la stratégie BuyHeikinGreen.
        Cette méthode est utilisée par l'application de backtest pour précalculer les indicateurs.
        
        Returns:
            dict: Un dictionnaire des indicateurs à précalculer au format:
                {'NOM_INDICATEUR': [[param1, param2, ...], [autre_config], ...]}
        """
        indicators = {}
        
        # EMA
        ema_periods = []
        if self.widgets['ema_short_filter_check'].isChecked():
            ema_periods.append([self.widgets['ema_short_spin'].value()])
        
        if self.widgets['ema_long_filter_check'].isChecked():
            ema_periods.append([self.widgets['ema_long_spin'].value()])
        
        # Ajouter une EMA moyenne pour l'affichage si elle existe
        if hasattr(self.widgets, 'ema_medium_check') and self.widgets.get('ema_medium_check') and self.widgets['ema_medium_check'].isChecked():
            ema_periods.append([self.widgets['ema_medium_spin'].value()])
        
        if ema_periods:
            indicators['EMA'] = ema_periods
        
        # Stochastique
        if self.widgets['stoch_filter_check'].isChecked():
            indicators['STOCH'] = [[
                self.widgets['fastk_spin'].value(),
                self.widgets['slowk_spin'].value(),
                self.widgets['slowd_spin'].value()
            ]]
        
        # ATR (pour le calcul des stops si nécessaire)
        if hasattr(self.widgets, 'atr_check') and self.widgets.get('atr_check') and self.widgets['atr_check'].isChecked():
            indicators['ATR'] = [[self.widgets['atr_period_spin'].value()]]
        
        return indicators
    