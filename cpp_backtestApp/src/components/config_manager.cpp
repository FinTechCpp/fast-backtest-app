#include "components/config_manager.h"
#include "app.h"
#include "panels/general_params_panel.h"
#include "panels/strategy_specific_panels/strategy_base_panel.h"
#include <QCoreApplication>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_config(nullptr)
    , m_currentProfile("DEFAULT")
    , m_mainWindow(nullptr)
{
    // Récupérer la référence à MainWindow si le parent est MainWindow
    m_mainWindow = qobject_cast<App*>(parent);
    
    // Déterminer le chemin du fichier de configuration
    m_configFile = getConfigFilePath();
    
    qInfo() << "Initialisation du ConfigManager avec le fichier:" << m_configFile;
    
    // Initialiser la configuration
    initializeConfig();
    setupDefaultValues();
    loadConfig();
    
    qInfo() << "ConfigManager initialisé avec succès";
}

ConfigManager::~ConfigManager()
{
    if (m_config) {
        delete m_config;
    }
}

QString ConfigManager::getConfigFilePath() const
{
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir currentDir(exeDir);
    
    // Remonte dans l'arborescence pour trouver le dossier ig-trading-bot
    QString projectRoot;
    do {
        QString currentPath = currentDir.absolutePath();
        
        // Vérifie si c'est le dossier ig-trading-bot
        if (currentDir.dirName() == "ig-trading-bot") {
            projectRoot = currentPath;
            break;
        }
        
        // Cherche un sous-dossier ig-trading-bot
        QString igTradingBotPath = currentDir.absoluteFilePath("ig-trading-bot");
        if (QFileInfo(igTradingBotPath).isDir()) {
            projectRoot = igTradingBotPath;
            break;
        }
        
    } while (currentDir.cdUp());
    
    QString configDir;
    if (!projectRoot.isEmpty()) {
        configDir = QDir(projectRoot).absoluteFilePath("backtest_app_cpp");
    } else {
        // Fallback vers le répertoire home
        configDir = QDir::homePath() + "/ig-trading-bot-config/backtest_app_cpp";
    }
    
    // Créer le répertoire de configuration s'il n'existe pas
    QDir().mkpath(configDir);
    
    return configDir + "/backtest_config.ini";
}

void ConfigManager::initializeConfig()
{
    m_config = new QSettings(m_configFile, QSettings::IniFormat, this);
    qDebug() << "Fichier de configuration initialisé:" << m_configFile;
}

void ConfigManager::setupDefaultValues()
{
    // Valeurs par défaut pour les paramètres généraux
    m_defaultValues["symbol"] = "NDX";
    m_defaultValues["period"] = "10d";
    m_defaultValues["interval"] = "20secs";
    m_defaultValues["end_date"] = "30/04/2025";  // Format dd/MM/yyyy
    m_defaultValues["spread"] = 0.0001;
    m_defaultValues["cash"] = 100000.0;
    m_defaultValues["strategy"] = "BuyHeikinGreenBA";
    
    // Valeurs par défaut pour les paramètres de stratégie de base
    m_defaultValues["stop_loss_distance"] = 20.0;
    m_defaultValues["take_profit_distance"] = 30.0;
    m_defaultValues["use_atr_for_sl"] = false;
    m_defaultValues["use_atr_for_tp"] = false;
    m_defaultValues["atr_period"] = 14;
    m_defaultValues["stop_loss_atr_multiplier"] = 2.0;
    m_defaultValues["take_profit_atr_multiplier"] = 3.0;
    
    // Paramètres spécifiques BuyHeikinGreen
    m_defaultValues["ema_short_period"] = 150;
    m_defaultValues["ema_long_period"] = 198;
    m_defaultValues["stoch_fastk"] = 10;
    m_defaultValues["stoch_slowk"] = 7;
    m_defaultValues["stoch_slowd"] = 3;
    m_defaultValues["stoch_threshold"] = 20;
    m_defaultValues["use_ema_short_filter"] = false;
    m_defaultValues["use_ema_long_filter"] = false;
    m_defaultValues["use_stoch_filter"] = false;
    
    qDebug() << "Valeurs par défaut configurées";
}

void ConfigManager::loadConfig()
{
    // S'assurer qu'il y a au moins un profil DEFAULT
    if (!m_config->childGroups().contains("DEFAULT") && !m_config->contains("symbol")) {
        qInfo() << "Création du profil DEFAULT avec les valeurs par défaut";
        
        // Sauvegarder les valeurs par défaut dans le profil DEFAULT
        m_config->beginGroup("DEFAULT");
        for (auto it = m_defaultValues.begin(); it != m_defaultValues.end(); ++it) {
            m_config->setValue(it.key(), it.value());
        }
        m_config->endGroup();
        m_config->sync();
    }
    
    qInfo() << "Configuration chargée, profils disponibles:" << listProfiles();
}

