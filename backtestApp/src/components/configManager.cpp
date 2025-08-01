#include "components/configManager.h"
#include "ui/app.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/strategySpecificPanels/strategyBasePanel.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>

template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const StrategyBaseConfig& config);
template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const GeneralParamsConfig& config);
template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const BuyHeikinGreenConfig& config);
template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const SellHeikinRedConfig& config);

template<>
StrategyBaseConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map);
template<>
GeneralParamsConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map);
template<>
BuyHeikinGreenConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map);
template<>
SellHeikinRedConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map);

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
    
    QString groupName = profileName;
    
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

// QMap<QString, QVariant> ConfigManager::getProfileFromUI() const
// {
//     QMap<QString, QVariant> profileData;
    
//     if (!m_mainWindow) {
//         qWarning() << "No reference to main application";
//         return profileData;
//     }

//     // Retrieve configuration data
//     profileData = m_mainWindow->getStrategyConfig();

//     qDebug() << "Data retrieved from UI:" << profileData.size() << "elements";
//     return profileData;
// }

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

    GeneralParamsConfig generalConfig = convertQMapToConfig<GeneralParamsConfig>(profileData);
    StrategyBaseConfig baseConfig = convertQMapToConfig<StrategyBaseConfig>(profileData);

    BuyHeikinGreenConfig buyConfig = convertQMapToConfig<BuyHeikinGreenConfig>(profileData);
    SellHeikinRedConfig sellConfig = convertQMapToConfig<SellHeikinRedConfig>(profileData);
    

    m_mainWindow->setGeneralParamsConfig(generalConfig);
    m_mainWindow->setStrategyBaseConfig(baseConfig);

    m_mainWindow->setBuyHeikinGreenConfig(buyConfig);
    m_mainWindow->setSellHeikinRedConfig(sellConfig);


    // Update current profile
    m_currentProfile = profileName;

    // Emit profile changed signal
    emit profileChanged(profileName);

    qInfo() << "Profile" << profileName << "applied successfully";
    return true;
}

bool ConfigManager::saveCurrentProfile(QWidget* parentWidget)
{
    // QMap<QString, QVariant> currentConfig = getProfileFromUI();

    QMap<QString, QVariant> currentConfig = convertConfigToQMap(m_mainWindow->getGeneralParamsConfig());
    currentConfig.insert(convertConfigToQMap(m_mainWindow->getStrategyBaseConfig()));
    currentConfig.insert(convertConfigToQMap(m_mainWindow->getBuyHeikinGreenConfig()));
    currentConfig.insert(convertConfigToQMap(m_mainWindow->getSellHeikinRedConfig()));


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
        // QMap<QString, QVariant> currentConfig = getProfileFromUI();
        QMap<QString, QVariant> currentConfig = convertConfigToQMap(m_mainWindow->getGeneralParamsConfig());
        currentConfig.insert(convertConfigToQMap(m_mainWindow->getStrategyBaseConfig()));
        currentConfig.insert(convertConfigToQMap(m_mainWindow->getBuyHeikinGreenConfig()));
        currentConfig.insert(convertConfigToQMap(m_mainWindow->getSellHeikinRedConfig()));

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
        // profileData = getProfileFromUI();
        profileData = convertConfigToQMap(m_mainWindow->getGeneralParamsConfig());
        profileData.insert(convertConfigToQMap(m_mainWindow->getStrategyBaseConfig()));
        profileData.insert(convertConfigToQMap(m_mainWindow->getBuyHeikinGreenConfig()));
        profileData.insert(convertConfigToQMap(m_mainWindow->getSellHeikinRedConfig()));
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


template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const GeneralParamsConfig& config) {
    QMap<QString, QVariant> map;
    
    // Paramètres généraux
    map["strategy_name"] = config.strategyName;
    map["symbol"] = config.symbol;
    map["interval"] = config.interval;
    map["period"] = config.period;
    map["cash"] = config.cash;
    map["spread"] = config.spread;
    map["commission"] = config.commission;
    map["leverage_limit"] = config.leverage_limit;
    map["trade_on_close"] = config.tradeOnClose;
    map["hedging"] = config.hedging;
    map["exclusive_orders"] = config.exclusiveOrders;
    map["finalize_trades"] = config.finalizeTrades;
    map["end_date"] = config.endDate.toString("dd/MM/yyyy");

    return map;
}

