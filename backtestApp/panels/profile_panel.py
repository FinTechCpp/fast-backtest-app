from PyQt5.QtWidgets import (QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, 
                            QPushButton, QComboBox)
from PyQt5.QtGui import QFont
from .base_panel import BasePanel

class ProfilePanel(BasePanel):
    """Panel de gestion des profils de configuration."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.config_manager = parent.config_manager if parent else None
    
    def create(self):
        """Crée le panel de gestion des profils."""
        # Créer le groupe pour les profils
        profile_group = QGroupBox("Profils de configuration")
        profile_layout = QVBoxLayout()
        
        # Affichage du profil actif
        if self.config_manager:
            current_profile = self.config_manager.current_profile
        else:
            current_profile = "DEFAULT"
            
        self.widgets['current_profile_label'] = QLabel(f"Profil actif: {current_profile}")
        self.widgets['current_profile_label'].setStyleSheet("font-weight: bold;")
        profile_layout.addWidget(self.widgets['current_profile_label'])
        
        # Actions de profil
        profile_buttons_layout = QHBoxLayout()
        
        # Bouton pour sauvegarder le profil actuel
        save_profile_btn = QPushButton("💾 Sauvegarder")
        save_profile_btn.setToolTip("Sauvegarder les paramètres actuels dans le profil courant")
        if self.parent and self.config_manager:
            save_profile_btn.clicked.connect(
                lambda: self.config_manager.save_current_profile(self.parent, self.parent)
            )
        profile_buttons_layout.addWidget(save_profile_btn)
        
        # Bouton pour créer un nouveau profil
        new_profile_btn = QPushButton("➕ Nouveau")
        new_profile_btn.setToolTip("Créer un nouveau profil avec les paramètres actuels")
        if self.parent and self.config_manager:
            new_profile_btn.clicked.connect(
                lambda: self.config_manager.prompt_create_new_profile(self.parent, self.parent)
            )
        profile_buttons_layout.addWidget(new_profile_btn)
        
        # Bouton pour supprimer un profil
        delete_profile_btn = QPushButton("🗑️ Supprimer")
        delete_profile_btn.setToolTip("Supprimer le profil actuel")
        if self.parent and self.config_manager:
            delete_profile_btn.clicked.connect(
                lambda: self.config_manager.delete_current_profile(self.parent, self.parent)
            )
        profile_buttons_layout.addWidget(delete_profile_btn)
        
        profile_layout.addLayout(profile_buttons_layout)
        
        # Menu déroulant pour sélectionner un profil
        profile_selector_layout = QHBoxLayout()
        profile_selector_layout.addWidget(QLabel("Charger un profil:"))
        
        self.widgets['profile_combo'] = QComboBox()
        if self.config_manager:
            self.widgets['profile_combo'].addItems(self.config_manager.list_profiles())
            self.widgets['profile_combo'].setCurrentText(self.config_manager.current_profile)
            if self.parent:
                self.widgets['profile_combo'].currentTextChanged.connect(self.parent.load_selected_profile)
        profile_selector_layout.addWidget(self.widgets['profile_combo'])
        
        profile_layout.addLayout(profile_selector_layout)
        
        # Finaliser le groupe de profils
        profile_group.setLayout(profile_layout)
        return profile_group
        
    def get_values(self):
        """Récupère les valeurs des widgets du panel."""
        return {
            'profile': self.widgets['profile_combo'].currentText() if 'profile_combo' in self.widgets else ""
        }
        
    def set_values(self, values):
        """Définit les valeurs des widgets du panel."""
        if 'profile' in values and 'profile_combo' in self.widgets:
            index = self.widgets['profile_combo'].findText(values['profile'])
            if index >= 0:
                self.widgets['profile_combo'].setCurrentIndex(index)