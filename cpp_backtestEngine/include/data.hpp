#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <stdexcept>
#include "date.hpp"

namespace be {

/**
 * @brief Structure qui représente une bougie (OHLCV)
 */
struct Candle {
    Date date;
    double open;
    double high;
    double low;
    double close;
    double volume;
    
    // Valeurs personnalisées ajoutées dynamiquement
    std::map<std::string, double> customValues;
};

/**
 * @brief Classe intégrée pour le stockage et l'accès séquentiel aux données de marché
 * 
 * Cette classe combine les fonctionnalités de stockage et d'itération séquentielle
 * pour garantir que le backtest traite les données bougie par bougie, sans accès
 * aux données futures.
 */
class Data {
public:
    /**
     * @brief Crée un objet Data à partir de données brutes
     * 
     * @param dates Dates des bougies
     * @param open Prix d'ouverture
     * @param high Prix les plus hauts
     * @param low Prix les plus bas
     * @param close Prix de clôture
     * @param volume Volumes (optionnel)
     * @throws std::invalid_argument Si les vecteurs n'ont pas tous la même taille
     */
    Data(const std::vector<Date>& dates,
         const std::vector<double>& open,
         const std::vector<double>& high,
         const std::vector<double>& low,
         const std::vector<double>& close,
         const std::vector<double>& volume = {});
    
    /**
     * @brief Crée un objet Data à partir d'un vecteur de bougies préexistant
     * 
     * @param candles Vecteur de bougies
     */
    explicit Data(std::vector<Candle> candles);
    
    /**
     * @brief Retourne le nombre total de bougies
     */
    size_t size() const { return _candles.size(); }
    
    /**
     * @brief Ajoute une colonne personnalisée à toutes les bougies
     * 
     * @param name Nom de la colonne
     * @param values Valeurs à ajouter
     * @throws std::invalid_argument Si la taille du vecteur values ne correspond pas
     */
    void addColumn(const std::string& name, const std::vector<double>& values);
    
    //----- Méthodes d'accès pour l'affichage et l'analyse post-backtest -----
    
    /**
     * @brief Accès direct à une bougie par index (pour affichage/analyse)
     * 
     * @param index Index de la bougie
     * @return Référence constante à la bougie
     * @throws std::out_of_range Si l'index est hors limites
     */
    const Candle& at(size_t index) const;
    
    /**
     * @brief Retourne une référence au vecteur sous-jacent de bougies
     * Note: Pour les besoins d'affichage et d'analyse post-backtest uniquement
     */
    const std::vector<Candle>& getAllCandles() const { return _candles; }
    
    /**
     * @brief Récupère tous les vecteurs de données en format brut (pour charting)
     */
    std::vector<Date> getDates() const;
    std::vector<double> getOpen() const;
    std::vector<double> getHigh() const;
    std::vector<double> getLow() const;
    std::vector<double> getClose() const;
    std::vector<double> getVolume() const;
    std::vector<double> getCustomColumn(const std::string& name) const;
    
    //----- Méthodes d'itération pour le backtest -----
    
    /**
     * @brief Vérifie s'il reste des bougies à traiter
     * 
     * @return true s'il reste des bougies, false sinon
     */
    bool hasNext() const { return _position < _candles.size(); }
    
    /**
     * @brief Avance à la bougie suivante
     * 
     * @return true si l'opération a réussi, false s'il n'y a plus de bougies
     */
    bool moveNext();
    
    /**
     * @brief Retourne la position actuelle dans la séquence
     * 
     * @return Position actuelle (0-based)
     */
    size_t position() const { return _position; }
    
    /**
     * @brief Réinitialise l'itérateur au début
     */
    void reset() { _position = 0; }
    
    /**
     * @brief Accès à la bougie courante
     * 
     * @return Référence constante à la bougie courante
     * @throws std::runtime_error Si l'on est à la fin des données ou avant le début
     */
    const Candle& current() const;
    
    /**
     * @brief Accès à la date courante
     */
    Date currentDate() const { return current().date; }
    
    /**
     * @brief Accès au prix d'ouverture courant
     */
    double currentOpen() const { return current().open; }
    
    /**
     * @brief Accès au prix le plus haut courant
     */
    double currentHigh() const { return current().high; }
    
    /**
     * @brief Accès au prix le plus bas courant
     */
    double currentLow() const { return current().low; }
    
    /**
     * @brief Accès au prix de clôture courant
     */
    double currentClose() const { return current().close; }
    
    /**
     * @brief Accès au volume courant
     */
    double currentVolume() const { return current().volume; }
    
    /**
     * @brief Accès à une valeur personnalisée courante
     * 
     * @param name Nom de la colonne personnalisée
     * @return Valeur courante
     * @throws std::out_of_range Si la colonne n'existe pas
     */
    double currentCustomValue(const std::string& name) const;
    
    /**
     * @brief Accès aux N dernières bougies pour calculer des indicateurs
     * 
     * @param n Nombre de bougies à récupérer (incluant la courante)
     * @return Vecteur des n dernières bougies (la plus récente en dernier)
     * @throws std::invalid_argument Si n <= 0
     * @throws std::runtime_error Si n est supérieur à la position actuelle + 1
     */
    std::vector<Candle> lookback(size_t n) const;
    
private:
    std::vector<Candle> _candles;  // Stockage des bougies
    size_t _position = 0;          // Position courante pour l'itération
};

} // namespace be