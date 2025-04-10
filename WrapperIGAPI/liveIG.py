import time
import sys
import os
parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

from Strategies.BuyTrendFollowingStrategy.BuyTrendFollowingStrategy import BuyTrendFollowingStrategy


class Brocker:
    def __init__(self):
        # Initialiser la connexion à l'API de votre broker ici.
        print("Connexion à l'API du broker établie.")

    def get_historical_candles(self, count=200):
        """
        Récupère les 'count' dernières bougies historiques depuis le broker.
        Ici, nous simulons des données historiques.
        """
        candles = []

        for i in range(count):
            candle = {
                'date': '2023-10-01 00:00:00',
                'Open': 100 + i,
                'High': 105 + i,
                'Low': 95 + i,
                'Close': 100 + i,
                'Volume': 1000 + i
            }
            candles.append(candle)
        return candles

    def get_live_candle(self):
        """
        Récupère la dernière bougie en live depuis l'API du broker.
        Ici, nous simulons une bougie live.
        """
        candle = {}
        return candle

    def place_order(self, signal):
        """
        Envoie l'ordre à l'API du broker en se basant sur le signal du bot.
        """
        # Ici, vous intègrerez la logique pour passer l'ordre réel via votre API
        print("Envoi de l'ordre au broker :", signal)


def main():
    # Création des instances
    broker = Brocker()
    strategy = BuyTrendFollowingStrategy()

    # Initialisation avec les 200 dernières bougies historiques
    historical_candles = broker.get_historical_candles(200)
    for candle in historical_candles:
        # Nous mettons à jour la stratégie avec les bougies historiques 
        # mais nous ignorons les signaux éventuels (l'objectif est d'initialiser les indicateurs)
        _ = strategy.update_candle(candle)

    print("Initialisation terminée avec les bougies historiques.")

    # Boucle pour récupérer les bougies en live toutes les 10 secondes
    while True:
        live_candle = broker.get_live_candle()
        signal = strategy.update_candle(live_candle)
        
        if signal is not None:
            # Si un signal est généré (achat, vente ou liquidation), on le transmet au broker
            broker.place_order(signal)
        else:
            print("Aucun signal pour cette bougie.")

        # Attendre 10 secondes avant de récupérer la prochaine bougie
        time.sleep(10)


if __name__ == '__main__':
    main()
