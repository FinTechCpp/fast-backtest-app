from PyQt5.QtWidgets import (QGroupBox, QVBoxLayout, QGridLayout, QCheckBox, 
                           QSpinBox, QLabel)
from ..base_panel import BasePanel

class BuyHeikinGreenPanel(BasePanel):
    """Panel spécifique à la stratégie BuyHeikinGreen."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
    
    def create(self):
        """Crée le panel des paramètres spécifiques à BuyHeikinGreen."""
        strategy_group = QGroupBox("Paramètres BuyHeikinGreen")
        strategy_layout = QVBoxLayout()
        
        # Configuration des filtres
        filters_group = QGroupBox("Configuration des filtres")
        filters_layout = QVBoxLayout()

        # EMA Filter
        self.widgets['ema_short_filter_check'] = QCheckBox("Activer EMA court Filter")
        self.widgets['ema_short_filter_check'].setChecked(True)
        self.widgets['ema_long_filter_check'] = QCheckBox("Activer EMA long Filter")
        self.widgets['ema_long_filter_check'].setChecked(True)

        # Stochastic Filter
        self.widgets['stoch_filter_check'] = QCheckBox("Activer Stochastic Filter")
        self.widgets['stoch_filter_check'].setChecked(True)

        # Previous HA Candle Red Filter
        self.widgets['previous_ha_candle_red_check'] = QCheckBox("Activer Previous HA Candle Red Filter")
        self.widgets['previous_ha_candle_red_check'].setChecked(True)
        
        filters_layout.addWidget(self.widgets['ema_short_filter_check'])
        filters_layout.addWidget(self.widgets['ema_long_filter_check'])
        filters_layout.addWidget(self.widgets['stoch_filter_check'])
        filters_layout.addWidget(self.widgets['previous_ha_candle_red_check'])
        filters_group.setLayout(filters_layout)
        strategy_layout.addWidget(filters_group)

        # Configuration des indicateurs
        indicators_group = QGroupBox("Configuration des indicateurs")
        indicators_layout = QVBoxLayout()
        
        # EMA
        ema_layout = QGridLayout()
        
        self.widgets['ema_short_check'] = QCheckBox("Activer EMA court")
        self.widgets['ema_short_check'].setChecked(True)
        self.widgets['ema_short_spin'] = QSpinBox()
        self.widgets['ema_short_spin'].setRange(1, 500)
        self.widgets['ema_short_spin'].setValue(20)
        
        self.widgets['ema_medium_check'] = QCheckBox("Activer EMA moyen")
        self.widgets['ema_medium_check'].setChecked(True)
        self.widgets['ema_medium_spin'] = QSpinBox()
        self.widgets['ema_medium_spin'].setRange(1, 500)
        self.widgets['ema_medium_spin'].setValue(50)
        
        self.widgets['ema_long_check'] = QCheckBox("Activer EMA long")
        self.widgets['ema_long_check'].setChecked(True)
        self.widgets['ema_long_spin'] = QSpinBox()
        self.widgets['ema_long_spin'].setRange(1, 500)
        self.widgets['ema_long_spin'].setValue(200)
        
        ema_layout.addWidget(self.widgets['ema_short_check'], 0, 0)
        ema_layout.addWidget(QLabel("Période:"), 0, 1)
        ema_layout.addWidget(self.widgets['ema_short_spin'], 0, 2)
        ema_layout.addWidget(self.widgets['ema_medium_check'], 1, 0)
        ema_layout.addWidget(QLabel("Période:"), 1, 1)
        ema_layout.addWidget(self.widgets['ema_medium_spin'], 1, 2)
        ema_layout.addWidget(self.widgets['ema_long_check'], 2, 0)
        ema_layout.addWidget(QLabel("Période:"), 2, 1)
        ema_layout.addWidget(self.widgets['ema_long_spin'], 2, 2)
        
        indicators_layout.addLayout(ema_layout)
        
        # Stochastic
        stoch_group = QGroupBox("Stochastic")
        stoch_layout = QGridLayout()
        
        self.widgets['stoch_check'] = QCheckBox("Activer Stochastic")
        self.widgets['stoch_check'].setChecked(True)
        self.widgets['fastk_spin'] = QSpinBox()
        self.widgets['fastk_spin'].setRange(1, 100)
        self.widgets['fastk_spin'].setValue(10)
        self.widgets['slowk_spin'] = QSpinBox()
        self.widgets['slowk_spin'].setRange(1, 100)
        self.widgets['slowk_spin'].setValue(10)
        self.widgets['slowd_spin'] = QSpinBox()
        self.widgets['slowd_spin'].setRange(1, 100)
        self.widgets['slowd_spin'].setValue(3)
        
        stoch_layout.addWidget(self.widgets['stoch_check'], 0, 0, 1, 3)
        stoch_layout.addWidget(QLabel("Fast %K:"), 1, 0)
        stoch_layout.addWidget(self.widgets['fastk_spin'], 1, 1)
        stoch_layout.addWidget(QLabel("Slow %K:"), 2, 0)
        stoch_layout.addWidget(self.widgets['slowk_spin'], 2, 1)
        stoch_layout.addWidget(QLabel("Slow %D:"), 3, 0)
        stoch_layout.addWidget(self.widgets['slowd_spin'], 3, 1)
        
        self.widgets['stoch_threshold_spin'] = QSpinBox()
        self.widgets['stoch_threshold_spin'].setRange(1, 99)
        self.widgets['stoch_threshold_spin'].setValue(50)
        stoch_layout.addWidget(QLabel("Seuil filtre %K:"), 4, 0)
        stoch_layout.addWidget(self.widgets['stoch_threshold_spin'], 4, 1)
        
        stoch_group.setLayout(stoch_layout)
        indicators_layout.addWidget(stoch_group)
        
        # ATR
        self.widgets['atr_check'] = QCheckBox("Activer ATR")
        self.widgets['atr_check'].setChecked(True)
        self.widgets['atr_period_spin'] = QSpinBox()
        self.widgets['atr_period_spin'].setRange(1, 100)
        self.widgets['atr_period_spin'].setValue(14)
        
        atr_layout = QGridLayout()
        atr_layout.addWidget(self.widgets['atr_check'], 0, 0)
        atr_layout.addWidget(QLabel("Période:"), 0, 1)
        atr_layout.addWidget(self.widgets['atr_period_spin'], 0, 2)
        indicators_layout.addLayout(atr_layout)
        
        indicators_group.setLayout(indicators_layout)
        strategy_layout.addWidget(indicators_group)

        # Connections des signaux
        self.widgets['ema_short_check'].toggled.connect(
            lambda checked: self.widgets['ema_short_spin'].setEnabled(checked))
        self.widgets['ema_medium_check'].toggled.connect(
            lambda checked: self.widgets['ema_medium_spin'].setEnabled(checked))
        self.widgets['ema_long_check'].toggled.connect(
            lambda checked: self.widgets['ema_long_spin'].setEnabled(checked))
        self.widgets['atr_check'].toggled.connect(
            lambda checked: self.widgets['atr_period_spin'].setEnabled(checked))
        self.widgets['stoch_check'].toggled.connect(self._toggle_stoch_widgets)
        
        strategy_group.setLayout(strategy_layout)
        return strategy_group
    
    def _toggle_stoch_widgets(self, checked):
        """Active/désactive les widgets Stochastic en fonction de la checkbox"""
        self.widgets['fastk_spin'].setEnabled(checked)
        self.widgets['slowk_spin'].setEnabled(checked)
        self.widgets['slowd_spin'].setEnabled(checked)
        self.widgets['stoch_threshold_spin'].setEnabled(checked)
    
    def get_values(self):
        """Récupère les valeurs des widgets du panel."""
        return {
            'ema_short_period': self.widgets['ema_short_spin'].value(),
            'ema_long_period': self.widgets['ema_long_spin'].value(),
            'stoch_fastk': self.widgets['fastk_spin'].value(),
            'stoch_slowk': self.widgets['slowk_spin'].value(),
            'stoch_slowd': self.widgets['slowd_spin'].value(),
            'stoch_threshold': self.widgets['stoch_threshold_spin'].value(),
            'use_ema_short_filter': self.widgets['ema_short_filter_check'].isChecked(),
            'use_ema_long_filter': self.widgets['ema_long_filter_check'].isChecked(),
            'use_stoch_filter': self.widgets['stoch_filter_check'].isChecked(),
            'use_previous_ha_candle_red_filter': self.widgets['previous_ha_candle_red_check'].isChecked(),
            'atr_period': self.widgets['atr_period_spin'].value(),
            'atr_enabled': self.widgets['atr_check'].isChecked()
        }
    
    def set_values(self, values):
        """Définit les valeurs des widgets du panel."""
        if 'ema_short_period' in values:
            self.widgets['ema_short_spin'].setValue(values['ema_short_period'])
        
        if 'ema_long_period' in values:
            self.widgets['ema_long_spin'].setValue(values['ema_long_period'])
        
        if 'stoch_fastk' in values:
            self.widgets['fastk_spin'].setValue(values['stoch_fastk'])
        
        if 'stoch_slowk' in values:
            self.widgets['slowk_spin'].setValue(values['stoch_slowk'])
        
        if 'stoch_slowd' in values:
            self.widgets['slowd_spin'].setValue(values['stoch_slowd'])
        
        if 'stoch_threshold' in values:
            self.widgets['stoch_threshold_spin'].setValue(values['stoch_threshold'])
        
        if 'use_ema_short_filter' in values:
            self.widgets['ema_short_filter_check'].setChecked(values['use_ema_short_filter'])
        
        if 'use_ema_long_filter' in values:
            self.widgets['ema_long_filter_check'].setChecked(values['use_ema_long_filter'])
        
        if 'use_stoch_filter' in values:
            self.widgets['stoch_filter_check'].setChecked(values['use_stoch_filter'])
        
        if 'use_previous_ha_candle_red_filter' in values:
            self.widgets['previous_ha_candle_red_check'].setChecked(values['use_previous_ha_candle_red_filter'])
        
        if 'atr_period' in values:
            self.widgets['atr_period_spin'].setValue(values['atr_period'])
        
        if 'atr_enabled' in values:
            self.widgets['atr_check'].setChecked(values['atr_enabled'])