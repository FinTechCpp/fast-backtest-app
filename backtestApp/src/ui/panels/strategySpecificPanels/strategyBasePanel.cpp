#include "ui/panels/strategySpecificPanels/strategyBasePanel.h"
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
    m_widgets["atr_period"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
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
    m_widgets["sl_atr_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    slLayout->addRow(new QLabel("Multiplicateur ATR SL:"), m_widgets["sl_atr_multiplier"]);
    
    // Paramètres Min/Max SL
    m_widgets["sl_minmax_periods"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setRange(1, 1000);
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setValue(5);
    static_cast<QSpinBox*>(m_widgets["sl_minmax_periods"])->setEnabled(false);
    m_widgets["sl_minmax_periods"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    slLayout->addRow(new QLabel("Périodes Min/Max:"), m_widgets["sl_minmax_periods"]);
    
    m_widgets["sl_minmax_delta"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setRange(0.1, 1000.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setValue(5.0);
    static_cast<QDoubleSpinBox*>(m_widgets["sl_minmax_delta"])->setEnabled(false);
    m_widgets["sl_minmax_delta"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
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
    static_cast<QComboBox*>(m_widgets["tp_method"])->addItems({"Fixe", "ATR", "Ratio SL", "SuperTrend", "RL"});
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
    m_widgets["tp_atr_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Multiplicateur ATR TP:"), m_widgets["tp_atr_multiplier"]);
    
    // Paramètres Ratio SL - Multiplicateur TP
    m_widgets["tp_sl_ratio"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["tp_sl_ratio"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_sl_ratio"])->setRange(0.1, 100.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_sl_ratio"])->setSingleStep(0.1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_sl_ratio"])->setValue(2.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_sl_ratio"])->setEnabled(false);
    m_widgets["tp_sl_ratio"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Ratio TP/SL:"), m_widgets["tp_sl_ratio"]);
    
    // Paramètres SuperTrend - Période ATR
    m_widgets["tp_supertrend_atr_period"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["tp_supertrend_atr_period"])->setRange(1, 100);
    static_cast<QSpinBox*>(m_widgets["tp_supertrend_atr_period"])->setValue(14);
    static_cast<QSpinBox*>(m_widgets["tp_supertrend_atr_period"])->setEnabled(false);
    m_widgets["tp_supertrend_atr_period"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Période ATR SuperTrend:"), m_widgets["tp_supertrend_atr_period"]);
    
    // Paramètres SuperTrend - Multiplicateur
    m_widgets["tp_supertrend_multiplier"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["tp_supertrend_multiplier"])->setDecimals(1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_supertrend_multiplier"])->setRange(0.1, 10.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_supertrend_multiplier"])->setSingleStep(0.1);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_supertrend_multiplier"])->setValue(3.0);
    static_cast<QDoubleSpinBox*>(m_widgets["tp_supertrend_multiplier"])->setEnabled(false);
    m_widgets["tp_supertrend_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Multiplicateur SuperTrend:"), m_widgets["tp_supertrend_multiplier"]);
    
    // Paramètres RL - Périodes de lookback
    m_widgets["rl_lookback_periods"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["rl_lookback_periods"])->setRange(1, 150);
    static_cast<QSpinBox*>(m_widgets["rl_lookback_periods"])->setEnabled(false);
    m_widgets["rl_lookback_periods"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    tpLayout->addRow(new QLabel("Périodes lookback RL:"), m_widgets["rl_lookback_periods"]);
    
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
    m_widgets["risk_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
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
    m_widgets["daily_max_loss_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
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
    m_widgets["daily_max_profit_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
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
    m_widgets["break_even_threshold"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Seuil Break Even:"), m_widgets["break_even_threshold"]);

    m_widgets["break_even_offset_per_mille"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_offset_per_mille"])->setDecimals(3);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_offset_per_mille"])->setRange(-5.0, 5.0);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_offset_per_mille"])->setSingleStep(0.05);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_offset_per_mille"])->setValue(0.0);
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_offset_per_mille"])->setSuffix("‰");
    static_cast<QDoubleSpinBox*>(m_widgets["break_even_offset_per_mille"])->setEnabled(false);
    m_widgets["break_even_offset_per_mille"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    riskLayout->addRow(new QLabel("Offset Break Even (‰):"), m_widgets["break_even_offset_per_mille"]);


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
        else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
            values[key] = lineEdit->text();
        }
    }
    
    // Ajouter les paramètres use_atr_for_sl et use_atr_for_tp basés sur les méthodes sélectionnées
    if (m_widgets.contains("sl_method")) {
        QComboBox* slMethod = static_cast<QComboBox*>(m_widgets["sl_method"]);
        values["use_atr_for_sl"] = (slMethod->currentIndex() == 1); // Index 1 = ATR
        values["use_minmax_for_sl"] = (slMethod->currentIndex() == 2); // Index 2 = Min/Max
    }
    
    if (m_widgets.contains("tp_method")) {
        QComboBox* tpMethod = static_cast<QComboBox*>(m_widgets["tp_method"]);
        values["use_atr_for_tp"] = (tpMethod->currentIndex() == 1); // Index 1 = ATR
        values["use_sl_ratio_for_tp"] = (tpMethod->currentIndex() == 2); // Index 2 = Ratio SL
        values["use_supertrend_for_tp"] = (tpMethod->currentIndex() == 3); // Index 3 = SuperTrend
        values["use_rl_for_tp"] = (tpMethod->currentIndex() == 4); // Index 4 = RL
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
        if (key == "use_atr_for_sl" || key == "use_atr_for_tp" || key == "use_minmax_for_sl" || key == "use_sl_ratio_for_tp" || key == "use_supertrend_for_tp" || key == "use_rl_for_tp") {
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
            else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
                lineEdit->setText(value.toString());
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
        if (isFixed)
            m_widgets["stop_loss_distance"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["stop_loss_distance"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    if (m_widgets.contains("sl_atr_multiplier")) {
        m_widgets["sl_atr_multiplier"]->setEnabled(isAtr);
        if (isAtr)
            m_widgets["sl_atr_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["sl_atr_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    if (m_widgets.contains("sl_minmax_periods")) {
        m_widgets["sl_minmax_periods"]->setEnabled(isMinMax);
        if (isMinMax)
            m_widgets["sl_minmax_periods"]->setStyleSheet("QSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["sl_minmax_periods"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    if (m_widgets.contains("sl_minmax_delta")) {
        m_widgets["sl_minmax_delta"]->setEnabled(isMinMax);
        if (isMinMax)
            m_widgets["sl_minmax_delta"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["sl_minmax_delta"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    
    // Mettre à jour le statut de la période ATR
    _updateAtrPeriodStatus();
}

void StrategyBasePanel::_toggleTpMethod(int index)
{
    bool isFixed = (index == 0);
    bool isAtr = (index == 1);
    bool isRatio = (index == 2);
    bool isSupertrend = (index == 3);
    bool isRL = (index == 4);
    
    if (m_widgets.contains("take_profit_distance")) {
        m_widgets["take_profit_distance"]->setEnabled(isFixed);
        if (isFixed)
            m_widgets["take_profit_distance"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["take_profit_distance"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    if (m_widgets.contains("tp_atr_multiplier")) {
        m_widgets["tp_atr_multiplier"]->setEnabled(isAtr);
        if (isAtr)
            m_widgets["tp_atr_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["tp_atr_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    if (m_widgets.contains("tp_sl_ratio")) {
        m_widgets["tp_sl_ratio"]->setEnabled(isRatio);
        if (isRatio)
            m_widgets["tp_sl_ratio"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["tp_sl_ratio"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    if (m_widgets.contains("tp_supertrend_atr_period")) {
        m_widgets["tp_supertrend_atr_period"]->setEnabled(isSupertrend);
        if (isSupertrend)
            m_widgets["tp_supertrend_atr_period"]->setStyleSheet("QSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["tp_supertrend_atr_period"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    if (m_widgets.contains("tp_supertrend_multiplier")) {
        m_widgets["tp_supertrend_multiplier"]->setEnabled(isSupertrend);
        if (isSupertrend)
            m_widgets["tp_supertrend_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["tp_supertrend_multiplier"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    
    // RL parameters
    if (m_widgets.contains("rl_lookback_periods")) {
        m_widgets["rl_lookback_periods"]->setEnabled(isRL);
        if (isRL)
            m_widgets["rl_lookback_periods"]->setStyleSheet("QSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["rl_lookback_periods"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
    
    // Mettre à jour le statut de la période ATR
    _updateAtrPeriodStatus();
}

void StrategyBasePanel::_toggleRiskControls(bool checked)
{
    if (m_widgets.contains("risk_percentage")) {
        m_widgets["risk_percentage"]->setEnabled(checked);
        if (checked)
            m_widgets["risk_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else 
            m_widgets["risk_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
}

void StrategyBasePanel::_toggleBreakEvenControls(bool checked) {
    if (m_widgets.contains("break_even_threshold")) {
        m_widgets["break_even_threshold"]->setEnabled(checked);
        if (checked)
            m_widgets["break_even_threshold"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else
            m_widgets["break_even_threshold"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }

    if (m_widgets.contains("break_even_offset_per_mille")) {
        m_widgets["break_even_offset_per_mille"]->setEnabled(checked);
        if (checked)
            m_widgets["break_even_offset_per_mille"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else
            m_widgets["break_even_offset_per_mille"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
}

void StrategyBasePanel::_toggleDailyMaxLossControls(bool checked)
{
    if (m_widgets.contains("daily_max_loss_percentage")) {
        m_widgets["daily_max_loss_percentage"]->setEnabled(checked);
        if (checked)
            m_widgets["daily_max_loss_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else
            m_widgets["daily_max_loss_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
}

void StrategyBasePanel::_toggleDailyMaxProfitControls(bool checked)
{
    if (m_widgets.contains("daily_max_profit_percentage")) {
        m_widgets["daily_max_profit_percentage"]->setEnabled(checked);
        if (checked)
            m_widgets["daily_max_profit_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #ffffff; color: #000000; }");
        else
            m_widgets["daily_max_profit_percentage"]->setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; color: #888888; }");
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
        if (atrNeeded)
            m_widgets["atr_period"]->setStyleSheet("QSpinBox { background-color: #ffffff; color: #000000; }");
        else
            m_widgets["atr_period"]->setStyleSheet("QSpinBox { background-color: #f0f0f0; color: #888888; }");
    }
}