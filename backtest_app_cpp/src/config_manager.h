#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QSettings>
#include <QWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTimeEdit>
#include <QDateEdit>
#include <QLineEdit>
#include <QDebug>

// Forward declaration
class App;

/**
 * @brief Gestionnaire de configuration avec support des profils
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();
    
    // Méthodes publiques
    QString getConfigFilePath() const;
    void initializeConfig();
    void loadConfig();
    void saveConfig();
    
    // Getter pour le profil actuel
    QString getCurrentProfile() const { return m_currentProfile; }
    
    // Gestion des profils
    QStringList listProfiles() const;
    bool profileExists(const QString& profileName) const;
    QMap<QString, QVariant> getProfile(const QString& profileName) const;
    bool saveProfile(const QString& profileName, const QMap<QString, QVariant>& profileData);
    bool deleteProfile(const QString& profileName);
    
    // Interface avec l'UI
    QVariant extractWidgetValue(QWidget* widget) const;
    bool setWidgetValue(QWidget* widget, const QVariant& value) const;
    QMap<QString, QWidget*> getWidgetMapping() const;
    QMap<QString, QVariant> getProfileFromUI() const;
    bool applyProfileToUI(const QString& profileName);
    
    // Actions utilisateur
    bool saveCurrentProfile(QWidget* parentWidget = nullptr);
    bool promptCreateNewProfile(QWidget* parentWidget = nullptr);
    bool deleteCurrentProfile(QWidget* parentWidget = nullptr);
    bool importConfigFromFile(QWidget* parentWidget = nullptr);
    bool exportConfigToFile(QWidget* parentWidget = nullptr, const QString& profileName = "");
    
    // Méthodes publiques pour les slots
    void updateProfileUI(const QString& selectedProfile);
    void onProfileChanged(const QString& profileName);

signals:
    void profileChanged(const QString& profileName);
    void profileListUpdated();

private:
    QSettings* m_config;
    QString m_configFile;
    QString m_currentProfile;
    QMap<QString, QVariant> m_defaultValues;
    App* m_mainWindow;
    
    void setupDefaultValues();
};

#endif // CONFIG_MANAGER_H