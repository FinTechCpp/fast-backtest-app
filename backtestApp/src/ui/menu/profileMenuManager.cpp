#include "ui/menu/profileMenuManager.h"
#include "ui/app.h"
#include "components/Managers/ProfileManager.h"
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QWidgetAction>
#include <QTimer>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAbstractItemView>

ProfileMenuManager::ProfileMenuManager(App* parent)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_configManager(nullptr)
    , m_profileMenu(nullptr)
    , m_loadProfileSubmenu(nullptr)
    , m_saveProfileAction(nullptr)
    , m_newProfileAction(nullptr)
    , m_manageProfilesAction(nullptr)
    , m_importAction(nullptr)
    , m_exportAction(nullptr)
    , m_openDirectoryAction(nullptr)
{
    qDebug() << "ProfileMenuManager created";
}

void ProfileMenuManager::createProfileMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "Null menuBar passed to createProfileMenu";
        return;
    }
    
    // Create the Profiles menu (before the Help menu)
    m_profileMenu = menuBar->addMenu(tr("&Profiles"));
    
    createActions();
    
    // Add actions to the menu
    m_profileMenu->addAction(m_saveProfileAction);
    m_profileMenu->addAction(m_newProfileAction);
    m_profileMenu->addSeparator();
    
    // Create the submenu for loading profiles
    m_loadProfileSubmenu = m_profileMenu->addMenu(tr("&Load profile"));
    connect(m_loadProfileSubmenu, &QMenu::aboutToShow, this, &ProfileMenuManager::updateProfileList);

    // Centralized profile management dialog
    m_profileMenu->addAction(m_manageProfilesAction);
    
    m_profileMenu->addSeparator();
    m_profileMenu->addAction(m_importAction);
    m_profileMenu->addAction(m_exportAction);

    // Add a separator then the action to open the directory
    m_profileMenu->addSeparator();
    m_profileMenu->addAction(m_openDirectoryAction);
    
    
    qDebug() << "Profiles menu created";
}

void ProfileMenuManager::onOpenProfilesDirectory()
{
    if (m_configManager) {
        if (!m_configManager->openProfilesDirectory()) {
            // Show an error message on failure
            QMessageBox::warning(m_mainWindow, tr("Error"),
                                tr("Unable to open profiles directory.\n"
                                   "Path: %1").arg(m_configManager->getProfilesDirectory()));
        }
    }
}

void ProfileMenuManager::createActions()
{
    // Save action
    m_saveProfileAction = new QAction(tr("&Save current profile"), this);
    m_saveProfileAction->setShortcut(QKeySequence::Save);
    m_saveProfileAction->setStatusTip(tr("Save current settings"));
    connect(m_saveProfileAction, &QAction::triggered, this, &ProfileMenuManager::onSaveCurrentProfile);
    
    // New profile action
    m_newProfileAction = new QAction(tr("&New profile..."), this);
    m_newProfileAction->setShortcut(QKeySequence::New);
    m_newProfileAction->setStatusTip(tr("Create a new profile"));
    connect(m_newProfileAction, &QAction::triggered, this, &ProfileMenuManager::onCreateNewProfile);

    // Manage profiles action
    m_manageProfilesAction = new QAction(tr("&Manage profiles..."), this);
    m_manageProfilesAction->setStatusTip(tr("Load, import, export or delete a profile"));
    connect(m_manageProfilesAction, &QAction::triggered, this, &ProfileMenuManager::onManageProfiles);
    
    // Import action
    m_importAction = new QAction(tr("&Import..."), this);
    m_importAction->setStatusTip(tr("Import a configuration"));
    connect(m_importAction, &QAction::triggered, this, &ProfileMenuManager::onImportProfile);
    
    // Export action
    m_exportAction = new QAction(tr("&Export..."), this);
    m_exportAction->setStatusTip(tr("Export the current configuration"));
    connect(m_exportAction, &QAction::triggered, this, &ProfileMenuManager::onExportProfile);
    
    // New action - Open profiles directory
    m_openDirectoryAction = new QAction(tr("&Open profiles directory"), this);
    m_openDirectoryAction->setStatusTip(tr("Open the folder containing the profile files"));
    connect(m_openDirectoryAction, &QAction::triggered, this, &ProfileMenuManager::onOpenProfilesDirectory);
}

