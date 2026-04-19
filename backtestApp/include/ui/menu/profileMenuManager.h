#pragma once

#include <QObject>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMap>
#include <QString>

// Forward declarations
class ProfileManager;
class App;

/**
 * @brief Profile menu manager in the main menu bar
 * 
 * This class manages the "Profiles" menu in the main menu bar
 * with actions to manage configuration profiles.
 */
class ProfileMenuManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Pointer to the main application
     */
    ProfileMenuManager(App* parent = nullptr);
    
    /**
     * @brief Creates and adds the profiles menu to the menu bar
     * @param menuBar Menu bar where to add the profiles menu
     */
    void createProfileMenu(QMenuBar* menuBar);
    
    /**
     * @brief Initializes the manager with the ProfileManager
     * @param configManager Pointer to the configuration manager
     */
    void setConfigManager(ProfileManager* configManager);
    
    /**
     * @brief Updates the list of profiles in the menu
     */
    void updateProfileList();

public slots:
    /**
     * @brief Updates the display of the current profile
     * @param profileName Name of the current profile
     */
    void onProfileChanged(const QString& profileName);

private slots:
    void onSaveCurrentProfile();
    void onCreateNewProfile();
    void onManageProfiles();
    void onImportProfile();
    void onExportProfile();
    void onLoadProfile();
    void onOpenProfilesDirectory();

private:
    App* m_mainWindow;
    ProfileManager* m_configManager;
    
    // Menu and actions
    QMenu* m_profileMenu;
    QMenu* m_loadProfileSubmenu;
    
    // Main actions
    QAction* m_saveProfileAction;
    QAction* m_newProfileAction;
    QAction* m_manageProfilesAction;
    QAction* m_importAction;
    QAction* m_exportAction;
    QAction* m_openDirectoryAction;

    
    // Dynamic actions for profiles
    QMap<QString, QAction*> m_profileActions;
    
    void createActions();
    void updateLoadSubmenu();
};
