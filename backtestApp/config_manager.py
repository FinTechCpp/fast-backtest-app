import os
import configparser
import logging
from datetime import datetime
from PyQt5.QtWidgets import QInputDialog, QMessageBox, QLineEdit, QTimeEdit, QDateEdit, QFileDialog
import shutil

# from app import BacktestApp

class ConfigManager:
    """Gestionnaire de profils de configuration pour l'application de backtesting.
    
    Cette classe permet de:
    - Sauvegarder plusieurs profils de configuration dans un fichier INI
    - Charger des profils existants
    - Appliquer les paramètres d'un profil à l'interface utilisateur
    - Supprimer des profils
    
    Le fichier de configuration est créé automatiquement s'il n'existe pas.
    """
    
    def __init__(self, config_path=None):
        """Initialise le gestionnaire de configuration.
        
        Args:
            config_path: Chemin vers le fichier de configuration (par défaut: dossier courant)
        """
        if config_path is None:
            # Utilise le même dossier que l'application
            self.config_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "backtest_config.ini")
        else:
            self.config_file = config_path
            
        self.config = configparser.ConfigParser()
        self.current_profile = 'DEFAULT'
        # Stockage pour les valeurs par défaut originales
        self.default_values = {}
        self.load_config()
    
    def load_config(self):
        """Charge la configuration existante ou crée un nouveau fichier si nécessaire."""
        if os.path.exists(self.config_file):
            self.config.read(self.config_file)
            logging.info(f"Configuration chargée depuis {self.config_file}")
            # Sauvegarder les valeurs DEFAULT originales dès le chargement
            if 'DEFAULT' in self.config:
                self.default_values = dict(self.config['DEFAULT'])
        else:
            # Créer un profil par défaut
            self.config['DEFAULT'] = {}  # Le contenu sera rempli dynamiquement
            self.save_config()
            logging.info(f"Nouveau fichier de configuration créé: {self.config_file}")
    
    def save_config(self):
        """Sauvegarde la configuration dans le fichier."""
        try:
            with open(self.config_file, 'w') as configfile:
                self.config.write(configfile)
            logging.info(f"Configuration sauvegardée dans {self.config_file}")
            return True
        except Exception as e:
            logging.error(f"Erreur lors de la sauvegarde de la configuration: {str(e)}")
            return False
    
    def list_profiles(self):
        """Liste tous les profils disponibles.
        
        Returns:
            list: Liste des noms de profils
        """
        return ['DEFAULT'] + [section for section in self.config.sections()]
    
    def profile_exists(self, profile_name):
        """Vérifie si un profil existe.
        
        Args:
            profile_name: Nom du profil à vérifier
            
        Returns:
            bool: True si le profil existe, False sinon
        """
        return profile_name == 'DEFAULT' or profile_name in self.config.sections()
    
    def get_profile(self, profile_name):
        """Récupère un profil par son nom.
        
        Args:
            profile_name: Nom du profil à récupérer
            
        Returns:
            dict: Dictionnaire contenant les paramètres du profil
        """
        if self.profile_exists(profile_name):
            return dict(self.config[profile_name])
        else:
            logging.warning(f"Profil {profile_name} non trouvé, utilisation du profil DEFAULT")
            return dict(self.config['DEFAULT'])
    
    def save_profile(self, profile_name, profile_data):
        """Sauvegarde un profil avec les données fournies.
        
        Args:
            profile_name: Nom du profil à sauvegarder
            profile_data: Dictionnaire des paramètres du profil
            
        Returns:
            bool: True si la sauvegarde a réussi, False sinon
        """
        try:
            if profile_name != 'DEFAULT':
                # Assurons-nous que la section existe
                if profile_name not in self.config:
                    self.config.add_section(profile_name)
                
                # Mettre à jour les valeurs
                for key, value in profile_data.items():
                    self.config[profile_name][key] = str(value)
                
                self.save_config()
                logging.info(f"Profil {profile_name} sauvegardé")
                return True
            else:
                logging.warning("Impossible de modifier le profil DEFAULT directement")
                return False
        except Exception as e:
            logging.error(f"Erreur lors de la sauvegarde du profil {profile_name}: {str(e)}")
            return False
    
    def delete_profile(self, profile_name):
        """Supprime un profil existant.
        
        Args:
            profile_name: Nom du profil à supprimer
            
        Returns:
            bool: True si la suppression a réussi, False sinon
        """
        if profile_name == 'DEFAULT':
            logging.warning("Impossible de supprimer le profil DEFAULT")
            return False
            
        if profile_name in self.config.sections():
            self.config.remove_section(profile_name)
            self.save_config()
            logging.info(f"Profil {profile_name} supprimé")
            return True
        else:
            logging.warning(f"Profil {profile_name} non trouvé")
            return False
        
    def delete_current_profile(self, app, window):
        """Supprime le profil actuellement sélectionné.
        
        Args:
            app: Instance de l'application BacktestApp
            window: Fenêtre parente pour les dialogues
            
        Returns:
            bool: True si la suppression a réussi, False sinon
        """
        if self.current_profile == "DEFAULT":
            QMessageBox.information(window, "Information", "Le profil DEFAULT ne peut pas être supprimé.")
            return False
        
        confirm = QMessageBox.question(
            window,
            "Confirmation",
            f"Êtes-vous sûr de vouloir supprimer le profil '{self.current_profile}' ?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )
        
        if confirm == QMessageBox.Yes:
            profile_name = self.current_profile
            success = self.delete_profile(profile_name)
            
            if success:
                # Revenir au profil DEFAULT après la suppression
                self.apply_profile_to_ui("DEFAULT", app)
                
                # Mettre à jour le combobox s'il existe
                if hasattr(app, 'profile_combo'):
                    app.profile_combo.clear()
                    app.profile_combo.addItems(self.list_profiles())
                    app.profile_combo.setCurrentText("DEFAULT")
                
                logging.info(f"Profil '{profile_name}' supprimé.")
            else:
                QMessageBox.warning(window, "Erreur", f"Échec de la suppression du profil '{profile_name}'.")
            
            return success
        return False
    
    def extract_widget_value(self, widget):
        """Extrait la valeur d'un widget selon son type.
        
        Méthode auxiliaire pour traiter différents types de widgets.
        
        Args:
            widget: Widget dont on veut extraire la valeur
            
        Returns:
            La valeur du widget (type variable)
        """
        widget_type = type(widget).__name__
        
        if widget_type == "QCheckBox":
            return widget.isChecked()
        elif widget_type == "QComboBox":
            return widget.currentText()
        elif widget_type == "QLineEdit":
            return widget.text()
        elif widget_type == "QSpinBox" or widget_type == "QDoubleSpinBox":
            return widget.value()
        elif widget_type == "QDateEdit":
            return widget.date().toString("dd/MM/yyyy")
        elif widget_type == "QTimeEdit":
            return widget.time().toString("HH:mm:ss")
        else:
            logging.warning(f"Type de widget non géré: {widget_type}")
            return None
    
    def set_widget_value(self, widget, value, default=None):
        """Définit la valeur d'un widget selon son type.
        
        Args:
            widget: Widget à modifier
            value: Valeur à définir (sous forme de chaîne)
            default: Valeur par défaut si la conversion échoue
        """
        if value is None and default is not None:
            value = default
            
        widget_type = type(widget).__name__
        
        try:
            if widget_type == "QCheckBox":
                widget.setChecked(str(value).lower() == 'true')
            elif widget_type == "QComboBox":
                index = widget.findText(str(value))
                if index >= 0:
                    widget.setCurrentIndex(index)
            elif widget_type == "QLineEdit":
                widget.setText(str(value))
            elif widget_type == "QSpinBox":
                widget.setValue(int(value))
            elif widget_type == "QDoubleSpinBox":
                widget.setValue(float(value))
            elif widget_type == "QDateEdit":
                # Format attendu: dd/MM/yyyy
                from PyQt5.QtCore import QDate
                try:
                    parts = str(value).split('/')
                    if len(parts) == 3:
                        date = QDate(int(parts[2]), int(parts[1]), int(parts[0]))
                        widget.setDate(date)
                except:
                    if default:
                        widget.setDate(QDate.currentDate())
            elif widget_type == "QTimeEdit":
                from PyQt5.QtCore import QTime
                try:
                    parts = str(value).split(':')
                    if len(parts) >= 2:
                        hour = int(parts[0])
                        minute = int(parts[1])
                        second = int(parts[2]) if len(parts) > 2 else 0
                        time = QTime(hour, minute, second)
                        widget.setTime(time)
                except Exception as e:
                    logging.warning(f"Erreur lors de l'application de la valeur de temps: {e}")
                    if default:
                        widget.setTime(QTime.currentTime())
            else:
                logging.warning(f"Type de widget non géré pour la définition: {widget_type}")
        except Exception as e:
            logging.warning(f"Erreur lors de la définition de la valeur {value} pour {widget_type}: {str(e)}")
            if default is not None:
                self.set_widget_value(widget, default)
    
    def apply_profile_to_ui(self, profile_name, app):
        """Applique les paramètres du profil à l'interface utilisateur."""
        if not self.profile_exists(profile_name):
            logging.error(f"Profil {profile_name} inexistant")
            return False
            
        try:
            # Utiliser les valeurs par défaut originales pour le profil DEFAULT
            if profile_name == 'DEFAULT':
                profile = self.default_values.copy()
                # Si c'est la première fois et default_values est vide, initialiser avec les valeurs actuelles de l'UI
                if not profile:
                    profile = self.get_profile_from_ui(app)
                    self.default_values = profile.copy()
            else:
                profile = self.get_profile(profile_name)
                
            widget_mapping = self.get_widget_mapping(app)
            
            # Pour chaque widget du mapping, appliquer la valeur correspondante du profil
            for widget_name, widget in widget_mapping.items():
                if widget_name in profile:
                    self.set_widget_value(widget, profile[widget_name])
            
            # Définir ce profil comme profil courant
            self.current_profile = profile_name
            # Mettre à jour l'affichage du profil actif si le widget existe
            if hasattr(app, 'current_profile_label'):
                app.current_profile_label.setText(f"Profil actif: {profile_name}")
                
            logging.info(f"Profil {profile_name} appliqué à l'interface")
            return True
            
        except Exception as e:
            logging.error(f"Erreur lors de l'application du profil {profile_name}: {str(e)}")
            return False
    
    def get_widget_mapping(self, app):
        """Génère un mapping des noms de paramètres vers les widgets correspondants.
        
        Cette méthode est dynamique et s'adaptera si de nouveaux widgets sont ajoutés.
        
        Args:
            app: Instance de l'application BacktestApp
            
        Returns:
            dict: Dictionnaire {nom_paramètre: widget}
        """
        # Mapping des attributs de l'application vers les noms de paramètres dans la configuration

        general_params = app.get_strategy_config()

        # Initialiser le dictionnaire de mapping
        mapping = {}
        
        # Ajouter les widgets du panel de paramètres généraux
        if hasattr(app, 'general_params_panel') and app.general_params_panel:
            for key, widget in app.general_params_panel.widgets.items():
                # Convertir les noms de widgets en noms de paramètres
                param_name = key
                if key == 'symbol_combo':
                    param_name = 'symbol'
                elif key == 'period_combo':
                    param_name = 'period'
                elif key == 'interval_combo':
                    param_name = 'interval'
                elif key == 'end_date':
                    param_name = 'end_date'
                elif key == 'strategy_combo':
                    param_name = 'strategy'
                elif key == 'candle_type_combo':
                    param_name = 'candle_type'

                mapping[param_name] = widget

        # Ajouter les widgets du panel de base de la stratégie
        if hasattr(app, 'strategy_base_panel') and app.strategy_base_panel:
            for key, widget in app.strategy_base_panel.widgets.items():
                # Gestion particulière pour les jours de trading qui sont dans une liste
                if key == 'trading_days_check':
                    for i, day_check in enumerate(widget):
                        mapping[f'trading_day_{i}'] = day_check
                else:
                    # Conserver le mapping direct pour les autres widgets
                    mapping[key] = widget
        
        # Ajouter les widgets du panel spécifique à la stratégie
        if hasattr(app, 'strategy_specific_panel') and app.strategy_specific_panel:
            for key, widget in app.strategy_specific_panel.widgets.items():
                mapping[key] = widget
        
        return mapping
    
    def get_profile_from_ui(self, app):
        """Récupère les paramètres actuels de l'interface utilisateur.
        
        Cette méthode est dynamique et s'adaptera si de nouveaux widgets sont ajoutés.
        
        Args:
            app: Instance de l'application BacktestApp
            
        Returns:
            dict: Dictionnaire des paramètres extraits de l'interface
        """
        profile = {}
        widget_mapping = self.get_widget_mapping(app)
        
        # Pour chaque widget du mapping, extraire sa valeur
        for param_name, widget in widget_mapping.items():
            value = self.extract_widget_value(widget)
            if value is not None:
                profile[param_name] = str(value)
        
        # Ajouter un timestamp
        profile['last_modified'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        return profile
    
    def prompt_save_profile(self, app, window):
        """Affiche une boîte de dialogue pour sauvegarder le profil actuel.
        
        Args:
            app: Instance de l'application BacktestApp
            window: Fenêtre parente pour le dialogue
            
        Returns:
            bool: True si le profil a été sauvegardé, False sinon
        """
        profile_name, ok = QInputDialog.getText(
            window, 
            "Sauvegarder le profil", 
            "Nom du profil:",
            QLineEdit.Normal,
            ""
        )
        
        if ok and profile_name and profile_name != "DEFAULT":
            profile_data = self.get_profile_from_ui(app)
            success = self.save_profile(profile_name, profile_data)
            
            if success:
                QMessageBox.information(window, "Succès", f"Profil '{profile_name}' sauvegardé.")
            else:
                QMessageBox.warning(window, "Erreur", f"Échec de la sauvegarde du profil '{profile_name}'.")
            
            return success
        return False
    
    def prompt_load_profile(self, app, window):
        """Affiche une boîte de dialogue pour charger un profil.
        
        Args:
            app: Instance de l'application BacktestApp
            window: Fenêtre parente pour le dialogue
            
        Returns:
            bool: True si un profil a été chargé, False sinon
        """
        profiles = self.list_profiles()
        
        profile_name, ok = QInputDialog.getItem(
            window,
            "Charger un profil",
            "Sélectionnez un profil:",
            profiles,
            0,  # Index par défaut (DEFAULT)
            False  # Non éditable
        )
        
        if ok and profile_name:
            success = self.apply_profile_to_ui(profile_name, app)
            
            if success:
                QMessageBox.information(window, "Succès", f"Profil '{profile_name}' chargé.")
            else:
                QMessageBox.warning(window, "Erreur", f"Échec du chargement du profil '{profile_name}'.")
            
            return success
        return False
    
    def prompt_delete_profile(self, app, window):
        """Affiche une boîte de dialogue pour supprimer un profil.
        
        Args:
            app: Instance de l'application BacktestApp
            window: Fenêtre parente pour le dialogue
            
        Returns:
            bool: True si un profil a été supprimé, False sinon
        """
        # Exclure DEFAULT de la liste
        profiles = [p for p in self.list_profiles() if p != 'DEFAULT']
        
        if not profiles:
            QMessageBox.information(window, "Information", "Aucun profil à supprimer.")
            return False
        
        profile_name, ok = QInputDialog.getItem(
            window,
            "Supprimer un profil",
            "Sélectionnez un profil à supprimer:",
            profiles,
            0,
            False
        )
        
        if ok and profile_name:
            confirm = QMessageBox.question(
                window,
                "Confirmation",
                f"Êtes-vous sûr de vouloir supprimer le profil '{profile_name}' ?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No
            )
            
            if confirm == QMessageBox.Yes:
                success = self.delete_profile(profile_name)
                
                if success:
                    QMessageBox.information(window, "Succès", f"Profil '{profile_name}' supprimé.")
                else:
                    QMessageBox.warning(window, "Erreur", f"Échec de la suppression du profil '{profile_name}'.")
                
                return success
        return False
    
    def save_current_profile(self, app, window):
        """Sauvegarde les paramètres actuels dans le profil courant.
        
        Args:
            app: Instance de l'application BacktestApp
            window: Fenêtre parente pour les dialogues
            
        Returns:
            bool: True si la sauvegarde a réussi, False sinon
        """
        if self.current_profile == "DEFAULT":
            # On ne peut pas modifier le profil DEFAULT, demander un nouveau nom
            QMessageBox.information(window, "Information", 
                                   "Le profil DEFAULT est protégé. Veuillez créer un nouveau profil à la place.")
            return self.prompt_create_new_profile(app, window)
        else:
            # Mettre à jour le profil existant
            profile_data = self.get_profile_from_ui(app)
            success = self.save_profile(self.current_profile, profile_data)
            
            if success:
                QMessageBox.information(window, "Succès", f"Profil '{self.current_profile}' mis à jour.")
            else:
                QMessageBox.warning(window, "Erreur", f"Échec de la mise à jour du profil '{self.current_profile}'.")
            
            return success
    
    def prompt_create_new_profile(self, app, window):
        """Affiche une boîte de dialogue pour créer un nouveau profil.
        
        Args:
            app: Instance de l'application BacktestApp
            window: Fenêtre parente pour le dialogue
            
        Returns:
            bool: True si le profil a été créé, False sinon
        """
        profile_name, ok = QInputDialog.getText(
            window, 
            "Créer un nouveau profil", 
            "Nom du profil:",
            QLineEdit.Normal,
            ""
        )
        
        if ok and profile_name and profile_name != "DEFAULT":
            profile_data = self.get_profile_from_ui(app)
            success = self.save_profile(profile_name, profile_data)
            
            if success:
                QMessageBox.information(window, "Succès", f"Profil '{profile_name}' créé.")
                # Définir ce profil comme profil courant
                self.current_profile = profile_name
                # Mettre à jour l'affichage
                if hasattr(app, 'current_profile_label'):
                    app.current_profile_label.setText(f"Profil actif: {profile_name}")
                # Mettre à jour le combobox s'il existe
                if hasattr(app, 'profile_combo'):
                    app.profile_combo.clear()
                    app.profile_combo.addItems(self.list_profiles())
                    index = app.profile_combo.findText(profile_name)
                    if index >= 0:
                        app.profile_combo.setCurrentIndex(index)
            else:
                QMessageBox.warning(window, "Erreur", f"Échec de la création du profil '{profile_name}'.")
            
            return success
        return False
    

    def import_config_from_file(self, app, window):
        """Importe et fusionne une configuration depuis un fichier externe.
        
        Args:
            app: Instance de l'application BacktestApp
            window: Fenêtre parente pour les dialogues
            
        Returns:
            bool: True si l'importation a réussi, False sinon
        """
        # Ouvrir une boîte de dialogue pour sélectionner le fichier à importer
        file_path, _ = QFileDialog.getOpenFileName(
            window,
            "Importer une configuration",
            "",
            "Fichiers INI (*.ini);;Tous les fichiers (*)"
        )
        
        if not file_path:
            return False  # L'utilisateur a annulé
        
        try:
            # Sauvegarde du fichier actuel avant modification
            backup_path = self.config_file + ".bak"
            shutil.copy2(self.config_file, backup_path)
            
            # Charger la configuration externe
            imported_config = configparser.ConfigParser()
            imported_config.read(file_path)
            
            # Vérifier si le fichier contient des sections
            if len(imported_config.sections()) == 0 and not imported_config['DEFAULT']:
                QMessageBox.warning(window, "Erreur", "Le fichier sélectionné ne contient aucune configuration valide.")
                return False
            
            # Fusionner les sections importées
            for section in imported_config.sections():
                if section not in self.config:
                    self.config.add_section(section)
                    
                for key, value in imported_config[section].items():
                    self.config[section][key] = value
                    
            # Fusionner les valeurs DEFAULT si présentes
            if imported_config['DEFAULT']:
                for key, value in imported_config['DEFAULT'].items():
                    if key not in self.config['DEFAULT']:  # Ne pas écraser les valeurs par défaut existantes
                        self.config['DEFAULT'][key] = value
            
            # Enregistrer la nouvelle configuration
            success = self.save_config()
            
            if success:
                # Mettre à jour la liste des profils
                if hasattr(app, 'profile_combo'):
                    app.profile_combo.clear()
                    app.profile_combo.addItems(self.list_profiles())
                    
                QMessageBox.information(
                    window, 
                    "Succès", 
                    f"Configuration importée et fusionnée avec succès depuis {file_path}"
                )
            else:
                QMessageBox.warning(window, "Erreur", "Échec de l'enregistrement de la configuration.")
                
            return success
        
        except Exception as e:
            QMessageBox.critical(
                window,
                "Erreur",
                f"Une erreur est survenue lors de l'importation de la configuration:\n{str(e)}"
            )
            logging.error(f"Erreur lors de l'importation de la configuration: {str(e)}")
            return False
    
    def export_config_to_file(self, window, profile_name=None):
        """Exporte la configuration vers le dossier de téléchargements.
        
        Args:
            window: Fenêtre parente pour les dialogues
            profile_name: Paramètre ignoré (conservé pour compatibilité)
            
        Returns:
            bool: True si l'exportation a réussi, False sinon
        """
        try:
            # Obtenir le chemin du dossier de téléchargements
            downloads_path = os.path.join(os.path.expanduser("~"), "Downloads")
            if not os.path.exists(downloads_path):  # Sur système en français
                downloads_path = os.path.join(os.path.expanduser("~"), "Téléchargements")
                if not os.path.exists(downloads_path):  # Fallback au dossier home
                    downloads_path = os.path.expanduser("~")
            
            # Générer un nom de fichier avec horodatage
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            file_path = os.path.join(downloads_path, f"backtest_config_{timestamp}.ini")
            
            # Enregistrer la configuration dans le fichier
            with open(file_path, 'w') as configfile:
                self.config.write(configfile)
            
            QMessageBox.information(
                window,
                "Succès",
                f"Configuration exportée avec succès vers:\n{file_path}"
            )
            
            return True
        except Exception as e:
            QMessageBox.critical(
                window,
                "Erreur",
                f"Une erreur est survenue lors de l'exportation de la configuration:\n{str(e)}"
            )
            logging.error(f"Erreur lors de l'exportation de la configuration: {str(e)}")
            return False