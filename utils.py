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

# Liste des options de direction pour les positions
direction_options = ["BUY", "SELL"]

# Dictionnaire contenant les EPICs et leurs noms communs
epics_dict = {
    "CS.D.EURUSD.MINI.IP": "EUR/USD Mini",
    "IX.D.NASDAQ.IFE.IP": "NASDAQ Index (1€)",
    "IX.D.SPTRD.IFE.IP": "S&P 500 Index (1€)",
    "IX.D.DAX.IFMM.IP": "DAX Index (1€)",
    "IX.D.CAC.IMF.IP": "CAC 40 Index (1€)",
    "CS.D.BITCOIN.CFE.IP": "Bitcoin (CFE)",
    "CS.D.XRPUSD.CFD.IP": "Ripple (CFD)",
    "UC.D.NVDA.CASH.IP": "NVIDIA (USD)",
    "UA.D.AAPL.CASH.IP": "Apple (USD)",
    "UA.D.AMZN.CASH.IP": "Amazon (USD)",
    "EC.D.HOFP.CASH.IP": "Thales (EUR)",
    "EC.D.RENA.CASH.IP": "Renault (EUR)",
    "CS.D.CFEGOLD.CFE.IP": "Gold (1€)",
}

# Dictionnaire contenant les résolutions et leurs noms
resolution_dict = {
    "M": "Minute",
    "H": "Hour",
    "D": "Day",
    "W": "Week",
    "M": "Month"
}

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
    Permet de sélectionner un élément depuis un dictionnaire avec navigation par flèches.

    Args:
        dict (dict): Le dictionnaire contenant les options.

    Returns:
        tuple: L'ID sélectionné et son nom associé.
    """
    try:
        options = []
        dict_list = []
        for id, name in dict.items():
            options.append(f"{name} ({id})")
            dict_list.append(id)
        title = "Sélectionnez un marché avec les flèches ↑↓ puis Entrée pour confirmer:"
        selected_option, index = pick(options, title)
        return dict_list[index], dict[dict_list[index]]
    except ImportError:
        print("📦 Le module 'pick' n'est pas installé. Utilisation du mode de sélection basique.")
        print("Pour une meilleure expérience, installez-le avec: pip install pick")
        print("\nMarchés disponibles:")
        for i, (id, name) in enumerate(dict.items(), 1):
            print(f"{i}. {name} ({id})")
        while True:
            try:
                choice = int(input("\nEntrez le numéro du marché: "))
                if 1 <= choice <= len(dict):
                    selected_id = list(dict.keys())[choice-1]
                    return selected_id, dict[selected_id]
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
    selected_epic, selected_name = select_from_dict(epics_dict)
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
    selected_epic, selected_name = select_from_dict(epics_dict)    
    print(f"Récupération des informations pour {selected_epic} alias {selected_name}...")
    try:
        market = ig_service.fetch_market_by_epic(selected_epic)
        print("\nInformations sur le marché:")
        print(f"Nom: {market['instrument']['name']}")
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
    selected_epic, selected_name = select_from_dict(epics_dict)
    selected_resolution, selected_resolution_name = select_from_dict(resolution_dict)
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
    selected_epic, selected_name = select_from_dict(epics_dict)
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
    Crée une position d'achat ou de vente.

    Args:
        ig_service (IGService): Le service IG initialisé.
    """
    selected_epic, selected_name = select_from_dict(epics_dict)
    direction_title = "Choisissez la direction avec les flèches ↑↓ puis Entrée pour confirmer:"
    try:
        direction, _ = pick(direction_options, direction_title)
    except Exception as e:
        print(f"⚠️ Erreur lors de la sélection interactive: {e}")
        direction = input("Direction (BUY/SELL): ").upper()
        while direction not in ["BUY", "SELL"]:
            print("⚠️ Direction invalide. Veuillez entrer BUY ou SELL.")
            direction = input("Direction (BUY/SELL): ").upper()
    try:
        size = 1.0
        min_size = 0.5
        max_size = 100.0
        step = 0.5
        print("\n📊 Taille de la position 📊")
        print(f"[↑] Augmenter | [↓] Diminuer | [Enter] Confirmer")
        print(f"{size:.1f}", end="", flush=True)
        while True:
            key = readchar.readkey()
            if key == readchar.key.UP:
                if size + step <= max_size:
                    size += step
            elif key == readchar.key.DOWN:
                if size - step >= min_size:
                    size -= step
            elif key == readchar.key.ENTER:
                print()
                break
            print("\r" + " " * 20 + "\r", end="", flush=True)
            print(f"{size:.1f}", end="", flush=True)
    except ImportError:
        print("\n📦 Le module 'readchar' n'est pas installé. Utilisation du mode basique.")
        print("Pour une meilleure expérience, installez-le avec: pip install readchar")
        size = float(input("\nTaille de la position: "))
    except Exception as e:
        print(f"\n⚠️ Erreur lors de la sélection interactive: {e}")
        size = float(input("Taille de la position: "))
    print(f"Création d'une position {direction} sur {selected_epic}...")
    try:
        resp = ig_service.create_open_position(
            currency_code='EUR',
            direction=direction,
            epic=selected_epic,
            order_type='MARKET',
            expiry='DFB',
            force_open='false',
            guaranteed_stop='false',
            size=size, level=None,
            limit_distance=None,
            limit_level=None,
            quote_id=None,
            stop_level=None,
            stop_distance=None,
            trailing_stop=None,
            trailing_stop_increment=None)
        print(f"✅ Position créée avec succès: {resp}")
    except Exception as e:
        print(f"⚠️ Erreur: {e}")
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