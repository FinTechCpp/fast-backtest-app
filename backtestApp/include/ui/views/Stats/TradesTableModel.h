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
    TradesTableModel(QObject* parent = nullptr);
    
    void updateData(const std::vector<be::TradeData>& trades);
    void clear();
    
    // Méthodes utilitaires statiques
    static QString formatNumber(double value, int precision = 2);
    static QDateTime dateToQDateTime(const be::Date& date);
    static QString formatDateTime(const QDateTime& dateTime);
};

class PctItem : public QStandardItem {
public:
    PctItem(const QString& text, double value) : QStandardItem(text), m_value(value) {}
    
    bool operator<(const QStandardItem& other) const override {
        // Comparer les valeurs numériques pour le tri
        if (const PctItem* pctOther = dynamic_cast<const PctItem*>(&other)) {
            return m_value < pctOther->m_value;
        }
        return QStandardItem::operator<(other);
    }
    
private:
    double m_value;
};