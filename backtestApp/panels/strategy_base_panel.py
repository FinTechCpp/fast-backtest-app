from PyQt5.QtWidgets import (QGroupBox, QFormLayout, QCheckBox, QDoubleSpinBox,
                            QTimeEdit, QLabel, QVBoxLayout, QHBoxLayout)
from PyQt5.QtCore import QTime
from .base_panel import BasePanel
import logging

class StrategyBasePanel(BasePanel):
    """Panel pour les paramètres communs à toutes les stratégies (StrategyBaseConfig)."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
    
    def create(self):
        """Crée le panel des paramètres de base pour les stratégies."""
        base_group = QGroupBox("Paramètres de base")
        base_layout = QVBoxLayout()

        # Section SL/TP avec choix entre valeurs fixes et ATR
        sl_tp_group = QGroupBox("Stop Loss et Take Profit")
        sl_tp_layout = QVBoxLayout()
        
        # Valeurs fixes de SL/TP
        fixed_values_layout = QFormLayout()

        # Stop Loss Distance (fixe)
        self.widgets['stop_loss_distance'] = QDoubleSpinBox()
        self.widgets['stop_loss_distance'].setDecimals(2)
        self.widgets['stop_loss_distance'].setRange(0, 1000)
        self.widgets['stop_loss_distance'].setSingleStep(1)
        self.widgets['stop_loss_distance'].setValue(20)
        fixed_values_layout.addRow(QLabel("Stop Loss Distance [pts]:"), self.widgets['stop_loss_distance'])
        
        # Take Profit Distance (fixe)
        self.widgets['take_profit_distance'] = QDoubleSpinBox()
        self.widgets['take_profit_distance'].setDecimals(2)
        self.widgets['take_profit_distance'].setRange(0, 1000)
        self.widgets['take_profit_distance'].setSingleStep(1)
        self.widgets['take_profit_distance'].setValue(30)
        fixed_values_layout.addRow(QLabel("Take Profit Distance [pts]:"), self.widgets['take_profit_distance'])
        
        sl_tp_layout.addLayout(fixed_values_layout)

        # Checkbox pour activer l'ATR pour SL/TP
        self.widgets['use_atr_check'] = QCheckBox("Utiliser l'ATR pour SL/TP")
        sl_tp_layout.addWidget(self.widgets['use_atr_check'])
        
        # Paramètres ATR
        atr_layout = QFormLayout()

        # Période ATR
        self.widgets['atr_period'] = QDoubleSpinBox()
        self.widgets['atr_period'].setDecimals(0)
        self.widgets['atr_period'].setRange(1, 100)
        self.widgets['atr_period'].setSingleStep(1)
        self.widgets['atr_period'].setValue(14)
        self.widgets['atr_period'].setEnabled(False)
        atr_layout.addRow("Période ATR:", self.widgets['atr_period'])
        
        # Multiplicateurs ATR
        self.widgets['sl_atr_multiplier'] = QDoubleSpinBox()
        self.widgets['sl_atr_multiplier'].setDecimals(1)
        self.widgets['sl_atr_multiplier'].setRange(0.1, 100.0)
        self.widgets['sl_atr_multiplier'].setSingleStep(0.1)
        self.widgets['sl_atr_multiplier'].setValue(2.0)
        self.widgets['sl_atr_multiplier'].setEnabled(False)
        atr_layout.addRow("Multiplicateur ATR pour SL:", self.widgets['sl_atr_multiplier'])
        
        self.widgets['tp_atr_multiplier'] = QDoubleSpinBox()
        self.widgets['tp_atr_multiplier'].setDecimals(1)
        self.widgets['tp_atr_multiplier'].setRange(0.1, 100.0)
        self.widgets['tp_atr_multiplier'].setSingleStep(0.1)
        self.widgets['tp_atr_multiplier'].setValue(3.0)
        self.widgets['tp_atr_multiplier'].setEnabled(False)
        atr_layout.addRow("Multiplicateur ATR pour TP:", self.widgets['tp_atr_multiplier'])
        
        # Valeurs minimales
        self.widgets['min_sl'] = QDoubleSpinBox()
        self.widgets['min_sl'].setDecimals(1)
        self.widgets['min_sl'].setRange(1.0, 100.0)
        self.widgets['min_sl'].setSingleStep(1.0)
        self.widgets['min_sl'].setValue(5.0)
        self.widgets['min_sl'].setEnabled(False)
        atr_layout.addRow("SL minimal [pts]:", self.widgets['min_sl'])
        
        self.widgets['min_tp'] = QDoubleSpinBox()
        self.widgets['min_tp'].setDecimals(1)
        self.widgets['min_tp'].setRange(1.0, 100.0)
        self.widgets['min_tp'].setSingleStep(1.0)
        self.widgets['min_tp'].setValue(5.0)
        self.widgets['min_tp'].setEnabled(False)
        atr_layout.addRow("TP minimal [pts]:", self.widgets['min_tp'])
        
        sl_tp_layout.addLayout(atr_layout)
    
        # Checkbox pour activer le minimum local pour SL
        self.widgets['use_local_minimum_check'] = QCheckBox("Utiliser le minimum local pour SL")
        sl_tp_layout.addWidget(self.widgets['use_local_minimum_check'])
        
        # Ajouter un disclaimer pour informer que cette fonctionnalité n'est pas opérationnelle
        disclaimer_label = QLabel(f"⚠ ATTENTION: Fonctionnalité en développement\n- Non fonctionnelle actuellement")
        disclaimer_label.setStyleSheet("color: red; font-style: italic; font-weight: bold;")
        sl_tp_layout.addWidget(disclaimer_label)
        
        # Paramètres du minimum local
        local_min_layout = QFormLayout()
        
        # Période pour calculer le minimum local
        self.widgets['local_minimum_period'] = QDoubleSpinBox()
        self.widgets['local_minimum_period'].setDecimals(0)
        self.widgets['local_minimum_period'].setRange(2, 50)
        self.widgets['local_minimum_period'].setSingleStep(1)
        self.widgets['local_minimum_period'].setValue(5)
        self.widgets['local_minimum_period'].setEnabled(False)
        local_min_layout.addRow("Période minimum local:", self.widgets['local_minimum_period'])
        
        # Delta pour l'ajustement
        self.widgets['local_minimum_delta'] = QDoubleSpinBox()
        self.widgets['local_minimum_delta'].setDecimals(1)
        self.widgets['local_minimum_delta'].setRange(0.1, 50.0)
        self.widgets['local_minimum_delta'].setSingleStep(0.5)
        self.widgets['local_minimum_delta'].setValue(1.0)
        self.widgets['local_minimum_delta'].setEnabled(False)
        local_min_layout.addRow("Delta minimum local:", self.widgets['local_minimum_delta'])
        
        sl_tp_layout.addLayout(local_min_layout)
        sl_tp_group.setLayout(sl_tp_layout)
        base_layout.addWidget(sl_tp_group)
        
        # Risk-based sizing
        risk_sizing_group = QGroupBox("Gestion du risque")
        risk_layout = QFormLayout()
        
        # Checkbox pour activer le risk-based sizing
        self.widgets['use_risk_based_sizing'] = QCheckBox("Utiliser Risk-Based Sizing")
        risk_layout.addRow("", self.widgets['use_risk_based_sizing'])
        
        # Spinner pour le pourcentage de risque
        self.widgets['risk_percentage'] = QDoubleSpinBox()
        self.widgets['risk_percentage'].setDecimals(2)
        self.widgets['risk_percentage'].setRange(0.1, 50.0)
        self.widgets['risk_percentage'].setSingleStep(0.1)
        self.widgets['risk_percentage'].setValue(1.0)  # 1% par défaut
        self.widgets['risk_percentage'].setEnabled(False)
        risk_layout.addRow("Risque par trade (%):", self.widgets['risk_percentage'])
        
        # Checkbox pour activer le Break-Even
        self.widgets['use_break_even'] = QCheckBox("Utiliser Break-Even")
        self.widgets['use_break_even'].toggled.connect(self._toggle_break_even_controls)
        risk_layout.addRow("", self.widgets['use_break_even'])
            
        # Break-Even Threshold
        self.widgets['break_even_threshold'] = QDoubleSpinBox()
        self.widgets['break_even_threshold'].setDecimals(2)
        self.widgets['break_even_threshold'].setRange(0.1, 1.0)
        self.widgets['break_even_threshold'].setSingleStep(0.05)
        self.widgets['break_even_threshold'].setValue(0.7)  # Default: 70% of take profit
        self.widgets['break_even_threshold'].setToolTip("Ajuste le stop loss au point d'équilibre quand le profit atteint ce % du take profit")
        self.widgets['break_even_threshold'].setEnabled(False)
        risk_layout.addRow("Break-Even Threshold (%):", self.widgets['break_even_threshold'])

        # Perte maximale par jour
        self.widgets['use_daily_max_loss'] = QCheckBox("Utiliser la perte maximale quotidienne")
        self.widgets['use_daily_max_loss'].toggled.connect(self._toggle_daily_max_loss_controls)
        risk_layout.addRow("", self.widgets['use_daily_max_loss'])

        # Pourcentage de perte maximale
        self.widgets['daily_max_loss_percentage'] = QDoubleSpinBox()
        self.widgets['daily_max_loss_percentage'].setDecimals(2)
        self.widgets['daily_max_loss_percentage'].setRange(0.1, 100.0)
        self.widgets['daily_max_loss_percentage'].setSingleStep(0.1)
        self.widgets['daily_max_loss_percentage'].setValue(2.0)  # 2% par défaut
        self.widgets['daily_max_loss_percentage'].setToolTip("Pourcentage de perte maximale autorisée par jour")
        self.widgets['daily_max_loss_percentage'].setEnabled(False)
        risk_layout.addRow("Perte maximale quotidienne (%):", self.widgets['daily_max_loss_percentage'])
        
        # Maximum leverage
        self.widgets['maximal_leverage'] = QDoubleSpinBox()
        self.widgets['maximal_leverage'].setDecimals(2)
        self.widgets['maximal_leverage'].setRange(1.0, 100.0)
        self.widgets['maximal_leverage'].setSingleStep(0.1)
        self.widgets['maximal_leverage'].setValue(20.0)  # Default: 20x leverage
        self.widgets['maximal_leverage'].setToolTip("Effet de levier maximum autorisé")
        risk_layout.addRow("Levier maximal autorisé:", self.widgets['maximal_leverage'])
                
        risk_sizing_group.setLayout(risk_layout)
        base_layout.addWidget(risk_sizing_group)
        
        # Trading hours
        trading_hours_group = QGroupBox("Heures de trading")
        trading_hours_layout = QFormLayout()
        
        # Trading start time
        start_time_layout = QHBoxLayout()
        self.widgets['trading_from_hour'] = QTimeEdit()
        self.widgets['trading_from_hour'].setTime(QTime(15, 30))
        self.widgets['trading_from_hour'].setDisplayFormat("HH:mm")
        start_time_layout.addWidget(self.widgets['trading_from_hour'])
        trading_hours_layout.addRow("Heure de début:", start_time_layout)
        
        # Trading end time
        end_time_layout = QHBoxLayout()
        self.widgets['trading_to_hour'] = QTimeEdit()
        self.widgets['trading_to_hour'].setTime(QTime(21, 59))
        self.widgets['trading_to_hour'].setDisplayFormat("HH:mm")
        end_time_layout.addWidget(self.widgets['trading_to_hour'])
        trading_hours_layout.addRow("Heure de fin:", end_time_layout)
        
        # Trading days
        days_layout = QHBoxLayout()
        self.widgets['trading_days_check'] = []
        days = ["Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"]
        for i, day in enumerate(days):
            check = QCheckBox(day)
            check.setChecked(i < 5)  # Check Mon-Fri by default
            self.widgets['trading_days_check'].append(check)
            days_layout.addWidget(check)
        trading_hours_layout.addRow("Jours de trading:", days_layout)
        
        trading_hours_group.setLayout(trading_hours_layout)
        base_layout.addWidget(trading_hours_group)
        
        # Connexion des signaux
        self.widgets['use_atr_check'].toggled.connect(self._toggle_atr_controls)
        self.widgets['use_local_minimum_check'].toggled.connect(self._toggle_local_minimum_controls)
        self.widgets['use_risk_based_sizing'].toggled.connect(self._toggle_risk_controls)
        
        base_group.setLayout(base_layout)
        return base_group
    
    def _toggle_atr_controls(self, checked):
        """Active ou désactive les contrôles pour les paramètres ATR"""
        self.widgets['sl_atr_multiplier'].setEnabled(checked)
        self.widgets['tp_atr_multiplier'].setEnabled(checked)
        self.widgets['min_sl'].setEnabled(checked)
        self.widgets['min_tp'].setEnabled(checked)
        self.widgets['atr_period'].setEnabled(checked)
        
        # Si ATR est activé et que minimum local est activé, désactiver minimum local
        if checked and self.widgets['use_local_minimum_check'].isChecked():
            self.widgets['use_local_minimum_check'].setChecked(False)
            
        # Désactiver SL fixe si l'ATR ou le minimum local est activé
        use_local_min = self.widgets['use_local_minimum_check'].isChecked()
        self.widgets['stop_loss_distance'].setEnabled(not (checked or use_local_min))
        self.widgets['take_profit_distance'].setEnabled(not checked)

    def _toggle_local_minimum_controls(self, checked):
        """Active ou désactive les contrôles pour le minimum local"""
        self.widgets['local_minimum_period'].setEnabled(checked)
        self.widgets['local_minimum_delta'].setEnabled(checked)
        
        # Si minimum local est activé et que ATR est activé, ne pas désactiver l'ATR
        # car l'ATR est toujours utilisé pour le TP
        
        # Désactiver SL fixe si l'ATR ou le minimum local est activé
        use_atr = self.widgets['use_atr_check'].isChecked()
        self.widgets['stop_loss_distance'].setEnabled(not (checked or use_atr))

    def _toggle_risk_controls(self, checked):
        """Active ou désactive les contrôles pour le risk-based sizing"""
        self.widgets['risk_percentage'].setEnabled(checked)

    def _toggle_break_even_controls(self, checked):
        """Active ou désactive les contrôles pour le Break-Even"""
        self.widgets['break_even_threshold'].setEnabled(checked)

    def _toggle_daily_max_loss_controls(self, checked):
        """Active ou désactive les contrôles pour la perte maximale quotidienne"""
        self.widgets['daily_max_loss_percentage'].setEnabled(checked)
    
    def get_values(self):
        """Récupère les valeurs des widgets du panel."""
        days = []
        for i, check in enumerate(self.widgets['trading_days_check']):
            if check.isChecked():
                days.append(i)
        
        trading_from = self.widgets['trading_from_hour'].time()
        trading_to = self.widgets['trading_to_hour'].time()
        
        return {
            'stop_loss_distance': self.widgets['stop_loss_distance'].value(),
            'take_profit_distance': self.widgets['take_profit_distance'].value(),
            'use_atr_for_sl_tp': self.widgets['use_atr_check'].isChecked(),
            'atr_period': self.widgets['atr_period'].value(),
            'stop_loss_atr_multiplier': self.widgets['sl_atr_multiplier'].value(),
            'take_profit_atr_multiplier': self.widgets['tp_atr_multiplier'].value(),
            'min_stop_loss_distance': self.widgets['min_sl'].value(),
            'min_take_profit_distance': self.widgets['min_tp'].value(),
            'use_last_local_minimum': self.widgets['use_local_minimum_check'].isChecked(),
            'last_local_minimum_period': int(self.widgets['local_minimum_period'].value()),
            'last_local_minimum_delta': self.widgets['local_minimum_delta'].value(),
            'use_risk_based_sizing': self.widgets['use_risk_based_sizing'].isChecked(),
            'risk_percentage': self.widgets['risk_percentage'].value(),
            'use_break_even': self.widgets['use_break_even'].isChecked(),
            'break_even_threshold': self.widgets['break_even_threshold'].value(),
            'use_daily_max_loss': self.widgets['use_daily_max_loss'].isChecked(),
            'daily_max_loss_percentage': self.widgets['daily_max_loss_percentage'].value(),
            'maximal_leverage': self.widgets['maximal_leverage'].value(),
            'trading_from': trading_from,
            'trading_to': trading_to,
            'trading_days': days
        }

    def get_required_indicators(self):
        """Récupère les indicateurs requis pour la stratégie."""
        return {
            'ATR': [[self.widgets['atr_period'].value()]],
        }