#include "profile_panel.h"
#include "../config_manager.h"
#include "../app.h"
#include <QFont>
#include <QDebug>

ProfilePanel::ProfilePanel(QWidget* parent)
    : QObject(parent), BasePanel(parent)
{
    // Récupérer le gestionnaire de configuration depuis le parent App
    if (parent) {
        App* mainApp = qobject_cast<App*>(parent->window());
        if (mainApp) {
            m_configManager = mainApp->getConfigManager();
        } else {
            qWarning() << "Impossible de récupérer l'instance App";
            m_configManager = nullptr;
        }
    } else {
        m_configManager = nullptr;
    }
}

QGroupBox* ProfilePanel::create()
{
    // Créer le groupe pour les profils
    QGroupBox* profileGroup = new QGroupBox("Profils de configuration");
    QVBoxLayout* profileLayout = new QVBoxLayout();
    
    // Affichage du profil actif
    QString currentProfile = "DEFAULT";
    if (m_configManager) {
        currentProfile = m_configManager->getCurrentProfile();
    }
    
    m_widgets["current_profile_label"] = new QLabel(QString("Profil actif: %1").arg(currentProfile));
    static_cast<QLabel*>(m_widgets["current_profile_label"])->setStyleSheet("font-weight: bold;");
    profileLayout->addWidget(static_cast<QLabel*>(m_widgets["current_profile_label"]));
    
    // Actions de profil (boutons)
    QHBoxLayout* profileButtonsLayout = new QHBoxLayout();
    
    // Bouton pour sauvegarder le profil actuel
    QPushButton* saveProfileBtn = new QPushButton("💾 Sauvegarder");
    saveProfileBtn->setToolTip("Sauvegarder les paramètres actuels dans le profil courant");
    
    // CORRECTION : Utiliser une connexion directe avec vérification
    connect(saveProfileBtn, &QPushButton::clicked, this, [this]() {
        if (m_configManager && m_parent) {
            App* mainApp = qobject_cast<App*>(m_parent->window());
            if (mainApp) {
                bool success = m_configManager->saveCurrentProfile(mainApp);
                if (success) {
                    updateProfileUI(m_configManager->getCurrentProfile());
                }
            }
        }
    });
    
    profileButtonsLayout->addWidget(saveProfileBtn);
    
    // Bouton pour créer un nouveau profil
    QPushButton* newProfileBtn = new QPushButton("➕ Nouveau");
    newProfileBtn->setToolTip("Créer un nouveau profil avec les paramètres actuels");
    
    connect(newProfileBtn, &QPushButton::clicked, this, [this]() {
        if (m_configManager && m_parent) {
            App* mainApp = qobject_cast<App*>(m_parent->window());
            if (mainApp) {
                bool success = m_configManager->promptCreateNewProfile(mainApp);
                if (success) {
                    // Mettre à jour la liste des profils
                    QComboBox* combo = static_cast<QComboBox*>(m_widgets["profile_combo"]);
                    if (combo) {
                        combo->clear();
                        combo->addItems(m_configManager->listProfiles());
                        combo->setCurrentText(m_configManager->getCurrentProfile());
                    }
                    updateProfileUI(m_configManager->getCurrentProfile());
                }
            }
        }
    });
    
    profileButtonsLayout->addWidget(newProfileBtn);
    
    // Bouton pour supprimer un profil
    QPushButton* deleteProfileBtn = new QPushButton("🗑️ Supprimer");
    deleteProfileBtn->setToolTip("Supprimer le profil actuel");
    
    connect(deleteProfileBtn, &QPushButton::clicked, this, [this]() {
        if (m_configManager && m_parent) {
            App* mainApp = qobject_cast<App*>(m_parent->window());
            if (mainApp) {
                bool success = m_configManager->deleteCurrentProfile(mainApp);
                if (success) {
                    // Mettre à jour la liste des profils
                    QComboBox* combo = static_cast<QComboBox*>(m_widgets["profile_combo"]);
                    if (combo) {
                        combo->clear();
                        combo->addItems(m_configManager->listProfiles());
                        combo->setCurrentText(m_configManager->getCurrentProfile());
                    }
                    updateProfileUI(m_configManager->getCurrentProfile());
                }
            }
        }
    });
    
    profileButtonsLayout->addWidget(deleteProfileBtn);
    
    profileLayout->addLayout(profileButtonsLayout);
    
    // Menu déroulant pour sélectionner un profil
    QHBoxLayout* profileSelectorLayout = new QHBoxLayout();
    profileSelectorLayout->addWidget(new QLabel("Charger un profil:"));
    
    m_widgets["profile_combo"] = new QComboBox();
    if (m_configManager) {
        QComboBox* combo = static_cast<QComboBox*>(m_widgets["profile_combo"]);
        combo->addItems(m_configManager->listProfiles());
        combo->setCurrentText(m_configManager->getCurrentProfile());
        
        // Connecter le signal de changement
        connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                this, &ProfilePanel::loadSelectedProfile);
    }
    profileSelectorLayout->addWidget(static_cast<QComboBox*>(m_widgets["profile_combo"]));
    
    profileLayout->addLayout(profileSelectorLayout);
    
    // Boutons d'import/export
    QHBoxLayout* importExportLayout = new QHBoxLayout();
    
    // Bouton d'importation
    QPushButton* importBtn = new QPushButton("📥 Importer");
    importBtn->setToolTip("Importer une configuration depuis un fichier");
    connect(importBtn, &QPushButton::clicked, this, [this]() {
        if (m_configManager && m_parent) {
            App* mainApp = qobject_cast<App*>(m_parent->window());
            if (mainApp) {
                m_configManager->importConfigFromFile(mainApp);
            }
        }
    });
    importExportLayout->addWidget(importBtn);
    
    // Bouton d'exportation
    QPushButton* exportBtn = new QPushButton("📤 Exporter");
    exportBtn->setToolTip("Exporter la configuration vers un fichier");
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        if (m_configManager && m_parent) {
            App* mainApp = qobject_cast<App*>(m_parent->window());
            if (mainApp) {
                m_configManager->exportConfigToFile(mainApp);
            }
        }
    });
    importExportLayout->addWidget(exportBtn);
    
    profileLayout->addLayout(importExportLayout);
    
    // Finaliser le groupe de profils
    profileGroup->setLayout(profileLayout);
    return profileGroup;
}

