#include "ui/panels/generalParamsPanel.h"
#include <QFormLayout>
#include <QLabel>
#include <QDate>
#include <QDateEdit>
#include <QDebug>

GeneralParamsPanel::GeneralParamsPanel(QWidget* parent)
    : BasePanel("Paramètres généraux", parent)
{
    initStrategyMap();
    setupUI();
    initializeBindings();
}

GeneralParamsConfig GeneralParamsPanel::getConfig() {
    updateConfigFromWidgets();
    return m_config;
}

void GeneralParamsPanel::setConfig(const GeneralParamsConfig &config) {
    m_config = config;
    updateWidgetsFromConfig();
}

void GeneralParamsPanel::addBinding(std::unique_ptr<PropertyBinder> binding) {
    m_bindings.push_back(std::move(binding));
}

void GeneralParamsPanel::updateConfigFromWidgets() {
    for (const auto& binding : m_bindings) {
        binding->updatePropertyFromWidget();
    }
}

void GeneralParamsPanel::updateWidgetsFromConfig() {
    for (const auto& binding : m_bindings) {
        binding->updateWidgetFromProperty();
    }
}

void GeneralParamsPanel::createDependencyGroup(QCheckBox* checkbox, const std::vector<QWidget*>& dependentWidgets) {
    auto updateFunc = [checkbox, dependentWidgets]() {
        bool checked = checkbox->isChecked();
        for (QWidget* widget : dependentWidgets) {
            widget->setEnabled(checked);
            // Mettre à jour le style
            if (checked)
                widget->setStyleSheet("background-color: #ffffff; color: #000000;");
            else
                widget->setStyleSheet("background-color: #f0f0f0; color: #888888;");
        }
    };
    
    // Connecter le signal toggled au callback
    connect(checkbox, &QCheckBox::toggled, this, updateFunc);
    
    // Appliquer l'état initial
    updateFunc();
}

void GeneralParamsPanel::initializeBindings() {

    // Binding pour la stratégie
    auto strategyBinding = PropertyBinderFactory::createStringComboBinding(
        static_cast<QComboBox*>(m_widgets["strategy"]),
        &m_config.strategyName);
    addBinding(std::move(strategyBinding));
    
    // Binding pour le symbole
    auto symbolBinding = PropertyBinderFactory::createStringComboBinding(
        static_cast<QComboBox*>(m_widgets["symbol"]),
        &m_config.symbol);
    addBinding(std::move(symbolBinding));
    
    // Binding pour l'intervalle
    auto intervalBinding = PropertyBinderFactory::createStringComboBinding(
        static_cast<QComboBox*>(m_widgets["interval"]),
        &m_config.interval);
    addBinding(std::move(intervalBinding));
    
    // Binding pour la période
    auto periodBinding = PropertyBinderFactory::createStringComboBinding(
        static_cast<QComboBox*>(m_widgets["period"]),
        &m_config.period);
    addBinding(std::move(periodBinding));

    // Date de fin
    auto endDateBinding = PropertyBinderFactory::createDateTimeBinding(
        static_cast<QDateEdit*>(m_widgets["end_date"]),
        &m_config.endDate);
    addBinding(std::move(endDateBinding));

    // cash
    auto cashBinding = PropertyBinderFactory::createDoubleBinding(
        static_cast<QDoubleSpinBox*>(m_widgets["cash"]),
        &m_config.cash);
    addBinding(std::move(cashBinding));

    // spread
    auto spreadBinding = PropertyBinderFactory::createDoubleBinding(
        static_cast<QDoubleSpinBox*>(m_widgets["spread"]),
        &m_config.spread);
    addBinding(std::move(spreadBinding));

    // commission
    // auto commissionBinding = PropertyBinderFactory::createDoubleBinding(
    //     static_cast<QDoubleSpinBox*>(m_widgets["commission"]),
    //     &m_config.commission);
    // addBinding(std::move(commissionBinding));

    // leverage_limit
    auto leverageBinding = PropertyBinderFactory::createDoubleBinding(
        static_cast<QDoubleSpinBox*>(m_widgets["leverage_limit"]),
        &m_config.leverage_limit);
    addBinding(std::move(leverageBinding));

    // trade on close
    // auto tradeOnCloseBinding = PropertyBinderFactory::createBoolBinding(
    //     static_cast<QCheckBox*>(m_widgets["trade_on_close"]),
    //     &m_config.tradeOnClose);
    // addBinding(std::move(tradeOnCloseBinding));

    // hedging
    // auto hedgingBinding = PropertyBinderFactory::createBoolBinding(
    //     static_cast<QCheckBox*>(m_widgets["hedging"]),
    //     &m_config.hedging);
    // addBinding(std::move(hedgingBinding));

    // exclusive orders
    // auto exclusiveOrdersBinding = PropertyBinderFactory::createBoolBinding(
    //     static_cast<QCheckBox*>(m_widgets["exclusive_orders"]),
    //     &m_config.exclusiveOrders);
    // addBinding(std::move(exclusiveOrdersBinding));

    // finalize trades
    // auto finalizeTradesBinding = PropertyBinderFactory::createBoolBinding(
    //     static_cast<QCheckBox*>(m_widgets["finalize_trades"]),
    //     &m_config.finalizeTrades);
    // addBinding(std::move(finalizeTradesBinding));
}

void GeneralParamsPanel::initStrategyMap()
{
    // Initialiser le dictionnaire des stratégies disponibles
    m_strategyMap["BuyHeikinGreenBA"] = "BuyHeikinGreenBA";
    m_strategyMap["SellHeikinRedBA"] = "SellHeikinRedBA";
}