void ProfileMenuManager::setConfigManager(ProfileManager* configManager)
{
    m_configManager = configManager;
    if (m_configManager) {
        qDebug() << "Connecting ProfileManager to ProfileMenuManager";
        
        // Connect the profile changed signal
        connect(m_configManager, &ProfileManager::profileChanged,
                this, &ProfileMenuManager::onProfileChanged);
        connect(m_configManager, &ProfileManager::profileListUpdated,
                this, &ProfileMenuManager::updateProfileList);
        
        qDebug() << "Signals connected, initial update of profile list";
        
        // Use a timer to ensure the configuration is fully loaded
        QTimer::singleShot(100, this, [this]() {
            qDebug() << "Deferred update of profile list";
            updateProfileList();
        });
        
        qDebug() << "ProfileManager connected to ProfileMenuManager";
    } else {
        qWarning() << "Null ProfileManager passed to setConfigManager";
    }
}

void ProfileMenuManager::updateProfileList()
{
    qDebug() << "updateProfileList() called";
    
    if (!m_configManager || !m_loadProfileSubmenu) {
        qWarning() << "ProfileManager or LoadProfileSubmenu missing";
        return;
    }
    
    // Clean up existing actions
    for (auto action : m_profileActions.values()) {
        m_loadProfileSubmenu->removeAction(action);
        delete action;
    }
    m_profileActions.clear();
    
    // Add available profiles
    QStringList profiles = m_configManager->listProfiles();
    QString currentProfile = m_configManager->getCurrentProfile();
    
    qDebug() << "Profiles retrieved:" << profiles;
    qDebug() << "Current profile:" << currentProfile;
    
    for (const QString& profile : profiles) {
        // Create a widget-based action so we can style the background for the active profile
        QWidgetAction* waction = new QWidgetAction(this);

        // Button acts as the clickable item inside the menu
        QString text = (profile == currentProfile) ? profile : profile;
        QPushButton* btn = new QPushButton(text);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { text-align: left; padding: 6px 12px; border: none; background: transparent; }"
        );

        if (profile == currentProfile) {
            // green background for active profile
            btn->setStyleSheet(
                "QPushButton { text-align: left; padding: 6px 12px; border: none; background-color: #c8facc; }"
            );
        }

        // When clicked, update UI and delegate to ProfileManager
        connect(btn, &QPushButton::clicked, this, [this, profile, btn]() {
            // update visuals immediately
            for (auto it = m_profileActions.begin(); it != m_profileActions.end(); ++it) {
                QWidgetAction* a = qobject_cast<QWidgetAction*>(it.value());
                if (!a) continue;
                QWidget* w = a->defaultWidget();
                if (!w) continue;
                QPushButton* b = qobject_cast<QPushButton*>(w);
                if (!b) continue;
                if (it.key() == profile) {
                    b->setText(it.key());
                    b->setStyleSheet("QPushButton { text-align: left; padding: 6px 12px; border: none; background-color: #c8facc; }");
                } else {
                    b->setText(it.key());
                    b->setStyleSheet("QPushButton { text-align: left; padding: 6px 12px; border: none; background: transparent; }");
                }
            }

            if (m_configManager) m_configManager->onProfileChanged(profile);
        });

        waction->setDefaultWidget(btn);
        waction->setData(profile);

        m_loadProfileSubmenu->addAction(waction);
        m_profileActions[profile] = waction;

        qDebug() << "Widget action created for profile:" << profile << " active=" << (profile == currentProfile);
    }
    
    qDebug() << "Profile list updated with" << profiles.size() << "profiles";
}

void ProfileMenuManager::onProfileChanged(const QString& profileName)
{
    // Update actions display: update text to indicate active profile
    for (auto it = m_profileActions.begin(); it != m_profileActions.end(); ++it) {
        const QString& name = it.key();
        QAction* action = it.value();
        if (!action) continue;

        // If this action is a QWidgetAction with a QPushButton, update the button text/style
        if (QWidgetAction* wa = qobject_cast<QWidgetAction*>(action)) {
            QWidget* w = wa->defaultWidget();
            if (QPushButton* b = qobject_cast<QPushButton*>(w)) {
                if (name == profileName) {
                    b->setText(name);
                    b->setStyleSheet("QPushButton { text-align: left; padding: 6px 12px; border: none; background-color: #c8facc; }");
                } else {
                    b->setText(name);
                    b->setStyleSheet("QPushButton { text-align: left; padding: 6px 12px; border: none; background: transparent; }");
                }
            } else {
                // Fallback: update action text
                action->setText(name);
            }
        } else {
            action->setText(name);
        }
        // Keep all actions enabled so user can reload any profile at any time
        action->setEnabled(true);
    }
    
    qDebug() << "Current profile updated to:" << profileName;
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
        if (!profileName.isEmpty()) 
            m_configManager->onProfileChanged(profileName);
    }
}

