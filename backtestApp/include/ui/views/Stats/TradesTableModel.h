#pragma once

#include <QStandardItemModel>
#include <QDateTime>
#include "date.hpp"
#include "trade.hpp"

/**
 * @brief Modèle de données pour la table des trades.
 * Gère l'affichage des transactions et leur formatage.
 */
class TradesTableModel : public QStandardItemModel {
    Q_OBJECT

public:
    /**
     * @brief Constructeur du modèle de table des trades
     * @param parent Le parent QObject
     */
    TradesTableModel(QObject* parent = nullptr);
    
    /**
     * @brief Met à jour les données du modèle avec un vecteur de trades
     * @param trades Les trades à afficher
     */
    void updateData(const std::vector<be::TradeData>& trades);
    
    /**
     * @brief Efface toutes les lignes du modèle
     */
    void clear();
    
    // Méthodes utilitaires statiques
    static QString formatNumber(double value, int precision = 2);
    static QDateTime dateToQDateTime(const be::Date& date);
    static QString formatDateTime(const QDateTime& dateTime);
    
private:
    static QString formatDuration(const QString& duration);
};