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
import sys
sys.path.insert(0, '/home/max/ig-trading-bot')
from trading_ig.rest import IGService
from trading_ig.config import config
import readchar
import readline
from pick import pick
import pandas as pd
import traceback
import time
from datetime import datetime, timezone
from scripts.markets import epics_dict
from pprint import pprint
import curses
from curses import wrapper
import plotext as plt
import numpy as np
import talib 

# Liste des options de direction pour les positions
direction_options = ["BUY", "SELL"]

# Dictionnaire contenant les résolutions et leurs noms
resolution_dict = {
    "1Min": "1Min",
    # "2Min": "2Min",
    # "3Min": "3Min",
    "5Min": "5Min",
    # "10Min": "10Min",
    "15Min": "15Min",
    # "30Min": "30Min",
    "1H": "1H",
    # "2H": "2H",
    # "3H": "3H",
    "4H": "4H",
    "D": "D",
    "W": "W",
    "M": "M"
    # "ME": "Minute",
    # "H": "Hour",
    # "D": "Day",
    # "W": "Week",
    # "M": "Month"
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
    current_time = now.strftime("%H:%ME")  # Ex: "14:30"

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

def select_from_dict(data_dict):
    """
    Permet de sélectionner un élément depuis un dictionnaire imbriqué ou simple avec navigation par flèches.

    Args:
        data_dict (dict): Le dictionnaire contenant les options.

    Returns:
        tuple: La clé sélectionnée et sa valeur associée.
    """
    try:
        options = []
        keys = list(data_dict.keys())
        
        # Construire les options en fonction du type des valeurs
        for key, value in data_dict.items():
            if isinstance(value, dict):  # Cas d'un dictionnaire imbriqué
                options.append(f"{value.get('name', key)} ({key})")
            else:  # Cas d'un dictionnaire simple
                options.append(f"{key}: {value}")
        
        title = "Sélectionnez une option avec les flèches ↑↓ puis Entrée pour confirmer:"
        selected_option, index = pick(options, title)
        return keys[index], data_dict[keys[index]]
    except ImportError:
        print("📦 Le module 'pick' n'est pas installé. Utilisation du mode de sélection basique.")
        print("Pour une meilleure expérience, installez-le avec: pip install pick")
        print("\nOptions disponibles:")
        for i, (key, value) in enumerate(data_dict.items(), 1):
            if isinstance(value, dict):
                print(f"{i}. {value.get('name', key)} ({key})")
            else:
                print(f"{i}. {key}: {value}")
        while True:
            try:
                choice = int(input("\nEntrez le numéro de l'option: "))
                if 1 <= choice <= len(data_dict):
                    selected_key = keys[choice - 1]
                    return selected_key, data_dict[selected_key]
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
    search_query = prefill_input("Recherche d'un marché (ex: 'EURUSD'): ", "EURUSD")
    try:
        result = ig_service.search_markets(input)
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
        result = ig_service.search_markets(search_query)
        print(f"\nRésultats de la recherche pour '{search_query}':")
        
        # Configurer pandas pour afficher toutes les colonnes et éviter la troncature
        with pd.option_context('display.max_rows', None, 'display.max_columns', None, 'display.width', 1000):
            if isinstance(result, pd.DataFrame) and not result.empty:
                print(result)
            elif isinstance(result, dict) and 'markets' in result:
                markets_df = pd.DataFrame(result['markets'])
                print(markets_df)
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
    # print(f"Récupération des informations pour {selected_epic} alias {selected_data["name"]}")
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
    
def plot_prices(prices_df):
    """
    Affiche un graphe des prix historiques dans le terminal.

    Args:
        prices_df (pd.DataFrame): Les données historiques des prix sous forme de DataFrame.
    """
    try:
        # Vérifier la résolution des timestamps
        date_diff = (prices_df.index[1] - prices_df.index[0]).days
        
        print(date_diff)

        if date_diff > 0:
            # Si plusieurs jours, utiliser la date complète
            x_values = list(prices_df.index.strftime('%d/%m/%Y'))
            x_labels = x_values
        else:
            # Sinon, afficher l'heure et les minutes sous forme décimale
            x_values = prices_df.index.astype(np.int64)
            x_labels = (prices_df.index.hour + prices_df.index.minute / 100.0).round(2)
        
        candlestick_data = pd.DataFrame({
            "Date": x_values,
            "Open": prices_df[('bid', 'Open')].values,
            "High": prices_df[('bid', 'High')].values,
            "Low": prices_df[('bid', 'Low')].values,
            "Close": prices_df[('bid', 'Close')].values
            # "Volume": prices_df[('last', 'Volume')].fillna(0).values  # Remplir NaN par 0
        })

        # Configurer le graphe
        plt.clear_data()
        plt.candlestick(candlestick_data["Date"], candlestick_data)
        plt.title("Prix historiques (Bougies)")
        plt.xlabel("Temps" if date_diff == 0 else "Date")
        plt.ylabel("Prix")
        plt.grid(horizontal=True)

        step = max(1, len(x_labels) // 10)  # Afficher environ 10 labels max
        plt.xticks(x_values[::step], x_labels[::step])  # Associer les valeurs continues aux labels lisibles
        plt.show()

    except KeyError as e:
        print(f"⚠️ Erreur : Clé manquante dans les données - {e}")
    except Exception as e:
        print(f"⚠️ Une erreur s'est produite lors de l'affichage du graphe : {e}")

def fetch_prices(ig_service, epic, resolution='1Min', num_points=50):
    prices = ig_service.fetch_historical_prices_by_epic(epic, resolution=resolution, numpoints=num_points)
    
    # Debug: Print the structure of the returned data
    #print("DEBUG: Fetched prices:", prices)
    
    if 'prices' not in prices:
        raise KeyError("'prices' key not found in the API response.")
    
    # Convert the 'prices' data into a DataFrame
    df = pd.DataFrame(prices['prices'])
    
    # Check if the 'bid' column exists and contains the 'Close' sub-column
    if 'bid' not in df.columns or 'Close' not in df['bid']:
        raise KeyError("'Close' sub-column not found in the 'bid' column.")
    
    # Extract the 'Close' prices from the 'bid' column
    df['close'] = df['bid']['Close']
    
    if df['close'].isnull().all():
        raise ValueError("No valid 'Close' prices found in the 'bid' column.")
    
    return df[['close']]

def get_historical_prices(ig_service):
    """
    Récupère les prix historiques d'un marché.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """
    selected_epic, selected_data = select_from_dict(epics_dict)
    selected_resolution, selected_resolution_data = select_from_dict(resolution_dict)
    num_points_default = "50"
    num_points = prefill_input(f"Nombre de points (1-1000): ", num_points_default) 
    print(f"Récupération des prix historiques pour '{selected_epic}'...")
    try:
        num_points = int(num_points)
        result = ig_service.fetch_historical_prices_by_epic(
            epic=selected_epic,
            resolution=selected_resolution,
            numpoints=num_points
        )

        print(f"Type: {type(result['prices'])}")
        print(result['prices'])
        plot_prices(result['prices'])
        
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
    
def position_output(result):                              
    if(result['status'] == 'OPEN'):
        print("\n✅ Position créée avec succès!")
        print(f"Deal reference: {result['dealReference']}")
        print(f"Deal ID: {result.get('dealId', 'N/A')}")
        print(f"Status: {result.get('status', 'N/A')}")
    else:
        print("\n⚠️ Erreur lors de la création de la position:")
        print(f"Raison: {result['reason']}")
        print(f"Status: {result['status']}")
        print("Détails : ", result)

def create_position(ig_service):
    """
    Crée une position d'achat ou de vente avec une interface interactive
    pour la configuration de tous les paramètres.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """
    def position_interface(stdscr):
        # Configuration initiale de curses
        curses.curs_set(0)  # Masquer le curseur
        stdscr.clear()
        stdscr.refresh()
        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)  # Titre
        curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)  # Valeurs validées
        curses.init_pair(3, curses.COLOR_RED, curses.COLOR_BLACK)    # Erreurs
        curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_GREEN)  # Sélection
        
        # Paramètres de la position avec valeurs par défaut
        position_params = {
            "epic": {"value": "", "required":True, "editable":False, "desc": "Identifiant du marché"},
            "direction": {"value": "BUY", "options": ["BUY", "SELL"], "required":True, "editable":True, "desc": "Direction de la position"},
            "size": {"value": "1.0", "required":True, "editable":True, "desc": "Taille de la position"},
            "currency_code": {"value": "EUR", "required":True, "editable":False, "desc": "Code de la devise"},
            "expiry": {"value": "DFB", "required":True, "editable":True, "desc": "Date d'expiration ('DFB' pour aucune)"},
            "order_type": {"value": "MARKET", "options": ["LIMIT", "MARKET"], "required":True, "editable":True, "desc": "Type d'ordre"},
            "level": {"value": "", "required":False, "editable":True, "desc": "Niveau de prix (requis pour les ordres LIMIT)"},
            "guaranteed_stop": {"value": "False", "options": ["True", "False"], "required":True, "editable":True, "desc": "Stop garanti"},
            "stop_level": {"value": "", "required":False, "editable":True, "desc": "Niveau de stop-loss"},
            "stop_distance": {"value": "", "required":False, "editable":True, "desc": "Distance du stop-loss"},
            "limit_level": {"value": "", "required":False, "editable":True, "desc": "Niveau de take-profit"},
            "limit_distance": {"value": "", "required":False, "editable":True, "desc": "Distance du take-profit"},
            "force_open": {"value": "True", "options": ["True", "False"], "required":True, "editable":True, "desc": "Forcer l'ouverture"},
            "trailing_stop": {"value": "False", "options": ["True", "False"], "required":True, "editable":True, "desc": "Stop suiveur"},
            "trailing_stop_increment": {"value": "", "required":False, "editable":True, "desc": "Incrément du stop suiveur"},
            "time_in_force": {"value": "FILL_OR_KILL", "options": ["FILL_OR_KILL", "EXECUTE_AND_ELIMINATE"], "required":False, "editable":True, "desc": "Validité de l'ordre"}
        }
        
        # Sélection de l'EPIC
        stdscr.clear()
        stdscr.addstr(0, 0, "Création d'une nouvelle position", curses.A_BOLD)
        stdscr.refresh()
        curses.endwin()  # Sortir temporairement de curses
        
        selected_epic, selected_data = select_from_dict(epics_dict)
        position_params["epic"]["value"] = selected_epic
        
        # Reprendre l'interface curses
        stdscr = curses.initscr()
        curses.noecho()
        curses.cbreak()
        stdscr.keypad(True)

        current_param = 1  # Index du paramètre actuel (commence à 1 car epic est déjà défini)
        param_keys = list(position_params.keys())
        max_y, max_x = stdscr.getmaxyx()
        scroll_offset = 0
        
        # Récupération des informations du marché pour afficher le prix actuel
        market_info = ig_service.fetch_market_by_epic(selected_epic)
        current_bid = market_info['snapshot']['bid']
        current_ask = market_info['snapshot']['offer']
        while True:
            
            stdscr.clear()
            stdscr.addstr(0, 0, f"Configuration de la position - {selected_data['name']}", curses.color_pair(1) | curses.A_BOLD)
            stdscr.addstr(1, 0, f"Prix actuel: Achat: {current_ask} | Vente: {current_bid}", curses.A_BOLD)
            stdscr.addstr(2, 0, "Utilisez ↑↓ pour naviguer, Entrée pour modifier, Échap pour quitter", curses.A_ITALIC)
            stdscr.addstr(3, 0, "-" * (max_x - 1))
            
            # Modifier cette section dans la fonction position_interface
            
            # Affichage des paramètres avec défilement
            visible_rows = max_y - 7
            for i in range(min(visible_rows, len(param_keys))):
                idx = i + scroll_offset
                if idx >= len(param_keys):
                    break
                
                key = param_keys[idx]
                param = position_params[key]
                
                # Vérifie si le paramètre est actuellement sélectionné
                is_selected = idx == current_param
                is_editable = param.get("editable", True)
                
                # Style basé sur l'état
                if is_selected:
                    attr = curses.color_pair(4)
                elif param["required"] and not param["value"]:
                    attr = curses.color_pair(3)
                else:
                    attr = curses.color_pair(2) if param["value"] else curses.A_NORMAL
                
                # Formatage de la ligne (ajout d'un indicateur pour les champs non éditables)
                if is_editable:
                    prefix = "  "
                else:
                    prefix = "🔒"
                
                line = f"{prefix} {key:23} : {param['value']:15} - {param['desc']}"
                stdscr.addstr(i + 4, 0, line, attr)
            
            # Affichage des instructions en bas
            stdscr.addstr(max_y-3, 0, "-" * (max_x - 1))
            stdscr.addstr(max_y-2, 0, "Appuyez sur 'C' pour confirmer et créer la position")
            
            stdscr.refresh()
            
            # Gestion des touches
            key = stdscr.getch()
            
            if key == curses.KEY_UP:
                current_param = max(0, current_param - 1)
                # Ajuster le défilement si nécessaire
                if current_param < scroll_offset:
                    scroll_offset = current_param
                    
            elif key == curses.KEY_DOWN:
                current_param = min(len(param_keys) - 1, current_param + 1)
                # Ajuster le défilement si nécessaire
                if current_param >= scroll_offset + visible_rows:
                    scroll_offset = current_param - visible_rows + 1
                                
            elif key == 10 or key == 13:  # Entrée
                current_key = param_keys[current_param]
                current_value = position_params[current_key]
                
                # Vérifier si le paramètre est éditable
                if not current_value.get("editable", True):
                    # Afficher un message indiquant que le paramètre n'est pas éditable
                    stdscr.addstr(max_y-1, 0, f"Le paramètre '{current_key}' n'est pas éditable", curses.color_pair(3))
                    stdscr.refresh()
                    time.sleep(1)  # Afficher le message pendant 1 seconde
                else:
                    # Si le paramètre a des options prédéfinies, utiliser pick
                    if "options" in current_value:
                        curses.endwin()  # Quitter curses temporairement
                        options = current_value["options"]
                        title = f"Sélectionnez une valeur pour '{current_key}':"
                        selected, _ = pick(options, title)
                        position_params[current_key]["value"] = selected
                        stdscr = curses.initscr()  # Réinitialiser curses
                        curses.noecho()
                        curses.cbreak()
                        stdscr.keypad(True)
                    else:
                        # Sinon, demander une saisie manuelle
                        stdscr.addstr(current_param + 4, 0, " " * (max_x - 1))
                        prompt = f"{current_key}: "
                        stdscr.addstr(current_param + 4, 0, prompt)
                        
                        # Activer le curseur et l'écho pour la saisie
                        curses.echo()
                        curses.curs_set(1)
                        
                        # Préparation de la zone de saisie
                        input_y, input_x = current_param + 4, len(prompt)
                        stdscr.move(input_y, input_x)
                        
                        # Récupération de la saisie
                        input_str = ""
                        while True:
                            ch = stdscr.getch()
                            if ch == 10 or ch == 13:  # Entrée
                                break
                            elif ch == 27:  # Échap
                                input_str = position_params[current_key]["value"]  # Annuler
                                break
                            elif ch == 127 or ch == 8:  # Retour arrière
                                if input_str:
                                    input_str = input_str[:-1]
                                    stdscr.addstr(input_y, input_x, " " * (max_x - input_x))
                                    stdscr.addstr(input_y, input_x, input_str)
                            else:
                                input_str += chr(ch)
                        
                        position_params[current_key]["value"] = input_str
                        curses.noecho()
                        curses.curs_set(0)
                    
            elif key == ord('c') or key == ord('C'):  # Confirmer
                # Vérifier les paramètres requis
                missing_params = [k for k, v in position_params.items() 
                                 if v["required"] and not v["value"]]
                
                if missing_params:
                    stdscr.addstr(max_y-1, 0, f"Erreur: Paramètres obligatoires manquants: {', '.join(missing_params)}", 
                                 curses.color_pair(3))
                    stdscr.refresh()
                    stdscr.getch()  # Attendre une touche
                else:
                    # Demander confirmation
                    stdscr.addstr(max_y-1, 0, "Êtes-vous sûr de vouloir créer cette position? (O/N)", curses.A_BOLD)
                    stdscr.refresh()
                    
                    while True:
                        confirm = stdscr.getch()
                        if confirm in [ord('o'), ord('O')]:
                            # Créer la position
                            try:
                                # Préparer les paramètres à envoyer
                                params = {}
                                for k, v in position_params.items():
                                    if v["value"] is not None:  # Inclure uniquement les paramètres définis
                                        # Convertir les valeurs booléennes et numériques
                                        if v["value"] in ["True", "False"]:
                                            params[k] = v["value"] == "True"
                                        elif k in ["size", "level", "limit_distance", "limit_level", 
                                                   "stop_distance", "stop_level", "trailing_stop_increment"]:
                                            try:
                                                params[k] = float(v["value"]) if v["value"] else None
                                            except ValueError:
                                                params[k] = None
                                        else:
                                            params[k] = v["value"]
                                
                                # Ajouter les paramètres manquants avec des valeurs par défaut
                                required_params = [
                                    "level", "limit_distance", "limit_level", "quote_id", 
                                    "stop_distance", "stop_level", "trailing_stop_increment"
                                ]
                                for param in required_params:
                                    if param not in params:
                                        params[param] = None  # Valeur par défaut
                                
                                # Sortir de curses pour afficher les résultats
                                curses.endwin()
                                # Debug: Afficher les paramètres avant l'appel à l'API
                                print("DEBUG: Paramètres envoyés à l'API:", params)
                                # Appel de l'API pour créer la position
                                result = ig_service.create_open_position(**params)
                                print(result)
                                
                                position_output(result)
                                                               
                                input("\nAppuyez sur Entrée pour continuer...")
                                return  # Sortir de la fonction
                                
                            except Exception as e:
                                curses.endwin()
                                print(f"\n⚠️ Erreur lors de la création de la position: {e}")
                                traceback.print_exc()
                                input("\nAppuyez sur Entrée pour continuer...")
                                return
                                
                        elif confirm in [ord('n'), ord('N')]:
                            break  # Revenir à l'édition
                        
            elif key == 27:  # Échap
                # Demander confirmation pour quitter
                stdscr.addstr(max_y-1, 0, "Êtes-vous sûr de vouloir quitter sans créer de position? (O/N)", curses.A_BOLD)
                stdscr.refresh()
                
                while True:
                    confirm = stdscr.getch()
                    if confirm in [ord('o'), ord('O')]:
                        return  # Sortir de la fonction
                    elif confirm in [ord('n'), ord('N')]:
                        break  # Continuer l'édition
    
    # Exécuter l'interface interactive
    try:
        wrapper(position_interface)
    except Exception as e:
        print(f"\n⚠️ Erreur dans l'interface: {e}")
        traceback.print_exc()
    finally:
        # S'assurer que le terminal est correctement restauré
        try:
            curses.endwin()
        except:
            pass
        
