"""
utils.py

Ce fichier contient des fonctions utilitaires pour interagir avec l'API IG Trading.
Il inclut des fonctionnalités pour rechercher des marchés, obtenir des informations,
récupérer des prix historiques, suivre les prix en temps réel et créer des positions.

Modules requis:
- trading_ig
- pandas
- pick
- readchar
- readline
- traceback
- sys
- time
"""
from trading_ig.rest import IGService
from trading_ig.config import config
import readchar
import readline
from pick import pick
import pandas as pd
import traceback
import sys
import time
from datetime import datetime, timezone
from markets import epics_dict
from pprint import pprint
import curses
from curses import wrapper

# Liste des options de direction pour les positions
direction_options = ["BUY", "SELL"]

# Dictionnaire contenant les résolutions et leurs noms
resolution_dict = {
    "M": "Minute",
    "H": "Hour",
    "D": "Day",
    "W": "Week",
    "ME": "Month"
}

def is_market_open(trading_hours):
    """
    Vérifie si le marché est actuellement ouvert.

    Args:
        trading_hours (dict): Les heures d'ouverture du marché.

    Returns:
        bool: True si le marché est ouvert, False sinon.
    """
    now = datetime.now(timezone.utc)
    current_day = now.strftime("%A")  # Ex: "Monday"
    current_time = now.strftime("%H:%M")  # Ex: "14:30"

    if current_day not in trading_hours:
        return False

    for start, end in trading_hours[current_day]:
        if start <= current_time <= end:
            return True

    return False

def prefill_input(prompt, text):
    """
    Pré-remplit une entrée utilisateur avec un texte par défaut.

    Args:
        prompt (str): Le message à afficher à l'utilisateur.
        text (str): Le texte par défaut à pré-remplir.

    Returns:
        str: La valeur saisie par l'utilisateur.
    """
    def hook():
        readline.insert_text(text)
        readline.redisplay()
    readline.set_pre_input_hook(hook)
    result = input(prompt)
    readline.set_pre_input_hook()
    return result

def initialize_service():
    """
    Initialise la connexion au service IG.

    Returns:
        IGService: Une instance du service IG connectée.
    """
    print("Connexion au service IG...")
    ig_service = IGService(
        config.username, 
        config.password, 
        config.api_key, 
        config.acc_type,
        acc_number=config.acc_number)
    ig_service.create_session(version='3')
    print(f"✅ Connexion réussie avec le compte de {config.username}")
    return ig_service

# Initialisation du service IG
ig_service = IGService(
    config.username, 
    config.password, 
    config.api_key, 
    config.acc_type,
    acc_number=config.acc_number)
ig = ig_service.create_session(version='3')

def select_from_dict(dict):
    """
    Permet de sélectionner un élément depuis un dictionnaire imbriqué avec navigation par flèches.

    Args:
        dict (dict): Le dictionnaire contenant les options.

    Returns:
        tuple: L'EPIC sélectionné et ses données associées.
    """
    try:
        options = []
        dict_list = []
        for epic, data in dict.items():
            options.append(f"{data['name']} ({epic})")
            dict_list.append(epic)
        title = "Sélectionnez un marché avec les flèches ↑↓ puis Entrée pour confirmer:"
        selected_option, index = pick(options, title)
        return dict_list[index], dict[dict_list[index]]
    except ImportError:
        print("📦 Le module 'pick' n'est pas installé. Utilisation du mode de sélection basique.")
        print("Pour une meilleure expérience, installez-le avec: pip install pick")
        print("\nMarchés disponibles:")
        for i, (epic, data) in enumerate(dict.items(), 1):
            print(f"{i}. {data['name']} ({epic})")
        while True:
            try:
                choice = int(input("\nEntrez le numéro du marché: "))
                if 1 <= choice <= len(dict):
                    selected_epic = list(dict.keys())[choice - 1]
                    return selected_epic, dict[selected_epic]
                else:
                    print("⚠️ Numéro invalide. Veuillez réessayer.")
            except ValueError:
                print("⚠️ Veuillez entrer un numéro valide.")