void GeneralParamsPanel::setupUI()
{
    // Utiliser "this" comme conteneur principal au lieu de créer un nouveau QGroupBox
    QFormLayout* paramsLayout = new QFormLayout(this);
    
    // Stratégie
    m_widgets["strategy"] = new QComboBox(this);
    QComboBox* strategyCombo = static_cast<QComboBox*>(m_widgets["strategy"]);
    strategyCombo->addItems(m_strategyMap.keys());
    paramsLayout->addRow(new QLabel("Stratégie:", this), m_widgets["strategy"]);
    
    // Connecter le signal de changement de stratégie
    connect(strategyCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &GeneralParamsPanel::onStrategyComboChanged);
    

    // Symbole
    m_widgets["symbol"] = new QComboBox(this);
    static_cast<QComboBox*>(m_widgets["symbol"])->addItems({"NDX", "EUR"});
    paramsLayout->addRow(new QLabel("Symbole:", this), m_widgets["symbol"]);
    
    // Période
    m_widgets["period"] = new QComboBox(this);
    static_cast<QComboBox*>(m_widgets["period"])->addItems({"3m", "6m", "1y", "3y", "5y", "10y", "20y"});
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
    dateEdit->setDate(QDate(2025, 7, 17));
    dateEdit->setCalendarPopup(true);
    paramsLayout->addRow(new QLabel("Date de fin:", this), m_widgets["end_date"]);
    
    // Spread (en pour mille)
    m_widgets["spread"] = new QDoubleSpinBox(this);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setDecimals(3);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setRange(0, 10);
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setSingleStep(0.001); 
    static_cast<QDoubleSpinBox*>(m_widgets["spread"])->setValue(0.100);
    paramsLayout->addRow(new QLabel("Spread (‰):", this), m_widgets["spread"]);
    
    // Cash initial
    m_widgets["cash"] = new QDoubleSpinBox(this);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setRange(1000, 10000000);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setSingleStep(1000);
    static_cast<QDoubleSpinBox*>(m_widgets["cash"])->setValue(10000);
    paramsLayout->addRow(new QLabel("Cash initial:", this), m_widgets["cash"]);

    // Levier maximal autorisé
    m_widgets["leverage_limit"] = new QDoubleSpinBox(this);
    static_cast<QDoubleSpinBox*>(m_widgets["leverage_limit"])->setDecimals(2);
    static_cast<QDoubleSpinBox*>(m_widgets["leverage_limit"])->setRange(1, 10000);
    static_cast<QDoubleSpinBox*>(m_widgets["leverage_limit"])->setSingleStep(1);
    static_cast<QDoubleSpinBox*>(m_widgets["leverage_limit"])->setValue(20);
    paramsLayout->addRow(new QLabel("Levier maximal autorisé :", this), m_widgets["leverage_limit"]);
}

// QMap<QString, QVariant> GeneralParamsPanel::getValues()
// {
//     QMap<QString, QVariant> values;
    
//     for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
//         QWidget* widget = it.value();
//         QString key = it.key();
        
//         if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
//             values[key] = combo->currentText();
//         }
//         else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
//             values[key] = spinBox->value();
//         }
//         else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
//             // Retourner la date sous le bon format
//             QDate date = dateEdit->date();
//             QString dateString = date.toString("dd/MM/yyyy");
//             values[key] = dateString;
//         }
//         else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
//             values[key] = lineEdit->text();
//         }
//     }
    
//     // S'assurer que end_date est correctement extraite
//     if (m_widgets.contains("end_date")) {
//         QDateEdit* dateEdit = static_cast<QDateEdit*>(m_widgets["end_date"]);
//         if (dateEdit) {
//             QDate date = dateEdit->date();
//             values["end_date"] = date;
//         }
//     }
    
//     return values;
// }

// void GeneralParamsPanel::setValues(const QMap<QString, QVariant>& values)
// {
//     for (auto it = values.begin(); it != values.end(); ++it) {
//         QString key = it.key();
//         QVariant value = it.value();
        
//         if (m_widgets.contains(key)) {
//             QWidget* widget = m_widgets[key];
            
//             if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
//                 QString text = value.toString();
//                 int index = combo->findText(text);
//                 if (index >= 0) {
//                     combo->setCurrentIndex(index);
//                 }
//             }
//             else if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
//                 spinBox->setValue(value.toDouble());
//             }
//             else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
//                 // Parser correctement la date
//                 if (value.typeId() == QMetaType::QString) {
//                     QString dateStr = value.toString();
//                     QStringList dateFormats = {"dd/MM/yyyy", "yyyy-MM-dd", "dd-MM-yyyy"};
                    
//                     QDate date;
//                     for (const QString& format : dateFormats) {
//                         date = QDate::fromString(dateStr, format);
//                         if (date.isValid()) {
//                             break;
//                         }
//                     }
                    
//                     if (date.isValid()) {
//                         dateEdit->setDate(date);
//                         qDebug() << "Date définie dans le widget:" << key << "=" << date.toString("dd/MM/yyyy");
//                     } else {
//                         qWarning() << "Impossible de parser la date:" << dateStr;
//                     }
//                 }
//                 else if (value.typeId() == QMetaType::QDate) {
//                     dateEdit->setDate(value.toDate());
//                 }
//             }
//             else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
//                 lineEdit->setText(value.toString());
//             }
//         }
//     }
// }

void GeneralParamsPanel::onStrategyComboChanged(const QString& strategy) {
    emit strategyChanged(strategy);
}