#pragma once

#include <string>
#include <memory>
#include <chrono>
#include "beTypes.h"


namespace be {

// Forward declarations
class Broker;
class Order;

/**
 * @brief Représentation d'une transaction exécutée
 * 
 * Un Trade représente une position ouverte sur le marché, résultant de l'exécution
 * d'un Order. Contrairement à un Order (qui est une intention), un Trade engage
 * réellement du capital et a un prix d'entrée réel.
 * 
 * Un Trade peut être:
 * - **Long (achat)**: position positive (size > 0), profite quand le prix monte
 * - **Short (vente)**: position négative (size < 0), profite quand le prix baisse
 * 
 * Chaque Trade peut avoir:
 * - Un Stop Loss (SL) pour limiter les pertes
 * - Un Take Profit (TP) pour sécuriser les gains
 * - Ces SL/TP sont implémentés comme des Orders contingents liés au Trade
 * 
 * Un Trade passe par plusieurs états:
 * 1. Ouvert: position active avec exitPrice = 0
 * 2. Fermé: position terminée avec exitPrice > 0
 * 
 * Un Trade peut être fermé via:
 * - L'activation d'un ordre SL ou TP
 * - Un appel explicite à close()
 * - Une fermeture par le Broker (ex: lors de la finalisation d'un backtest)
 */
class Trade : public std::enable_shared_from_this<Trade> {
public:
    /**
     * @brief Construit un nouveau Trade
     * 
     * @param broker Référence partagée vers le broker qui gère ce trade
     * @param size Taille de la position (positive pour achat, négative pour vente)
     * @param entryPrice Prix d'entrée de la position
     * @param entryBar Indice de la barre où le trade a été ouvert
     * @param entryDate Date d'entrée du trade
     * @param tag Étiquette descriptive pour faciliter l'identification
     */
    Trade(std::shared_ptr<Broker> broker,
          double size,
          double entryPrice,
          size_t entryBar,
          const Date& entryDate,
          const std::string& tag = "");
    
    /**
     * @brief Ferme la position (partiellement ou complètement)
     * 
     * Crée un ordre de marché pour fermer la position. L'ordre sera exécuté
     * lors de la prochaine mise à jour du broker.
     * 
     * @param portion Proportion de la position à fermer (1.0 = fermeture complète)
     * @throws std::invalid_argument Si portion n'est pas entre 0 et 1
     */
    void close(double portion = 1.0);

    /**
     * @brief Passe le trade en mode break-even
     * 
     * Déplace le stop loss au niveau du prix d'entrée (ou avec un léger offset)
     * pour éliminer le risque de perte.
     * 
     * @param offset Décalage optionnel par rapport au prix d'entrée
     * @return true si le passage en break-even a réussi
     */
    bool setBreakEven(double price, double triggerPrice = 0.0);
    
    // Getters
    TradeData data() { 
        _updateDynamicData();
        return _data; 
    } ///< Retourne les données
    double size() const { return _data.size; } ///< Taille de la position (+ achat, - vente)
    double entryPrice() const { return _data.entryPrice; } ///< Prix d'entrée
    double exitPrice() const { return _data.exitPrice; } ///< Prix de sortie (0 si encore actif)
    size_t entryBar() const { return _data.entryBar; } ///< Barre d'entrée
    size_t exitBar() const { return _data.exitBar; } ///< Barre de sortie (0 si encore actif)
    Date entryDate() const { return _data.entryDate; } ///< Date d'entrée
    double commissions() const { return _data.commissions; } ///< Commissions payées pour ce trade
    Date exitDate() const { return _data.exitDate; } ///< Date de sortie (Date() si encore actif)
    std::string tag() const { return _data.tag; } ///< Étiquette descriptive
    CloseReason closeReason() const { return _data.closeReason; }
    void setCloseReason(CloseReason reason) { _data.closeReason = reason; }
    bool isBreakEven() const { return _data.isBreakEven; }
    double breakEvenTriggerPrice() const { return _data.breakEvenTriggerPrice; }
    double initialSlPrice() const { return _data.initialSlPrice; }

