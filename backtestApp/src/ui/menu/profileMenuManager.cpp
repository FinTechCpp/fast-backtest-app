#include "ui/menu/profileMenuManager.h"
#include "ui/app.h"
#include "components/Managers/ProfileManager.h"
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QTimer>

ProfileMenuManager::ProfileMenuManager(App* parent)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_configManager(nullptr)
    , m_profileMenu(nullptr)
    , m_loadProfileSubmenu(nullptr)
    , m_saveProfileAction(nullptr)
    , m_newProfileAction(nullptr)
    , m_deleteProfileAction(nullptr)
    , m_importAction(nullptr)
    , m_exportAction(nullptr)
    , m_openDirectoryAction(nullptr)
{
    qDebug() << "ProfileMenuManager créé";
}

void ProfileMenuManager::createProfileMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar null passé à createProfileMenu";
        return;
    }
    
    // Créer le menu Profils (avant le menu Aide)
    m_profileMenu = menuBar->addMenu(tr("&Profils"));
    
    createActions();
    
    // Ajouter les actions au menu
    m_profileMenu->addAction(m_saveProfileAction);
    m_profileMenu->addAction(m_newProfileAction);
    m_profileMenu->addAction(m_deleteProfileAction);
    m_profileMenu->addSeparator();
    
    // Créer le sous-menu pour charger les profils
    m_loadProfileSubmenu = m_profileMenu->addMenu(tr("&Charger un profil"));
    
    m_profileMenu->addSeparator();
    m_profileMenu->addAction(m_importAction);
    m_profileMenu->addAction(m_exportAction);

    // Ajouter un séparateur puis l'action pour ouvrir le dossier
    m_profileMenu->addSeparator();
    m_profileMenu->addAction(m_openDirectoryAction);
    
    
    qDebug() << "Menu Profils créé";
}

void ProfileMenuManager::onOpenProfilesDirectory()
{
    if (m_configManager) {
        if (!m_configManager->openProfilesDirectory()) {
            // Afficher un message d'erreur en cas d'échec
            QMessageBox::warning(m_mainWindow, tr("Erreur"),
                                tr("Impossible d'ouvrir le dossier des profils.\n"
                                   "Chemin: %1").arg(m_configManager->getProfilesDirectory()));
        }
    }
}

void ProfileMenuManager::createActions()
{
    // Action Sauvegarder
    m_saveProfileAction = new QAction(tr("&Sauvegarder le profil actuel"), this);
    m_saveProfileAction->setShortcut(QKeySequence::Save);
    m_saveProfileAction->setStatusTip(tr("Sauvegarder les paramètres actuels"));
    connect(m_saveProfileAction, &QAction::triggered, this, &ProfileMenuManager::onSaveCurrentProfile);
    
    // Action Nouveau profil
    m_newProfileAction = new QAction(tr("&Nouveau profil..."), this);
    m_newProfileAction->setShortcut(QKeySequence::New);
    m_newProfileAction->setStatusTip(tr("Créer un nouveau profil"));
    connect(m_newProfileAction, &QAction::triggered, this, &ProfileMenuManager::onCreateNewProfile);
    
    // Action Supprimer profil
    m_deleteProfileAction = new QAction(tr("&Supprimer le profil actuel"), this);
    m_deleteProfileAction->setShortcut(QKeySequence::Delete);
    m_deleteProfileAction->setStatusTip(tr("Supprimer le profil actuel"));
    connect(m_deleteProfileAction, &QAction::triggered, this, &ProfileMenuManager::onDeleteCurrentProfile);
    
    // Action Importer
    m_importAction = new QAction(tr("&Importer..."), this);
    m_importAction->setStatusTip(tr("Importer une configuration"));
    connect(m_importAction, &QAction::triggered, this, &ProfileMenuManager::onImportProfile);
    
    // Action Exporter
    m_exportAction = new QAction(tr("&Exporter..."), this);
    m_exportAction->setStatusTip(tr("Exporter la configuration actuelle"));
    connect(m_exportAction, &QAction::triggered, this, &ProfileMenuManager::onExportProfile);
    
    // Nouvelle action - Ouvrir le dossier des profils
    m_openDirectoryAction = new QAction(tr("&Ouvrir le dossier des profils"), this);
    m_openDirectoryAction->setStatusTip(tr("Ouvrir le dossier contenant les fichiers de profils"));
    connect(m_openDirectoryAction, &QAction::triggered, this, &ProfileMenuManager::onOpenProfilesDirectory);
}

