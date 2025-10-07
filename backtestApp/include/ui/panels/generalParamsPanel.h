#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDateTime>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QMap>
#include <QString>
#include <QVariant>
#include "ui/panels/ConfigPanel.h"
#include <sstream>
#include <string>
#include <QFileSystemWatcher>


struct GeneralParamsConfig {
    std::string strategyName;
    std::string symbol;
    std::string interval;
    std::string period;
    QDateTime endDate;
    double cash;
    double spread;
    double commission;
    double leverage_limit;
    bool tradeOnClose;
    bool hedging;
    bool exclusiveOrders;
    bool finalizeTrades;
};

// surcharge de l'operateur << pour GeneralParamsConfig
inline std::ostream& operator<<(std::ostream& os, const GeneralParamsConfig& config) {
    os << "GeneralParamsConfig {\n"
       << "  strategyName: " << config.strategyName << ", \n"
       << "  symbol: " << config.symbol << ", \n"
       << "  interval: " << config.interval << ", \n"
       << "  period: " << config.period << ", \n"
       << "  endDate: " << config.endDate.toString("dd/MM/yyyy").toStdString() << ", \n"
       << "  cash: " << config.cash << ", \n"
       << "  spread: " << config.spread << ", \n"
       << "  commission: " << config.commission << ", \n"
       << "  leverage_limit: " << config.leverage_limit << ", \n"
       << "  tradeOnClose: " << config.tradeOnClose << ", \n"
       << "  hedging: " << config.hedging << ", \n"
       << "  exclusiveOrders: " << config.exclusiveOrders << ", \n"
       << "  finalizeTrades: " << config.finalizeTrades << "\n"
       << "}\n";
    return os;
}

/**
 * @brief Panel des paramètres généraux du backtest
 * 
 * Ce panel contient les paramètres généraux comme le symbole,
 * la période, l'intervalle, le spread, etc.
 */
class GeneralParamsPanel : public ConfigPanel<GeneralParamsConfig>
{
    Q_OBJECT

public:
    GeneralParamsPanel(QWidget* parent = nullptr);

private:
    void setupUI();
    void refreshSymbols();

    // UI member so we can update it when marketData changes
    QComboBox* m_symbolCombo = nullptr;
    QFileSystemWatcher m_watcher;
    QString m_marketDataDir = QStringLiteral("./marketData");
};

