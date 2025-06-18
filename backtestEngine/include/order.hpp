#pragma once

#include <string>
#include <memory>
#include <map>

namespace be {

// Forward declarations
class Broker;
class Trade;

/**
 * @brief Représentation d'un ordre de trading
 * 
 * Un ordre est une instruction pour exécuter une transaction sur le marché.
 * Il représente une intention d'acheter ou de vendre qui n'est pas encore réalisée.
 * Les ordres peuvent être de différents types:
 * 
 * - **Ordre au marché**: Exécuté immédiatement au prix actuel (limitPrice=0, stopPrice=0)
 * - **Ordre limite**: Exécuté uniquement si le prix est favorable (limitPrice>0)
 *   - Pour un achat: le prix doit être <= limitPrice
 *   - Pour une vente: le prix doit être >= limitPrice
 * - **Ordre stop**: Exécuté lorsque le prix atteint un niveau spécifié (stopPrice>0)
 *   - Pour un achat: le prix doit être >= stopPrice (achat à la montée)
 *   - Pour une vente: le prix doit être <= stopPrice (vente à la baisse)
 * - **Ordre contingent**: Lié à un trade existant (SL/TP) via parentTrade
 * 
 * Les ordres peuvent également spécifier des niveaux Stop Loss (SL) et Take Profit (TP)
 * qui seront automatiquement convertis en ordres contingents une fois l'ordre principal exécuté.
 */
class Order {
public:
    /**
     * @brief Construit un nouvel ordre de trading
     * 
     * @param broker Référence partagée vers le broker qui exécutera l'ordre
     * @param size Taille de l'ordre (positive pour achat, négative pour vente)
     * @param limitPrice Prix limite pour un ordre limite (0 si non applicable)
     * @param stopPrice Prix de déclenchement pour un ordre stop (0 si non applicable)
     * @param slPrice Prix du Stop Loss (0 si non applicable)
     * @param tpPrice Prix du Take Profit (0 si non applicable)
     * @param parentTrade Trade parent si cet ordre est contingent (nullptr sinon)
     * @param tag Étiquette descriptive pour faciliter l'identification
     * @param slPoints Distance du SL en points depuis le prix d'entrée (alternative à slPrice)
     * @param tpPoints Distance du TP en points depuis le prix d'entrée (alternative à tpPrice)
     * 
     * @throws std::invalid_argument Si size est 0
     * @throws std::invalid_argument Si les niveaux SL/TP sont invalides
     */
    Order(std::shared_ptr<Broker> broker,
          double size,
          double limitPrice = 0.0,
          double stopPrice = 0.0,
          double slPrice = 0.0,
          double tpPrice = 0.0,
          std::shared_ptr<Trade> parentTrade = nullptr,
          const std::string& tag = "",
          double slPoints = 0.0,
          double tpPoints = 0.0);
    
    // Getters
    double size() const { return _size; } ///< Taille de l'ordre (positive pour achat, négative pour vente)
    double limit() const { return _limitPrice; } ///< Prix limite pour un ordre limite (0 si non applicable)
    double stop() const { return _stopPrice; } ///< Prix stop pour un ordre stop (0 si non applicable)
    double sl() const { return _slPrice; } ///< Prix du Stop Loss (0 si non applicable)
    double tp() const { return _tpPrice; } ///< Prix du Take Profit (0 si non applicable)
    double slPoints() const { return _slPoints; } ///< Distance du SL en points depuis le prix d'entrée
    double tpPoints() const { return _tpPoints; } ///< Distance du TP en points depuis le prix d'entrée
    std::string tag() const { return _tag; } ///< Étiquette descriptive de l'ordre
    std::shared_ptr<Trade> parentTrade() const { return _parentTrade; } ///< Trade parent si ordre contingent
    
    /**
     * @brief Vérifie si l'ordre est un ordre d'achat
     * @return true si size > 0, false sinon
     */
    bool isLong() const { return _size > 0; }
    
    /**
     * @brief Vérifie si l'ordre est un ordre de vente
     * @return true si size < 0, false sinon
     */
    bool isShort() const { return _size < 0; }
    
    /**
     * @brief Détermine si cet ordre est contingent (lié à un trade parent)
     * 
     * Un ordre est considéré comme contingent s'il a un trade parent et qu'il
     * est soit un Stop Loss, soit un Take Profit pour ce trade.
     * 
     * @return true si l'ordre est contingent, false sinon
     */
    bool isContingent() const;
    
    // Helper methods pour la modification des attributs (utilisé dans broker.next)
    // TODO : Methodes assez moches, ils y a des string hardcodés, à améliorer
    void _replace(const std::string& attr, double value);
    void _replace(std::map<std::string, double> changes);
    
    /**
     * @brief Compare deux ordres pour déterminer s'ils sont identiques
     * 
     * Deux ordres sont considérés identiques s'ils ont les mêmes propriétés
     * essentielles (size, limitPrice, stopPrice, slPrice, tpPrice, parentTrade, tag).
     * 
     * @param other Ordre à comparer
     * @return true si les ordres sont égaux, false sinon
     */
    bool operator==(const Order& other) const;
    
    /**
     * @brief Convertit l'ordre en chaîne de caractères pour le débug et les logs
     * @return Description textuelle de l'ordre
     */
    std::string toString() const;
    
private:
    std::shared_ptr<Broker> _broker;       ///< Broker responsable de l'exécution
    double _size;                          ///< Taille (positive pour achat, négative pour vente)
    double _limitPrice;                    ///< Prix limite (ordre limite)
    double _stopPrice;                     ///< Prix stop (ordre stop)
    double _slPrice;                       ///< Prix du Stop Loss
    double _tpPrice;                       ///< Prix du Take Profit
    std::shared_ptr<Trade> _parentTrade;   ///< Trade parent (pour ordres SL/TP)
    std::string _tag;                      ///< Étiquette descriptive
    double _slPoints;                      ///< Distance SL en points
    double _tpPoints;                      ///< Distance TP en points
    
    friend class Broker;
    friend class Trade;
};

} // namespace be