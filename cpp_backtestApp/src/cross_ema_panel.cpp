#include "cross_ema_panel.h"
#include <QDebug>
#include <QMessageBox>

CrossEMAPanel::CrossEMAPanel(QWidget* parent)
    : QObject(parent), BasePanel(parent)
{
}

QGroupBox* CrossEMAPanel::create()
{
    QGroupBox* strategyGroup = new QGroupBox("Paramètres CrossEMA");
    QVBoxLayout* strategyLayout = new QVBoxLayout();
    
    // Configuration des indicateurs EMA
    QGroupBox* emaGroup = new QGroupBox("Configuration des EMA");
    QGridLayout* emaLayout = new QGridLayout();
    
    // EMA court
    emaLayout->addWidget(new QLabel("Période EMA court:"), 0, 0);
    m_widgets["ema_short_period"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["ema_short_period"])->setRange(5, 500);
    static_cast<QSpinBox*>(m_widgets["ema_short_period"])->setSingleStep(1);
    static_cast<QSpinBox*>(m_widgets["ema_short_period"])->setValue(50);  // Valeur par défaut
    static_cast<QSpinBox*>(m_widgets["ema_short_period"])->setToolTip("Période de l'EMA court");
    emaLayout->addWidget(m_widgets["ema_short_period"], 0, 1);
    
    // EMA long
    emaLayout->addWidget(new QLabel("Période EMA long:"), 1, 0);
    m_widgets["ema_long_period"] = new QSpinBox();
    static_cast<QSpinBox*>(m_widgets["ema_long_period"])->setRange(20, 1000);
    static_cast<QSpinBox*>(m_widgets["ema_long_period"])->setSingleStep(5);
    static_cast<QSpinBox*>(m_widgets["ema_long_period"])->setValue(200);  // Valeur par défaut
    static_cast<QSpinBox*>(m_widgets["ema_long_period"])->setToolTip("Période de l'EMA long");
    emaLayout->addWidget(m_widgets["ema_long_period"], 1, 1);
    
    // Ajout d'une validation pour s'assurer que EMA long > EMA court
    connect(static_cast<QSpinBox*>(m_widgets["ema_short_period"]), 
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CrossEMAPanel::validateEmaPeriods);
    connect(static_cast<QSpinBox*>(m_widgets["ema_long_period"]), 
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CrossEMAPanel::validateEmaPeriods);
    
    emaGroup->setLayout(emaLayout);
    strategyLayout->addWidget(emaGroup);
    
    // Section d'aide/information sur la stratégie
    QGroupBox* helpGroup = new QGroupBox("Informations sur la stratégie");
    QVBoxLayout* helpLayout = new QVBoxLayout();
    QLabel* helpText = new QLabel(
        "La stratégie CrossEMA génère un signal d'achat lorsque l'EMA court "
        "croise l'EMA long à la hausse, et un signal de vente lorsque l'EMA court "
        "croise l'EMA long à la baisse.\n\n"
        "Pour un meilleur fonctionnement:\n"
        "- L'EMA court doit être significativement plus petit que l'EMA long\n"
        "- Les périodes typiques sont 50/200 pour les tendances longues\n"
        "- Ou 9/21 pour les mouvements à court terme"
    );
    helpText->setWordWrap(true);
    helpLayout->addWidget(helpText);
    helpGroup->setLayout(helpLayout);
    strategyLayout->addWidget(helpGroup);
    
    strategyGroup->setLayout(strategyLayout);
    return strategyGroup;
}

QMap<QString, QVariant> CrossEMAPanel::getValues()
{
    QMap<QString, QVariant> values;
    
    if (m_widgets.contains("ema_short_period")) {
        values["ema_short_period"] = static_cast<QSpinBox*>(m_widgets["ema_short_period"])->value();
    }
    
    if (m_widgets.contains("ema_long_period")) {
        values["ema_long_period"] = static_cast<QSpinBox*>(m_widgets["ema_long_period"])->value();
    }
    
    qDebug() << "CrossEMAPanel::getValues() returning:" << values;
    return values;
}

void CrossEMAPanel::setValues(const QMap<QString, QVariant>& values)
{
    qDebug() << "CrossEMAPanel::setValues() called with:" << values;
    
    if (values.contains("ema_short_period") && m_widgets.contains("ema_short_period")) {
        bool ok;
        int shortPeriod = values["ema_short_period"].toInt(&ok);
        if (ok) {
            static_cast<QSpinBox*>(m_widgets["ema_short_period"])->setValue(shortPeriod);
        }
    }
    
    if (values.contains("ema_long_period") && m_widgets.contains("ema_long_period")) {
        bool ok;
        int longPeriod = values["ema_long_period"].toInt(&ok);
        if (ok) {
            static_cast<QSpinBox*>(m_widgets["ema_long_period"])->setValue(longPeriod);
        }
    }
}

void CrossEMAPanel::validateEmaPeriods()
{
    if (!m_widgets.contains("ema_short_period") || !m_widgets.contains("ema_long_period")) {
        return;
    }
    
    QSpinBox* shortSpinBox = static_cast<QSpinBox*>(m_widgets["ema_short_period"]);
    QSpinBox* longSpinBox = static_cast<QSpinBox*>(m_widgets["ema_long_period"]);
    
    int shortPeriod = shortSpinBox->value();
    int longPeriod = longSpinBox->value();
    
    // Vérifier que l'EMA long est supérieur à l'EMA court
    if (longPeriod <= shortPeriod) {
        // Ajuster automatiquement l'EMA long pour qu'il soit au moins shortPeriod + 10
        int newLongPeriod = shortPeriod + 10;
        longSpinBox->setValue(newLongPeriod);
        
        // Optionnel: Afficher un message d'information
        qDebug() << "EMA long automatiquement ajusté à" << newLongPeriod 
                 << "pour rester supérieur à l'EMA court (" << shortPeriod << ")";
        
        // Optionnel: Afficher une boîte de dialogue d'information (décommentez si souhaité)
        /*
        QMessageBox::information(
            static_cast<QWidget*>(parent()),
            "Ajustement automatique",
            QString("L'EMA long a été automatiquement ajusté à %1 pour rester supérieur à l'EMA court (%2).")
                .arg(newLongPeriod)
                .arg(shortPeriod)
        );
        */
    }
}