void ProfileMenuManager::onManageProfiles()
{
    if (!m_configManager || !m_mainWindow) {
        return;
    }

    QDialog dlg(m_mainWindow);
    dlg.setWindowTitle(tr("Manage Profiles"));
    dlg.resize(700, 420);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QLabel* label = new QLabel(&dlg);
    layout->addWidget(label);

    QListWidget* list = new QListWidget(&dlg);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* loadBtn = new QPushButton(tr("Load"), &dlg);
    QPushButton* exportBtn = new QPushButton(tr("Export"), &dlg);
    QPushButton* deleteBtn = new QPushButton(tr("Delete"), &dlg);
    QPushButton* importBtn = new QPushButton(tr("Import"), &dlg);
    QPushButton* openDirBtn = new QPushButton(tr("Open Directory"), &dlg);
    QPushButton* closeBtn = new QPushButton(tr("Close"), &dlg);

    buttonLayout->addWidget(loadBtn);
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addWidget(deleteBtn);
    buttonLayout->addWidget(importBtn);
    buttonLayout->addWidget(openDirBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    auto updateButtonStates = [&]() {
        QListWidgetItem* selected = list->currentItem();
        const bool hasSelection = selected != nullptr;
        loadBtn->setEnabled(hasSelection);
        exportBtn->setEnabled(hasSelection);
        const QString selectedProfile = hasSelection
            ? selected->data(Qt::UserRole).toString()
            : QString();
        deleteBtn->setEnabled(hasSelection && selectedProfile != "DEFAULT");
    };

    auto refreshProfiles = [&]() {
        list->clear();

        const QStringList profiles = m_configManager->listProfiles();
        const QString currentProfile = m_configManager->getCurrentProfile();

        label->setText(tr("Select a profile (current: %1):").arg(currentProfile));

        int selectedRow = -1;
        for (const QString& profile : profiles) {
            const QString display = (profile == currentProfile)
                ? tr("%1 (current)").arg(profile)
                : profile;

            QListWidgetItem* item = new QListWidgetItem(display, list);
            item->setData(Qt::UserRole, profile);

            if (profile == currentProfile) {
                selectedRow = list->count() - 1;
            }
        }

        if (selectedRow >= 0) {
            list->setCurrentRow(selectedRow);
        } else if (list->count() > 0) {
            list->setCurrentRow(0);
        }

        updateButtonStates();
    };

    refreshProfiles();

    connect(list, &QListWidget::itemSelectionChanged, &dlg, updateButtonStates);

    connect(loadBtn, &QPushButton::clicked, &dlg, [&]() {
        QListWidgetItem* selected = list->currentItem();
        if (!selected) {
            return;
        }

        const QString profileName = selected->data(Qt::UserRole).toString();
        if (!profileName.isEmpty()) {
            m_configManager->onProfileChanged(profileName);
            refreshProfiles();
        }
    });

    connect(exportBtn, &QPushButton::clicked, &dlg, [&]() {
        QListWidgetItem* selected = list->currentItem();
        if (!selected) {
            return;
        }

        const QString profileName = selected->data(Qt::UserRole).toString();
        if (!profileName.isEmpty()) {
            m_configManager->exportConfigToFile(&dlg, profileName);
        }
    });

    connect(deleteBtn, &QPushButton::clicked, &dlg, [&]() {
        QListWidgetItem* selected = list->currentItem();
        if (!selected) {
            return;
        }

        const QString profileName = selected->data(Qt::UserRole).toString();
        if (profileName == "DEFAULT") {
            QMessageBox::information(&dlg, tr("Information"), tr("The DEFAULT profile cannot be deleted."));
            return;
        }

        if (m_configManager->getCurrentProfile() != profileName) {
            m_configManager->onProfileChanged(profileName);
        }

        if (m_configManager->deleteCurrentProfile(&dlg)) {
            updateProfileList();
            refreshProfiles();
        }
    });

    connect(importBtn, &QPushButton::clicked, &dlg, [&]() {
        if (m_configManager->importConfigFromFile(&dlg)) {
            updateProfileList();
            refreshProfiles();
        }
    });

    connect(openDirBtn, &QPushButton::clicked, &dlg, [&]() {
        onOpenProfilesDirectory();
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    connect(list, &QListWidget::itemDoubleClicked, &dlg, [&]() {
        loadBtn->click();
    });

    dlg.exec();
}
