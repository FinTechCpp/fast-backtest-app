#include "strategy_base_panel.h"
#include <QDebug>
#include <QObject>
#include <QFormLayout>  // AJOUT OBLIGATOIRE
#include <QComboBox>    // AJOUT OBLIGATOIRE
#include <QFrame>       // AJOUT OBLIGATOIRE

StrategyBasePanel::StrategyBasePanel(QWidget* parent)
    : QObject(parent), BasePanel(parent)
{
}

QGroupBox* StrategyBasePanel::create()
{
    QGroupBox* baseGroup = new QGroupBox("Paramètres de base");
    QVBoxLayout* baseLayout = new QVBoxLayout();

    // Section SL/TP
    QGroupBox* slTpGroup = new QGroupBox("Stop Loss et Take Profit");
    QVBoxLayout* slTpLayout = new QVBoxLayout();
    
    // Groupe Stop Loss
    QGroupBox* slGroup = new QGroupBox("Stop Loss");
    QFormLayout* slLayout = new QFormLayout();
    
    // Méthode de calcul pour le Stop Loss
    m_widgets["sl_method"] = new QComboBox();
    static_cast<QComboBox*>(m_widgets["sl_method"])->addItems({"Fixe", "ATR", "Min/Max"});
    static_cast<QComboBox*>(m_widgets["sl_method"])->setCurrentIndex(0);
    
    QObject::connect(static_cast<QComboBox*>(m_widgets["sl_method"]), 
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &StrategyBasePanel::_toggleSlMethod);
    
    slLayout->addRow(new QLabel("Méthode:"), m_widgets["sl_method"]);
    
    // Stop Loss Distance (fixe)
    m_widgets["stop_loss_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setRange(0, 1000);
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setSingleStep(1);
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setValue(20);
    slLayout->addRow(new QLabel("Distance [pts]:"), m_widgets["stop_loss_distance"]);
    
    // Paramètres ATR - Multiplicateur SL
    m_widgets["sl_atr_multiplier"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setSingleStep(0.1);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setValue(2.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setEnabled(false);
    slLayout->addRow(new QLabel("Multiplicateur ATR SL:"), m_widgets["sl_atr_multiplier"]);
    
    // Paramètres Min/Max SL
    m_widgets["sl_minmax_periods"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setValue(5);
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setEnabled(false);
    slLayout->addRow(new QLabel("Périodes Min/Max:"), m_widgets["sl_minmax_periods"]);
    
    m_widgets["sl_minmax_delta"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setValue(5.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setEnabled(false);
    slLayout->addRow(new QLabel("Delta Min/Max:"), m_widgets["sl_minmax_delta"]);
    
    // SL minimum
    m_widgets["min_stop_loss_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["min_stop_loss_distance"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["min_stop_loss_distance"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["min_stop_loss_distance"])->setValue(5.0);
    slLayout->addRow(new QLabel("SL Minimum [pts]:"), m_widgets["min_stop_loss_distance"]);
    
    slGroup->setLayout(slLayout);
    slTpLayout->addWidget(slGroup);
    
    // Groupe Take Profit
    QGroupBox* tpGroup = new QGroupBox("Take Profit");
    QFormLayout* tpLayout = new QFormLayout();
    
    // Méthode de calcul pour le Take Profit
    m_widgets["tp_method"] = new QComboBox();
    static_cast<QComboBox*>(m_widgets["tp_method"])->addItems({"Fixe", "ATR"});
    static_cast<QComboBox*>(m_widgets["tp_method"])->setCurrentIndex(0);
    QObject::connect(static_cast<QComboBox*>(m_widgets["tp_method"]), 
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &StrategyBasePanel::_toggleTpMethod);
    tpLayout->addRow(new QLabel("Méthode:"), m_widgets["tp_method"]);
    
    // Take Profit Distance (fixe)
    m_widgets["take_profit_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setRange(0, 1000);
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setSingleStep(1);
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setValue(30);
    tpLayout->addRow(new QLabel("Distance [pts]:"), m_widgets["take_profit_distance"]);
    
    // Paramètres ATR - Multiplicateur TP
    m_widgets["tp_atr_multiplier"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setSingleStep(0.1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setValue(3.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setEnabled(false);
    tpLayout->addRow(new QLabel("Multiplicateur ATR TP:"), m_widgets["tp_atr_multiplier"]);
    
    // TP minimum
    m_widgets["min_take_profit_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["min_take_profit_distance"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["min_take_profit_distance"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["min_take_profit_distance"])->setValue(5.0);
    tpLayout->addRow(new QLabel("TP Minimum [pts]:"), m_widgets["min_take_profit_distance"]);
    
    tpGroup->setLayout(tpLayout);
    slTpLayout->addWidget(tpGroup);
    
    slTpGroup->setLayout(slTpLayout);
    baseLayout->addWidget(slTpGroup);
    
    // Section ATR avec checkbox
    QGroupBox* atrGroup = new QGroupBox("ATR (Average True Range)");
    QFormLayout* atrLayout = new QFormLayout();
    
    m_widgets["use_atr_for_sl"] = new QCheckBox("Utiliser ATR pour SL");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_atr_for_sl"]), 
                     &QCheckBox::toggled,
                     this, &StrategyBasePanel::_toggleAtrControls);
    atrLayout->addRow(m_widgets["use_atr_for_sl"]);
    
    m_widgets["use_atr_for_tp"] = new QCheckBox("Utiliser ATR pour TP");
    atrLayout->addRow(m_widgets["use_atr_for_tp"]);
    
    m_widgets["atr_period"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["atr_period"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["atr_period"])->setValue(14);
    atrLayout->addRow(new QLabel("Période ATR:"), m_widgets["atr_period"]);
    
    atrGroup->setLayout(atrLayout);
    baseLayout->addWidget(atrGroup);
    
    // Section Heures de trading
    QGroupBox* timeGroup = new QGroupBox("Heures de trading");
    QFormLayout* timeLayout = new QFormLayout();
    
    m_widgets["trading_from"] = new QTimeEdit();
    static_cast<QTimeEdit*>(m_widgets["trading_from"])->setTime(QTime(7, 0));
    timeLayout->addRow(new QLabel("De:"), m_widgets["trading_from"]);
    
    m_widgets["trading_to"] = new QTimeEdit();
    static_cast<QTimeEdit*>(m_widgets["trading_to"])->setTime(QTime(23, 0));
    timeLayout->addRow(new QLabel("À:"), m_widgets["trading_to"]);
    
    timeGroup->setLayout(timeLayout);
    baseLayout->addWidget(timeGroup);
    
    // Section Gestion du risque
    QGroupBox* riskGroup = new QGroupBox("Gestion du risque");
    QVBoxLayout* riskLayout = new QVBoxLayout();
    
    m_widgets["use_risk_based_sizing"] = new QCheckBox("Utiliser le sizing basé sur le risque");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_risk_based_sizing"]), 
                     &QCheckBox::toggled,
                     this, &StrategyBasePanel::_toggleRiskControls);
    riskLayout->addWidget(m_widgets["use_risk_based_sizing"]);
    
    QFrame* riskFrame = new QFrame();
    QFormLayout* riskFrameLayout = new QFormLayout(riskFrame);
    
    m_widgets["risk_percentage"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setRange(0.01, 10.0);
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setValue(1.0);
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setEnabled(false);
    riskFrameLayout->addRow(new QLabel("% Risque par trade:"), m_widgets["risk_percentage"]);
    
    riskLayout->addWidget(riskFrame);
    riskGroup->setLayout(riskLayout);
    baseLayout->addWidget(riskGroup);
    
    // Section Break Even
    QGroupBox* breakEvenGroup = new QGroupBox("Break Even");
    QVBoxLayout* breakEvenLayout = new QVBoxLayout();
    
    m_widgets["use_break_even"] = new QCheckBox("Activer le break even");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_break_even"]), 
                     &QCheckBox::toggled,
                     this, &StrategyBasePanel::_toggleBreakEvenControls);
    breakEvenLayout->addWidget(m_widgets["use_break_even"]);
    
    QFrame* breakEvenFrame = new QFrame();
    QFormLayout* breakEvenFrameLayout = new QFormLayout(breakEvenFrame);
    
    m_widgets["break_even_threshold"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setRange(0.1, 10.0);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setValue(0.7);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setEnabled(false);
    breakEvenFrameLayout->addRow(new QLabel("Seuil (x TP):"), m_widgets["break_even_threshold"]);
    
    breakEvenLayout->addWidget(breakEvenFrame);
    breakEvenGroup->setLayout(breakEvenLayout);
    baseLayout->addWidget(breakEvenGroup);
    
    // Section Perte maximale quotidienne
    QGroupBox* maxLossGroup = new QGroupBox("Perte maximale quotidienne");
    QVBoxLayout* maxLossLayout = new QVBoxLayout();
    
    m_widgets["use_daily_max_loss"] = new QCheckBox("Activer la limite de perte quotidienne");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_daily_max_loss"]), 
                     &QCheckBox::toggled,
                     this, &StrategyBasePanel::_toggleDailyMaxLossControls);
    maxLossLayout->addWidget(m_widgets["use_daily_max_loss"]);
    
    QFrame* maxLossFrame = new QFrame();
    QFormLayout* maxLossFrameLayout = new QFormLayout(maxLossFrame);
    
    m_widgets["daily_max_loss_percentage"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setRange(0.1, 50.0);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setValue(2.0);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setEnabled(false);
    maxLossFrameLayout->addRow(new QLabel("% du capital:"), m_widgets["daily_max_loss_percentage"]);
    
    maxLossLayout->addWidget(maxLossFrame);
    maxLossGroup->setLayout(maxLossLayout);
    baseLayout->addWidget(maxLossGroup);
    
    // Section Jours de trading
    QGroupBox* tradingDaysGroup = new QGroupBox("Jours de trading");
    QVBoxLayout* tradingDaysLayout = new QVBoxLayout();
    
    // Checkboxes pour chaque jour
    QStringList dayNames = {"Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"};
    for (int i = 0; i < dayNames.size(); ++i) {
        QString widgetName = QString("trading_day_%1").arg(i);
        m_widgets[widgetName] = new QCheckBox(dayNames[i]);
        
        // Par défaut, activer Lundi-Vendredi (indices 0-4)
        static_cast<QCheckBox*>(m_widgets[widgetName])->setChecked(i < 5);
        
        tradingDaysLayout->addWidget(m_widgets[widgetName]);
    }
    
    tradingDaysGroup->setLayout(tradingDaysLayout);
    baseLayout->addWidget(tradingDaysGroup);
    
    baseGroup->setLayout(baseLayout);
    
    // Initialiser l'état des contrôles
    _updateAtrPeriodStatus();
    
    return baseGroup;
}

// Implémentation des slots
void StrategyBasePanel::_toggleAtrControls(bool checked)
{
    Q_UNUSED(checked)
    _updateAtrPeriodStatus();
}

void StrategyBasePanel::_toggleRiskControls(bool checked)
{
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setEnabled(checked);
}

void StrategyBasePanel::_toggleBreakEvenControls(bool checked)
{
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setEnabled(checked);
}

void StrategyBasePanel::_toggleDailyMaxLossControls(bool checked)
{
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setEnabled(checked);
}

void StrategyBasePanel::_toggleSlMethod(int index)
{
    // Index 0: Fixe, Index 1: ATR, Index 2: Min/Max
    bool isATR = (index == 1);
    bool isMinMax = (index == 2);
    
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setEnabled(isATR);
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setEnabled(isMinMax);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setEnabled(isMinMax);
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setEnabled(index == 0);
}

void StrategyBasePanel::_toggleTpMethod(int index)
{
    // Index 0: Fixe, Index 1: ATR
    bool isATR = (index == 1);
    
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setEnabled(isATR);
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setEnabled(index == 0);
}

void StrategyBasePanel::_updateAtrPeriodStatus()
{
    bool slUsesATR = static_cast<QCheckBox*>(m_widgets["use_atr_for_sl"])->isChecked();
    bool tpUsesATR = static_cast<QCheckBox*>(m_widgets["use_atr_for_tp"])->isChecked();
    
    static_cast<QSpinBox*>(m_widgets["atr_period"])->setEnabled(slUsesATR || tpUsesATR);
}

QMap<QString, QVariant> StrategyBasePanel::getValues()
{
    QMap<QString, QVariant> values;
    
    // Récupérer toutes les valeurs des widgets existants
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        const QString& key = it.key();
        QWidget* widget = it.value();
        
        if (QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
            values[key] = doubleSpinBox->value();
        }
        else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
            values[key] = spinBox->value();
        }
        else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
            values[key] = checkBox->isChecked();
        }
        else if (QTimeEdit* timeEdit = qobject_cast<QTimeEdit*>(widget)) {
            values[key] = timeEdit->time();
        }
        else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
            values[key] = comboBox->currentIndex();
        }
    }
    
    // AJOUT OBLIGATOIRE: Récupérer les jours de trading
    QList<QVariant> tradingDaysList;
    for (int i = 0; i < 7; ++i) {
        QString widgetName = QString("trading_day_%1").arg(i);
        if (m_widgets.contains(widgetName)) {
            QCheckBox* dayCheckBox = static_cast<QCheckBox*>(m_widgets[widgetName]);
            if (dayCheckBox && dayCheckBox->isChecked()) {
                tradingDaysList.append(i);
            }
        }
    }
    values["trading_days"] = tradingDaysList;
    
    return values;
}

void StrategyBasePanel::setValues(const QMap<QString, QVariant>& values)
{
    // SL Method
    if (values.contains("use_atr_for_sl") && values.contains("use_minmax_for_sl")) {
        int methodIndex = 0; // Fixe par défaut
        if (values["use_atr_for_sl"].toBool()) {
            methodIndex = 1; // ATR
        } else if (values["use_minmax_for_sl"].toBool()) {
            methodIndex = 2; // Min/Max
        }
        static_cast<QComboBox*>(m_widgets["sl_method"])->setCurrentIndex(methodIndex);
    }
    
    if (values.contains("stop_loss_distance")) {
        static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setValue(values["stop_loss_distance"].toDouble());
    }
    
    if (values.contains("sl_atr_multiplier")) {
        static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setValue(values["sl_atr_multiplier"].toDouble());
    }
    
    if (values.contains("sl_minmax_periods")) {
        static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setValue(values["sl_minmax_periods"].toInt());
    }
    
    if (values.contains("sl_minmax_delta")) {
        static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setValue(values["sl_minmax_delta"].toDouble());
    }
    
    if (values.contains("min_stop_loss_distance")) {
        static_cast<QDoubleSpinBox*>(m_widgets["min_stop_loss_distance"])->setValue(values["min_stop_loss_distance"].toDouble());
    }
    
    // TP Method
    if (values.contains("use_atr_for_tp")) {
        int methodIndex = values["use_atr_for_tp"].toBool() ? 1 : 0;
        static_cast<QComboBox*>(m_widgets["tp_method"])->setCurrentIndex(methodIndex);
    }
    
    if (values.contains("take_profit_distance")) {
        static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setValue(values["take_profit_distance"].toDouble());
    }
    
    if (values.contains("tp_atr_multiplier")) {
        static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setValue(values["tp_atr_multiplier"].toDouble());
    }
    
    if (values.contains("min_take_profit_distance")) {
        static_cast<QDoubleSpinBox*>(m_widgets["min_take_profit_distance"])->setValue(values["min_take_profit_distance"].toDouble());
    }
    
    // ATR
    if (values.contains("atr_period")) {
        static_cast<QSpinBox*>(m_widgets["atr_period"])->setValue(values["atr_period"].toInt());
    }
    
    // Heures de trading
    if (values.contains("trading_from")) {
        static_cast<QTimeEdit*>(m_widgets["trading_from"])->setTime(values["trading_from"].toTime());
    }
    
    if (values.contains("trading_to")) {
        static_cast<QTimeEdit*>(m_widgets["trading_to"])->setTime(values["trading_to"].toTime());
    }
    
    // Gestion du risque
    if (values.contains("use_risk_based_sizing")) {
        static_cast<QCheckBox*>(m_widgets["use_risk_based_sizing"])->setChecked(values["use_risk_based_sizing"].toBool());
        _toggleRiskControls(values["use_risk_based_sizing"].toBool());
    }
    
    if (values.contains("risk_percentage")) {
        static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setValue(values["risk_percentage"].toDouble());
    }
    
    // Break Even
    if (values.contains("use_break_even")) {
        static_cast<QCheckBox*>(m_widgets["use_break_even"])->setChecked(values["use_break_even"].toBool());
        _toggleBreakEvenControls(values["use_break_even"].toBool());
    }
    
    if (values.contains("break_even_threshold")) {
        static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setValue(values["break_even_threshold"].toDouble());
    }
    
    // Perte maximale quotidienne
    if (values.contains("use_daily_max_loss")) {
        static_cast<QCheckBox*>(m_widgets["use_daily_max_loss"])->setChecked(values["use_daily_max_loss"].toBool());
        _toggleDailyMaxLossControls(values["use_daily_max_loss"].toBool());
    }
    
    if (values.contains("daily_max_loss_percentage")) {
        static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setValue(values["daily_max_loss_percentage"].toDouble());
    }
    
    // Mettre à jour l'état des contrôles
    _updateAtrPeriodStatus();
}