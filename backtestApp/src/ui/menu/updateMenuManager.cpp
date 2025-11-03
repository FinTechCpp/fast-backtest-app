#include "ui/menu/updateMenuManager.h"
#include "components/updateChecker.h"
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QProcess> 
#include <QPushButton>
#include <QAbstractButton>
#include <QTimer>
#include <QMenuBar>
#include <QMenu>

UpdateMenuManager::UpdateMenuManager(QObject* parent)
    : QObject(parent)
    , m_checkForUpdatesAction(nullptr)
    , m_updateChecker(nullptr)
    , m_pendingDownloadVersion()
    , m_progressDialog(nullptr)
{
    qDebug() << "UpdateMenuManager constructor called";

    // Create the UpdateChecker instance
    m_updateChecker = new UpdateChecker(this);
    
    // Connect the signals for downloads
    connect(m_updateChecker, &UpdateChecker::downloadCompleted,
            this, &UpdateMenuManager::onDownloadCompleted);
    connect(m_updateChecker, &UpdateChecker::downloadError,
            this, &UpdateMenuManager::onDownloadError);

    // Connect the signals for update checking
    connect(m_updateChecker, &UpdateChecker::updateCheckCompleted,
            this, &UpdateMenuManager::onUpdateCheckCompleted);
    connect(m_updateChecker, &UpdateChecker::updateCheckError,
            this, &UpdateMenuManager::onUpdateCheckError);

    // Connect the signals for automatic installation
    connect(m_updateChecker, &UpdateChecker::installationProgress,
            this, &UpdateMenuManager::onInstallationProgress);
    connect(m_updateChecker, &UpdateChecker::installationCompleted,
            this, &UpdateMenuManager::onInstallationCompleted);
    connect(m_updateChecker, &UpdateChecker::installationError,
            this, &UpdateMenuManager::onInstallationError);
}

UpdateMenuManager::~UpdateMenuManager()
{
    qDebug() << "UpdateMenuManager destructor called";
    
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
}

void UpdateMenuManager::createUpdateMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar is null, cannot create update menu";
        return;
    }
    
    qDebug() << "Creating update menu...";

    // Create the update menu
    QMenu* updateMenu = menuBar->addMenu(tr("&Update"));

    // Create the check for updates action (new feature)
    m_checkForUpdatesAction = new QAction(tr("Check for updates"), this);
    m_checkForUpdatesAction->setStatusTip(tr("Check if a new version of the app is available"));

    // Connect the actions
    connect(m_checkForUpdatesAction, &QAction::triggered,
            this, &UpdateMenuManager::onCheckForUpdates);

    // Add the actions to the menu (check first)
    updateMenu->addAction(m_checkForUpdatesAction);
    updateMenu->addSeparator();
    
    qDebug() << "Update menu created successfully";
}

void UpdateMenuManager::onCheckForUpdates()
{
    qDebug() << "Check for updates action triggered";
    
    if (!m_updateChecker) {
        qCritical() << "UpdateChecker is null!";
        QMessageBox::critical(nullptr, tr("Error"), 
                             tr("Internal error: UpdateChecker not initialized"));
        return;
    }

    // Start the update check
    qDebug() << "Starting update check from UpdateMenuManager...";
    m_updateChecker->checkForUpdates();
}

