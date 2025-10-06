#include "components/Managers/ProfileManager.h"
#include "ui/app.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>

ProfileManager::ProfileManager(QObject *parent)
    : QObject(parent)
    , m_currentProfile("DEFAULT")
    , m_mainWindow(nullptr)
{
    // Get a reference to MainWindow if the parent is MainWindow
    m_mainWindow = qobject_cast<App*>(parent);

    // Setup profile directory
    initializeDirectories();
    
    // Ensure DEFAULT profile exists
    ensureDefaultProfileExists();
    
    qInfo() << "ProfileManager successfully initialized with profile directory:" << m_configDir;
}

ProfileManager::~ProfileManager()
{
    // Clean-up if needed
}

void ProfileManager::initializeDirectories()
{
    // Use standard locations for application data
    m_configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
    // m_configDir = QCoreApplication::applicationDirPath() + "/profiles";

    // Create directory if it doesn't exist
    QDir dir;
    if (!dir.exists(m_configDir)) {
        dir.mkpath(m_configDir);
        qInfo() << "Created profiles directory:" << m_configDir;
    }
}

QStringList ProfileManager::listProfiles() const
{
    QStringList profiles;
    
    // Scan directory for JSON profile files
    QDir dir(m_configDir);
    dir.setNameFilters(QStringList() << "*.json");
    dir.setFilter(QDir::Files);
    
    foreach (QString file, dir.entryList()) {
        // Remove .json extension from filename
        QString profileName = file;
        profileName.chop(5);  // Remove ".json"
        profiles << profileName;
    }
    
    // Always ensure DEFAULT is in the list (first)
    if (!profiles.contains("DEFAULT")) {
        profiles.prepend("DEFAULT");
    }
    
    qDebug() << "Profiles found:" << profiles;
    return profiles;
}

bool ProfileManager::saveProfile(const QString& profileName, const ProfileConfig& config)
{
    QString profilePath = getProfilePath(profileName);
    return SerializationUtils::saveToJsonFile(profilePath, config);
}

bool ProfileManager::loadProfileFromJson(const QString& profileName, ProfileConfig& config)
{
    QString profilePath = getProfilePath(profileName);
    
    if (!QFile::exists(profilePath)) {
        if (profileName == "DEFAULT") {
            // Create and save default profile
            config = createDefaultProfile();
            saveProfile("DEFAULT", config);
            return true;
        }
        return false;
    }
    
    return SerializationUtils::loadFromJsonFile(profilePath, config);
}

QString ProfileManager::getProfilePath(const QString& profileName) const
{
    return m_configDir + "/" + profileName + ".json";
}

bool ProfileManager::profileExists(const QString& profileName) const
{
    QString profilePath = getProfilePath(profileName);
    return QFile::exists(profilePath);
}

bool ProfileManager::deleteProfile(const QString& profileName)
{
    if (profileName == "DEFAULT") {
        return false;  // Cannot delete DEFAULT profile
    }
    
    QString profilePath = getProfilePath(profileName);
    return QFile::remove(profilePath);
}

ProfileConfig ProfileManager::createDefaultProfile() const
{
    ProfileConfig config;
    config.name = "DEFAULT";
    config.version = "1.0";
    config.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    
    return config;
}

bool ProfileManager::openProfilesDirectory() const
{
    QDir dir(m_configDir);
    if (!dir.exists()) {
        qWarning() << "Profiles directory does not exist:" << m_configDir;
        return false;
    }
    
    bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(m_configDir));
    if (success) {
        qInfo() << "Opened profiles directory:" << m_configDir;
    } else {
        qWarning() << "Failed to open profiles directory:" << m_configDir;
    }
    
    return success;
}

void ProfileManager::ensureDefaultProfileExists()
{
    if (!profileExists("DEFAULT")) {
        ProfileConfig defaultConfig = createDefaultProfile();
        saveProfile("DEFAULT", defaultConfig);
        qInfo() << "Created DEFAULT profile";
    }
}

bool ProfileManager::applyProfileToUI(const QString& profileName)
{
    if (!m_mainWindow) {
        qWarning() << "MainWindow not defined";
        return false;
    }
    
    ProfileConfig profileConfig;
    bool success = loadProfileFromJson(profileName, profileConfig);
    
    if (success) {
        // Apply configurations to UI
        m_mainWindow->setGeneralParamsConfig(profileConfig.generalParams);
        m_mainWindow->setStrategyConfig(profileConfig.strategyConfig);

        
        m_currentProfile = profileName;
        emit profileChanged(profileName);
        
        qInfo() << "Successfully applied profile:" << profileName;
    } else {
        qWarning() << "Failed to load profile:" << profileName;
    }
    
    return success;
}

bool ProfileManager::saveCurrentProfile(QWidget* parentWidget)
{
    if (!m_mainWindow) {
        qWarning() << "MainWindow not defined";
        return false;
    }
    
    ProfileConfig profileConfig;
    profileConfig.name = m_currentProfile.toStdString();
    profileConfig.version = "1.0";
    profileConfig.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    
    // Get configurations from UI
    profileConfig.generalParams = m_mainWindow->getGeneralParamsConfig();
    profileConfig.strategyConfig = m_mainWindow->getStrategyConfig();

    bool success = saveProfile(m_currentProfile, profileConfig);
    
    if (success && parentWidget) {
        QMessageBox::information(parentWidget, "Succès", 
                                QString("Profil '%1' sauvegardé.").arg(m_currentProfile));
    } else if (parentWidget && !success) {
        QMessageBox::warning(parentWidget, "Erreur", 
                            QString("Échec de la sauvegarde du profil '%1'.").arg(m_currentProfile));
    }
    
    return success;
}

