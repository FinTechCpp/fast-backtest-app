## Installation

### 1. Cloner le dépôt

Clonez le dépôt et placez-vous à la racine du projet :

```bash
git clone http://10.8.0.1:9000/finance/ig-trading-bot.git
cd ig-trading-bot
```

### 2. Configurer l'environnement virtuel

Nous vous conseillons d’utiliser un environnement virtuel :

```bash
python3 -m venv venv
source venv/bin/activate  # Sous Linux/MacOS
venv\Scripts\activate     # Sous Windows
```

### 3. Installer les dépendances

Installez les dépendances nécessaires à partir du fichier `requirements.txt` :

```bash
pip install -r requirements.txt
```

### 4. Installer le package en mode développement

Le projet est organisé en tant que package Python (nommé par exemple **igtradingbot** ou **ig_trading_bot**). Pour que les imports soient correctement résolus, installez le package en mode développement :

```bash
pip install -e .
```

L’installation en mode développement vous permet de modifier le code source et de voir immédiatement les modifications sans avoir à réinstaller le package.

### 5. Configurer vos identifiants IG

Copiez le fichier `.env-example` en `.env` et renseignez-y vos identifiants et configurations :

```bash
cp .env-example .env
```

Ouvrez ensuite `.env` dans votre éditeur et ajoutez vos informations.

---

## Structure du projet

La structure du projet a été réorganisée pour centraliser le code source dans un package unique et séparer les points d'entrée. Par exemple :

```
ig-trading-bot/
├── setup.py                  # Script d'installation en mode développement
├── README.md
├── requirements.txt
├── igtradingbot/             # Package principal
│   ├── __init__.py
│   ├── Strategies/         # Sous-package pour vos stratégies
│   │   ├── __init__.py
│   │   ├── Strategy.py
│   │   ├── Helpers.py
│   │   ├── BuyTrendFollowingStrategy/
│   │   │   ├── __init__.py
│   │   │   ├── BuyTrendFollowingStrategy.py
│   │   │   └── BacktestAdapter.py
│   │   └── SellTrendFollowingStrategy/
│   │       ├── __init__.py
│   │       ├── SellTrendFollowingStrategy.py
│   │       └── BacktestAdapter.py
│   └── trading_ig/         # Sous-package pour l'intégration avec IG (Broker, API, etc.)
│       ├── __init__.py
│       └── Broker.py
└── scripts/                  # Points d'entrée et scripts divers
    ├── liveIG.py
    ├── backtest_runner.py
    └── main.py
```

Dans vos scripts (dans le dossier `scripts/`), vous utiliserez des imports absolus. Par exemple, dans `liveIG.py` :

```python
from igtradingbot.trading_ig.Broker import Broker
from igtradingbot.Strategies.BuyTrendFollowingStrategy.BuyTrendFollowingStrategy import BuyTrendFollowingStrategy

def main():
    broker = Broker(epic="IX.D.NASDAQ.IFE.IP", working_resolution='1Min')
    strategy = BuyTrendFollowingStrategy()
    # Logique de live trading...
    print("Live trading started")

if __name__ == "__main__":
    main()
```

---

## Utilisation

### En mode live

Pour exécuter le bot en mode live :

```bash
python scripts/liveIG.py
```

### En mode backtesting

Pour exécuter le backtest :

```bash
python scripts/backtest_runner.py
```

---

En structurant ainsi votre projet et en installant votre package en mode développement, vous n'aurez plus besoin d'ajouter dynamiquement des chemins via `sys.path` dans vos fichiers. Tous les modules s'importeront de manière cohérente grâce aux imports absolus, et vous pourrez disposer de plusieurs points d'entrée adaptés à vos différents cas d'utilisation.