void ConfigManager::saveConfig()
{
    if (m_config) {
        m_config->sync();
        qDebug() << "Configuration sauvegardée";
    }
}

QStringList ConfigManager::listProfiles() const
{
    QStringList profiles;
    profiles << "DEFAULT";
    
    if (m_config) {
        QStringList groups = m_config->childGroups();
        for (const QString& group : groups) {
            if (group != "DEFAULT") {
                profiles << group;
            }
        }
    }
    
    return profiles;
}

bool ConfigManager::profileExists(const QString& profileName) const
{
    if (profileName == "DEFAULT") {
        return true;
    }
    
    if (m_config) {
        return m_config->childGroups().contains(profileName);
    }
    
    return false;
}

QMap<QString, QVariant> ConfigManager::getProfile(const QString& profileName) const
{
    QMap<QString, QVariant> profileData;
    
    if (!m_config) {
        return profileData;
    }
    
    QString groupName = (profileName == "DEFAULT") ? "DEFAULT" : profileName;
    
    if (profileName == "DEFAULT" || m_config->childGroups().contains(profileName)) {
        m_config->beginGroup(groupName);
        QStringList keys = m_config->allKeys();
        for (const QString& key : keys) {
            profileData[key] = m_config->value(key);
        }
        m_config->endGroup();
    }
    
    // Compléter avec les valeurs par défaut si nécessaire
    for (auto it = m_defaultValues.begin(); it != m_defaultValues.end(); ++it) {
        if (!profileData.contains(it.key())) {
            profileData[it.key()] = it.value();
        }
    }
    
    return profileData;
}

bool ConfigManager::saveProfile(const QString& profileName, const QMap<QString, QVariant>& profileData)
{
    if (!m_config) {
        return false;
    }
    
    try {
        QString groupName = (profileName == "DEFAULT") ? "DEFAULT" : profileName;
        
        m_config->beginGroup(groupName);
        
        // Sauvegarder toutes les valeurs
        for (auto it = profileData.begin(); it != profileData.end(); ++it) {
            m_config->setValue(it.key(), it.value());
        }
        
        m_config->endGroup();
        m_config->sync();
        
        qInfo() << "Profil" << profileName << "sauvegardé avec succès";
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Erreur lors de la sauvegarde du profil:" << e.what();
        return false;
    }
}

bool ConfigManager::deleteProfile(const QString& profileName)
{
    if (profileName == "DEFAULT" || !m_config) {
        return false;
    }
    
    if (m_config->childGroups().contains(profileName)) {
        m_config->beginGroup(profileName);
        m_config->remove("");  // Supprime tout le groupe
        m_config->endGroup();
        m_config->sync();
        
        qInfo() << "Profil" << profileName << "supprimé";
        return true;
    }
    
    return false;
}

QMap<QString, QVariant> ConfigManager::getProfileFromUI() const
{
    QMap<QString, QVariant> profileData;
    
    if (!m_mainWindow) {
        qWarning() << "Pas de référence à l'application principale";
        return profileData;
    }
    
    // Récupérer les données de configuration
    profileData = m_mainWindow->getStrategyConfig();
    
    qDebug() << "Données récupérées de l'UI:" << profileData.size() << "éléments";
    return profileData;
}

bool ConfigManager::applyProfileToUI(const QString& profileName)
{
    if (!m_mainWindow) {
        qWarning() << "MainWindow non définie";
        return false;
    }
    
    QMap<QString, QVariant> profileData = getProfile(profileName);
    if (profileData.isEmpty()) {
        qWarning() << "Profil" << profileName << "non trouvé ou vide";
        return false;
    }
    
    qDebug() << "Application du profil" << profileName << "à l'UI";
    
    try {
        // Appliquer les valeurs aux panels
        if (m_mainWindow->getGeneralParamsPanel()) {
            m_mainWindow->getGeneralParamsPanel()->setValues(profileData);
        }
        
        if (m_mainWindow->getStrategyBasePanel()) {
            m_mainWindow->getStrategyBasePanel()->setValues(profileData);
        }
        
        if (m_mainWindow->getStrategySpecificPanel()) {
            m_mainWindow->getStrategySpecificPanel()->setValues(profileData);
        }
        
        // SUPPRESSION de l'appel au ProfilePanel (maintenant géré par le menu)
        
        // Mettre à jour le profil actuel
        m_currentProfile = profileName;
        
        // Émettre le signal de changement
        emit profileChanged(profileName);
        
        qInfo() << "Profil" << profileName << "appliqué avec succès";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de l'application du profil:" << e.what();
        return false;
    }
}