template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const StrategyBaseConfig& config) {
    QMap<QString, QVariant> map;
    
    // Paramètres de logging
    map["enable_logging"] = config.enable_logging;
    
    // Méthodes SL/TP avec les nouveaux enums
    map["sl_method"] = static_cast<int>(config.sl_method);
    map["tp_method"] = static_cast<int>(config.tp_method);
    
    // Pour maintenir la compatibilité avec le code existant pendant la transition
    map["use_atr_for_sl"] = (config.sl_method == StopLossMethod::ATR);
    map["use_minmax_for_sl"] = (config.sl_method == StopLossMethod::MinMax);
    map["use_atr_for_tp"] = (config.tp_method == TakeProfitMethod::ATR);
    map["use_sl_ratio_for_tp"] = (config.tp_method == TakeProfitMethod::SLRatio);
    map["use_supertrend_for_tp"] = (config.tp_method == TakeProfitMethod::SuperTrend);
    map["use_rl_for_tp"] = (config.tp_method == TakeProfitMethod::RL);
    map["use_nth_heikin_ashi_tp"] = (config.tp_method == TakeProfitMethod::NthHeikinAshi);
    
    // Paramètres de temps
    map["trading_from_hour"] = config.trading_from.hour;
    map["trading_from_minute"] = config.trading_from.minute;
    map["trading_to_hour"] = config.trading_to.hour;
    map["trading_to_minute"] = config.trading_to.minute;
    
    // Paramètres SL/TP
    map["stop_loss_distance"] = config.stop_loss_distance;
    map["take_profit_distance"] = config.take_profit_distance;
    map["atr_period"] = config.atr_period;
    map["stop_loss_atr_multiplier"] = config.stop_loss_atr_multiplier;
    map["take_profit_atr_multiplier"] = config.take_profit_atr_multiplier;
    map["min_stop_loss_distance"] = config.min_stop_loss_distance;
    map["min_take_profit_distance"] = config.min_take_profit_distance;
    
    // MinMax SL
    map["sl_minmax_periods"] = config.sl_minmax_periods;
    map["sl_minmax_delta"] = config.sl_minmax_delta;
    
    // TP basé sur SL
    map["tp_sl_ratio"] = config.tp_sl_ratio;
    
    // SuperTrend TP
    map["tp_supertrend_atr_period"] = config.tp_supertrend_atr_period;
    map["tp_supertrend_multiplier"] = config.tp_supertrend_multiplier;
    
    // RL TP
    map["rl_model_path"] = QString::fromStdString(config.rl_model_path);
    map["rl_lookback_periods"] = config.rl_lookback_periods;
    map["rl_tp_max_multiplier"] = config.rl_tp_max_multiplier;
    map["rl_tp_min_multiplier"] = config.rl_tp_min_multiplier;
    
    // Nth Heikin-Ashi TP
    map["nth_heikin_ashi_count"] = config.nth_heikin_ashi_count;
    
    // Paramètres de gestion de risque
    map["use_risk_based_sizing"] = config.use_risk_based_sizing;
    map["risk_percentage"] = config.risk_percentage;
    // map["cash"] = config.cash;
    map["leverage_limit"] = config.leverage_limit;
    
    // Break-even
    map["use_break_even"] = config.use_break_even;
    map["break_even_threshold"] = config.break_even_threshold;
    map["break_even_offset_per_mille"] = config.break_even_offset_per_mille;
    
    // Daily max loss
    map["use_daily_max_loss"] = config.use_daily_max_loss;
    map["daily_max_loss_percentage"] = config.daily_max_loss_percentage;
    map["daily_max_loss_amount"] = config.daily_max_loss_amount;
    
    // Daily max profit
    map["use_daily_max_profit"] = config.use_daily_max_profit;
    map["daily_max_profit_percentage"] = config.daily_max_profit_percentage;
    map["daily_max_profit_amount"] = config.daily_max_profit_amount;
    
    // Daily max drawdown
    map["use_daily_max_drawdown"] = config.use_daily_max_drawdown;
    map["daily_max_drawdown_percentage"] = config.daily_max_drawdown_percentage;
    map["daily_max_drawdown_amount"] = config.daily_max_drawdown_amount;
    
    // Trading days
    QVariantList tradingDays;
    for (int day : config.trading_days) {
        tradingDays.append(day);
    }
    map["trading_days"] = tradingDays;
    
    return map;
}

