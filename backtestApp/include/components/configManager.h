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

/// TRUC A VOIR AVEC CLAUD : IDÉE : NE PAS MAPPER MANUELLEMENT
/*
// Exemple conceptuel avec des macros de réflexion (pas du code C++ standard)
#define SERIALIZABLE_PROPERTY(Type, Name) \
    private: Type Name##_; \
    public: Type get##Name() const { return Name##_; } \
    public: void set##Name(Type value) { Name##_ = value; }

class StrategyBaseConfig {
    SERIALIZABLE_PROPERTY(bool, EnableLogging)
    SERIALIZABLE_PROPERTY(double, StopLossDistance)
    // ...
};

// Utilisation
auto json = Serializer::toJson(myConfig); 
auto config = Serializer::fromJson<StrategyBaseConfig>(json);
*/

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
    
    QStringList listProfiles() const;
    QMap<QString, QVariant> getProfile(const QString& profileName) const;


    // User actions
    bool saveCurrentProfile(QWidget* parentWidget = nullptr);
    bool promptCreateNewProfile(QWidget* parentWidget = nullptr);
    bool deleteCurrentProfile(QWidget* parentWidget = nullptr);
    bool importConfigFromFile(QWidget* parentWidget = nullptr);
    bool exportConfigToFile(QWidget* parentWidget = nullptr, const QString& profileName = "");
    
    // Public methods for slots
    void updateProfileUI(const QString& selectedProfile);
    void onProfileChanged(const QString& profileName);

    // Convertir une structure typée en QMap pour la sauvegarde
    template<typename ConfigType>
    static QMap<QString, QVariant> convertConfigToQMap(const ConfigType& config);
    
    // Convertir un QMap en structure typée pour le chargement
    template<typename ConfigType>
    static ConfigType convertQMapToConfig(const QMap<QString, QVariant>& map);
    

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

    // QMap<QString, QVariant> getProfileFromUI() const;
    bool applyProfileToUI(const QString& profileName);

    // Profile management
    bool profileExists(const QString& profileName) const;
    bool saveProfile(const QString& profileName, const QMap<QString, QVariant>& profileData);
    bool deleteProfile(const QString& profileName);
};