bool ConfigManager::saveCurrentProfile(QWidget* parentWidget)
{
    QMap<QString, QVariant> currentConfig = getProfileFromUI();
    bool success = saveProfile(m_currentProfile, currentConfig);
    
    if (success && parentWidget) {
        QMessageBox::information(parentWidget, "Succès", 
                                QString("Profil '%1' sauvegardé.").arg(m_currentProfile));
    } else if (parentWidget) {
        QMessageBox::warning(parentWidget, "Erreur", 
                            QString("Échec de la sauvegarde du profil '%1'.").arg(m_currentProfile));
    }
    
    return success;
}

bool ConfigManager::promptCreateNewProfile(QWidget* parentWidget)
{
    bool ok;
    QString profileName = QInputDialog::getText(parentWidget, 
                                               "Créer un nouveau profil", 
                                               "Nom du profil:",
                                               QLineEdit::Normal,
                                               "", &ok);
    
    if (ok && !profileName.isEmpty() && profileName != "DEFAULT") {
        QMap<QString, QVariant> currentConfig = getProfileFromUI();
        bool success = saveProfile(profileName, currentConfig);
        
        if (success) {
            m_currentProfile = profileName;
            emit profileChanged(profileName);
            emit profileListUpdated();
            
            if (parentWidget) {
                QMessageBox::information(parentWidget, "Succès", 
                                        QString("Profil '%1' créé.").arg(profileName));
            }
        } else if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                QString("Échec de la création du profil '%1'.").arg(profileName));
        }
        
        return success;
    }
    
    return false;
}

bool ConfigManager::deleteCurrentProfile(QWidget* parentWidget)
{
    if (m_currentProfile == "DEFAULT") {
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Information", 
                                    "Le profil DEFAULT ne peut pas être supprimé.");
        }
        return false;
    }
    
    QMessageBox::StandardButton confirm = QMessageBox::question(parentWidget,
                                                               "Confirmation",
                                                               QString("Êtes-vous sûr de vouloir supprimer le profil '%1' ?").arg(m_currentProfile),
                                                               QMessageBox::Yes | QMessageBox::No,
                                                               QMessageBox::No);
    
    if (confirm == QMessageBox::Yes) {
        bool success = deleteProfile(m_currentProfile);
        
        if (success) {
            // Revenir au profil DEFAULT
            m_currentProfile = "DEFAULT";
            applyProfileToUI("DEFAULT");
            emit profileChanged("DEFAULT");
            emit profileListUpdated();
            
            if (parentWidget) {
                QMessageBox::information(parentWidget, "Succès", 
                                        QString("Profil supprimé."));
            }
        } else if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                "Échec de la suppression du profil.");
        }
        
        return success;
    }
    
    return false;
}

bool ConfigManager::importConfigFromFile(QWidget* parentWidget)
{
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
                                                   "Importer une configuration",
                                                   QDir::homePath(),
                                                   "Fichiers de configuration (*.ini)");
    
    if (!fileName.isEmpty()) {
        // TODO: Implémenter l'importation
        QMessageBox::information(parentWidget, "Information", "Fonctionnalité d'import en cours de développement");
    }
    
    return false;
}

bool ConfigManager::exportConfigToFile(QWidget* parentWidget, const QString& profileName)
{
    Q_UNUSED(profileName);
    
    QString fileName = QFileDialog::getSaveFileName(parentWidget,
                                                   "Exporter la configuration",
                                                   QDir::homePath() + "/backtest_config_export.ini",
                                                   "Fichiers de configuration (*.ini)");
    
    if (!fileName.isEmpty()) {
        // TODO: Implémenter l'exportation
        QMessageBox::information(parentWidget, "Information", "Fonctionnalité d'export en cours de développement");
    }
    
    return false;
}

void ConfigManager::updateProfileUI(const QString& selectedProfile)
{
    // Émettre les signaux pour que l'UI se mette à jour
    emit profileChanged(selectedProfile);
}

void ConfigManager::onProfileChanged(const QString& profileName)
{
    if (profileName != m_currentProfile) {
        applyProfileToUI(profileName);
    }
}
