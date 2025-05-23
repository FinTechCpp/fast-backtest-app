#include "general_params_panel.h"
#include <QFormLayout>
#include <QLabel>
#include <QDate>
#include <QDateEdit>  // AJOUT MANQUANT
#include <QDebug>

GeneralParamsPanel::GeneralParamsPanel(QWidget* parent)
    : QObject(parent), BasePanel(parent)  // CORRECTION: Hériter de QObject
{
    initStrategyMap();
}

void GeneralParamsPanel::initStrategyMap()
{
    // Initialiser le dictionnaire des stratégies disponibles
    m_strategyMap["BuyHeikinGreenBA"] = "BuyHeikinGreenBA";
    m_strategyMap["SellHeikinRedBA"] = "SellHeikinRedBA";
    m_strategyMap["BuyTrendFollowingBA"] = "BuyTrendFollowingBA";
    m_strategyMap["SellTrendFollowingBA"] = "SellTrendFollowingBA";
    m_strategyMap["CrossEMABA"] = "CrossEMABA";
}

QGroupBox* GeneralParamsPanel::create()
{
    QGroupBox* paramsGroup = new QGroupBox("Paramètres généraux");
    QFormLayout* paramsLayout = new QFormLayout();
    
    // Stratégie
    m_widgets["strategy"] = new QComboBox();
    QComboBox* strategyCombo = static_cast<QComboBox*>(m_widgets["strategy"]);
    strategyCombo->addItems(m_strategyMap.keys());
    paramsLayout->addRow(new QLabel("Stratégie:"), m_widgets["strategy"]);
    
    // Symbole
    m_widgets["symbol"] = new QComboBox();
    static_cast<QComboBox*>(m_widgets["symbol"])->addItems({"NDX", "IBUST100", "EURUSD"});
    paramsLayout->addRow(new QLabel("Symbole:"), m_widgets["symbol"]);
    
    // Période
    m_widgets["period"] = new QComboBox();
    static_cast<QComboBox*>(m_widgets["period"])->addItems({"5d", "10d", "30d", "1m", "2m", "3m", "6m", "1y", "2y"});
    static_cast<QComboBox*>(m_widgets["period"])->setCurrentIndex(1);
    paramsLayout->addRow(new QLabel("Période de données:"), m_widgets["period"]);
    
    // Intervalle
    m_widgets["interval"] = new QComboBox();
    static_cast<QComboBox*>(m_widgets["interval"])->addItems({
        "10secs", "20secs", "30secs", "1min", "2min", "3min", "5min", 
        "10min", "15min", "30min", "1h", "2h", "4h", "1d"
    });
    static_cast<QComboBox*>(m_widgets["interval"])->setCurrentIndex(1);
    paramsLayout->addRow(new QLabel("Intervalle:"), m_widgets["interval"]);
    
    // Date de fin - CORRECTION: Utiliser QDateEdit correctement
    m_widgets["end_date"] = new QDateEdit();
    QDateEdit* dateEdit = static_cast<QDateEdit*>(m_widgets["end_date"]);
    dateEdit->setDate(QDate(2025, 4, 30));
    dateEdit->setCalendarPopup(true);
    paramsLayout->addRow(new QLabel("Date de fin:"), m_widgets["end_date"]);
    
    // Spread
    m_widgets["spread"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setDecimals(4);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setRange(0, 0.001);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setSingleStep(0.0001);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setValue(0.0001);
    paramsLayout->addRow(new QLabel("Spread:"), m_widgets["spread"]);
    
    // Cash initial
    m_widgets["cash"] = new QDoubleSpinBox();
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setRange(1000, 10000000);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setSingleStep(1000);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setValue(100000);
    paramsLayout->addRow(new QLabel("Cash initial:"), m_widgets["cash"]);
    
    paramsGroup->setLayout(paramsLayout);
    return paramsGroup;
}

QMap<QString, QVariant> GeneralParamsPanel::getValues()
{
    QMap<QString, QVariant> values;
    
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        QString key = it.key();
        QWidget* widget = it.value();
        
        if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
            values[key] = combo->currentText();
        } else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
            values[key] = spinBox->value();
        } else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
            values[key] = dateEdit->date().toString("dd/MM/yyyy");
        }
    }
    
    return values;
}

void GeneralParamsPanel::setValues(const QMap<QString, QVariant>& values)
{
    for (auto it = values.begin(); it != values.end(); ++it) {
        QString key = it.key();
        QVariant value = it.value();
        
        if (m_widgets.contains(key)) {
            QWidget* widget = m_widgets[key];
            
            if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
                QString textValue = value.toString();
                int index = combo->findText(textValue);
                if (index >= 0) {
                    combo->setCurrentIndex(index);
                }
            } else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
                spinBox->setValue(value.toDouble());
            } else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
                QDate date = QDate::fromString(value.toString(), "dd/MM/yyyy");
                if (date.isValid()) {
                    dateEdit->setDate(date);
                }
            }
        }
    }
}

QWidget* GeneralParamsPanel::getWidgetByName(const QString& name) const
{
    return m_widgets.value(name, nullptr);
}