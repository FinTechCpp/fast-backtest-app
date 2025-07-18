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

static const QUrl GITHUB_PAGES_RELEASES_URL = QUrl(QStringLiteral("https://fintechcpp.github.io/fast-backtest-app-releases/"));

UpdateMenuManager::UpdateMenuManager(QObject* parent)
    : QObject(parent),
      m_updateMenu(nullptr),
      m_checkUpdateAction(nullptr),
      m_aboutVersionAction(nullptr),
      m_updateChecker(nullptr),
      m_progressDialog(nullptr)
{
    // Create the UpdateChecker instance
    m_updateChecker = new UpdateChecker(this);

    // Connect signals
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &UpdateMenuManager::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::noUpdateAvailable,
            this, &UpdateMenuManager::onNoUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed,
            this, &UpdateMenuManager::onUpdateCheckFailed);
    connect(m_updateChecker, &UpdateChecker::downloadProgress,
            this, &UpdateMenuManager::onDownloadProgress);
    connect(m_updateChecker, &UpdateChecker::updateCompleted,
            this, &UpdateMenuManager::onUpdateCompleted);
    connect(m_updateChecker, &UpdateChecker::updateFailed,
            this, &UpdateMenuManager::onUpdateFailed);
}

UpdateMenuManager::~UpdateMenuManager()
{
    if (m_progressDialog) {
        delete m_progressDialog;
    }
}

void UpdateMenuManager::createUpdateMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar null switched to createUpdateMenu";
        return;
    }

    // Create the Update menu (before the Help menu)
    m_updateMenu = menuBar->addMenu(tr("&Mise à jour"));
    
    createActions();

    // Add actions to the menu
    m_updateMenu->addAction(m_checkUpdateAction);
    m_updateMenu->addSeparator();
    m_updateMenu->addAction(m_aboutVersionAction);

    qDebug() << "Menu Update created";
}

void UpdateMenuManager::createActions()
{
    // Check for updates action
    m_checkUpdateAction = new QAction(tr("&Vérifier les mises à jour"), this);
    m_checkUpdateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    m_checkUpdateAction->setStatusTip(tr("Vérifier s'il existe une nouvelle version disponible"));
    connect(m_checkUpdateAction, &QAction::triggered, this, &UpdateMenuManager::onCheckForUpdates);

    // About version action
    m_aboutVersionAction = new QAction(tr("À propos de la &version"), this);
    m_aboutVersionAction->setStatusTip(tr("Afficher les informations sur la version actuelle"));
    connect(m_aboutVersionAction, &QAction::triggered, this, [this]() {
        QString currentVer = UpdateChecker::currentVersion();
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Version actuelle"),
            tr("Version installée: %1\n\nPour vérifier les mises à jour, utilisez le menu \"Vérifier les mises à jour\".").arg(currentVer)
        );
    });
}

void UpdateMenuManager::onCheckForUpdates()
{
    qInfo() << "Manual update check requested";

    // Disable the button during the check
    m_checkUpdateAction->setEnabled(false);
    m_checkUpdateAction->setText(tr("Checking for updates..."));

    // Create a progress dialog
    if (m_progressDialog) {
        delete m_progressDialog;
    }
    
    m_progressDialog = new QProgressDialog(
        tr("Vérification des mises à jour en cours..."),
        tr("Annuler"),
        0, 0,  // Indéterminé
        qobject_cast<QWidget*>(parent())
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->show();
    
    // Connect cancel button (optional)
    connect(m_progressDialog, &QProgressDialog::canceled, this, [this]() {
        qInfo() << "Update check canceled by user";
        m_checkUpdateAction->setEnabled(true);
        m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));
    });
    
    // Launch the update check
    qInfo() << "Starting update check...";
    m_updateChecker->checkForUpdates();
}