void ProfileMenuManager::setConfigManager(ProfileManager* configManager)
{
    m_configManager = configManager;
    if (m_configManager) {
        qDebug() << "Connexion du ProfileManager au ProfileMenuManager";
        
        // Connecter le signal de changement de profil
        connect(m_configManager, &ProfileManager::profileChanged,
                this, &ProfileMenuManager::onProfileChanged);
        connect(m_configManager, &ProfileManager::profileListUpdated,
                this, &ProfileMenuManager::updateProfileList);
        
        qDebug() << "Signaux connectés, mise à jour initiale de la liste des profils";
        
        // Utiliser un timer pour s'assurer que la configuration est entièrement chargée
        QTimer::singleShot(100, this, [this]() {
            qDebug() << "Mise à jour différée de la liste des profils";
            updateProfileList();
        });
        
        qDebug() << "ProfileManager connecté au ProfileMenuManager";
    } else {
        qWarning() << "ProfileManager null passé à setConfigManager";
    }
}

void ProfileMenuManager::updateProfileList()
{
    qDebug() << "updateProfileList() appelé";
    
    if (!m_configManager || !m_loadProfileSubmenu) {
        qWarning() << "ProfileManager ou LoadProfileSubmenu manquant";
        return;
    }
    
    // Nettoyer les actions existantes
    for (auto action : m_profileActions.values()) {
        m_loadProfileSubmenu->removeAction(action);
        delete action;
    }
    m_profileActions.clear();
    
    // Ajouter les profils disponibles
    QStringList profiles = m_configManager->listProfiles();
    QString currentProfile = m_configManager->getCurrentProfile();
    
    qDebug() << "Profils récupérés:" << profiles;
    qDebug() << "Profil actuel:" << currentProfile;
    
    for (const QString& profile : profiles) {
        QAction* action = new QAction(profile, this);
        action->setCheckable(true);
        action->setChecked(profile == currentProfile);
        action->setData(profile);
        
        connect(action, &QAction::triggered, this, &ProfileMenuManager::onLoadProfile);
        
        m_loadProfileSubmenu->addAction(action);
        m_profileActions[profile] = action;
        
        qDebug() << "Action créée pour le profil:" << profile;
    }
    
    qDebug() << "Liste des profils mise à jour avec" << profiles.size() << "profils";
}

void ProfileMenuManager::onProfileChanged(const QString& profileName)
{
    // Mettre à jour les coches dans le menu
    for (auto it = m_profileActions.begin(); it != m_profileActions.end(); ++it) {
        it.value()->setChecked(it.key() == profileName);
    }
    
    // Désactiver la suppression pour le profil DEFAULT
    if (m_deleteProfileAction) {
        m_deleteProfileAction->setEnabled(profileName != "DEFAULT");
    }
    
    qDebug() << "Profil actuel mis à jour vers:" << profileName;
}

void ProfileMenuManager::onSaveCurrentProfile()
{
    if (m_configManager) {
        m_configManager->saveCurrentProfile(m_mainWindow);
    }
}

void ProfileMenuManager::onCreateNewProfile()
{
    if (m_configManager) {
        if (m_configManager->promptCreateNewProfile(m_mainWindow)) {
            updateProfileList();
        }
    }
}

void ProfileMenuManager::onDeleteCurrentProfile()
{
    if (m_configManager) {
        if (m_configManager->deleteCurrentProfile(m_mainWindow)) {
            updateProfileList();
        }
    }
}

void ProfileMenuManager::onImportProfile()
{
    if (m_configManager) {
        if (m_configManager->importConfigFromFile(m_mainWindow)) {
            updateProfileList();
        }
    }
}

void ProfileMenuManager::onExportProfile()
{
    if (m_configManager) 
        m_configManager->exportConfigToFile(m_mainWindow);
    
}

void ProfileMenuManager::onLoadProfile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && m_configManager) {
        QString profileName = action->data().toString();
        if (!profileName.isEmpty()) {
            m_configManager->onProfileChanged(profileName);
        }
    }
}