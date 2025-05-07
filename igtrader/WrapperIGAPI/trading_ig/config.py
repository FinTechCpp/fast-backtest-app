#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import logging
import sys
import importlib.util
from pathlib import Path

# Ajout du chemin parent pour trouver trading_ig_config.py
parent_dir = str(Path(__file__).resolve().parent.parent)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

ENV_VAR_ROOT = "IG_SERVICE"
CONFIG_FILE_NAME = "trading_ig_config.py"

logger = logging.getLogger(__name__)


class ConfigEnvVar(object):
    def __init__(self, env_var_base):
        self.ENV_VAR_BASE = env_var_base
        # Forcer le chargement des variables d'environnement ici
        try:
            from dotenv import load_dotenv
            load_dotenv()
            logger.info("Variables d'environnement chargées depuis .env")
        except ImportError:
            logger.warning("dotenv n'est pas installé, impossible de charger le fichier .env")

    def _env_var(self, key):
        return self.ENV_VAR_BASE + "_" + key.upper()

    def get(self, key, default_value=None):
        env_var = self._env_var(key)
        return os.environ.get(env_var, default_value)

    def __getattr__(self, key):
        env_var = self._env_var(key)
        try:
            return os.environ[env_var]
        except KeyError:
            # Afficher toutes les variables d'environnement disponibles commençant par IG_SERVICE
            available_vars = [v for v in os.environ if v.startswith(self.ENV_VAR_BASE)]
            logger.error(f"Variables disponibles: {available_vars}")
            raise Exception(f"Environment variable '{env_var}' doesn't exist")


try:
    # Tenter d'importer le module directement
    from igtrader.WrapperIGAPI.trading_ig_config import config
    logger.info(f"Import config from igtrader.WrapperIGAPI.trading_ig_config")
except ImportError:
    try:
        # Alternative: essayer d'importer directement depuis le chemin parent
        from trading_ig_config import config
        logger.info(f"Import config from trading_ig_config")
    except ImportError:
        logger.warning(f"Can't import config from config file")
        try:
            config = ConfigEnvVar(ENV_VAR_ROOT)
            logger.info(f"Import config from environment variables '{ENV_VAR_ROOT}_...'")
        except Exception as e:
            logger.error(f"Failed to load config: {e}")
            raise Exception(
                f"Can't import config - you might create a '{CONFIG_FILE_NAME}' filename or use "
                f"environment variables such as '{ENV_VAR_ROOT}_...'"
            )