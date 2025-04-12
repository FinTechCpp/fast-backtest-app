# Documentation - trading-ig

### Prérequis
1. Installer les dépendances :
   ```bash
   pip install -r requirements.txt
   ```

### Générer la documentation
1. Dans le dossier `docs`, exécute :
   ```bash
   make html
   ```

2. Ouvrir la documentation générée dans ton navigateur :
   ```bash
   xdg-open build/html/index.html
   ```

### Autres formats
- Pour générer un PDF, utilise :
  ```bash
  make latexpdf
  ```

### Commandes utiles
- Pour voir toutes les options de `make` :
  ```bash
  make help
  ```
