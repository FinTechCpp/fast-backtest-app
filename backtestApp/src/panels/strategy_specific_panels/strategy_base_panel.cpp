#include "panels/strategy_specific_panels/strategy_base_panel.h"
#include <QDebug>

StrategyBasePanel::StrategyBasePanel(QWidget* parent)
    : BasePanel("Paramètres de base", parent)
{
}

void StrategyBasePanel::initialize()
{
    // Utiliser "this" comme conteneur principal au lieu de créer un nouveau QGroupBox
    QVBoxLayout* baseLayout = new QVBoxLayout(this);

    // Section Paramètres avancés
    // Option pour activer/désactiver les logs
    m_widgets["enable_logging"] = new QCheckBox("Activer la journalisation (logs)");
    static_cast<QCheckBox*>(m_widgets["enable_logging"])->setChecked(false); // Désactivé par défaut
    baseLayout->addWidget(m_widgets["enable_logging"]);

    // Section SL/TP
    QGroupBox* slTpGroup = new QGroupBox("Stop Loss et Take Profit", this);
    QVBoxLayout* slTpLayout = new QVBoxLayout();    
    // Période ATR commune (en haut du groupe)
    QFormLayout* atrLayout = new QFormLayout();
    m_widgets["atr_period"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["atr_period"])->setRange(1, 1000);
    static_cast<QSpinBox*>(m_widgets["atr_period"])->setValue(14);
    static_cast<QSpinBox*>(m_widgets["atr_period"])->setEnabled(false);
    atrLayout->addRow(new QLabel("Période ATR:"), m_widgets["atr_period"]);
    slTpLayout->addLayout(atrLayout);
    
    // Groupe Stop Loss
    QGroupBox* slGroup = new QGroupBox("Stop Loss", slTpGroup);
    QFormLayout* slLayout = new QFormLayout();
    
    // Méthode de calcul pour le Stop Loss
    m_widgets["sl_method"] = new QComboBox();
    static_cast<QComboBox*>(m_widgets["sl_method"])->addItems({"Fixe", "ATR", "Min/Max"});
    static_cast<QComboBox*>(m_widgets["sl_method"])->setCurrentIndex(0);
    
    connect(static_cast<QComboBox*>(m_widgets["sl_method"]), 
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &StrategyBasePanel::_toggleSlMethod);
    
    slLayout->addRow(new QLabel("Méthode:"), m_widgets["sl_method"]);
    
    // Stop Loss Distance (fixe)
    m_widgets["stop_loss_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setRange(0, 10000);
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setSingleStep(1);
    static_cast<QDoubleSpinBox*>(m_widgets["stop_loss_distance"])->setValue(20);
    slLayout->addRow(new QLabel("Distance [pts]:"), m_widgets["stop_loss_distance"]);
    
    // Paramètres ATR - Multiplicateur SL
    m_widgets["sl_atr_multiplier"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setRange(0.1, 1000.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setSingleStep(0.1);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setValue(2.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_atr_multiplier"])->setEnabled(false);
    slLayout->addRow(new QLabel("Multiplicateur ATR SL:"), m_widgets["sl_atr_multiplier"]);
    
    // Paramètres Min/Max SL
    m_widgets["sl_minmax_periods"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setRange(1, 1000);
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setValue(5);
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setEnabled(false);
    slLayout->addRow(new QLabel("Périodes Min/Max:"), m_widgets["sl_minmax_periods"]);
    
    m_widgets["sl_minmax_delta"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setRange(0.1, 1000.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setValue(5.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setEnabled(false);
    slLayout->addRow(new QLabel("Delta Min/Max:"), m_widgets["sl_minmax_delta"]);
    
    // SL minimum
    m_widgets["min_stop_loss_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["min_stop_loss_distance"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["min_stop_loss_distance"])->setRange(0.1, 1000.0);
    static_cast<QDoubleSpinBox*>(m_widgets["min_stop_loss_distance"])->setValue(5.0);
    slLayout->addRow(new QLabel("SL Minimum [pts]:"), m_widgets["min_stop_loss_distance"]);
    
    slGroup->setLayout(slLayout);
    slTpLayout->addWidget(slGroup);
    
    // Groupe Take Profit
    QGroupBox* tpGroup = new QGroupBox("Take Profit", slTpGroup);
    QFormLayout* tpLayout = new QFormLayout();
    
    // Méthode de calcul pour le Take Profit
    m_widgets["tp_method"] = new QComboBox();
    static_cast<QComboBox*>(m_widgets["tp_method"])->addItems({"Fixe", "ATR"});
    static_cast<QComboBox*>(m_widgets["tp_method"])->setCurrentIndex(0);
    connect(static_cast<QComboBox*>(m_widgets["tp_method"]), 
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &StrategyBasePanel::_toggleTpMethod);
    tpLayout->addRow(new QLabel("Méthode:"), m_widgets["tp_method"]);
    
    // Take Profit Distance (fixe)
    m_widgets["take_profit_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setRange(0, 10000);
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setSingleStep(1);
    static_cast<QDoubleSpinBox*>(m_widgets["take_profit_distance"])->setValue(30);
    tpLayout->addRow(new QLabel("Distance [pts]:"), m_widgets["take_profit_distance"]);
    
    // Paramètres ATR - Multiplicateur TP
    m_widgets["tp_atr_multiplier"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setRange(0.1, 1000.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setSingleStep(0.1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setValue(3.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_atr_multiplier"])->setEnabled(false);
    tpLayout->addRow(new QLabel("Multiplicateur ATR TP:"), m_widgets["tp_atr_multiplier"]);
    
    // TP minimum
    m_widgets["min_take_profit_distance"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["min_take_profit_distance"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["min_take_profit_distance"])->setRange(0.1, 1000.0);
    static_cast<QDoubleSpinBox*>(m_widgets["min_take_profit_distance"])->setValue(5.0);
    tpLayout->addRow(new QLabel("TP Minimum [pts]:"), m_widgets["min_take_profit_distance"]);
    
    tpGroup->setLayout(tpLayout);
    slTpLayout->addWidget(tpGroup);
    
    slTpGroup->setLayout(slTpLayout);
    baseLayout->addWidget(slTpGroup);
    
    // Section Heures de trading
    QGroupBox* tradingHoursGroup = new QGroupBox("Heures de trading");
    QFormLayout* tradingHoursLayout = new QFormLayout();
    
    m_widgets["trading_from"] = new QTimeEdit();
    static_cast<QTimeEdit*>(m_widgets["trading_from"])->setTime(QTime(15, 30));
    static_cast<QTimeEdit*>(m_widgets["trading_from"])->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de début:"), m_widgets["trading_from"]);
    
    m_widgets["trading_to"] = new QTimeEdit();
    static_cast<QTimeEdit*>(m_widgets["trading_to"])->setTime(QTime(22, 0));
    static_cast<QTimeEdit*>(m_widgets["trading_to"])->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de fin:"), m_widgets["trading_to"]);
    
    // Jours de trading
    QWidget* tradingDaysWidget = new QWidget();
    QHBoxLayout* tradingDaysLayout = new QHBoxLayout(tradingDaysWidget);
    
    QStringList dayNames = {"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
    for (int i = 0; i < 7; ++i) {
        QString key = QString("trading_day_%1").arg(i);
        m_widgets[key] = new QCheckBox(dayNames[i]);
        // Par défaut, activer du lundi au vendredi
        static_cast<QCheckBox*>(m_widgets[key])->setChecked(i < 5);
        tradingDaysLayout->addWidget(m_widgets[key]);
    }
    
    tradingHoursLayout->addRow(new QLabel("Jours de trading:"), tradingDaysWidget);
    
    tradingHoursGroup->setLayout(tradingHoursLayout);
    baseLayout->addWidget(tradingHoursGroup);
    
    // Section Gestion du risque
    QGroupBox* riskGroup = new QGroupBox("Gestion du risque");
    QFormLayout* riskLayout = new QFormLayout();
    
    m_widgets["use_risk_based_sizing"] = new QCheckBox("Taille basée sur le risque");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_risk_based_sizing"]), 
                     &QCheckBox::toggled,
                     this, &StrategyBasePanel::_toggleRiskControls);
    riskLayout->addRow(m_widgets["use_risk_based_sizing"]);
    
    m_widgets["risk_percentage"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setValue(1.0);
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setSuffix("%");
    static_cast<QDoubleSpinBox*>(m_widgets["risk_percentage"])->setEnabled(false);
    riskLayout->addRow(new QLabel("Risque par trade:"), m_widgets["risk_percentage"]);
    
    m_widgets["use_daily_max_loss"] = new QCheckBox("Perte max journalière");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_daily_max_loss"]), 
                     &QCheckBox::toggled,
                     this, &StrategyBasePanel::_toggleDailyMaxLossControls);
    riskLayout->addRow(m_widgets["use_daily_max_loss"]);
    
    m_widgets["daily_max_loss_percentage"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setValue(2.0);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setSuffix("%");
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_loss_percentage"])->setEnabled(false);
    riskLayout->addRow(new QLabel("Perte max journalière:"), m_widgets["daily_max_loss_percentage"]);
    
    m_widgets["use_daily_max_profit"] = new QCheckBox("Profit max journalier");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_daily_max_profit"]), 
                     &QCheckBox::toggled,
                     this, &StrategyBasePanel::_toggleDailyMaxProfitControls);
    riskLayout->addRow(m_widgets["use_daily_max_profit"]);
    
    m_widgets["daily_max_profit_percentage"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_profit_percentage"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_profit_percentage"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_profit_percentage"])->setValue(5.0);
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_profit_percentage"])->setSuffix("%");
    static_cast<QDoubleSpinBox*>(m_widgets["daily_max_profit_percentage"])->setEnabled(false);
    riskLayout->addRow(new QLabel("Profit max journalier:"), m_widgets["daily_max_profit_percentage"]);
    
    m_widgets["use_break_even"] = new QCheckBox("Activer Break Even");
    QObject::connect(static_cast<QCheckBox*>(m_widgets["use_break_even"]), 
                    &QCheckBox::toggled, 
                    this, &StrategyBasePanel::_toggleBreakEvenControls);
    riskLayout->addRow(m_widgets["use_break_even"]);
    
    m_widgets["break_even_threshold"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setRange(0.0, 1.0);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setSingleStep(0.05);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setValue(0.7);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_threshold"])->setEnabled(false);
    riskLayout->addRow(new QLabel("Seuil Break Even:"), m_widgets["break_even_threshold"]);

    riskGroup->setLayout(riskLayout);
    baseLayout->addWidget(riskGroup);
    setLayout(baseLayout);
    
    // Initialiser l'état des widgets
    _updateAtrPeriodStatus();
}

QMap<QString, QVariant> StrategyBasePanel::getValues()
{
    QMap<QString, QVariant> values;
    
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        QWidget* widget = it.value();
        QString key = it.key();
        
        if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
            values[key] = combo->currentIndex();
        }
        else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
            values[key] = spinBox->value();
        }
        else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
            values[key] = spinBox->value();
        }
        else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
            values[key] = checkBox->isChecked();
        }
        else if (QTimeEdit* timeEdit = qobject_cast<QTimeEdit*>(widget)) {
            values[key] = timeEdit->time().toString("hh:mm:ss");
        }
    }
    
    // Ajouter les paramètres use_atr_for_sl et use_atr_for_tp basés sur les méthodes sélectionnées
    if (m_widgets.contains("sl_method")) {
        QComboBox* slMethod = static_cast<QComboBox*>(m_widgets["sl_method"]);
        values["use_atr_for_sl"] = (slMethod->currentIndex() == 1); // Index 1 = ATR
    }
    
    if (m_widgets.contains("tp_method")) {
        QComboBox* tpMethod = static_cast<QComboBox*>(m_widgets["tp_method"]);
        values["use_atr_for_tp"] = (tpMethod->currentIndex() == 1); // Index 1 = ATR
    }
    
    // Ajouter la liste des jours de trading
    QVariantList tradingDays;
    for (int i = 0; i < 7; ++i) {
        QString key = QString("trading_day_%1").arg(i);
        if (m_widgets.contains(key)) {
            QCheckBox* checkBox = static_cast<QCheckBox*>(m_widgets[key]);
            if (checkBox->isChecked()) {
                tradingDays.append(i);
            }
        }
    }
    values["trading_days"] = tradingDays;
    
    return values;
}

void StrategyBasePanel::setValues(const QMap<QString, QVariant>& values)
{
    for (auto it = values.begin(); it != values.end(); ++it) {
        QString key = it.key();
        QVariant value = it.value();
        
        // Ignorer les paramètres calculés use_atr_for_sl et use_atr_for_tp
        if (key == "use_atr_for_sl" || key == "use_atr_for_tp") {
            continue;
        }
        
        if (m_widgets.contains(key)) {
            QWidget* widget = m_widgets[key];
            
            if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
                combo->setCurrentIndex(value.toInt());
            }
            else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
                spinBox->setValue(value.toDouble());
            }
            else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
                spinBox->setValue(value.toInt());
            }
            else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
                checkBox->setChecked(value.toBool());
            }
            else if (QTimeEdit* timeEdit = qobject_cast<QTimeEdit*>(widget)) {
                QTime time = QTime::fromString(value.toString(), "hh:mm:ss");
                if (time.isValid()) {
                    timeEdit->setTime(time);
                }
            }
        }
    }
    
    // Traiter les jours de trading
    if (values.contains("trading_days")) {
        QVariantList tradingDays = values["trading_days"].toList();
        
        // Réinitialiser tous les jours
        for (int i = 0; i < 7; ++i) {
            QString key = QString("trading_day_%1").arg(i);
            if (m_widgets.contains(key)) {
                static_cast<QCheckBox*>(m_widgets[key])->setChecked(false);
            }
        }
        
        // Activer les jours spécifiés
        for (const QVariant& day : tradingDays) {
            int dayIndex = day.toInt();
            if (dayIndex >= 0 && dayIndex < 7) {
                QString key = QString("trading_day_%1").arg(dayIndex);
                if (m_widgets.contains(key)) {
                    static_cast<QCheckBox*>(m_widgets[key])->setChecked(true);
                }
            }
        }
    }
    
    // Mettre à jour l'état ATR après avoir défini toutes les valeurs
    _updateAtrPeriodStatus();
}

// Implémentation des slots
void StrategyBasePanel::_toggleSlMethod(int index)
{
    bool isFixed = (index == 0);
    bool isAtr = (index == 1);
    bool isMinMax = (index == 2);
    
    if (m_widgets.contains("stop_loss_distance")) {
        m_widgets["stop_loss_distance"]->setEnabled(isFixed);
    }
    if (m_widgets.contains("sl_atr_multiplier")) {
        m_widgets["sl_atr_multiplier"]->setEnabled(isAtr);
    }
    if (m_widgets.contains("sl_minmax_periods")) {
        m_widgets["sl_minmax_periods"]->setEnabled(isMinMax);
    }
    if (m_widgets.contains("sl_minmax_delta")) {
        m_widgets["sl_minmax_delta"]->setEnabled(isMinMax);
    }
    
    // Mettre à jour le statut de la période ATR
    _updateAtrPeriodStatus();
}

void StrategyBasePanel::_toggleTpMethod(int index)
{
    bool isFixed = (index == 0);
    bool isAtr = (index == 1);
    
    if (m_widgets.contains("take_profit_distance")) {
        m_widgets["take_profit_distance"]->setEnabled(isFixed);
    }
    if (m_widgets.contains("tp_atr_multiplier")) {
        m_widgets["tp_atr_multiplier"]->setEnabled(isAtr);
    }
    
    // Mettre à jour le statut de la période ATR
    _updateAtrPeriodStatus();
}

void StrategyBasePanel::_toggleRiskControls(bool checked)
{
    if (m_widgets.contains("risk_percentage")) {
        m_widgets["risk_percentage"]->setEnabled(checked);
    }
}

void StrategyBasePanel::_toggleBreakEvenControls(bool checked)
{
    if (m_widgets.contains("break_even_threshold")) {
        m_widgets["break_even_threshold"]->setEnabled(checked);
    }
}

void StrategyBasePanel::_toggleDailyMaxLossControls(bool checked)
{
    if (m_widgets.contains("daily_max_loss_percentage")) {
        m_widgets["daily_max_loss_percentage"]->setEnabled(checked);
    }
}

void StrategyBasePanel::_toggleDailyMaxProfitControls(bool checked)
{
    if (m_widgets.contains("daily_max_profit_percentage")) {
        m_widgets["daily_max_profit_percentage"]->setEnabled(checked);
    }
}

void StrategyBasePanel::_updateAtrPeriodStatus()
{
    bool atrNeeded = false;
    
    // Vérifier si Stop Loss utilise ATR
    if (m_widgets.contains("sl_method")) {
        QComboBox* slMethod = static_cast<QComboBox*>(m_widgets["sl_method"]);
        atrNeeded |= (slMethod->currentIndex() == 1); // Index 1 = ATR
    }
    
    // Vérifier si Take Profit utilise ATR
    if (m_widgets.contains("tp_method")) {
        QComboBox* tpMethod = static_cast<QComboBox*>(m_widgets["tp_method"]);
        atrNeeded |= (tpMethod->currentIndex() == 1); // Index 1 = ATR
    }
    
    if (m_widgets.contains("atr_period")) {
        m_widgets["atr_period"]->setEnabled(atrNeeded);
    }
}