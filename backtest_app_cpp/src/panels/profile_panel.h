#ifndef PROFILE_PANEL_H
#define PROFILE_PANEL_H

#include <QObject>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QString>
#include "base_panel.h"

// Forward declaration
class ConfigManager;

/**
 * @brief Panel de gestion des profils de configuration
 * 
 * Ce panel permet de gérer les profils de configuration de l'application :
 * - Sauvegarder le profil actuel
 * - Créer un nouveau profil
 * - Charger un profil existant
 * - Supprimer un profil
 * - Importer/Exporter des configurations
 */
class ProfilePanel : public QObject, public BasePanel
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    ProfilePanel(QWidget* parent = nullptr);
    
    /**
     * @brief Crée et retourne l'interface graphique du panel
     * @return QGroupBox contenant les widgets du panel
     */
    QGroupBox* create() override;
    
    /**
     * @brief Récupère les valeurs des widgets du panel
     * @return Map contenant les valeurs sous forme de QVariant
     */
    QMap<QString, QVariant> getValues() override;
    
    /**
     * @brief Définit les valeurs des widgets du panel
     * @param values Map contenant les valeurs à affecter aux widgets
     */
    void setValues(const QMap<QString, QVariant>& values) override;
    
    /**
     * @brief Met à jour l'interface utilisateur pour un profil spécifique
     * @param profileName Nom du profil à afficher
     */
    void updateProfileUI(const QString& profileName);

    /**
     * @brief Initialise le panel avec le ConfigManager
     * @param configManager Pointeur vers le gestionnaire de configuration
     */
    void setConfigManager(ConfigManager* configManager);

public slots:
    /**
     * @brief Charge le profil sélectionné dans le menu déroulant
     * @param profileName Nom du profil à charger
     */
    void loadSelectedProfile(const QString& profileName);

private:
    /** Gestionnaire de configuration */
    ConfigManager* m_configManager;
};

#endif // PROFILE_PANEL_H