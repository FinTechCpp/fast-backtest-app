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
{
    qDebug() << "UpdateMenuManager constructor called";
    
    // Créer l'instance UpdateChecker
    m_updateChecker = new UpdateChecker(this);
    
    // Connecter les signaux pour les téléchargements
    connect(m_updateChecker, &UpdateChecker::downloadCompleted,
            this, &UpdateMenuManager::onDownloadCompleted);
    connect(m_updateChecker, &UpdateChecker::downloadError,
            this, &UpdateMenuManager::onDownloadError);
            
    // Connecter les signaux pour la vérification de mise à jour
    connect(m_updateChecker, &UpdateChecker::updateCheckCompleted,
            this, &UpdateMenuManager::onUpdateCheckCompleted);
    connect(m_updateChecker, &UpdateChecker::updateCheckError,
            this, &UpdateMenuManager::onUpdateCheckError);
}

UpdateMenuManager::~UpdateMenuManager()
{
    qDebug() << "UpdateMenuManager destructor called";
}

void UpdateMenuManager::createUpdateMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar is null, cannot create update menu";
        return;
    }
    
    qDebug() << "Creating update menu...";
    
    // Créer le menu Mise à jour
    QMenu* updateMenu = menuBar->addMenu(tr("&Mise à jour"));

    // Créer l'action de vérification de mise à jour (nouvelle fonctionnalité)
    m_checkForUpdatesAction = new QAction(tr("Vérification mise à jour"), this);
    m_checkForUpdatesAction->setStatusTip(tr("Vérifier si une nouvelle version de l'app est disponible"));
    
    // Connecter les actions
    connect(m_checkForUpdatesAction, &QAction::triggered, 
            this, &UpdateMenuManager::onCheckForUpdates);

    // Ajouter les actions au menu (vérification en premier)
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
    
    // Démarrer la vérification de mise à jour
    qDebug() << "Starting update check from UpdateMenuManager...";
    m_updateChecker->checkForUpdates();
}

void UpdateMenuManager::onUpdateCheckCompleted(bool updateAvailable, const QString& latestVersion, const QString& currentVersion)
{
    qDebug() << "Update check completed - Available:" << updateAvailable 
             << "Latest:" << latestVersion << "Current:" << currentVersion;
    
    if (updateAvailable) {
        // Stocker la version à télécharger
        m_pendingDownloadVersion = latestVersion;
        
        // Une mise à jour est disponible, proposer le téléchargement
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(tr("Mise à jour disponible"));
        msgBox.setText(tr("Une nouvelle version est disponible !"));
        msgBox.setInformativeText(tr("Version actuelle: %1\nNouvelle version: %2\n\nVoulez-vous télécharger la mise à jour ?")
                                  .arg(currentVersion)
                                  .arg(latestVersion));
        
        QPushButton* downloadButton = msgBox.addButton(tr("Télécharger"), QMessageBox::AcceptRole);
        QPushButton* laterButton = msgBox.addButton(tr("Plus tard"), QMessageBox::RejectRole);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == downloadButton) {
            // L'utilisateur veut télécharger - utiliser la version stockée
            qDebug() << "User chose to download the update, version:" << m_pendingDownloadVersion;
            m_updateChecker->downloadLatestRelease(m_pendingDownloadVersion);
        } else {
            qDebug() << "User chose to skip the update";
            m_pendingDownloadVersion.clear();  // Nettoyer
        }
    } else {
        // Pas de mise à jour disponible
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
    
    // Nettoyer la version en attente
    m_pendingDownloadVersion.clear();
    
    if (success) {
        QMessageBox::information(nullptr, tr("Succès"), 
                                tr("Téléchargement réussi!\nFichier sauvé: %1").arg(filePath));
    } else {
        QMessageBox::warning(nullptr, tr("Échec"), 
                            tr("Le téléchargement a échoué ou le fichier est vide."));
    }
}

void UpdateMenuManager::onDownloadError(const QString& errorMessage)
{
    qDebug() << "Download error:" << errorMessage;
    
    // Nettoyer la version en attente
    m_pendingDownloadVersion.clear();
    
    QMessageBox::critical(nullptr, tr("Erreur de téléchargement"), 
                         tr("Erreur lors du téléchargement:\n%1").arg(errorMessage));
}
