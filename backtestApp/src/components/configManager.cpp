#include "components/configManager.h"
#include "ui/app.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/strategySpecificPanels/strategyBasePanel.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_config(nullptr)
    , m_currentProfile("DEFAULT")
    , m_mainWindow(nullptr)
{
    // Get a reference to MainWindow if the parent is MainWindow
    m_mainWindow = qobject_cast<App*>(parent);

    // Determine the configuration file path
    m_configFile = getConfigFilePath();

    qInfo() << "Initialization of ConfigManager with file:" << m_configFile;

    // Initialize the configuration
    initializeConfig();
    setupDefaultValues();
    loadConfig();

    qInfo() << "ConfigManager successfully initialized";
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
    
    // First, try the original development path logic
    QString projectRoot;
    do {
        QString currentPath = currentDir.absolutePath();
        
        // Check if we are in the project root directory
        if (currentDir.dirName() == "fast-backtest-app") {
            projectRoot = currentPath;
            break;
        }

        // Look for a subdirectory fast-backtest-app
        QString igTradingBotPath = currentDir.absoluteFilePath("fast-backtest-app");
        if (QFileInfo(igTradingBotPath).isDir()) {
            projectRoot = igTradingBotPath;
            break;
        }
        
    } while (currentDir.cdUp());
    
    QString configDir;
    QString configFile;
    
    // Development environment: project structure found
    if (!projectRoot.isEmpty()) {
        configDir = QDir(projectRoot).absoluteFilePath("backtestApp");
        configFile = configDir + "/backtest_config.ini";
        
        // Check if we can write to this directory
        QDir().mkpath(configDir);
        QFileInfo dirInfo(configDir);
        
        if (dirInfo.isWritable()) {
            qDebug() << "Using development config path:" << configFile;
            return configFile;
        } else {
            qDebug() << "Development path not writable, falling back to user data directory";
        }
    }
    
    // Fallback for distributed applications or when development path is not writable
    QString userDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    configDir = userDataDir;
    configFile = QDir(configDir).absoluteFilePath("backtest_config.ini");
    
    // Create the user data directory if it doesn't exist
    QDir().mkpath(configDir);
    
    // Try to migrate existing config from development location if it exists
    if (!projectRoot.isEmpty()) {
        QString devConfigPath = QDir(projectRoot).absoluteFilePath("backtestApp/backtest_config.ini");
        if (QFile::exists(devConfigPath) && !QFile::exists(configFile)) {
            if (QFile::copy(devConfigPath, configFile)) {
                qInfo() << "Migrated config from development location:" << devConfigPath << "to user data:" << configFile;
            }
        }
    }
    
    // Also try to migrate from home directory fallback (old behavior)
    QString oldHomePath = QDir::homePath() + "/fast-backtest-app-config/backtestApp/backtest_config.ini";
    if (QFile::exists(oldHomePath) && !QFile::exists(configFile)) {
        if (QFile::copy(oldHomePath, configFile)) {
            qInfo() << "Migrated config from home directory:" << oldHomePath << "to user data:" << configFile;
        }
    }
    
    qDebug() << "Using user data config path:" << configFile;
    return configFile;
}

void ConfigManager::initializeConfig()
{
    m_config = new QSettings(m_configFile, QSettings::IniFormat, this);
    qDebug() << "Configuration file initialized:" << m_configFile;
}