def search_market(ig_service):
    """
    Recherche un marché spécifique en utilisant une sélection interactive.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """
    selected_epic, selected_data = select_from_dict(epics_dict)
    print(f"Recherche de '{selected_epic}'...")
    try:
        result = ig_service.search_markets(selected_epic)
        print("\nRésultats de la recherche:")
        if isinstance(result, pd.DataFrame) and not result.empty:
            print(f"Trouvé {len(result)} marchés:")
            for index, row in result.iterrows():
                print(f"- {row['epic']}: {row['instrumentName']}")
        elif isinstance(result, dict) and 'markets' in result:
            for item in result['markets']:
                print(f"- {item['epic']}: {item['instrumentName']}")
        else:
            print("Aucun marché trouvé avec ce terme de recherche.")
    except Exception as e:
        print(f"\n⚠️ Erreur lors de la recherche: {e}")
    input("\nAppuyez sur Entrée pour continuer...")

def get_market_info(ig_service):
    """
    Obtient des informations détaillées sur un marché.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """
    selected_epic, selected_data = select_from_dict(epics_dict)    
    print(f"Récupération des informations pour {selected_epic} alias {selected_data["name"]}")
    try:
        market = ig_service.fetch_market_by_epic(selected_epic)
        print("\nInformations sur le marché:")
        print(f"Nom: {market['instrument']['name']}")
        if is_market_open(selected_data["trading_hours"]):
            print("🟢 OUVERT")
        else:
            print("🔴 FERMÉ")
        bid = market['snapshot']['bid']
        ask = market['snapshot']['offer']
        spread = round((ask - bid) * 10000, 1)
        print(f"Bid: {bid}")
        print(f"Ask: {ask}")
        print(f"Spread: {spread} pips")
        if 'dealingRules' in market:
            print("\nRègles de trading:")
            if 'minDealSize' in market['dealingRules']:
                print(f"Taille minimale: {market['dealingRules']['minDealSize']['value']}")
            if 'maxDealSize' in market['dealingRules']:
                print(f"Taille maximale: {market['dealingRules']['maxDealSize']['value']}")
    except Exception as e:
        print(f"⚠️ Erreur: {e}")
        traceback.print_exc()
    input("\nAppuyez sur Entrée pour continuer...")

def get_historical_prices(ig_service):
    """
    Récupère les prix historiques d'un marché.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """
    selected_epic, selected_data = select_from_dict(epics_dict)
    selected_resolution, selected_resolution_data = select_from_dict(resolution_dict)
    num_points_default = "10"
    num_points = prefill_input(f"Nombre de points (1-100): ", num_points_default) 
    print(f"Récupération des prix historiques pour '{selected_epic}'...")
    try:
        num_points = int(num_points)
        result = ig_service.fetch_historical_prices_by_epic(
            epic=selected_epic,
            resolution=selected_resolution,
            numpoints=num_points
        )
        if isinstance(result, pd.DataFrame):
            print("\nDerniers prix historiques (DataFrame):")
            print(result.head())
        elif isinstance(result, dict) and 'prices' in result:
            if isinstance(result['prices'], list):
                print("\nDerniers prix historiques:")
                for price in result['prices'][:5]:
                    print(f"Date: {price['snapshotTime']}, Ouv: {price['openPrice']['ask']}, Ferm: {price['closePrice']['ask']}")
            else:
                print(result['prices'])
        else:
            print("\nStructure de la réponse inconnue:")
            print(f"Type: {type(result)}")
            print(result)
    except Exception as e:
        print(f"⚠️ Erreur: {e}")
        traceback.print_exc()
    input("\nAppuyez sur Entrée pour continuer...")

def track_realtime_prices(ig_service):
    """
    Suit les prix d'un marché en temps réel.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """
    selected_epic, selected_data = select_from_dict(epics_dict)
    iterations = int(prefill_input("Nombre d'itérations (1-100): ", "10"))
    delay = int(prefill_input("Délai entre les itérations (en secondes): ", "5"))
    print(f"🔄 Récupération des prix de {selected_epic} en temps réel...")
    for i in range(min(iterations, 100)):
        try:
            result = ig_service.fetch_market_by_epic(selected_epic)
            if "snapshot" in result:
                bid = result["snapshot"]["bid"]
                ask = result["snapshot"]["offer"]
                print(f"⏱️ {i+1}/{iterations} - Bid: {bid}, Ask: {ask}")
            else:
                print("⚠️ Impossible de récupérer les données.")
        except Exception as e:
            print(f"⚠️ Erreur: {e}")
        time.sleep(delay)
    print("✅ Fin de la récupération des prix.")
    input("\nAppuyez sur Entrée pour continuer...")

