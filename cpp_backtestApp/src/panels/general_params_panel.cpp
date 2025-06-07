#include "panels/general_params_panel.h"
#include <QFormLayout>
#include <QLabel>
#include <QDate>
#include <QDateEdit>
#include <QDebug>

GeneralParamsPanel::GeneralParamsPanel(QWidget* parent)
    : BasePanel("Paramètres généraux", parent)
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

void GeneralParamsPanel::initialize()
{
    // Utiliser "this" comme conteneur principal au lieu de créer un nouveau QGroupBox
    QFormLayout* paramsLayout = new QFormLayout(this);
    
    // Stratégie
    m_widgets["strategy"] = new QComboBox(this);
    QComboBox* strategyCombo = static_cast<QComboBox*>(m_widgets["strategy"]);
    strategyCombo->addItems(m_strategyMap.keys());
    paramsLayout->addRow(new QLabel("Stratégie:", this), m_widgets["strategy"]);
    
    // Symbole
    m_widgets["symbol"] = new QComboBox(this);
    static_cast<QComboBox*>(m_widgets["symbol"])->addItems({"NDX", "IBUST100", "EURUSD"});
    paramsLayout->addRow(new QLabel("Symbole:", this), m_widgets["symbol"]);
    
    // Période
    m_widgets["period"] = new QComboBox(this);
    static_cast<QComboBox*>(m_widgets["period"])->addItems({"1m", "2m", "3m", "6m", "1y", "3y"});
    static_cast<QComboBox*>(m_widgets["period"])->setCurrentIndex(5);
    paramsLayout->addRow(new QLabel("Période de données:", this), m_widgets["period"]);
    
    // Intervalle
    m_widgets["interval"] = new QComboBox(this);
    static_cast<QComboBox*>(m_widgets["interval"])->addItems({
        "10secs", "20secs", "30secs", "1min", "2min", "3min", "5min", 
        "10min", "15min", "30min", "1h", "2h", "4h", "1d"
    });
    static_cast<QComboBox*>(m_widgets["interval"])->setCurrentIndex(1);
    paramsLayout->addRow(new QLabel("Intervalle:", this), m_widgets["interval"]);
    
    // Date de fin
    m_widgets["end_date"] = new QDateEdit(this);
    QDateEdit* dateEdit = static_cast<QDateEdit*>(m_widgets["end_date"]);
    dateEdit->setDate(QDate(2025, 5, 30));
    dateEdit->setCalendarPopup(true);
    paramsLayout->addRow(new QLabel("Date de fin:", this), m_widgets["end_date"]);
    
    // Spread
    m_widgets["spread"] = new QDoubleSpinBox(this);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setDecimals(4);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setRange(0, 0.001);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setSingleStep(0.0001);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setValue(0.0001);
    paramsLayout->addRow(new QLabel("Spread:", this), m_widgets["spread"]);
    
    // Cash initial
    m_widgets["cash"] = new QDoubleSpinBox(this);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setRange(1000, 10000000);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setSingleStep(1000);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setValue(100000);
    paramsLayout->addRow(new QLabel("Cash initial:", this), m_widgets["cash"]);
}

QMap<QString, QVariant> GeneralParamsPanel::getValues()
{
    QMap<QString, QVariant> values;
    
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        QWidget* widget = it.value();
        QString key = it.key();
        
        if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
            values[key] = combo->currentText();
        }
        else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
            values[key] = spinBox->value();
        }
        else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
            // Retourner la date sous le bon format
            QDate date = dateEdit->date();
            QString dateString = date.toString("dd/MM/yyyy");
            values[key] = dateString;
            qDebug() << "Date extraite du widget:" << key << "=" << dateString;
        }
        else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
            values[key] = lineEdit->text();
        }
    }
    
    // S'assurer que end_date est correctement extraite
    if (m_widgets.contains("end_date")) {
        QDateEdit* dateEdit = static_cast<QDateEdit*>(m_widgets["end_date"]);
        if (dateEdit) {
            QDate date = dateEdit->date();
            values["end_date"] = date;
            qDebug() << "Date extraite du widget end_date:" << date.toString("dd/MM/yyyy");
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
                QString text = value.toString();
                int index = combo->findText(text);
                if (index >= 0) {
                    combo->setCurrentIndex(index);
                }
            }
            else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
                spinBox->setValue(value.toDouble());
            }
            else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
                // Parser correctement la date
                if (value.typeId() == QMetaType::QString) {
                    QString dateStr = value.toString();
                    QStringList dateFormats = {"dd/MM/yyyy", "yyyy-MM-dd", "dd-MM-yyyy"};
                    
                    QDate date;
                    for (const QString& format : dateFormats) {
                        date = QDate::fromString(dateStr, format);
                        if (date.isValid()) {
                            break;
                        }
                    }
                    
                    if (date.isValid()) {
                        dateEdit->setDate(date);
                        qDebug() << "Date définie dans le widget:" << key << "=" << date.toString("dd/MM/yyyy");
                    } else {
                        qWarning() << "Impossible de parser la date:" << dateStr;
                    }
                }
                else if (value.typeId() == QMetaType::QDate) {
                    dateEdit->setDate(value.toDate());
                }
            }
            else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
                lineEdit->setText(value.toString());
            }
        }
    }
}

QWidget* GeneralParamsPanel::getWidgetByName(const QString& name) const
{
    return m_widgets.value(name, nullptr);
}