void ConfigManager::setupDefaultValues()
{
    // Default values for general settings
    m_defaultValues["symbol"];
    m_defaultValues["period"];
    m_defaultValues["interval"];
    m_defaultValues["end_date"];  // Format dd/MM/yyyy
    m_defaultValues["spread"];
    m_defaultValues["cash"];
    m_defaultValues["strategy"];
    m_defaultValues["leverage_limit"];

    // Default values for base strategy parameters
    m_defaultValues["stop_loss_distance"];
    m_defaultValues["take_profit_distance"];
    m_defaultValues["use_atr_for_sl"] = false;
    m_defaultValues["use_atr_for_tp"] = false;
    m_defaultValues["atr_period"];
    m_defaultValues["stop_loss_atr_multiplier"];
    m_defaultValues["take_profit_atr_multiplier"];
    
    // RL parameters for TP
    m_defaultValues["use_rl_for_tp"] = false;
    m_defaultValues["rl_model_path"] = "./Models/tp_model.onnx";
    m_defaultValues["rl_lookback_periods"];
    m_defaultValues["rl_tp_min_multiplier"];
    m_defaultValues["rl_tp_max_multiplier"];

    // Specific parameters for BuyHeikinGreen
    m_defaultValues["ema_short_period"];
    m_defaultValues["ema_long_period"];
    m_defaultValues["stoch_fastk"];
    m_defaultValues["stoch_slowk"];
    m_defaultValues["stoch_slowd"];
    m_defaultValues["stoch_threshold"];
    m_defaultValues["use_rsi_filter"] = false;
    m_defaultValues["use_ema_short_filter"] = false;
    m_defaultValues["use_ema_long_filter"] = false;
    m_defaultValues["use_stoch_filter"] = false;

    qDebug() << "Default values set";
}

void ConfigManager::loadConfig()
{
    // Ensure there is at least one DEFAULT profile
    if (!m_config->childGroups().contains("DEFAULT") && !m_config->contains("symbol")) {
        qInfo() << "Creating DEFAULT profile with default values";

        // Save default values to DEFAULT profile
        m_config->beginGroup("DEFAULT");
        for (auto it = m_defaultValues.begin(); it != m_defaultValues.end(); ++it) {
            m_config->setValue(it.key(), it.value());
        }
        m_config->endGroup();
        m_config->sync();
    }

    qInfo() << "Configuration loaded, available profiles:" << listProfiles();

    // Emit signal to update UI
    emit profileListUpdated();
}

void ConfigManager::saveConfig()
{
    if (m_config) {
        m_config->sync();
        qDebug() << "Configuration saved";
    }
}

QStringList ConfigManager::listProfiles() const
{
    QStringList profiles;
    profiles << "DEFAULT";
    
    if (m_config) {
        QStringList groups = m_config->childGroups();
        qDebug() << "Groups found in config file:" << groups;
        for (const QString& group : groups) {
            if (group != "DEFAULT") {
                profiles << group;
            }
        }
    }

    qDebug() << "Profiles listed:" << profiles;
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

    // Complete with default values if necessary
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

        // Save all values
        for (auto it = profileData.begin(); it != profileData.end(); ++it) {
            m_config->setValue(it.key(), it.value());
        }
        
        m_config->endGroup();
        m_config->sync();

        qInfo() << "Profile" << profileName << "saved successfully";
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Error saving profile:" << e.what();
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
        m_config->remove("");  // Remove the entire group
        m_config->endGroup();
        m_config->sync();

        qInfo() << "Profile" << profileName << "deleted";
        return true;
    }
    
    return false;
}

QMap<QString, QVariant> ConfigManager::getProfileFromUI() const
{
    QMap<QString, QVariant> profileData;
    
    if (!m_mainWindow) {
        qWarning() << "No reference to main application";
        return profileData;
    }

    // Retrieve configuration data
    profileData = m_mainWindow->getStrategyConfig();

    qDebug() << "Data retrieved from UI:" << profileData.size() << "elements";
    return profileData;
}

