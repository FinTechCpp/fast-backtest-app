import igtrader.WrapperIGAPI.cmd_app.utils as utils
import sys
import logging

#MAIN FUNCTION
def main():
    """
    main.py
    Ce fichier contient la fonction principale pour exécuter le bot de trading IG. 
    Le programme permet d'interagir avec l'API IG pour effectuer diverses opérations 
    comme la recherche de marchés, l'obtention d'informations sur les marchés, 
    la récupération des prix historiques, le suivi des prix en temps réel, 
    et la création de positions.
    Fonctions:
        main(): Point d'entrée principal du programme. Gère le menu interactif 
                et les différentes fonctionnalités du bot.
    """
    """Fonction principale"""
    try:
        ig_service = utils.initialize_service()
        
        while True:
            choice = utils.display_menu()
            
            if choice == "1":
                utils.search_market(ig_service)
            elif choice == "2":
                utils.get_market_info(ig_service)
            elif choice == "3":
                utils.get_historical_prices(ig_service)
            elif choice == "4":
                utils.track_realtime_prices(ig_service)
            elif choice == "5":
                utils.create_position(ig_service)
            elif choice == "0":
                logging.info("Au revoir!")
                sys.exit(0)
            else:
                logging.warning("⚠️ Option invalide. Veuillez réessayer.")
    
    except Exception as e:
        logging.error(f"⚠️ Erreur critique: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
