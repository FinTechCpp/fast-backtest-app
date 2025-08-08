#pragma once

#include <QObject>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMap>
#include <QString>

// Forward declarations
class ConfigManager;
class App;

/**
 * @brief Gestionnaire de menu des profils dans la barre de menu
 * 
 * Cette classe gère le menu "Profils" dans la barre de menu principale
 * avec les actions pour gérer les profils de configuration.
 */
class ProfileMenuManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'application principale
     */
    ProfileMenuManager(App* parent = nullptr);
    
    /**
     * @brief Crée et ajoute le menu des profils à la barre de menu
     * @param menuBar Barre de menu où ajouter le menu des profils
     */
    void createProfileMenu(QMenuBar* menuBar);
    
    /**
     * @brief Initialise le gestionnaire avec le ConfigManager
     * @param configManager Pointeur vers le gestionnaire de configuration
     */
    void setConfigManager(ConfigManager* configManager);
    
    /**
     * @brief Met à jour la liste des profils dans le menu
     */
    void updateProfileList();

public slots:
    /**
     * @brief Met à jour l'affichage du profil actuel
     * @param profileName Nom du profil actuel
     */
    void onProfileChanged(const QString& profileName);

private slots:
    void onSaveCurrentProfile();
    void onCreateNewProfile();
    void onDeleteCurrentProfile();
    void onImportProfile();
    void onExportProfile();
    void onLoadProfile();
    void onOpenProfilesDirectory();

private:
    App* m_mainWindow;
    ConfigManager* m_configManager;
    
    // Menu et actions
    QMenu* m_profileMenu;
    QMenu* m_loadProfileSubmenu;
    
    // Actions principales
    QAction* m_saveProfileAction;
    QAction* m_newProfileAction;
    QAction* m_deleteProfileAction;
    QAction* m_importAction;
    QAction* m_exportAction;
    QAction* m_openDirectoryAction;

    
    // Actions dynamiques pour les profils
    QMap<QString, QAction*> m_profileActions;
    
    void createActions();
    void updateLoadSubmenu();
};