bool ConfigManager::applyProfileToUI(const QString& profileName)
{
    if (!m_mainWindow) {
        qWarning() << "MainWindow not defined";
        return false;
    }
    
    QMap<QString, QVariant> profileData = getProfile(profileName);
    if (profileData.isEmpty()) {
        qWarning() << "Profile" << profileName << "not found or empty";
        return false;
    }

    qDebug() << "Applying profile" << profileName << "to UI";

    try {
        // Apply values to panels
        if (m_mainWindow->getGeneralParamsPanel()) {
            m_mainWindow->getGeneralParamsPanel()->setValues(profileData);
        }
        
        if (m_mainWindow->getStrategyBasePanel()) {
            m_mainWindow->getStrategyBasePanel()->setValues(profileData);
        }
        
        if (m_mainWindow->getStrategySpecificPanel()) {
            m_mainWindow->getStrategySpecificPanel()->setValues(profileData);
        }

        // Update current profile
        m_currentProfile = profileName;

        // Emit profile changed signal
        emit profileChanged(profileName);

        qInfo() << "Profile" << profileName << "applied successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "Error applying profile:" << e.what();
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
            // Revert to DEFAULT profile
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
    
    if (fileName.isEmpty()) {
        return false; // User canceled
    }
    
    if (!QFile::exists(fileName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Error", "The selected file does not exist.");
        }
        return false;
    }
    
    try {
        // Read import file
        QSettings importSettings(fileName, QSettings::IniFormat);

        // Check file status
        if (importSettings.status() != QSettings::NoError) {
            if (parentWidget) {
                QMessageBox::critical(parentWidget, "Erreur d'importation", 
                                    "Impossible de lire le fichier de configuration.");
            }
            return false;
        }

        // Read metadata if available
        QString originalProfile = "Imported";
        QString exportVersion = "Unknown";
        
        if (importSettings.childGroups().contains("ExportInfo")) {
            importSettings.beginGroup("ExportInfo");
            originalProfile = importSettings.value("exported_from_profile", "Imported").toString();
            exportVersion = importSettings.value("export_format_version", "Unknown").toString();
            QDateTime exportedAt = QDateTime::fromString(importSettings.value("exported_at").toString(), Qt::ISODate);
            importSettings.endGroup();

            qInfo() << "Import configuration - Original profile:" << originalProfile << "Version:" << exportVersion;
        }

        // Read configuration data
        QMap<QString, QVariant> importedData;
        
        if (importSettings.childGroups().contains("ProfileData")) {
            // Modern format with ProfileData group
            importSettings.beginGroup("ProfileData");
            QStringList keys = importSettings.allKeys();
            for (const QString& key : keys) {
                importedData[key] = importSettings.value(key);
            }
            importSettings.endGroup();
        } else {
            // Legacy format - read all keys directly
            QStringList allKeys = importSettings.allKeys();
            for (const QString& key : allKeys) {
                importedData[key] = importSettings.value(key);
            }
        }
        
        if (importedData.isEmpty()) {
            if (parentWidget) {
                QMessageBox::warning(parentWidget, "Erreur d'importation", 
                                    "Aucune donnée de configuration trouvée dans le fichier.");
            }
            return false;
        }
        
        // Ask the name of the new profile or if to overwrite the current profile
        QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
                                                                   "Mode d'importation",
                                                                   QString("Voulez-vous:\n\n"
                                                                          "• Créer un nouveau profil avec ces données ?\n"
                                                                          "• Ou remplacer la configuration actuelle du profil '%1' ?").arg(m_currentProfile),
                                                                   QMessageBox::Save | QMessageBox::Apply | QMessageBox::Cancel,
                                                                   QMessageBox::Save);
        
        if (choice == QMessageBox::Cancel) {
            return false;
        }
        
        QString targetProfile = m_currentProfile;
        bool success = false;
        
        if (choice == QMessageBox::Save) {
            // Create a new profile
            bool ok;
            QString newProfileName = QInputDialog::getText(parentWidget, 
                                                          "Nom du nouveau profil", 
                                                          QString("Nom du profil (original: %1):").arg(originalProfile),
                                                          QLineEdit::Normal,
                                                          originalProfile, &ok);
            
            if (ok && !newProfileName.isEmpty() && newProfileName != "DEFAULT") {
                targetProfile = newProfileName;
                success = saveProfile(targetProfile, importedData);
                
                if (success) {
                    // Switch to the new profile
                    m_currentProfile = targetProfile;
                    applyProfileToUI(targetProfile);
                    emit profileChanged(targetProfile);
                    emit profileListUpdated();
                }
            }
        } else if (choice == QMessageBox::Apply) {
            // Replace the current profile
            success = saveProfile(targetProfile, importedData);
            
            if (success) {
                // Reload configuration into UI
                applyProfileToUI(targetProfile);
                emit profileChanged(targetProfile);
            }
        }
        
        if (success) {
            if (parentWidget) {
                QMessageBox::information(parentWidget, "Import réussi", 
                                        QString("Configuration imported successfully into profile '%1'.").arg(targetProfile));
            }
            qInfo() << "Configuration imported successfully from:" << fileName << "to profile:" << targetProfile;
        } else {
            if (parentWidget) {
                QMessageBox::warning(parentWidget, "Import Error", 
                                    "Failed to import configuration.");
            }
        }
        
        return success;
        
    } catch (const std::exception& e) {
        qCritical() << "Error importing configuration:" << e.what();
        if (parentWidget) {
            QMessageBox::critical(parentWidget, "Erreur d'importation", 
                                QString("Erreur lors de l'importation: %1").arg(e.what()));
        }
        return false;
    }
}

