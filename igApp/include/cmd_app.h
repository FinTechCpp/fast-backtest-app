#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <chrono>
#include "trading_ig_config.hpp"
#include "rest.h"

/**
 * Efface le buffer d'entrée pour éviter les problèmes lors de la saisie
 */
void clearInputBuffer();

/**
 * Affiche le menu principal de l'application
 */
void displayMenu();

/**
 * Récupère et affiche les prix historiques pour un instrument donné
 * @param service L'instance du service IG à utiliser
 */
void fetchHistoricalPrices(ig::IGService& service);
