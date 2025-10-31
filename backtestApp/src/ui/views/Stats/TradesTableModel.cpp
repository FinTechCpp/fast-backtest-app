#include "ui/views/Stats/TradesTableModel.h"

// Utilitary for converting be::Date to QDateTime
QDateTime TradesTableModel::dateToQDateTime(const be::Date& date) {
    // Use public getters instead of directly accessing private members
    return QDateTime(
        QDate(static_cast<int>(date.year), static_cast<int>(date.month), static_cast<int>(date.day)),
        QTime(static_cast<int>(date.hour), static_cast<int>(date.minute), static_cast<int>(date.second))
    );
}

// Implementation of TradesTableModel
TradesTableModel::TradesTableModel(QObject* parent)
    : QStandardItemModel(parent)
{
    // Set default headers
    QStringList headers;
    headers << "Id" << "Date" << "Type" << "Entry" << "Exit" << "Duration" << "PnL" << "PnL%" << "SL" << "TP";
    setHorizontalHeaderLabels(headers);
}

void TradesTableModel::clear() {
    removeRows(0, rowCount());
}

// New implementation to work with be::Trade
void TradesTableModel::updateData(const std::vector<be::TradeData>& trades)
{
    beginResetModel();

    // Clear existing data
    removeRows(0, rowCount());
    
    if (trades.empty()) {
        endResetModel();
        return;
    }

    // Set headers
    QStringList headers;
    headers << "Id" << "Type" << "Size" << "Entry Price" << "Exit Price" 
            << "PnL" << "PnL %" << "Duration" << "Entry Date" << "Exit Date"
            << "Initial SL" << "TP" << "Close Reason" << "Tag";
    setHorizontalHeaderLabels(headers);

    // Add new data
    setRowCount(static_cast<int>(trades.size()));
    
    for (int row = 0; row < static_cast<int>(trades.size()); ++row) {
        const auto& trade = trades[row];
        
        // Id ( trade number)
        setItem(row, 0, new PctItem(QString::number(trade.id), trade.id));
        
        // Type (LONG/SHORT based on size)
        QString tradeType = trade.side == be::OrderSide::BUY ? "LONG" : "SHORT";
        QStandardItem* typeItem = new QStandardItem(tradeType);
        typeItem->setForeground(trade.side == be::OrderSide::BUY ? Qt::darkGreen : Qt::darkRed);
        setItem(row, 1, typeItem);

        // Size (absolute value)
        setItem(row, 2, new PctItem(formatNumber(trade.size, 1), trade.size));

        // Entry Price
        setItem(row, 3, new PctItem(formatNumber(trade.entryPrice, 2), trade.entryPrice));

        // Exit Price
        setItem(row, 4, new PctItem(formatNumber(trade.exitPrice, 2), trade.exitPrice));

        // PnL
        double pnl = trade.pl;
        be::CloseReason closeReason = trade.closeReason;
        PctItem* pnlItem = new PctItem(formatNumber(pnl, 2), pnl);
        pnlItem->setForeground(
            closeReason == be::CloseReason::TakeProfit ? Qt::darkGreen : (
            closeReason == be::CloseReason::StopLoss ? Qt::darkRed : (
            closeReason == be::CloseReason::BreakEven ? Qt::darkBlue : Qt::darkGray)));
        QFont boldFont = pnlItem->font(); // Retrieve current font
        boldFont.setBold(true);           // Set bold
        pnlItem->setFont(boldFont);       // Apply modified font
        setItem(row, 5, pnlItem);
        
        // PnL %
        PctItem* pctItem = new PctItem(formatNumber(trade.plPercent, 2) + "%", trade.plPercent);
        pctItem->setForeground(
            closeReason == be::CloseReason::TakeProfit ? Qt::darkGreen : (
            closeReason == be::CloseReason::StopLoss ? Qt::darkRed : (
            closeReason == be::CloseReason::BreakEven ? Qt::darkBlue : Qt::darkGray)));
        QFont pctFont = pctItem->font();
        pctFont.setBold(true);
        pctItem->setFont(pctFont);
        setItem(row, 6, pctItem);

        // Duration - calculate from dates
        QDateTime entryDT = dateToQDateTime(trade.entryDate);
        QDateTime exitDT = dateToQDateTime(trade.exitDate);
        be::Duration duration = trade.exitDate - trade.entryDate;
                
        setItem(row, 7, new PctItem(QString::fromStdString(duration.toString()), duration.seconds));

        // Entry Date
        setItem(row, 8, new PctItem(formatDateTime(entryDT), static_cast<double>(entryDT.toSecsSinceEpoch())));

        // Exit Date
        setItem(row, 9, new PctItem(formatDateTime(exitDT), static_cast<double>(exitDT.toSecsSinceEpoch())));

        // Stop Loss (if available)
        QString slText = "-";
        if (trade.initialSlPrice > 0) {
            slText = formatNumber(trade.initialSlPrice, 2);
        } else if (trade.lastSlPrice > 0) {
            slText = formatNumber(trade.lastSlPrice, 2);
        }
        setItem(row, 10, new QStandardItem(slText));

        // Take Profit (if available)
        QString tpText = "-";
        if (trade.tpPrice > 0) {
            tpText = formatNumber(trade.tpPrice, 2);
        }
        setItem(row, 11, new QStandardItem(tpText));

        // Close Reason (close reason with color coding)
        QString closeReasonText = "-";
        QColor textColor = Qt::darkGray; // Default gray for Unknown/ManualClose

        // DDetermine text and color based on close reason
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
                closeReasonText = "Manual";
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

        // Tag (if available)
        QString tag = "-";
        if (!trade.tag.empty()) {
            tag = QString::fromStdString(trade.tag);
        }
        setItem(row, 13, new QStandardItem(tag));

        // Apply background color based on close reason and PnL
        QColor rowColor;
        if (trade.closeReason == be::CloseReason::TakeProfit) {
            rowColor = QColor(220, 255, 220); // Light green
        } else if (trade.closeReason == be::CloseReason::StopLoss) {
            rowColor = QColor(255, 220, 220); // Light red
        } else if (trade.closeReason == be::CloseReason::BreakEven) {
            rowColor = QColor(220, 240, 255); // Light blue
        } else {
            // ManualClose or Unknown
            if (pnl > 0) {
                rowColor = QColor(240, 255, 240); // Very light green
            } else if (pnl < 0) {
                rowColor = QColor(255, 240, 240); // Very light red
            } else {
                rowColor = QColor(240, 240, 240); // Very light gray
            }
        }

        // Apply color to all cells in the row
        for (int col = 0; col < columnCount(); ++col) {
            QStandardItem* item = this->item(row, col);
            if (item) 
                item->setData(rowColor, Qt::BackgroundRole);
        }
    }

    endResetModel();
}

// Utility methods
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
