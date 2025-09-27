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
    QMenu* updateMenu = menuBar->addMenu(tr("&Mise à jour"));

    // Create the check for updates action (new feature)
    m_checkForUpdatesAction = new QAction(tr("Vérification mise à jour"), this);
    m_checkForUpdatesAction->setStatusTip(tr("Vérifier si une nouvelle version de l'app est disponible"));

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
        QMessageBox::critical(nullptr, tr("Erreur"), 
                             tr("Erreur interne: UpdateChecker non initialisé"));
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
        msgBox.setWindowTitle(tr("Mise à jour disponible"));
        msgBox.setText(tr("Une nouvelle version est disponible !"));
        msgBox.setInformativeText(tr("Version actuelle: %1\nNouvelle version: %2\n\nQue souhaitez-vous faire ?")
                                  .arg(currentVersion)
                                  .arg(latestVersion));
        
        QPushButton* downloadInstallButton = msgBox.addButton(tr("Télécharger et installer"), QMessageBox::AcceptRole);
        QPushButton* downloadOnlyButton = msgBox.addButton(tr("Télécharger seulement"), QMessageBox::ActionRole);
        QPushButton* laterButton = msgBox.addButton(tr("Plus tard"), QMessageBox::RejectRole);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == downloadInstallButton) {
            // User chose to download and install automatically
            qDebug() << "User chose to download and install, version:" << m_pendingDownloadVersion;

            // Create the progress dialog
            m_progressDialog = new QProgressDialog(tr("Téléchargement et installation en cours..."),
                                                   tr("Annuler"), 0, 0, nullptr);
            m_progressDialog->setWindowTitle(tr("Mise à jour"));
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
        QMessageBox::information(nullptr, tr("Aucune mise à jour"), 
                                tr("Vous avez déjà la dernière version !\n\nVersion actuelle: %1\nDernière version disponible: %2")
                                .arg(currentVersion)
                                .arg(latestVersion));
    }
}

void UpdateMenuManager::onUpdateCheckError(const QString& errorMessage)
{
    qDebug() << "Update check error:" << errorMessage;
    
    QMessageBox::warning(nullptr, tr("Erreur de vérification"), 
                        tr("Impossible de vérifier les mises à jour:\n%1\n\nVérifiez votre connexion internet et réessayez.")
                        .arg(errorMessage));
}

void UpdateMenuManager::onDownloadCompleted(bool success, const QString& filePath)
{
    qDebug() << "Download completed with success:" << success << "File path:" << filePath;

    // Update the progress dialog if in installation mode
    if (m_progressDialog && success) {
        m_progressDialog->setLabelText(tr("Téléchargement terminé, début de l'installation..."));
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
            QMessageBox::information(nullptr, tr("Succès"), 
                                    tr("Téléchargement réussi!\nFichier enregistré: %1").arg(filePath));
        } else {
            QMessageBox::warning(nullptr, tr("Échec"), 
                                tr("Le téléchargement a échoué ou le fichier est vide."));
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
    
    QMessageBox::critical(nullptr, tr("Erreur de téléchargement"), 
                         tr("Erreur lors du téléchargement:\n%1").arg(errorMessage));
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
        QMessageBox::information(nullptr, tr("Installation réussie"), message);
    } else {
        QMessageBox::warning(nullptr, tr("Installation échouée"), message);
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
    
    QMessageBox::critical(nullptr, tr("Erreur d'installation"), 
                         tr("Erreur lors de l'installation automatique:\n%1\n\nVous pouvez essayer d'installer manuellement le fichier téléchargé.")
                         .arg(errorMessage));
}