template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const BuyHeikinGreenConfig& config) {
    QMap<QString, QVariant> map;

    // Paramètres spécifiques à BuyHeikinGreen
    map["bhg_ema_short_period"] = config.ema_short_period;
    map["bhg_ema_long_period"] = config.ema_long_period;
    map["bhg_stoch_fastk"] = config.stoch_fastk;
    map["bhg_stoch_slowk"] = config.stoch_slowk;
    map["bhg_stoch_slowd"] = config.stoch_slowd;
    map["bhg_stoch_threshold"] = config.stoch_threshold;
    map["bhg_rsi_period"] = config.rsi_period;
    map["bhg_rsi_threshold"] = config.rsi_threshold;
    map["bhg_supertrend_atr_period"] = config.supertrend_atr_period;
    map["bhg_supertrend_multiplier"] = config.supertrend_multiplier;
    map["bhg_previous_ha_candle_red_filter_n"] = config.previous_ha_candle_red_filter_n;

    map["bhg_rsi_history_periods"] = config.rsi_history_periods;
    map["bhg_stoch_history_periods"] = config.stoch_history_periods;

    map["bhg_use_ema_short_filter"] = config.use_ema_short_filter;
    map["bhg_use_ema_long_filter"] = config.use_ema_long_filter;
    map["bhg_use_stoch_filter"] = config.use_stoch_filter;
    map["bhg_use_rsi_filter"] = config.use_rsi_filter;
    map["bhg_use_previous_ha_candle_red_filter"] = config.use_previous_ha_candle_red_filter;
    map["bhg_use_supertrend_filter"] = config.use_supertrend_filter;

    return map;
}

template<>
QMap<QString, QVariant> ConfigManager::convertConfigToQMap(const SellHeikinRedConfig& config) {
    QMap<QString, QVariant> map;

    // Paramètres spécifiques à SellHeikinRed
    map["shr_ema_short_period"] = config.ema_short_period;
    map["shr_ema_long_period"] = config.ema_long_period;
    map["shr_stoch_fastk"] = config.stoch_fastk;
    map["shr_stoch_slowk"] = config.stoch_slowk;
    map["shr_stoch_slowd"] = config.stoch_slowd;
    map["shr_stoch_threshold"] = config.stoch_threshold;
    map["shr_rsi_period"] = config.rsi_period;
    map["shr_rsi_threshold"] = config.rsi_threshold;
    map["shr_supertrend_atr_period"] = config.supertrend_atr_period;
    map["shr_supertrend_multiplier"] = config.supertrend_multiplier;
    map["shr_previous_ha_candle_green_filter_n"] = config.previous_ha_candle_green_filter_n;

    map["shr_rsi_history_periods"] = config.rsi_history_periods;
    map["shr_stoch_history_periods"] = config.stoch_history_periods;

    map["shr_use_ema_short_filter"] = config.use_ema_short_filter;
    map["shr_use_ema_long_filter"] = config.use_ema_long_filter;
    map["shr_use_stoch_filter"] = config.use_stoch_filter;
    map["shr_use_rsi_filter"] = config.use_rsi_filter;
    map["shr_use_previous_ha_candle_green_filter"] = config.use_previous_ha_candle_green_filter;
    map["shr_use_supertrend_filter"] = config.use_supertrend_filter;

    return map;
}

template<>
GeneralParamsConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map) {
    GeneralParamsConfig config;

    // Paramètres généraux
    config.strategyName = map.value("strategy_name").toString();
    config.symbol = map.value("symbol").toString();
    config.interval = map.value("interval").toString();
    config.period = map.value("period").toString();
    config.cash = map.value("cash").toDouble();
    config.spread = map.value("spread").toDouble();
    config.commission = map.value("commission").toDouble();
    config.leverage_limit = map.value("leverage_limit").toDouble();
    config.tradeOnClose = map.value("trade_on_close").toBool();
    config.hedging = map.value("hedging").toBool();
    config.exclusiveOrders = map.value("exclusive_orders").toBool();
    config.finalizeTrades = map.value("finalize_trades").toBool();

    // Date de fin
    QString endDateStr = map.value("end_date").toString();
    if (!endDateStr.isEmpty()) {
        QDateTime endDate = QDateTime::fromString(endDateStr, "dd/MM/yyyy");
        if (endDate.isValid()) {
            config.endDate = endDate;
        }
    }

    return config;
}