void UpdateMenuManager::onUpdateAvailable(const QString& version, const QString& downloadUrl)
{
    // Close the progress dialog
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Reactivate the button
    m_checkUpdateAction->setEnabled(true);
    m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));

    // Save update information
    m_latestVersion = version;
    m_downloadUrl = downloadUrl;

    qInfo() << "Update available:" << version;

    // Show the update dialog with more information
    QMessageBox msgBox(qobject_cast<QWidget*>(parent()));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle(tr("Mise à jour disponible"));
    msgBox.setText(tr("Une nouvelle version est disponible !"));
    msgBox.setInformativeText(tr("Version actuelle: %1\nNouvelle version: %2\n\n"
                                 "Voulez-vous télécharger et installer la mise à jour maintenant ?\n\n"
                                 "📥 Téléchargement automatique depuis GitHub\n"
                                 "🔄 Installation automatique\n"
                                 "🔒 Vos données seront préservées (config, marketData, logs)\n"
                                 "⚠️ L'application redémarrera automatiquement")
                             .arg(UpdateChecker::currentVersion())
                             .arg(version));
    
    QPushButton* downloadButton = msgBox.addButton(tr("Télécharger et installer"), QMessageBox::ActionRole);
    QPushButton* laterButton = msgBox.addButton(tr("Plus tard"), QMessageBox::RejectRole);
    QPushButton* viewButton = msgBox.addButton(tr("Voir sur GitHub"), QMessageBox::ActionRole);
    
    msgBox.setDefaultButton(downloadButton);
    
    msgBox.exec();
    
    QAbstractButton* clickedBtn = msgBox.clickedButton();

    if (clickedBtn == static_cast<QAbstractButton*>(downloadButton)) {
        // Create a progress dialog for the download
        if (m_progressDialog) {
            delete m_progressDialog;
        }
        
        m_progressDialog = new QProgressDialog(
            tr("Préparation du téléchargement..."),
            tr("Annuler"),
            0, 100,
            qobject_cast<QWidget*>(parent())
        );
        m_progressDialog->setWindowModality(Qt::WindowModal);
        m_progressDialog->setMinimumDuration(0);
        m_progressDialog->setAutoClose(false);
        m_progressDialog->setAutoReset(false);
        m_progressDialog->show();
        
        // Connect cancel signal properly to abort the download
        connect(m_progressDialog, &QProgressDialog::canceled, this, [this]() {
            qInfo() << "Download canceled by user";
            if (m_updateChecker) {
                m_updateChecker->abortDownload();
            }
            if (m_progressDialog) {
                m_progressDialog->close();
                delete m_progressDialog;
                m_progressDialog = nullptr;
            }
        });
        
        // Download and install with the new system
        m_updateChecker->downloadAndInstallUpdate(m_downloadUrl);
    } else if (clickedBtn == static_cast<QAbstractButton*>(viewButton)) {
        // Open the release page on GitHub Pages (with download section)
        QString releaseUrl = GITHUB_PAGES_RELEASES_URL.toString() + "#download";

        #ifdef Q_OS_WIN
        QProcess::startDetached("cmd", {"/c", "start", releaseUrl});
        #elif defined(Q_OS_MAC)
        QProcess::startDetached("open", {releaseUrl});
        #else // Linux
        QProcess::startDetached("xdg-open", {releaseUrl});
        #endif
    }
    // If "Later" is clicked, do nothing
}

void UpdateMenuManager::onNoUpdateAvailable()
{
    // Close the progress dialog
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Reactivate the button
    m_checkUpdateAction->setEnabled(true);
    m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));
    
    qInfo() << "No update available";

    // Show an informative message
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Aucune mise à jour"),
        tr("Vous utilisez déjà la dernière version (%1).").arg(UpdateChecker::currentVersion())
    );
}

void UpdateMenuManager::onUpdateCheckFailed(const QString& error)
{
    // Close the progress dialog
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Reactivate the button
    m_checkUpdateAction->setEnabled(true);
    m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));
    
    qWarning() << "Update check failed:" << error;

    // Show an error message
    QMessageBox::warning(
        qobject_cast<QWidget*>(parent()),
        tr("Erreur de vérification"),
        tr("Impossible de vérifier les mises à jour.\n\nErreur: %1\n\nVérifiez votre connexion internet et réessayez.").arg(error)
    );
}

void UpdateMenuManager::onDownloadProgress(int percentage)
{
    qDebug() << "UpdateMenuManager received download progress:" << percentage << "%";
    
    if (m_progressDialog && !m_progressDialog->wasCanceled()) {
        m_progressDialog->setLabelText(tr("Téléchargement en cours... %1%").arg(percentage));
        m_progressDialog->setValue(percentage);
        m_progressDialog->setMaximum(100);
        qDebug() << "Progress dialog updated to" << percentage << "%";
    } else {
        qDebug() << "Progress dialog not available or canceled";
    }
}

