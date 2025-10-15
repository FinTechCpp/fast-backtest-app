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
#include "beTypes.h"
#include "ui/dialogs/BacktestEngineDialog.h"


struct GeneralParamsConfig {
    std::string strategyName;
    // data
    std::string symbol;
    std::string interval;
    std::string period;
    QDateTime endDate;
    // BE
    double cash;
    double spread;
    double commission;
    double leverage_limit = 20.0;
    bool tradeOnClose = false;
    be::PositionMode positionMode = be::PositionMode::Netting;
    bool executeLimitOnLimitPrice = true;
    bool executeStopOnOpen = true;
    bool finalizeTrades = true;
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
       << "  positionMode: " << (config.positionMode == be::PositionMode::Hedging ? "Hedging" : "Netting") << ", \n"
       << "  finalizeTrades: " << config.finalizeTrades << ", \n"
       << "  executeLimitOnLimitPrice: " << config.executeLimitOnLimitPrice << ", \n"
       << "  executeStopOnOpen: " << config.executeStopOnOpen << "\n"
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
    void openAdvancedConfigDialog();

    // UI member so we can update it when marketData changes
    QComboBox* m_symbolCombo = nullptr;
    QFileSystemWatcher m_watcher;
    QString m_marketDataDir = QStringLiteral("./marketData");
};