def create_position(ig_service):
    """
    Crée une position d'achat ou de vente avec une interface interactive
    pour la configuration de tous les paramètres.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """

    # Fonction principale pour l'interface curses
    def main_interface(stdscr):
        # Initialiser l'écran
        curses.curs_set(0)  # Cacher le curseur
        stdscr.clear()
        height, width = stdscr.getmaxyx()
        
        # Paramètres par défaut
        selected_epic, selected_data = select_from_dict(epics_dict)
        params = {
            "epic": selected_epic,
            "direction": "BUY",
            "size": 1.0,
            "currency_code": "EUR",
            "order_type": "MARKET",
            "expiry": "DFB",
            "force_open": "false",
            "guaranteed_stop": "false",
            "level": None,
            "limit_distance": None,
            "limit_level": None,
            "stop_distance": None,
            "stop_level": None,
            "trailing_stop": "false",
            "trailing_stop_increment": None
        }
        
        # Options pour certains paramètres
        options = {
            "direction": ["BUY", "SELL"],
            "order_type": ["MARKET", "LIMIT", "QUOTE"],
            "expiry": ["DFB", "1", "2", "3", "7"],
            "force_open": ["false", "true"],
            "guaranteed_stop": ["false", "true"],
            "trailing_stop": ["false", "true"],
        }
        
        # Types pour chaque paramètre
        param_types = {
            "epic": "str",
            "direction": "option",
            "size": "float",
            "currency_code": "str",
            "order_type": "option",
            "expiry": "option",
            "force_open": "option",
            "guaranteed_stop": "option",
            "level": "float_nullable",
            "limit_distance": "float_nullable",
            "limit_level": "float_nullable",
            "stop_distance": "float_nullable",
            "stop_level": "float_nullable",
            "trailing_stop": "option",
            "trailing_stop_increment": "float_nullable",
        }
        
        # Paramètres à afficher et leur description
        display_params = [
            ("epic", "Marché"),
            ("direction", "Direction"),
            ("size", "Taille"),
            ("currency_code", "Devise"),
            ("order_type", "Type d'ordre"),
            ("expiry", "Expiration"),
            ("force_open", "Forcer ouverture"),
            ("guaranteed_stop", "Stop garanti"),
            ("level", "Niveau (prix)"),
            ("limit_distance", "Distance limite"),
            ("limit_level", "Niveau limite"),
            ("stop_distance", "Distance stop"),
            ("stop_level", "Niveau stop"),
            ("trailing_stop", "Stop suiveur"),
            ("trailing_stop_increment", "Incrément stop suiveur")
        ]
        
        current_param_index = 0
        edit_mode = False
        current_edit_value = ""
        option_index = 0
        confirm_position = False
        
        # Boucle principale
        while True:
            stdscr.clear()
            
            # Titre
            title = "CRÉATION DE POSITION - CONFIGURATION"
            stdscr.addstr(1, (width - len(title)) // 2, title, curses.A_BOLD)
            
            # Instructions
            instructions = [
                "Utilisez ↑/↓ pour naviguer, Entrée pour modifier/confirmer",
                "Tab pour passer au paramètre suivant, Échap pour annuler",
                "F10 pour confirmer et créer la position"
            ]
            for i, instr in enumerate(instructions):
                stdscr.addstr(3 + i, 2, instr)
            
            # Afficher les paramètres
            for i, (param, desc) in enumerate(display_params):
                y_pos = 7 + i
                
                # Mise en évidence du paramètre sélectionné
                if i == current_param_index:
                    attr = curses.A_REVERSE
                else:
                    attr = curses.A_NORMAL
                
                # Afficher la description et la valeur
                value = params[param]
                if value is None:
                    value_str = "Non défini"
                else:
                    value_str = str(value)
                
                # Affichage du paramètre
                stdscr.addstr(y_pos, 2, f"{desc:<20}: ", attr)
                
                # Si en mode édition et c'est le paramètre actuel
                if edit_mode and i == current_param_index:
                    if param_types[param] == "option":
                        # Afficher l'option actuelle avec un indicateur de sélection
                        option_list = " | ".join(options[param])
                        stdscr.addstr(y_pos, 24, option_list)
                        opt_pos = 24
                        for j, opt in enumerate(options[param]):
                            if opt == current_edit_value:
                                stdscr.addstr(y_pos, opt_pos, opt, curses.A_REVERSE)
                            opt_pos += len(opt) + 3  # +3 pour " | "
                    else:
                        # Afficher la valeur en cours d'édition
                        stdscr.addstr(y_pos, 24, current_edit_value)
                else:
                    stdscr.addstr(y_pos, 24, value_str)
            
            # Bouton de confirmation
            confirmation_text = "[ CRÉER POSITION ]"
            if confirm_position:
                stdscr.addstr(7 + len(display_params) + 2, (width - len(confirmation_text)) // 2, 
                             confirmation_text, curses.A_REVERSE)
            else:
                stdscr.addstr(7 + len(display_params) + 2, (width - len(confirmation_text)) // 2, 
                             confirmation_text)
            
            # Rafraîchir l'écran
            stdscr.refresh()
            
            # Gestion des touches
            key = stdscr.getch()
            
            if edit_mode:
                # Mode édition
                if key == 27:  # Échap
                    edit_mode = False
                elif key == 10:  # Entrée
                    edit_mode = False
                    param = display_params[current_param_index][0]
                    if param_types[param] == "float" or param_types[param] == "float_nullable":
                        try:
                            if current_edit_value == "":
                                params[param] = None
                            else:
                                params[param] = float(current_edit_value)
                        except ValueError:
                            # Ignorer les valeurs non numériques
                            pass
                    elif param_types[param] == "option":
                        params[param] = current_edit_value
                    else:
                        params[param] = current_edit_value
                elif param_types[display_params[current_param_index][0]] == "option":
                    param = display_params[current_param_index][0]
                    opts = options[param]
                    if key == curses.KEY_RIGHT and opts.index(current_edit_value) < len(opts) - 1:
                        current_edit_value = opts[opts.index(current_edit_value) + 1]
                    elif key == curses.KEY_LEFT and opts.index(current_edit_value) > 0:
                        current_edit_value = opts[opts.index(current_edit_value) - 1]
                else:
                    # Éditer la valeur
                    if key == curses.KEY_BACKSPACE or key == 127:
                        current_edit_value = current_edit_value[:-1]
                    elif 32 <= key <= 126:  # Caractères ASCII imprimables
                        current_edit_value += chr(key)
            else:
                # Mode navigation
                if key == curses.KEY_UP:
                    if current_param_index > 0:
                        current_param_index -= 1
                    elif confirm_position:
                        confirm_position = False
                        current_param_index = len(display_params) - 1
                elif key == curses.KEY_DOWN:
                    if current_param_index < len(display_params) - 1:
                        current_param_index += 1
                    else:
                        confirm_position = True
                elif key == 9:  # Tab
                    if current_param_index < len(display_params) - 1:
                        current_param_index += 1
                    else:
                        current_param_index = 0
                elif key == 10:  # Entrée
                    if confirm_position:
                        # Créer la position
                        return params
                    else:
                        edit_mode = True
                        param = display_params[current_param_index][0]
                        current_edit_value = str(params[param]) if params[param] is not None else ""
                        if param_types[param] == "option":
                            current_edit_value = params[param]
                elif key == 27:  # Échap
                    return None
                elif key == curses.KEY_F10:
                    # Créer la position
                    return params
    
    # Exécuter l'interface curses
    try:
        position_params = wrapper(main_interface)
        
        if position_params is None:
            print("❌ Création de position annulée.")
            return
        
        print(f"Création d'une position {position_params['direction']} sur {position_params['epic']}...")
        
        # Filtrer les paramètres None
        filtered_params = {k: v for k, v in position_params.items() if v is not None}
        
        try:
            resp = ig_service.create_open_position(**filtered_params)
            
            if resp['dealStatus'] == 'ACCEPTED':
                print("✅ Position créée avec succès. Détails de la réponse :")
                pprint(resp, width=80, sort_dicts=False)
            else:
                print(f"⚠️ Échec de la création de la position:")
                print(f"Raison : {resp['reason']}")
        except Exception as e:
            print(f"⚠️ Erreur lors de la création de la position: {e}")
            traceback.print_exc()
    except Exception as e:
        print(f"⚠️ Erreur dans l'interface: {e}")
        traceback.print_exc()
    
    input("\nAppuyez sur Entrée pour continuer...")

def display_menu():
    """
    Affiche le menu principal.

    Returns:
        str: Le choix de l'utilisateur.
    """
    print("\n" + "="*50)
    print("             IG TRADING BOT - MENU PRINCIPAL")
    print("="*50)
    print("1. Rechercher un marché")
    print("2. Obtenir des informations sur un marché")
    print("3. Récupérer les prix historiques")
    print("4. Suivre les prix en temps réel")
    print("5. Créer une position d'achat/vente")
    print("0. Quitter")
    print("="*50)
    return input("Votre choix: ")