QMap<QString, QVariant> ProfilePanel::getValues()
{
    QMap<QString, QVariant> values;
    // Ce panel ne stocke pas de valeurs directement
    return values;
}

void ProfilePanel::setValues(const QMap<QString, QVariant>& values)
{
    Q_UNUSED(values);
    // Ce panel ne définit pas de valeurs directement
}

void ProfilePanel::loadSelectedProfile(const QString& profileName)
{
    if (m_configManager && profileName != m_configManager->getCurrentProfile()) {
        App* mainApp = qobject_cast<App*>(m_parent->window());
        if (mainApp) {
            bool success = m_configManager->applyProfileToUI(profileName);
            if (success) {
                updateProfileUI(profileName);
            } else {
                // En cas d'échec, rétablir le profil précédent dans le combobox
                QComboBox* combo = static_cast<QComboBox*>(m_widgets["profile_combo"]);
                if (combo) {
                    combo->blockSignals(true);
                    combo->setCurrentText(m_configManager->getCurrentProfile());
                    combo->blockSignals(false);
                }
            }
        }
    }
}

void ProfilePanel::updateProfileUI(const QString& profileName)
{
    // Mettre à jour l'étiquette du profil actif
    QLabel* label = static_cast<QLabel*>(m_widgets["current_profile_label"]);
    if (label) {
        label->setText(QString("Profil actif: %1").arg(profileName));
    }
    
    // Mettre à jour la sélection dans le combo
    QComboBox* combo = static_cast<QComboBox*>(m_widgets["profile_combo"]);
    if (combo) {
        combo->blockSignals(true);
        combo->setCurrentText(profileName);
        combo->blockSignals(false);
    }
}