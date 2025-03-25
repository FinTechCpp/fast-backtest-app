#IMPORTS
from trading_ig.rest import IGService
from trading_ig.config import config
import time
import sys
import pandas as pd
import readline
import traceback
from pick import pick
import readchar

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

resolution_dict = {
    "M": "Minute",
    "H": "Hour",
    "D": "Day",
    "W": "Week",
    "M": "Month"
}

#FUNCTIONS DEFINITIONS
def prefill_input(prompt, text):
        def hook():
            readline.insert_text(text)
            readline.redisplay()
        readline.set_pre_input_hook(hook)
        result = input(prompt)
        readline.set_pre_input_hook()
        return result

def initialize_service():
    """Initialise la connexion au service IG"""
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


ig_service = IGService(
    config.username, 
    config.password, 
    config.api_key, 
    config.acc_type,
    acc_number=config.acc_number)
ig = ig_service.create_session(version='3')

def select_from_dict(dict):
    """Permet de sélectionner un id depuis le dictionnaire avec navigation par flèches"""
    try:
        
        # Créer une liste de choix à partir du dictionnaire dict
        options = []
        dict_list = []
        
        for id, name in dict.items():
            options.append(f"{name} ({id})")
            dict_list.append(id)
        
        title = "Sélectionnez un marché avec les flèches ↑↓ puis Entrée pour confirmer:"
        selected_option, index = pick(options, title)
        
        # Récupérer l'EPIC sélectionné
        return dict_list[index], dict[dict_list[index]]
        
    except ImportError:
        print("📦 Le module 'pick' n'est pas installé. Utilisation du mode de sélection basique.")
        print("Pour une meilleure expérience, installez-le avec: pip install pick")
        print("\nMarchés disponibles:")
        
        # Afficher les options numérotées
        for i, (id, name) in enumerate(dict.items(), 1):
            print(f"{i}. {name} ({id})")
        
        # Demander à l'utilisateur de choisir
        while True:
            try:
                choice = int(input("\nEntrez le numéro du marché: "))
                if 1 <= choice <= len(dict):
                    # Récupérer l'EPIC correspondant au choix
                    selected_id = list(dict.keys())[choice-1]
                    return selected_id, dict[selected_id]
                else:
                    print("⚠️ Numéro invalide. Veuillez réessayer.")
            except ValueError:
                print("⚠️ Veuillez entrer un numéro valide.")
                
def search_market(ig_service):
    """Recherche un marché spécifique en utilisant une sélection interactive"""

    selected_epic, selected_name = select_from_dict(epics_dict)
    print(f"Recherche de '{selected_epic}'...")
    
    try:
        result = ig_service.search_markets(selected_epic)
        print("\nRésultats de la recherche:")
        
        # Vérifier le type de résultat et traiter en conséquence
        if isinstance(result, pd.DataFrame) and not result.empty:
            print(f"Trouvé {len(result)} marchés:")
            for index, row in result.iterrows():
                print(f"- {row['epic']}: {row['instrumentName']}")
        elif isinstance(result, dict) and 'markets' in result:
            # Format de dictionnaire (ancienne version possible de l'API)
            for item in result['markets']:
                print(f"- {item['epic']}: {item['instrumentName']}")
        else:
            print("Aucun marché trouvé avec ce terme de recherche.")
    except Exception as e:
        print(f"\n⚠️ Erreur lors de la recherche: {e}")
    
    input("\nAppuyez sur Entrée pour continuer...")

def get_market_info(ig_service):
    """Obtient des informations sur un marché"""
        
    selected_epic, selected_name = select_from_dict(epics_dict)    
    print(f"Récupération des informations pour {selected_epic} alias {selected_name}...")
    
    try:
        market = ig_service.fetch_market_by_epic(selected_epic)
        print("\nInformations sur le marché:")
        print(f"Nom: {market['instrument']['name']}")
        
        # Récupérer le bid et ask
        bid = market['snapshot']['bid']
        ask = market['snapshot']['offer']
        
        # Calculer le spread
        spread = round((ask - bid) * 10000, 1)  # Calculé en pips pour les devises
        
        print(f"Bid: {bid}")
        print(f"Ask: {ask}")
        print(f"Spread: {spread} pips")
        
        # Afficher des informations supplémentaires si disponibles
        if 'dealingRules' in market:
            print("\nRègles de trading:")
            if 'minDealSize' in market['dealingRules']:
                print(f"Taille minimale: {market['dealingRules']['minDealSize']['value']}")
            if 'maxDealSize' in market['dealingRules']:
                print(f"Taille maximale: {market['dealingRules']['maxDealSize']['value']}")
    except Exception as e:
        print(f"⚠️ Erreur: {e}")
        # Afficher plus de détails sur l'erreur pour le débogage
        traceback.print_exc()
    input("\nAppuyez sur Entrée pour continuer...")

