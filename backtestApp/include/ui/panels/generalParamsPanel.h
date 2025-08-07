#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
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
    os << "GeneralParamsConfig("
       << "strategyName: " << config.strategyName << ", "
       << "symbol: " << config.symbol << ", "
       << "interval: " << config.interval << ", "
       << "period: " << config.period << ", "
       << "endDate: " << config.endDate.toString("dd/MM/yyyy").toStdString() << ", "
       << "cash: " << config.cash << ", "
       << "spread: " << config.spread << ", "
       << "commission: " << config.commission << ", "
       << "leverage_limit: " << config.leverage_limit << ", "
       << "tradeOnClose: " << config.tradeOnClose << ", "
       << "hedging: " << config.hedging << ", "
       << "exclusiveOrders: " << config.exclusiveOrders << ", "
       << "finalizeTrades: " << config.finalizeTrades
       << ")";
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

signals:
    void strategyChanged(const QString& strategy);
private slots:
    void onStrategyComboChanged(const QString& strategy);


private:
    QMap<QString, QString> m_strategyMap;
    void initStrategyMap();

    void setupUI();
};

