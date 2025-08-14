#include "ui/views/Stats/TradesTableModel.h"

// Utilitaire pour convertir be::Date en QDateTime
QDateTime TradesTableModel::dateToQDateTime(const be::Date& date) {
    // Utiliser les getters publics au lieu d'accéder directement aux membres privés
    return QDateTime(
        QDate(static_cast<int>(date.year), static_cast<int>(date.month), static_cast<int>(date.day)),
        QTime(static_cast<int>(date.hour), static_cast<int>(date.minute), static_cast<int>(date.second))
    );
}

// Implémentation de TradesTableModel
TradesTableModel::TradesTableModel(QObject* parent)
    : QStandardItemModel(parent)
{
    // Définir les en-têtes par défaut
    QStringList headers;
    headers << "#" << "Date" << "Type" << "Entrée" << "Sortie" << "Dur." << "PnL" << "PnL%" << "SL" << "TP";
    setHorizontalHeaderLabels(headers);
}

void TradesTableModel::clear() {
    removeRows(0, rowCount());
}

// Nouvelle implémentation pour travailler avec des be::Trade
void TradesTableModel::updateData(const std::vector<be::TradeData>& trades)
{
    beginResetModel();
    
    // Effacer les données existantes
    removeRows(0, rowCount());
    
    if (trades.empty()) {
        endResetModel();
        return;
    }
    
    // Configurer les en-têtes
    QStringList headers;
    headers << "#" << "Type" << "Taille" << "Prix d'entrée" << "Prix de sortie" 
            << "PnL" << "PnL %" << "Durée" << "Date d'entrée" << "Date de sortie"
            << "SL initial" << "TP" << "Clôture" << "Tag";
    setHorizontalHeaderLabels(headers);
    
    // Ajouter les nouvelles données
    setRowCount(static_cast<int>(trades.size()));
    
    for (int row = 0; row < static_cast<int>(trades.size()); ++row) {
        const auto& trade = trades[row];
        
        // # (numéro de trade)
        setItem(row, 0, new QStandardItem(QString::number(row + 1)));
        
        // Type (LONG/SHORT basé sur la taille)
        QString tradeType = trade.size > 0 ? "LONG" : "SHORT";
        QStandardItem* typeItem = new QStandardItem(tradeType);
        typeItem->setForeground(trade.size > 0 ? Qt::darkGreen : Qt::darkRed);
        setItem(row, 1, typeItem);
        
        // Taille (valeur absolue)
        setItem(row, 2, new QStandardItem(formatNumber(std::abs(trade.size), 4)));

        // Prix d'entrée
        setItem(row, 3, new QStandardItem(formatNumber(trade.entryPrice, 2)));

        // Prix de sortie
        setItem(row, 4, new QStandardItem(formatNumber(trade.exitPrice, 2)));

        // PnL
        double pnl = trade.pl;
        be::CloseReason closeReason = trade.closeReason;
        QStandardItem* pnlItem = new QStandardItem(formatNumber(pnl, 2));
        pnlItem->setForeground(
            closeReason == be::CloseReason::TakeProfit ? Qt::darkGreen : (
            closeReason == be::CloseReason::StopLoss ? Qt::darkRed : (
            closeReason == be::CloseReason::BreakEven ? Qt::darkBlue : Qt::darkGray)));
        QFont boldFont = pnlItem->font(); // Récupère la police actuelle
        boldFont.setBold(true);           // Active le gras
        pnlItem->setFont(boldFont);       // Applique la police modifiée
        setItem(row, 5, pnlItem);
        
        // PnL %
        double returnPct = trade.plPercent;
        QStandardItem* pctItem = new QStandardItem(formatNumber(returnPct * 100, 2) + "%");
        pctItem->setForeground(
            closeReason == be::CloseReason::TakeProfit ? Qt::darkGreen : (
            closeReason == be::CloseReason::StopLoss ? Qt::darkRed : (
            closeReason == be::CloseReason::BreakEven ? Qt::darkBlue : Qt::darkGray)));
        QFont pctFont = pctItem->font();
        pctFont.setBold(true);
        pctItem->setFont(pctFont);
        setItem(row, 6, pctItem);
        
        // Durée - calculer à partir des dates
        QDateTime entryDT = dateToQDateTime(trade.entryDate);
        QDateTime exitDT = dateToQDateTime(trade.exitDate);
        qint64 durationSecs = entryDT.secsTo(exitDT);
        
        QString durationStr;
        if (durationSecs < 60) {
            durationStr = QString("%1s").arg(durationSecs);
        } else if (durationSecs < 3600) {
            durationStr = QString("%1m %2s").arg(durationSecs / 60).arg(durationSecs % 60);
        } else if (durationSecs < 86400) {
            int hours = durationSecs / 3600;
            int mins = (durationSecs % 3600) / 60;
            durationStr = QString("%1h %2m").arg(hours).arg(mins);
        } else {
            int days = durationSecs / 86400;
            int hours = (durationSecs % 86400) / 3600;
            durationStr = QString("%1j %2h").arg(days).arg(hours);
        }
        setItem(row, 7, new QStandardItem(durationStr));
        
        // Date d'entrée
        setItem(row, 8, new QStandardItem(formatDateTime(entryDT)));
        
        // Date de sortie
        setItem(row, 9, new QStandardItem(formatDateTime(exitDT)));
        
        // Stop Loss (si disponible)
        QString slText = "-";
        if (trade.initialSlPrice > 0) {
            slText = formatNumber(trade.initialSlPrice, 2);
        } else if (trade.lastSlPrice > 0) {
            slText = formatNumber(trade.lastSlPrice, 2);
        }
        setItem(row, 10, new QStandardItem(slText));
        
        // Take Profit (si disponible)
        QString tpText = "-";
        if (trade.tpPrice > 0) {
            tpText = formatNumber(trade.tpPrice, 2);
        }
        setItem(row, 11, new QStandardItem(tpText));

        // Close Reason (raison de clôture)
        QString closeReasonText = "-";
        QColor textColor = Qt::darkGray; // Par défaut gris pour Unknown/ManualClose

        // Déterminer le texte et la couleur selon le type de clôture
        switch (trade.closeReason) {
            case be::CloseReason::TakeProfit:
                closeReasonText = "TP";
                textColor = Qt::darkGreen;
                break;
            case be::CloseReason::StopLoss:
                closeReasonText = "SL";
                textColor = Qt::darkRed;
                break;
            case be::CloseReason::BreakEven:
                closeReasonText = "BE";
                textColor = Qt::darkBlue;
                break;
            case be::CloseReason::ManualClose:
                closeReasonText = "Manuel";
                textColor = Qt::darkGray;
                break;
            default:
                closeReasonText = "-";
                textColor = Qt::darkGray;
        }

        QStandardItem* closeReasonItem = new QStandardItem(closeReasonText);
        closeReasonItem->setForeground(textColor);
        QFont closeReasonFont = closeReasonItem->font();
        closeReasonFont.setBold(true);
        closeReasonItem->setFont(closeReasonFont);
        setItem(row, 12, closeReasonItem);
        
        // Tag (si disponible)
        QString tag = "-";
        if (!trade.tag.empty()) {
            tag = QString::fromStdString(trade.tag);
        }
        setItem(row, 13, new QStandardItem(tag));

        // Appliquer la couleur de fond selon la raison de clôture
        QColor rowColor;
        if (trade.closeReason == be::CloseReason::TakeProfit) {
            rowColor = QColor(220, 255, 220); // Vert très clair
        } else if (trade.closeReason == be::CloseReason::StopLoss) {
            rowColor = QColor(255, 220, 220); // Rouge très clair
        } else if (trade.closeReason == be::CloseReason::BreakEven) {
            rowColor = QColor(220, 240, 255); // Bleu très clair
        } else {
            // ManualClose ou Unknown
            if (pnl > 0) {
                rowColor = QColor(240, 255, 240); // Vert très pâle
            } else if (pnl < 0) {
                rowColor = QColor(255, 240, 240); // Rouge très pâle
            } else {
                rowColor = QColor(240, 240, 240); // Gris très clair
            }
        }

        // Appliquer la couleur à toutes les cellules de la ligne
        for (int col = 0; col < columnCount(); ++col) {
            QStandardItem* item = this->item(row, col);
            if (item) {
                item->setData(rowColor, Qt::BackgroundRole);
            }
        }
    }

    endResetModel();
}

// Méthodes utilitaires
QString TradesTableModel::formatNumber(double value, int precision)
{
    return QString::number(value, 'f', precision);
}

QString TradesTableModel::formatDateTime(const QDateTime& dateTime)
{
    if (!dateTime.isValid()) {
        return "-";
    }
    return dateTime.toString("dd/MM hh:mm:ss");
}