void UpdateMenuManager::onUpdateCompleted()
{
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    // Show a confirmation message with countdown
    QMessageBox msgBox(qobject_cast<QWidget*>(parent()));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle(tr("Mise à jour terminée"));
    msgBox.setText(tr("Installation réussie !"));
    msgBox.setInformativeText(tr("La mise à jour a été installée avec succès.\n\n"
                                 "✅ Nouvelle version installée: v%1\n"
                                 "💾 Vos données ont été préservées\n"
                                 "🔄 L'application va se fermer dans 5 secondes\n\n"
                                 "Cliquez sur OK pour fermer maintenant.")
                             .arg(m_latestVersion));
    msgBox.setStandardButtons(QMessageBox::Ok);

    // Create a timer to automatically close after 5 seconds
    QTimer* timer = new QTimer(&msgBox);
    int countdown = 5;
    
    connect(timer, &QTimer::timeout, [&msgBox, &countdown, timer]() {
        countdown--;
        if (countdown > 0) {
            msgBox.setInformativeText(tr("La mise à jour a été installée avec succès.\n\n"
                                         "✅ Nouvelle version installée\n"
                                         "💾 Vos données ont été préservées\n"
                                         "🔄 L'application va se fermer dans %1 secondes\n\n"
                                         "Cliquez sur OK pour fermer maintenant.")
                                     .arg(countdown));
        } else {
            timer->stop();
            msgBox.accept();
        }
    });
    
    timer->start(1000); // 1 second
    msgBox.exec();

    // Close the application
    QApplication::quit();
}

void UpdateMenuManager::onUpdateFailed(const QString& error)
{
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    
    QMessageBox::critical(
        qobject_cast<QWidget*>(parent()),
        tr("Échec de la mise à jour"),
        tr("Impossible de télécharger ou d'installer la mise à jour.\n\nErreur: %1").arg(error)
    );
}

// Add missing methods
void UpdateMenuManager::checkForUpdatesAuto()
{
    // Silent check (without UI)

    // Temporarily disconnect signals to avoid showing UI
    disconnect(m_updateChecker, &UpdateChecker::updateAvailable, 
               this, &UpdateMenuManager::onUpdateAvailable);
    disconnect(m_updateChecker, &UpdateChecker::noUpdateAvailable, 
               this, &UpdateMenuManager::onNoUpdateAvailable);
    disconnect(m_updateChecker, &UpdateChecker::updateCheckFailed, 
               this, &UpdateMenuManager::onUpdateCheckFailed);

    // Connect temporary handlers
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this, 
        [this](const QString& version, const QString& downloadUrl) {
            // Reconnect to be able to use the menu afterwards
            reconnectSignals();

            // Show a non-blocking notification
            QWidget* parent = qobject_cast<QWidget*>(this->parent());
            if (parent) {
                QMessageBox* notification = new QMessageBox(
                    QMessageBox::Information,
                    tr("Mise à jour disponible"),
                    tr("Une nouvelle version (%1) est disponible.\nUtilisez le menu Mise à jour pour l'installer.")
                        .arg(version),
                    QMessageBox::Ok,
                    parent
                );
                notification->setModal(false);
                notification->show();
            }
        }, Qt::SingleShotConnection);
    
    connect(m_updateChecker, &UpdateChecker::noUpdateAvailable, this,
        [this]() {
            // Simply reconnect signals without notification
            reconnectSignals();
        }, Qt::SingleShotConnection);
    
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed, this,
        [this](const QString& error) {
            // Reconnect signals
            reconnectSignals();
            // Log the error silently
            qWarning() << "Automatic check failed:" << error;
        }, Qt::SingleShotConnection);

    // Start the silent check
    m_updateChecker->checkForUpdates();
}

void UpdateMenuManager::reconnectSignals()
{
    // Reconnect standard signals
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &UpdateMenuManager::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::noUpdateAvailable,
            this, &UpdateMenuManager::onNoUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed,
            this, &UpdateMenuManager::onUpdateCheckFailed);
}