# ------------Indicateurs techniques----------------

def calculate_atr(data, period=14):
    """
    Calcule l'Average True Range (ATR) pour une série de données OHLC.
    
    Args:
        data (pd.DataFrame): Les données avec colonnes High, Low, Close
        period (int): La période pour le calcul de l'ATR
        
    Returns:
        pd.Series: La série ATR calculée
    """
    # Identifier les colonnes requises
    if ('High' in data.columns and 'Low' in data.columns and 'Close' in data.columns):
        high = data['High'].values
        low = data['Low'].values
        close = data['Close'].values
    elif ('close', '') in data.columns:  # Si seule la colonne 'close' est disponible
        close = data[('close', '')].values
        high = close  # Approximation : utilisez 'close' comme 'high'
        low = close   # Approximation : utilisez 'close' comme 'low'
    else:
        raise KeyError("Les colonnes 'High', 'Low', et 'Close' ou leurs équivalents ne sont pas disponibles dans les données.")
    
    # Utiliser TA-Lib directement si disponible
    try:
        import talib
        atr = talib.ATR(high, low, close, timeperiod=period)
        return pd.Series(atr, index=data.index)
    except ImportError:
        # Implémentation manuelle si TA-Lib n'est pas disponible
        import numpy as np
        
        # Calcul du True Range manuellement
        true_range = np.zeros(len(close))
        
        for i in range(1, len(close)):
            high_low = high[i] - low[i]
            high_close = abs(high[i] - close[i-1])
            low_close = abs(low[i] - close[i-1])
            true_range[i] = max(high_low, high_close, low_close)
        
        # Calculer l'ATR comme moyenne mobile simple
        atr = np.zeros_like(true_range)
        for i in range(period, len(true_range)):
            atr[i] = np.mean(true_range[i-period+1:i+1])
        
        # Marquer les premières valeurs comme NaN
        atr[:period] = np.nan
        
        return pd.Series(atr, index=data.index)