template<>
StrategyBaseConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map) {
    StrategyBaseConfig config;
    
    // Paramètres de logging
    if (map.contains("enable_logging"))
        config.enable_logging = map["enable_logging"].toBool();
    
    if (map.contains("logLevel"))
        config.logLevel = static_cast<LogLevel>(map["logLevel"].toInt());
    
    // Méthodes SL/TP
    if (map.contains("sl_method") && false) {
        config.sl_method = static_cast<StopLossMethod>(map["sl_method"].toInt());
    } else {
        // Compatibilité avec l'ancien format
        if (map.contains("use_atr_for_sl") && map["use_atr_for_sl"].toBool())
            config.sl_method = StopLossMethod::ATR;
        else if (map.contains("use_minmax_for_sl") && map["use_minmax_for_sl"].toBool())
            config.sl_method = StopLossMethod::MinMax;
        else
            config.sl_method = StopLossMethod::Fixed;
    }
    
    if (map.contains("tp_method") && false) {
        config.tp_method = static_cast<TakeProfitMethod>(map["tp_method"].toInt());
    } else {
        // Compatibilité avec l'ancien format
        if (map.contains("use_atr_for_tp") && map["use_atr_for_tp"].toBool())
            config.tp_method = TakeProfitMethod::ATR;
        else if (map.contains("use_sl_ratio_for_tp") && map["use_sl_ratio_for_tp"].toBool())
            config.tp_method = TakeProfitMethod::SLRatio;
        else if (map.contains("use_supertrend_for_tp") && map["use_supertrend_for_tp"].toBool())
            config.tp_method = TakeProfitMethod::SuperTrend;
        else if (map.contains("use_rl_for_tp") && map["use_rl_for_tp"].toBool())
            config.tp_method = TakeProfitMethod::RL;
        else if (map.contains("use_nth_heikin_ashi_tp") && map["use_nth_heikin_ashi_tp"].toBool())
            config.tp_method = TakeProfitMethod::NthHeikinAshi;
        else
            config.tp_method = TakeProfitMethod::Fixed;
    }
    
    // Pour la compatibilité avec le code qui utilise encore ces flags
    config.use_atr_for_sl = (config.sl_method == StopLossMethod::ATR);
    config.use_minmax_for_sl = (config.sl_method == StopLossMethod::MinMax);
    config.use_atr_for_tp = (config.tp_method == TakeProfitMethod::ATR);
    config.use_sl_ratio_for_tp = (config.tp_method == TakeProfitMethod::SLRatio);
    config.use_supertrend_for_tp = (config.tp_method == TakeProfitMethod::SuperTrend);
    config.use_rl_for_tp = (config.tp_method == TakeProfitMethod::RL);
    config.use_nth_heikin_ashi_tp = (config.tp_method == TakeProfitMethod::NthHeikinAshi);
    
    // Paramètres de temps
    if (map.contains("trading_from")) {
        QString fromStr = map["trading_from"].toString();
        QTime fromTime = QTime::fromString(fromStr, "HH:mm:ss");
        if (fromTime.isValid()) {
            config.trading_from.hour = fromTime.hour();
            config.trading_from.minute = fromTime.minute();
        }
    } else if (map.contains("trading_from_hour") && map.contains("trading_from_minute")) {
        config.trading_from.hour = map["trading_from_hour"].toInt();
        config.trading_from.minute = map["trading_from_minute"].toInt();
    }

    if (map.contains("trading_to")) {
        QString toStr = map["trading_to"].toString();
        QTime toTime = QTime::fromString(toStr, "HH:mm:ss");
        if (toTime.isValid()) {
            config.trading_to.hour = toTime.hour();
            config.trading_to.minute = toTime.minute();
        }
    } else if (map.contains("trading_to_hour") && map.contains("trading_to_minute")) {
        config.trading_to.hour = map["trading_to_hour"].toInt();
        config.trading_to.minute = map["trading_to_minute"].toInt();
    }
    
    // Paramètres SL/TP
    if (map.contains("stop_loss_distance"))
        config.stop_loss_distance = map["stop_loss_distance"].toDouble();
    
    if (map.contains("take_profit_distance"))
        config.take_profit_distance = map["take_profit_distance"].toDouble();
    
    if (map.contains("atr_period"))
        config.atr_period = map["atr_period"].toInt();
    
    if (map.contains("stop_loss_atr_multiplier"))
        config.stop_loss_atr_multiplier = map["stop_loss_atr_multiplier"].toDouble();
    
    if (map.contains("take_profit_atr_multiplier"))
        config.take_profit_atr_multiplier = map["take_profit_atr_multiplier"].toDouble();
    
    if (map.contains("min_stop_loss_distance"))
        config.min_stop_loss_distance = map["min_stop_loss_distance"].toDouble();
    
    if (map.contains("min_take_profit_distance"))
        config.min_take_profit_distance = map["min_take_profit_distance"].toDouble();
    
    // MinMax SL Parameters
    if (map.contains("sl_minmax_periods"))
        config.sl_minmax_periods = map["sl_minmax_periods"].toInt();
    
    if (map.contains("sl_minmax_delta"))
        config.sl_minmax_delta = map["sl_minmax_delta"].toDouble();
    
    // TP based on SL
    if (map.contains("tp_sl_ratio"))
        config.tp_sl_ratio = map["tp_sl_ratio"].toDouble();
    
    // SuperTrend TP parameters
    if (map.contains("tp_supertrend_atr_period"))
        config.tp_supertrend_atr_period = map["tp_supertrend_atr_period"].toInt();
    
    if (map.contains("tp_supertrend_multiplier"))
        config.tp_supertrend_multiplier = map["tp_supertrend_multiplier"].toDouble();
    
    // RL (Reinforcement Learning) TP parameters
    if (map.contains("rl_model_path"))
        config.rl_model_path = map["rl_model_path"].toString().toStdString();
    
    if (map.contains("rl_lookback_periods"))
        config.rl_lookback_periods = map["rl_lookback_periods"].toInt();
    
    if (map.contains("rl_tp_max_multiplier"))
        config.rl_tp_max_multiplier = map["rl_tp_max_multiplier"].toDouble();
    
    if (map.contains("rl_tp_min_multiplier"))
        config.rl_tp_min_multiplier = map["rl_tp_min_multiplier"].toDouble();
    
    // Nth Heikin-Ashi TP
    if (map.contains("nth_heikin_ashi_count"))
        config.nth_heikin_ashi_count = map["nth_heikin_ashi_count"].toInt();
    
    // Risk management
    if (map.contains("use_risk_based_sizing"))
        config.use_risk_based_sizing = map["use_risk_based_sizing"].toBool();
    
    if (map.contains("risk_percentage"))
        config.risk_percentage = map["risk_percentage"].toDouble();
    
    if (map.contains("cash"))
        config.cash = map["cash"].toDouble();
    
    if (map.contains("leverage_limit"))
        config.leverage_limit = map["leverage_limit"].toDouble();
    
    // Break-even parameters
    if (map.contains("use_break_even"))
        config.use_break_even = map["use_break_even"].toBool();
    
    if (map.contains("break_even_threshold"))
        config.break_even_threshold = map["break_even_threshold"].toDouble();
    
    if (map.contains("break_even_offset_per_mille"))
        config.break_even_offset_per_mille = map["break_even_offset_per_mille"].toDouble();
    
    // Daily maximum loss
    if (map.contains("use_daily_max_loss"))
        config.use_daily_max_loss = map["use_daily_max_loss"].toBool();
    
    if (map.contains("daily_max_loss_percentage"))
        config.daily_max_loss_percentage = map["daily_max_loss_percentage"].toDouble();
    
    if (map.contains("daily_max_loss_amount"))
        config.daily_max_loss_amount = map["daily_max_loss_amount"].toDouble();
    
    // Daily maximum profit
    if (map.contains("use_daily_max_profit"))
        config.use_daily_max_profit = map["use_daily_max_profit"].toBool();
    
    if (map.contains("daily_max_profit_percentage"))
        config.daily_max_profit_percentage = map["daily_max_profit_percentage"].toDouble();
    
    if (map.contains("daily_max_profit_amount"))
        config.daily_max_profit_amount = map["daily_max_profit_amount"].toDouble();
    
    // Daily maximum drawdown
    if (map.contains("use_daily_max_drawdown"))
        config.use_daily_max_drawdown = map["use_daily_max_drawdown"].toBool();
    
    if (map.contains("daily_max_drawdown_percentage"))
        config.daily_max_drawdown_percentage = map["daily_max_drawdown_percentage"].toDouble();
    
    if (map.contains("daily_max_drawdown_amount"))
        config.daily_max_drawdown_amount = map["daily_max_drawdown_amount"].toDouble();
    
    // Trading days
    if (map.contains("trading_days")) {
        QVariantList days = map["trading_days"].toList();
        config.trading_days.clear();
        for (const QVariant& day : days) {
            config.trading_days.push_back(day.toInt());
        }
        
        // Également mettre à jour le tableau deprecated pour compatibilité
        for (int i = 0; i < 7 && i < config.trading_days.size(); ++i) {
            config.trading_days_array[i] = config.trading_days[i];
        }
    }
    
    return config;
}

