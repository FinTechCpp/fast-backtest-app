#pragma once

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
 * @brief Configuration manager for handling and storind backtest parameters
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();
    
    // Public methods
    QString getConfigFilePath() const;
    void initializeConfig();
    void loadConfig();
    void saveConfig();
    
    // Getter for the current profile
    QString getCurrentProfile() const { return m_currentProfile; }
    
    // Profile management
    QStringList listProfiles() const;
    bool profileExists(const QString& profileName) const;
    QMap<QString, QVariant> getProfile(const QString& profileName) const;
    bool saveProfile(const QString& profileName, const QMap<QString, QVariant>& profileData);
    bool deleteProfile(const QString& profileName);
    
    // Interface with the UI
    QVariant extractWidgetValue(QWidget* widget) const;
    bool setWidgetValue(QWidget* widget, const QVariant& value) const;
    QMap<QString, QWidget*> getWidgetMapping() const;
    QMap<QString, QVariant> getProfileFromUI() const;
    bool applyProfileToUI(const QString& profileName);

    // User actions
    bool saveCurrentProfile(QWidget* parentWidget = nullptr);
    bool promptCreateNewProfile(QWidget* parentWidget = nullptr);
    bool deleteCurrentProfile(QWidget* parentWidget = nullptr);
    bool importConfigFromFile(QWidget* parentWidget = nullptr);
    bool exportConfigToFile(QWidget* parentWidget = nullptr, const QString& profileName = "");
    
    // Public methods for slots
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

