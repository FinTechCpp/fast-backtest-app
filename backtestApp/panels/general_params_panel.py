from PyQt5.QtWidgets import (QGroupBox, QFormLayout, QLabel, QComboBox, 
                            QDateEdit, QDoubleSpinBox, QCheckBox)
from PyQt5.QtCore import QDate
from .base_panel import BasePanel

class GeneralParamsPanel(BasePanel):
    """Panel des paramètres généraux du backtest."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.strategy_map = parent.strategy_map if parent else {}
    
    def create(self):
        """Crée le panel des paramètres généraux."""
        params_group = QGroupBox("Paramètres généraux")
        params_layout = QFormLayout()
        
        # Symbole
        self.widgets['symbol'] = QComboBox()
        self.widgets['symbol'].addItems(["NDX", "IBUST100", "EURUSD"])
        params_layout.addRow(QLabel("Symbole:"), self.widgets['symbol'])
        
        # Période
        self.widgets['period'] = QComboBox()
        self.widgets['period'].addItems(["5d", "10d", "30d", "1m", "2m","3m", "6m", "1y", "2y"])
        self.widgets['period'].setCurrentIndex(1)
        params_layout.addRow(QLabel("Période de données:"), self.widgets['period'])
        
        # Intervalle
        self.widgets['interval'] = QComboBox()
        self.widgets['interval'].addItems(["10secs", "20secs", "30secs", "1min", "2min", "3min", "5min", "10min", "15min", "30min", "1h", "2h", "4h", "1d"])
        self.widgets['interval'].setCurrentIndex(1)
        params_layout.addRow(QLabel("Intervalle:"), self.widgets['interval'])
        
        # Date de fin
        self.widgets['end_date'] = QDateEdit()
        self.widgets['end_date'].setDate(QDate(2025, 4, 30))
        self.widgets['end_date'].setCalendarPopup(True)
        params_layout.addRow(QLabel("Date de fin:"), self.widgets['end_date'])
        
        # Fuseau horaire => Plus necessaire car on convertit les dates par default entre 15:30h et 22h
        # self.widgets['timezone'] = QComboBox()
        # self.widgets['timezone'].addItems(["America/New_York", "Europe/Paris"])
        # params_layout.addRow(QLabel("Fuseau horaire:"), self.widgets['timezone'])
        
        # Spread
        self.widgets['spread'] = QDoubleSpinBox()
        self.widgets['spread'].setDecimals(4)
        self.widgets['spread'].setRange(0, 0.001)
        self.widgets['spread'].setSingleStep(0.0001)
        self.widgets['spread'].setValue(0.0001)
        params_layout.addRow(QLabel("Spread:"), self.widgets['spread'])
        
        # Cash initial
        self.widgets['cash'] = QDoubleSpinBox()
        self.widgets['cash'].setDecimals(2)
        self.widgets['cash'].setRange(0, 10000000)
        self.widgets['cash'].setSingleStep(1000)
        self.widgets['cash'].setValue(100000)
        params_layout.addRow(QLabel("Cash initial:"), self.widgets['cash'])
        
        # Stratégie
        self.widgets['strategy'] = QComboBox()
        self.widgets['strategy'].addItems(list(self.strategy_map.keys()))
        params_layout.addRow(QLabel("Stratégie:"), self.widgets['strategy'])
        
        # Type de bougie
        self.widgets['candle_type'] = QComboBox()
        self.widgets['candle_type'].addItems(["Heikin Ashi", "Standard"])
        params_layout.addRow(QLabel("Type de bougie:"), self.widgets['candle_type'])
        
        # Sauvegarder les résultats
        self.widgets['save_results'] = QCheckBox("Sauvegarder les résultats")
        params_layout.addRow("", self.widgets['save_results'])
        
        params_group.setLayout(params_layout)
        return params_group
    
    def get_values(self):
        """Récupère les valeurs des widgets du panel."""
        return {
            'symbol': self.widgets['symbol'].currentText(),
            'period': self.widgets['period'].currentText(),
            'interval': self.widgets['interval'].currentText(),
            'end_date': self.widgets['end_date'].date().toString("dd/MM/yyyy"),
            # 'timezone': self.widgets['timezone'].currentText(),
            'spread': self.widgets['spread'].value(),
            'cash': self.widgets['cash'].value(),
            'strategy': self.widgets['strategy'].currentText(),
            'candle_type': self.widgets['candle_type'].currentText(),
            'save_results': self.widgets['save_results'].isChecked()
        }
    
    def set_values(self, values):
        """Définit les valeurs des widgets du panel."""
        if 'symbol' in values:
            index = self.widgets['symbol_combo'].findText(values['symbol'])
            if index >= 0:
                self.widgets['symbol_combo'].setCurrentIndex(index)
        
        if 'period' in values:
            index = self.widgets['period_combo'].findText(values['period'])
            if index >= 0:
                self.widgets['period_combo'].setCurrentIndex(index)
        
        if 'interval' in values:
            index = self.widgets['interval_combo'].findText(values['interval'])
            if index >= 0:
                self.widgets['interval_combo'].setCurrentIndex(index)
        
        if 'end_date' in values:
            self.widgets['end_date'].setDate(QDate.fromString(values['end_date'], "dd/MM/yyyy"))
        
        # if 'timezone' in values:
        #     self.widgets['timezone'].findText(values['timezone'])
        #     if index >= 0:
        #         self.widgets['timezone'].setCurrentIndex(index)
        
        if 'spread' in values:
            self.widgets['spread'].setValue(values['spread'])
        
        if 'cash' in values:
            self.widgets['cash'].setValue(values['cash'])
        
        if 'strategy' in values:
            index = self.widgets['strategy_combo'].findText(values['strategy'])
            if index >= 0:
                self.widgets['strategy_combo'].setCurrentIndex(index)
        
        if 'candle_type' in values:
            index = self.widgets['candle_type_combo'].findText(values['candle_type'])
            if index >= 0:
                self.widgets['candle_type_combo'].setCurrentIndex(index)
        
        if 'save_results' in values:
            self.widgets['save_results'].setChecked(values['save_results'])