    /**
     * @brief Calcule le profit/perte actuel ou final de la position
     * 
     * Si le trade est fermé, utilise exitPrice.
     * Si le trade est ouvert, utilise le dernier prix du marché.
     * 
     * @return Profit/perte en valeur absolue (positif = profit, négatif = perte)
     */
    double pl() const;
    
    /**
     * @brief Calcule le profit/perte en pourcentage
     * 
     * @return Profit/perte en pourcentage (positif = profit, négatif = perte)
     */
    double plPercent() const;
    
    /**
     * @brief Obtient le prix du Stop Loss actuel
     * 
     * @return Prix du SL ou 0 si aucun SL n'est défini
     */
    double sl() const;
    
    /**
     * @brief Définit ou modifie le Stop Loss
     * 
     * Crée ou modifie un ordre contingent de type Stop Loss lié à ce trade.
     * Si price=0, supprime tout SL existant.
     * 
     * @param price Nouveau prix du SL (0 pour supprimer)
     * @throws std::invalid_argument Si le prix est invalide ou incohérent avec la direction du trade
     */
    void sl(double price);
    
    /**
     * @brief Obtient le prix du Take Profit actuel
     * 
     * @return Prix du TP ou 0 si aucun TP n'est défini
     */
    double tp() const;
    
    /**
     * @brief Définit ou modifie le Take Profit
     * 
     * Crée ou modifie un ordre contingent de type Take Profit lié à ce trade.
     * Si price=0, supprime tout TP existant.
     * 
     * @param price Nouveau prix du TP (0 pour supprimer)
     * @throws std::invalid_argument Si le prix est invalide ou incohérent avec la direction du trade
     */
    void tp(double price);
    
    /**
     * @brief Vérifie si la position est longue (achat)
     * @return true si size > 0, false sinon
     */
    bool isLong() const { return _data.size > 0; }
    
    /**
     * @brief Vérifie si la position est courte (vente)
     * @return true si size < 0, false sinon
     */
    bool isShort() const { return _data.size < 0; }
    
    /**
     * @brief Vérifie si la position est fermée
     * @return true si exitPrice > 0, false sinon
     */
    bool isClosed() const { return _data.exitPrice > 0; }
    
    /**
     * @brief Convertit le trade en chaîne de caractères pour le débug et les logs
     * @return Description textuelle du trade
     */
    std::string toString() const;
    
    /**
     * @brief Accède à l'ordre de Stop Loss associé
     * @return Pointeur partagé vers l'ordre SL ou nullptr si aucun
     */
    std::shared_ptr<Order> slOrder() const { return _slOrder; }
    
    /**
     * @brief Accède à l'ordre de Take Profit associé
     * @return Pointeur partagé vers l'ordre TP ou nullptr si aucun
     */
    std::shared_ptr<Order> tpOrder() const { return _tpOrder; }
    
private:
    std::shared_ptr<Broker> _broker;     ///< Broker gérant ce trade
    TradeData _data;                ///< Données du trade (taille, prix, dates, etc.)
    std::shared_ptr<Order> _slOrder;     ///< Ordre de Stop Loss
    std::shared_ptr<Order> _tpOrder;     ///< Ordre de Take Profit

    // c'est dommage on ne peux pas mettre le mot clé const
    void _updateDynamicData() {
        _data.lastSlPrice = this->sl();
        _data.initialSlPrice = this->initialSlPrice();
        _data.tpPrice = this->tp();
        _data.pl = this->pl();
        _data.plPercent = this->plPercent();
    }
    
    // Méthodes auxiliaires pour que le broker puisse modifier les propriétés du trade
    void setExitPrice(double price);
    void setExitBar(size_t bar);
    void setExitDate(Date date);
    void setSize(double size);
    void setCommissions(double commissions);
    void setSlOrder(Order order);
    void setTpOrder(Order order);
    friend class Broker;
};

} // namespace be