template<>
BuyHeikinGreenConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map) {
    BuyHeikinGreenConfig config;

    // Paramètres spécifiques à BuyHeikinGreen
    config.ema_short_period = map.value("bhg_ema_short_period").toInt();
    config.ema_long_period = map.value("bhg_ema_long_period").toInt();
    config.stoch_fastk = map.value("bhg_stoch_fastk").toInt();
    config.stoch_slowk = map.value("bhg_stoch_slowk").toInt();
    config.stoch_slowd = map.value("bhg_stoch_slowd").toInt();
    config.stoch_threshold = map.value("bhg_stoch_threshold").toInt();
    config.rsi_period = map.value("bhg_rsi_period").toInt();
    config.rsi_threshold = map.value("bhg_rsi_threshold").toInt();
    config.supertrend_atr_period = map.value("bhg_supertrend_atr_period").toInt();
    config.supertrend_multiplier = map.value("bhg_supertrend_multiplier").toDouble();
    config.previous_ha_candle_red_filter_n = map.value("bhg_previous_ha_candle_red_filter_n").toInt();

    // Historique RSI et Stoch
    config.rsi_history_periods = map.value("bhg_rsi_history_periods").toInt();
    config.stoch_history_periods = map.value("bhg_stoch_history_periods").toInt();

    // Filtres
    config.use_ema_short_filter = map.value("bhg_use_ema_short_filter", false).toBool();
    config.use_ema_long_filter = map.value("bhg_use_ema_long_filter", false).toBool();
    config.use_stoch_filter = map.value("bhg_use_stoch_filter", false).toBool();
    config.use_rsi_filter = map.value("bhg_use_rsi_filter", false).toBool();
    config.use_previous_ha_candle_red_filter = map.value("bhg_use_previous_ha_candle_red_filter", false).toBool();
    config.use_supertrend_filter = map.value("bhg_use_supertrend_filter", false).toBool();

    return config;
}

