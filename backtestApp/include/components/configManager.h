#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include "components/ConfigSerializerCereal.hpp"

// Forward declaration
class App;

/**
 * @brief Configuration manager for handling and storing backtest parameters
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();
    
    // Getter for the current profile
    QString getCurrentProfile() const { return m_currentProfile; }
    QStringList listProfiles() const;

    // User actions - public API preserved
    bool saveCurrentProfile(QWidget* parentWidget = nullptr);
    bool promptCreateNewProfile(QWidget* parentWidget = nullptr);
    bool deleteCurrentProfile(QWidget* parentWidget = nullptr);
    bool importConfigFromFile(QWidget* parentWidget = nullptr);
    bool exportConfigToFile(QWidget* parentWidget = nullptr, const QString& profileName = "");
    
    // Public methods for slots
    void onProfileChanged(const QString& profileName);

signals:
    void profileChanged(const QString& profileName);
    void profileListUpdated();

private:
    QString m_configDir;         // Directory where profiles are stored
    QString m_currentProfile;    // Current active profile name
    App* m_mainWindow;           // Reference to main application window
    
    // Core profile management methods
    bool profileExists(const QString& profileName) const;
    bool deleteProfile(const QString& profileName);
    bool applyProfileToUI(const QString& profileName);
    
    // Cereal-based serialization methods
    bool saveProfileToJson(const QString& profileName, const ProfileConfig& config);
    bool loadProfileFromJson(const QString& profileName, ProfileConfig& config);
    QString getProfilePath(const QString& profileName) const;
    
    // Default profile creation
    ProfileConfig createDefaultProfile() const;
    void ensureDefaultProfileExists();
    
    // Initialize application directories
    void initializeDirectories();
};
