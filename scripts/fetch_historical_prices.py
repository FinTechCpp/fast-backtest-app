from ib_async import *
import pandas as pd
import time
from tqdm import tqdm
import logging

# Configuration des logs
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Connexion à TWS ou IB Gateway
try:
    ib = IB()
    ib.connect('127.0.0.1', 7497, clientId=1)
    logging.info("Connexion réussie à TWS/IB Gateway.")
except Exception as e:
    logging.error(f"Erreur de connexion : {e}")
    exit()

# Définir le contrat pour l'indice NASDAQ-100
contract = Index('NDX', 'NASDAQ', 'USD')


# Période de récupération des données
end_date = pd.Timestamp.now()
start_date = end_date - pd.DateOffset(years=5)

# Durée maximale par requête selon les limitations de l'API
duration = '1 D'  # 1 jour
bar_size = '1 min'

# Liste pour stocker les données
all_data = []

# Calcul du nombre total de jours pour la barre de progression
total_days = (end_date - start_date).days

# Initialisation de la barre de progression
with tqdm(total=total_days, desc="Téléchargement des données") as pbar:
    while end_date > start_date:
        # Format the end_date in UTC with the required format
        end_date_str = end_date.tz_localize('UTC').strftime('%Y%m%d-%H:%M:%S')
        
        logging.info(f"Récupération des données de {start_date} jusqu'à {end_date_str}...")
        
        # Requête pour obtenir les données historiques
        bars = ib.reqHistoricalData(
            contract,
            endDateTime=end_date_str,
            durationStr=duration,
            barSizeSetting=bar_size,
            whatToShow='TRADES',
            useRTH=False,
            formatDate=1
        )
        
        if not bars:
            logging.warning("Aucune donnée retournée. Cela peut indiquer un abonnement de données manquant ou un contrat invalide.")
            break
        
        # Convertir les barres en DataFrame et les ajouter à la liste
        df = util.df(bars)
        df.index = pd.to_datetime(df.index)
        all_data.append(df)
        logging.info(f"{len(df)} barres récupérées.")
        
        # Mettre à jour la date de fin pour la prochaine requête
        new_end_date = df.index.min() - pd.Timedelta(seconds=1)
        
        # Validate the new_end_date
        if new_end_date >= end_date or new_end_date <= start_date:
            logging.warning("La nouvelle date de fin est invalide ou aucune donnée supplémentaire n'est disponible.")
            break
        
        end_date = new_end_date
        logging.info(f"Nouvelle end_date: {end_date}, start_date: {start_date}")
        
        # Mise à jour de la barre de progression
        pbar.update(1)
        
        # Pause pour respecter les limitations de l'API
        time.sleep(60)

# Concaténer toutes les données en un seul DataFrame
historical_data = pd.concat(all_data) if all_data else pd.DataFrame()

# Sauvegarder les données dans un fichier CSV si non vide
output_file = 'nasdaq100_5years_1min.csv'
if not historical_data.empty:
    historical_data.to_csv(output_file, index=False)
    logging.info(f"Données sauvegardées dans le fichier {output_file}.")
else:
    logging.warning("Aucune donnée à sauvegarder, le DataFrame final est vide.")

# Déconnexion
ib.disconnect()
logging.info("Déconnexion réussie.")