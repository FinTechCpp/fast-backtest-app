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
#include "ui/panels/basePanel.h"
#include <sstream>
#include <string>


struct GeneralParamsConfig {
    QString strategyName;
    QString symbol;
    QString interval;
    QString period;
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
       << "strategyName: " << config.strategyName.toStdString() << ", "
       << "symbol: " << config.symbol.toStdString() << ", "
       << "interval: " << config.interval.toStdString() << ", "
       << "period: " << config.period.toStdString() << ", "
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
class GeneralParamsPanel : public BasePanel
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    GeneralParamsPanel(QWidget* parent = nullptr);

    GeneralParamsConfig getConfig();
    void setConfig(const GeneralParamsConfig& config);

signals:
    void strategyChanged(const QString& strategy);
private slots:
    void onStrategyComboChanged(const QString& strategy);


private:
    GeneralParamsConfig m_config;
    std::vector<std::unique_ptr<PropertyBinder>> m_bindings;

    // Dictionnaire associant les noms de stratégies à leurs classes
    QMap<QString, QString> m_strategyMap;
    
    // Initialisation du dictionnaire des stratégies
    void initStrategyMap();


    void setupUI();
    
    void initializeBindings();
    void updateConfigFromWidgets();
    void updateWidgetsFromConfig();
    void addBinding(std::unique_ptr<PropertyBinder> binding);
    void createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& dependentWidgets);
};