void UpdateMenuManager::onUpdateCheckCompleted(bool updateAvailable, const QString& latestVersion, const QString& currentVersion)
{
    qDebug() << "Update check completed - Available:" << updateAvailable 
             << "Latest:" << latestVersion << "Current:" << currentVersion;
    
    if (updateAvailable) {
        // Store the version to download
        m_pendingDownloadVersion = latestVersion;

        // Update available, propose download options
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(tr("Update available"));
        msgBox.setText(tr("A new version is available!"));
        msgBox.setInformativeText(tr("Current version: %1\nNew version: %2\n\nWhat would you like to do?")
                                  .arg(currentVersion)
                                  .arg(latestVersion));
        
        QPushButton* downloadInstallButton = msgBox.addButton(tr("Download and install"), QMessageBox::AcceptRole);
        QPushButton* downloadOnlyButton = msgBox.addButton(tr("Download only"), QMessageBox::ActionRole);
        QPushButton* laterButton = msgBox.addButton(tr("Later"), QMessageBox::RejectRole);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == downloadInstallButton) {
            // User chose to download and install automatically
            qDebug() << "User chose to download and install, version:" << m_pendingDownloadVersion;

            // Create the progress dialog
            m_progressDialog = new QProgressDialog(tr("Downloading and installing..."),
                                                   tr("Cancel"), 0, 0, nullptr);
            m_progressDialog->setWindowTitle(tr("Update"));
            m_progressDialog->setModal(true);
            m_progressDialog->setMinimumDuration(0);
            m_progressDialog->setValue(0);
            m_progressDialog->show();

            // Connect the cancel button
            connect(m_progressDialog, &QProgressDialog::canceled, this, [this]() {
                qDebug() << "User canceled installation";
                if (m_progressDialog) {
                    m_progressDialog->close();
                    delete m_progressDialog;
                    m_progressDialog = nullptr;
                }
                m_pendingDownloadVersion.clear();
            });

            // Start the download and installation
            m_updateChecker->downloadAndInstallUpdate(m_pendingDownloadVersion);
            
        } else if (msgBox.clickedButton() == downloadOnlyButton) {
            // User chose to download only
            qDebug() << "User chose to download only, version:" << m_pendingDownloadVersion;
            m_updateChecker->downloadLatestRelease(m_pendingDownloadVersion);
        } else {
            qDebug() << "User chose to skip the update";
            m_pendingDownloadVersion.clear();  // Clear
        }
    } else {
        // No update available
        QMessageBox::information(nullptr, tr("No updates"), 
                                tr("You already have the latest version!\n\nCurrent version: %1\nLatest available version: %2")
                                .arg(currentVersion)
                                .arg(latestVersion));
    }
}

void UpdateMenuManager::onUpdateCheckError(const QString& errorMessage)
{
    qDebug() << "Update check error:" << errorMessage;
    
    QMessageBox::warning(nullptr, tr("Update check error"), 
                        tr("Unable to check for updates:\n%1\n\nCheck your internet connection and try again.")
                        .arg(errorMessage));
}

void UpdateMenuManager::onDownloadCompleted(bool success, const QString& filePath)
{
    qDebug() << "Download completed with success:" << success << "File path:" << filePath;

    // Update the progress dialog if in installation mode
    if (m_progressDialog && success) {
        m_progressDialog->setLabelText(tr("Download finished, starting installation..."));
        QApplication::processEvents();
    } else if (m_progressDialog) {
        // Close the dialog in case of failure
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Clear the pending download version only if not in installation mode
    if (!m_progressDialog) {
        m_pendingDownloadVersion.clear();
        
        if (success) {
            QMessageBox::information(nullptr, tr("Success"), 
                                    tr("Download successful!\nFile saved: %1").arg(filePath));
        } else {
            QMessageBox::warning(nullptr, tr("Failure"), 
                                tr("The download failed or the file is empty."));
        }
    }
}

void UpdateMenuManager::onDownloadError(const QString& errorMessage)
{
    qDebug() << "Download error:" << errorMessage;

    // Close the progress dialog if open
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Clear the pending download version
    m_pendingDownloadVersion.clear();
    
    QMessageBox::critical(nullptr, tr("Download error"), 
                         tr("Error during download:\n%1").arg(errorMessage));
}

void UpdateMenuManager::onInstallationProgress(const QString& message)
{
    qDebug() << "Installation progress:" << message;
    
    if (m_progressDialog) {
        m_progressDialog->setLabelText(message);
        QApplication::processEvents(); // Allow UI to update
    }
}

void UpdateMenuManager::onInstallationCompleted(bool success, const QString& message)
{
    qDebug() << "Installation completed with success:" << success << "Message:" << message;

    // Close the progress dialog if open
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Clear the pending download version
    m_pendingDownloadVersion.clear();
    
    if (success) {
        QMessageBox::information(nullptr, tr("Installation successful"), message);
    } else {
        QMessageBox::warning(nullptr, tr("Installation failed"), message);
    }
}

void UpdateMenuManager::onInstallationError(const QString& errorMessage)
{
    qDebug() << "Installation error:" << errorMessage;

    // Close the progress dialog if open
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Clear the pending download version
    m_pendingDownloadVersion.clear();
    
    QMessageBox::critical(nullptr, tr("Installation error"), 
                         tr("Error during automatic installation:\n%1\n\nYou can try installing the downloaded file manually.")
                         .arg(errorMessage));
}