def get_historical_prices(ig_service):
    """Récupère les prix historiques d'un marché"""
    selected_epic, selected_name = select_from_dict(epics_dict)
    selected_resolution, selected_resolution_name = select_from_dict(resolution_dict)
    
    num_points_default = "10"
    num_points = prefill_input(f"Nombre de points (1-100): ", num_points_default) 
    
    print(f"Récupération des prix historiques pour '{selected_epic}'...")
    try:
        # Convert num_points to integer
        num_points = int(num_points)
        
        # Fetch historical prices with parameters - using correct parameter 'numpoints'
        result = ig_service.fetch_historical_prices_by_epic(
            epic=selected_epic,
            resolution=selected_resolution,
            numpoints=num_points  # Changed from num_points to numpoints
        )
                
        # Check if result is a DataFrame
        if isinstance(result, pd.DataFrame):
            print("\nDerniers prix historiques (DataFrame):")
            print(result.head())
        # Check if result is a dict with 'prices' key
        elif isinstance(result, dict) and 'prices' in result:
            if isinstance(result['prices'], list):
                print("\nDerniers prix historiques:")
                for price in result['prices'][:5]:
                    print(f"Date: {price['snapshotTime']}, Ouv: {price['openPrice']['ask']}, Ferm: {price['closePrice']['ask']}")
            else:
                #print(f"\nStructure des prix: {type(result['prices'])}")
                print(result['prices'])
        else:
            # Print the structure to understand what we're working with
            print("\nStructure de la réponse inconnue:")
            print(f"Type: {type(result)}")
            print(result)
    except Exception as e:
        print(f"⚠️ Erreur: {e}")
        traceback.print_exc()
    
    input("\nAppuyez sur Entrée pour continuer...")

def track_realtime_prices(ig_service):
    """Suit les prix d'un marché en temps réel"""
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
    """Crée une position d'achat ou de vente"""
    selected_epic, selected_name = select_from_dict(epics_dict)
    # Create a binary selection for direction using the pick function
    
    direction_title = "Choisissez la direction avec les flèches ↑↓ puis Entrée pour confirmer:"
    try:
        direction, _ = pick(direction_options, direction_title)
    except Exception as e:
        print(f"⚠️ Erreur lors de la sélection interactive: {e}")
        direction = input("Direction (BUY/SELL): ").upper()
        while direction not in ["BUY", "SELL"]:
            print("⚠️ Direction invalide. Veuillez entrer BUY ou SELL.")
            direction = input("Direction (BUY/SELL): ").upper()
            
    # Size selection with arrow keys to dynamically modify the value
    try:
        
        size = 1.0  # Default value
        min_size = 0.5
        max_size = 100.0
        step = 0.5
        
        print("\n📊 Taille de la position 📊")
        print(f"[↑] Augmenter | [↓] Diminuer | [Enter] Confirmer")
        print(f"{size:.1f}", end="", flush=True)
        
        while True:
            key = readchar.readkey()
            
            if key == readchar.key.UP:
                # Increase size
                if size + step <= max_size:
                    size += step
            elif key == readchar.key.DOWN:
                # Decrease size
                if size - step >= min_size:
                    size -= step
            elif key == readchar.key.ENTER:
                # Confirm selection
                print()  # Move to next line
                break
            
            # Clear the current value and reprint
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
    """Affiche le menu principal"""
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


#MAIN FUNCTION
def main():
    """Fonction principale"""
    try:
        ig_service = initialize_service()
        
        while True:
            choice = display_menu()
            
            if choice == "1":
                search_market(ig_service)
            elif choice == "2":
                get_market_info(ig_service)
            elif choice == "3":
                get_historical_prices(ig_service)
            elif choice == "4":
                track_realtime_prices(ig_service)
            elif choice == "5":
                create_position(ig_service)
            elif choice == "0":
                print("Au revoir!")
                sys.exit(0)
            else:
                print("⚠️ Option invalide. Veuillez réessayer.")
    
    except Exception as e:
        print(f"⚠️ Erreur critique: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