template<>
SellHeikinRedConfig ConfigManager::convertQMapToConfig(const QMap<QString, QVariant>& map) {
    SellHeikinRedConfig config;

    // Paramètres spécifiques à SellHeikinRed
    config.ema_short_period = map.value("shr_ema_short_period").toInt();
    config.ema_long_period = map.value("shr_ema_long_period").toInt();
    config.stoch_fastk = map.value("shr_stoch_fastk").toInt();
    config.stoch_slowk = map.value("shr_stoch_slowk").toInt();
    config.stoch_slowd = map.value("shr_stoch_slowd").toInt();
    config.stoch_threshold = map.value("shr_stoch_threshold").toInt();
    config.rsi_period = map.value("shr_rsi_period").toInt();
    config.rsi_threshold = map.value("shr_rsi_threshold").toInt();
    config.supertrend_atr_period = map.value("shr_supertrend_atr_period").toInt();
    config.supertrend_multiplier = map.value("shr_supertrend_multiplier").toDouble();
    config.previous_ha_candle_green_filter_n = map.value("shr_previous_ha_candle_green_filter_n").toInt();

    // Historique RSI et Stoch
    config.rsi_history_periods = map.value("shr_rsi_history_periods").toInt();
    config.stoch_history_periods = map.value("shr_stoch_history_periods").toInt();

    // Filtres
    config.use_ema_short_filter = map.value("shr_use_ema_short_filter", false).toBool();
    config.use_ema_long_filter = map.value("shr_use_ema_long_filter", false).toBool();
    config.use_stoch_filter = map.value("shr_use_stoch_filter", false).toBool();
    config.use_rsi_filter = map.value("shr_use_rsi_filter", false).toBool();
    config.use_previous_ha_candle_green_filter = map.value("shr_use_previous_ha_candle_green_filter", false).toBool();
    config.use_supertrend_filter = map.value("shr_use_supertrend_filter", false).toBool();

    return config;
}