bool ConfigManager::exportConfigToFile(QWidget* parentWidget, const QString& profileName)
{
    
    QString targetProfile = profileName.isEmpty() ? m_currentProfile : profileName;

    // Get data from the profile to export
    QMap<QString, QVariant> profileData;
    if (targetProfile == m_currentProfile) {
        // If it's the current profile, retrieve values from the UI
        profileData = getProfileFromUI();
    } else {
        // Otherwise, retrieve from the saved configuration
        profileData = getProfile(targetProfile);
    }
    
    if (profileData.isEmpty()) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                QString("Impossible de récupérer les données du profil '%1'.").arg(targetProfile));
        }
        return false;
    }

    // Default file name with profile name and timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString defaultFileName = QString("backtest_config_%1_%2.ini").arg(targetProfile).arg(timestamp);
    
    QString fileName = QFileDialog::getSaveFileName(parentWidget,
                                                   QString("Exporter la configuration - Profil: %1").arg(targetProfile),
                                                   QDir::homePath() + "/" + defaultFileName,
                                                   "Fichiers de configuration (*.ini)");
    
    if (fileName.isEmpty()) {
        return false; // User canceled
    }
    
    try {
        // Create a temporary QSettings object for export
        QSettings exportSettings(fileName, QSettings::IniFormat);
        
        // Write export metadata
        exportSettings.beginGroup("ExportInfo");
        exportSettings.setValue("exported_at", QDateTime::currentDateTime().toString(Qt::ISODate));
        exportSettings.setValue("exported_from_profile", targetProfile);
        exportSettings.setValue("application_version", QCoreApplication::applicationVersion());
        exportSettings.setValue("export_format_version", "1.0");
        exportSettings.endGroup();

        // Export profile data
        exportSettings.beginGroup("ProfileData");
        for (auto it = profileData.begin(); it != profileData.end(); ++it) {
            exportSettings.setValue(it.key(), it.value());
        }
        exportSettings.endGroup();

        // Ensure everything is written
        exportSettings.sync();

        // Check that the file was created successfully
        if (exportSettings.status() != QSettings::NoError) {
            if (parentWidget) {
                QMessageBox::critical(parentWidget, "Erreur d'exportation", 
                                    "Erreur lors de l'écriture du fichier de configuration.");
            }
            return false;
        }
        
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Export réussi", 
                                    QString("Configuration du profil '%1' exportée vers:\n%2").arg(targetProfile).arg(fileName));
        }

        qInfo() << "Configuration successfully exported to:" << fileName;
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "Error exporting configuration:" << e.what();
        if (parentWidget) {
            QMessageBox::critical(parentWidget, "Erreur d'exportation", 
                                QString("Erreur lors de l'exportation: %1").arg(e.what()));
        }
        return false;
    }
}

void ConfigManager::updateProfileUI(const QString& selectedProfile)
{
    // Emit signals to update the UI
    emit profileChanged(selectedProfile);
}

void ConfigManager::onProfileChanged(const QString& profileName)
{
    if (profileName != m_currentProfile) {
        applyProfileToUI(profileName);
    }
}
