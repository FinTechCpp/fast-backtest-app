#IMPORTS
from trading_ig.rest import IGService
from trading_ig.config import config
import time
import sys
import pandas as pd
import readline
import traceback


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

def search_market(ig_service):
    """Recherche un marché spécifique"""
    search_term_default = "EUR/USD"
    search_term = prefill_input(f"Entrez le terme de recherche (ex: {search_term_default}): ", search_term_default)
    print(f"Recherche de '{search_term}'...")
    try:
        result = ig_service.search_markets(search_term)
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
    epic_default = "CS.D.EURUSD.MINI.IP"   
    epic = prefill_input(f"Entrez l'EPIC du marché : ", epic_default)
    print(f"Récupération des informations pour '{epic}'...")
    try:
        market = ig_service.fetch_market_by_epic(epic)
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
        import traceback
        traceback.print_exc()
    input("\nAppuyez sur Entrée pour continuer...")

def get_historical_prices(ig_service):
    """Récupère les prix historiques d'un marché"""
    epic_default = "CS.D.EURUSD.MINI.IP"
    epic = prefill_input(f"Entrez l'EPIC du marché (ex: {epic_default}): ", epic_default)
    resolution_default = "D"  # Daily resolution
    resolution = input(f"Résolution (M, H, D, W, M): {resolution_default}") or f"{resolution_default}"
    num_points_default = "10"
    num_points = input(f"Nombre de points (1-100): {num_points_default}") or f"{num_points_default}"
    
    print(f"Récupération des prix historiques pour '{epic}'...")
    try:
        # Convert num_points to integer
        num_points = int(num_points)
        
        # Fetch historical prices with parameters - using correct parameter 'numpoints'
        result = ig_service.fetch_historical_prices_by_epic(
            epic=epic,
            resolution=resolution,
            numpoints=num_points  # Changed from num_points to numpoints
        )
        
        # Debug information
        #print(f"\nType de données reçu: {type(result)}")
        
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
    epic_default = "IX.D.NASDAQ.IFE.IP"
    epic = prefill_input(f"Entrez l'EPIC du marché (ex: {epic_default}): ", epic_default)
    iterations = int(input("Nombre d'itérations (1-100): "))
    delay = int(input("Délai entre les mises à jour (secondes): "))
    
    print(f"🔄 Récupération des prix de {epic} en temps réel...")
    for i in range(min(iterations, 100)):
        try:
            result = ig_service.fetch_market_by_epic(epic)
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
    epic_default = "CS.D.EURUSD.MINI.IP"
    epic = prefill_input(f"Entrez l'EPIC du marché (ex: {epic_default}): ", epic_default)
    direction = input("Direction (BUY/SELL): ").upper()
    size = float(input("Taille de la position: "))
    
    if direction not in ["BUY", "SELL"]:
        print("⚠️ Direction invalide. Utilisez BUY ou SELL.")
        input("\nAppuyez sur Entrée pour continuer...")
        return
    
    print(f"Création d'une position {direction} sur {epic}...")
    try:
        resp = ig_service.create_open_position(
            currency_code='EUR',
            direction=direction,
            epic=epic,
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
