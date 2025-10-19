#include "ui/panels/StrategyPanel.h"
#include "ui/dialogs/StrategyConfigDialog.h"
#include <QDebug>

StrategyPanel::StrategyPanel(QWidget* parent)
    : ConfigPanel("Paramètres de base", parent)
{
    // Initialiser le tableau des jours de trading (Lun-Ven activés par défaut)
    for (int i = 0; i < 7; ++i) {
        m_config.trading_days_array[i] = (i < 5); // Jours 0-4 (Lun-Ven) activés
    }
    
    setupUI();
}

void StrategyPanel::setupUI() {    
    QVBoxLayout* baseLayout = new QVBoxLayout(this);
    
    // Créer un bouton pour ouvrir le dialog de configuration
    QPushButton* configButton = new QPushButton("Configurer la stratégie", this);
    configButton->setMinimumHeight(50);
    configButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #0078d4;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #005a9e;"
        "}"
    );
    
    connect(configButton, &QPushButton::clicked, this, &StrategyPanel::openStrategyConfigDialog);
    
    baseLayout->addWidget(configButton);
    baseLayout->addStretch();
    
    setLayout(baseLayout);
}

void StrategyPanel::openStrategyConfigDialog() {
    StrategyConfigDialog dialog(this);
    dialog.setConfig(m_config);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_config = dialog.getConfig();
    }
}