bool ProfileManager::promptCreateNewProfile(QWidget* parentWidget)
{
    bool ok;
    QString profileName = QInputDialog::getText(parentWidget, 
                                               "Créer un nouveau profil", 
                                               "Nom du profil:",
                                               QLineEdit::Normal,
                                               "", &ok);
    
    if (ok && !profileName.isEmpty() && profileName != "DEFAULT") {
        if (profileExists(profileName)) {
            QMessageBox::warning(parentWidget, "Profil existant", 
                                QString("Un profil nommé '%1' existe déjà.").arg(profileName));
            return false;
        }
        
        // Create profile with current UI configuration
        ProfileConfig profileConfig;
        profileConfig.name = profileName.toStdString();
        profileConfig.version = "1.0";
        profileConfig.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
        
        // Get configurations from UI
        profileConfig.generalParams = m_mainWindow->getGeneralParamsConfig();
        profileConfig.strategyConfig = m_mainWindow->getStrategyConfig();
        
        bool success = saveProfile(profileName, profileConfig);
        
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

bool ProfileManager::deleteCurrentProfile(QWidget* parentWidget)
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
                                        "Profil supprimé.");
            }
        } else if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                "Échec de la suppression du profil.");
        }
        
        return success;
    }
    
    return false;
}

bool ProfileManager::importConfigFromFile(QWidget* parentWidget)
{
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
                                                   "Importer une configuration",
                                                   QDir::homePath(),
                                                   "Fichiers JSON (*.json)");
    
    if (fileName.isEmpty()) {
        return false; // User canceled
    }
    
    if (!QFile::exists(fileName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", "Le fichier sélectionné n'existe pas.");
        }
        return false;
    }
    
    // Try to load the profile configuration from the file
    ProfileConfig importedConfig;
    bool loadSuccess = SerializationUtils::loadFromJsonFile(fileName, importedConfig);
    
    if (!loadSuccess) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur d'importation", 
                                "Le fichier n'est pas une configuration valide.");
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
                                                      "Nom du profil:",
                                                      QLineEdit::Normal,
                                                      importedConfig.name.c_str(), &ok);
        
        if (ok && !newProfileName.isEmpty() && newProfileName != "DEFAULT") {
            targetProfile = newProfileName;
            importedConfig.name = newProfileName.toStdString();
            success = saveProfile(targetProfile, importedConfig);
            
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
        importedConfig.name = m_currentProfile.toStdString();
        success = saveProfile(targetProfile, importedConfig);
        
        if (success) {
            // Reload configuration into UI
            applyProfileToUI(targetProfile);
        }
    }
    
    if (success) {
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Import réussi", 
                                    QString("Configuration importée avec succès dans le profil '%1'.").arg(targetProfile));
        }
    } else {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur d'importation", 
                                "Échec de l'importation de la configuration.");
        }
    }
    
    return success;
}

bool ProfileManager::exportConfigToFile(QWidget* parentWidget, const QString& profileName)
{
    QString targetProfile = profileName.isEmpty() ? m_currentProfile : profileName;

    // Load the profile configuration
    ProfileConfig profileConfig;
    if (targetProfile == m_currentProfile && m_mainWindow) {
        // If it's the current profile, get values from UI
        profileConfig.name = targetProfile.toStdString();
        profileConfig.version = "1.0";
        profileConfig.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
        
        profileConfig.generalParams = m_mainWindow->getGeneralParamsConfig();
        profileConfig.strategyConfig = m_mainWindow->getStrategyConfig();

    } else {
        // Otherwise, load from saved profile
        if (!loadProfileFromJson(targetProfile, profileConfig)) {
            if (parentWidget) {
                QMessageBox::warning(parentWidget, "Erreur", 
                                    QString("Impossible de charger le profil '%1'.").arg(targetProfile));
            }
            return false;
        }
    }

    // Default file name with profile name and timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString defaultFileName = QString("backtest_config_%1_%2.json").arg(targetProfile).arg(timestamp);
    
    QString fileName = QFileDialog::getSaveFileName(parentWidget,
                                                   QString("Exporter la configuration - Profil: %1").arg(targetProfile),
                                                   QDir::homePath() + "/" + defaultFileName,
                                                   "Fichiers JSON (*.json)");
    
    if (fileName.isEmpty()) {
        return false; // User canceled
    }
    
    // Save the profile to the selected file
    bool success = SerializationUtils::saveToJsonFile(fileName, profileConfig);
    
    if (success) {
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Export réussi", 
                                    QString("Configuration du profil '%1' exportée vers:\n%2").arg(targetProfile).arg(fileName));
        }
    } else {
        if (parentWidget) {
            QMessageBox::critical(parentWidget, "Erreur d'exportation", 
                                "Erreur lors de l'écriture du fichier de configuration.");
        }
    }
    
    return success;
}

void ProfileManager::onProfileChanged(const QString& profileName)
{
    // Always apply the profile to allow re-loading the same profile multiple times
    applyProfileToUI(profileName);
}