def calculate_stochastic(data, k_period=10, smoothing_period=3, d_period=7):
    if not {'High', 'Low', 'Close'}.issubset(data.columns):
        raise KeyError("Les colonnes 'High', 'Low', et 'Close' sont requises dans les données.")

    # Vérifier si les données sont suffisantes pour calculer les indicateurs
    if len(data) < max(k_period, smoothing_period, d_period):
        #print("Pas assez de données pour calculer le stochastique.")
        return pd.DataFrame(columns=['%K', '%D'])

    # Calcul du plus haut et du plus bas sur la période k_period
    low_min = data['Low'].rolling(window=k_period).min()
    high_max = data['High'].rolling(window=k_period).max()

    # Calcul de %K
    data['%K'] = 100 * ((data['Close'] - low_min) / (high_max - low_min))

    # Lissage de %K
    data['%K'] = data['%K'].rolling(window=smoothing_period).mean()

    # Calcul de %D (moyenne mobile de %K)
    data['%D'] = data['%K'].rolling(window=d_period).mean()

    # Supprimer les lignes avec des NaN
    data = data.dropna(subset=['%K', '%D'])

    return data[['%K', '%D']]

def calculate_supertrend(data, atr_period=50, multiplier=100):
    """
    Calcule l'indicateur technique Supertrend.
    
    Args:
        data (pd.DataFrame): DataFrame contenant les données OHLC
        atr_period (int): Période pour le calcul de l'ATR. Par défaut 14.
        multiplier (float): Multiplicateur pour les bandes. Par défaut 3.
    
    Returns:
        pd.DataFrame: DataFrame contenant les valeurs du Supertrend
    """
    # Extraire les colonnes high, low, close selon la structure du DataFrame
    high, low, close = None, None, None
    
    # Vérifier différentes structures possibles de DataFrame
    if {'High', 'Low', 'Close'}.issubset(data.columns):
        high = data['High']
        low = data['Low']
        close = data['Close']
    elif {('bid', 'High'), ('bid', 'Low'), ('bid', 'Close')}.issubset(data.columns):
        high = data[('bid', 'High')]
        low = data[('bid', 'Low')]
        close = data[('bid', 'Close')]
    else:
        # Si on a uniquement le prix de clôture, on l'utilise comme approximation
        if 'close' in data.columns:
            close = data['close']
            high = close
            low = close
        elif ('close', '') in data.columns:
            close = data[('close', '')]
            high = close
            low = close
        else:
            raise KeyError("Les colonnes High, Low, Close ou une colonne close sont requises.")
    
    # Calcul de l'ATR
    atr = calculate_atr(data, period=atr_period)
    
    # Calcul des bandes
    hl2 = (high + low) / 2
    upper_band = hl2 + (multiplier * atr)
    lower_band = hl2 - (multiplier * atr)
    
    # Initialisation du DataFrame résultat
    st = pd.DataFrame(index=data.index)
    st['UpperBand'] = upper_band
    st['LowerBand'] = lower_band
    st['SuperTrend'] = np.nan
    st['Direction'] = np.nan
    
    # Trouver l'index de départ (premier point non-NaN)
    start_idx = 0
    for i in range(len(data)):
        if not np.isnan(atr.iloc[i]):
            start_idx = i
            break
    
    if np.isnan(atr).all():
        return pd.DataFrame(index=data.index, columns=['UpperBand','LowerBand','SuperTrend', 'Direction'], data=np.nan)
    
    # Premier calcul
    st.iloc[start_idx, 2] = lower_band.iloc[start_idx]  # Supertrend initial
    st.iloc[start_idx, 3] = 1  # Direction initiale haussière
    
    # Calcul du Supertrend pour chaque point suivant
    for i in range(start_idx + 1, len(data)):
        prev_supertrend = st.iloc[i-1, 2]
        prev_direction = st.iloc[i-1, 3]
        
        # Si la tendance précédente était haussière
        if prev_direction == 1:
            curr_lower_band = max(lower_band.iloc[i], prev_supertrend)
            
            if close.iloc[i] < curr_lower_band:
                # Changement vers tendance baissière
                st.iloc[i, 2] = upper_band.iloc[i]
                st.iloc[i, 3] = -1
            else:
                # Maintien tendance haussière
                st.iloc[i, 2] = curr_lower_band
                st.iloc[i, 3] = 1
        
        # Si la tendance précédente était baissière
        else:
            curr_upper_band = min(upper_band.iloc[i], prev_supertrend)
            
            if close.iloc[i] > curr_upper_band:
                # Changement vers tendance haussière
                st.iloc[i, 2] = lower_band.iloc[i]
                st.iloc[i, 3] = 1
            else:
                # Maintien tendance baissière
                st.iloc[i, 2] = curr_upper_band
                st.iloc[i, 3] = -1 
    return st

# ------------Menu principal----------------
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