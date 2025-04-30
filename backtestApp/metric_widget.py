import logging
from PyQt5.QtWidgets import QFrame, QVBoxLayout, QLabel, QSizePolicy
from PyQt5.QtGui import QFont

class MetricWidget(QFrame):
    """Widget pour afficher une métrique avec un titre, une valeur et une variation optionnelle"""
    def __init__(self, title, value="", delta="", delta_color="normal", parent=None):
        super().__init__(parent)
        self.setFrameShape(QFrame.StyledPanel)
        self.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # Titre
        self.title_label = QLabel(title)
        title_font = QFont()
        title_font.setPointSize(10)
        self.title_label.setFont(title_font)
        
        # Valeur
        self.value_label = QLabel(value)
        value_font = QFont()
        value_font.setPointSize(12)
        value_font.setBold(True)
        self.value_label.setFont(value_font)
        
        # Delta
        self.delta_label = QLabel(delta)
        delta_font = QFont()
        delta_font.setPointSize(10)
        self.delta_label.setFont(delta_font)
        if delta_color == "normal":
            self.delta_label.setStyleSheet("color: green")
        elif delta_color == "inverse":
            self.delta_label.setStyleSheet("color: red")
        
        layout.addWidget(self.title_label)
        layout.addWidget(self.value_label)
        if delta:
            layout.addWidget(self.delta_label)
        
        self.setLayout(layout)
    
    def update_values(self, value, delta="", delta_color="normal"):
        self.value_label.setText(value)
        if delta:
            self.delta_label.setText(delta)
            if delta_color == "normal":
                self.delta_label.setStyleSheet("color: green")
            elif delta_color == "inverse":
                self.delta_label.setStyleSheet("color: red")