#include "menu/update_menu_manager.h"
#include "components/UpdateChecker.h"
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QProcess> 
#include <QPushButton>
#include <QAbstractButton>

UpdateMenuManager::UpdateMenuManager(QObject* parent)
    : QObject(parent),
      m_updateMenu(nullptr),
      m_checkUpdateAction(nullptr),
      m_aboutVersionAction(nullptr),
      m_updateChecker(nullptr),
      m_progressDialog(nullptr)
{
    // Créer l'instance d'UpdateChecker
    m_updateChecker = new UpdateChecker(this);
    
    // Connecter les signaux
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
        qWarning() << "MenuBar null passé à createUpdateMenu";
        return;
    }
    
    // Créer le menu Mise à jour (avant le menu Aide)
    m_updateMenu = menuBar->addMenu(tr("&Mise à jour"));
    
    createActions();
    
    // Ajouter les actions au menu
    m_updateMenu->addAction(m_checkUpdateAction);
    m_updateMenu->addSeparator();
    m_updateMenu->addAction(m_aboutVersionAction);
    
    qDebug() << "Menu Mise à jour créé";
}

void UpdateMenuManager::createActions()
{
    // Action Vérifier les mises à jour
    m_checkUpdateAction = new QAction(tr("&Vérifier les mises à jour"), this);
    m_checkUpdateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    m_checkUpdateAction->setStatusTip(tr("Vérifier s'il existe une nouvelle version disponible"));
    connect(m_checkUpdateAction, &QAction::triggered, this, &UpdateMenuManager::onCheckForUpdates);
    
    // Action À propos de la version
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
    qInfo() << "Vérification manuelle des mises à jour demandée";
    
    // Désactiver le bouton pendant la vérification
    m_checkUpdateAction->setEnabled(false);
    m_checkUpdateAction->setText(tr("Vérification en cours..."));
    
    // Créer une boîte de dialogue de progression
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
    
    // Connecter le bouton annuler (optionnel)
    connect(m_progressDialog, &QProgressDialog::canceled, this, [this]() {
        qInfo() << "Vérification des mises à jour annulée par l'utilisateur";
        m_checkUpdateAction->setEnabled(true);
        m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));
    });
    
    // Lancer la vérification
    m_updateChecker->checkForUpdates();
}

void UpdateMenuManager::onUpdateAvailable(const QString& version, const QString& downloadUrl)
{
    // Fermer la boîte de dialogue de progression
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    
    // Réactiver le bouton
    m_checkUpdateAction->setEnabled(true);
    m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));
    
    // Sauvegarder les informations de mise à jour
    m_latestVersion = version;
    m_downloadUrl = downloadUrl;
    
    qInfo() << "Mise à jour disponible:" << version;
    
    // Afficher la boîte de dialogue de mise à jour avec plus d'informations
    QMessageBox msgBox(qobject_cast<QWidget*>(parent()));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle(tr("Mise à jour disponible"));
    msgBox.setText(tr("Une nouvelle version est disponible !"));
    msgBox.setInformativeText(tr("Version actuelle: %1\nNouvelle version: %2\n\nVoulez-vous télécharger et installer la mise à jour maintenant ?\n\n⚠️ L'application redémarrera automatiquement après l'installation.\n✅ Vos configurations seront préservées.")
                             .arg(UpdateChecker::currentVersion())
                             .arg(version));
    
    QPushButton* downloadButton = msgBox.addButton(tr("Télécharger et installer"), QMessageBox::ActionRole);
    QPushButton* laterButton = msgBox.addButton(tr("Plus tard"), QMessageBox::RejectRole);
    QPushButton* viewButton = msgBox.addButton(tr("Voir sur GitHub"), QMessageBox::ActionRole);
    
    msgBox.setDefaultButton(downloadButton);
    
    msgBox.exec();
    
    QAbstractButton* clickedBtn = msgBox.clickedButton();

    if (clickedBtn == static_cast<QAbstractButton*>(downloadButton)) {
        // Télécharger et installer avec le nouveau système
        m_updateChecker->downloadAndInstallUpdate();
    } else if (clickedBtn == static_cast<QAbstractButton*>(viewButton)) {
        // Ouvrir la page de release sur GitHub
        QString releaseUrl = QString("https://github.com/hugoMiCode/ig-trading-bot/releases/tag/v%1").arg(version);
        
        #ifdef Q_OS_WIN
        QProcess::startDetached("cmd", {"/c", "start", releaseUrl});
        #elif defined(Q_OS_MAC)
        QProcess::startDetached("open", {releaseUrl});
        #else // Linux
        QProcess::startDetached("xdg-open", {releaseUrl});
        #endif
    }
    // Si "Plus tard" est cliqué, ne rien faire
}

void UpdateMenuManager::onNoUpdateAvailable()
{
    // Fermer la boîte de dialogue de progression
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    
    // Réactiver le bouton
    m_checkUpdateAction->setEnabled(true);
    m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));
    
    qInfo() << "Aucune mise à jour disponible";
    
    // Afficher un message informatif
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Aucune mise à jour"),
        tr("Vous utilisez déjà la dernière version (%1).").arg(UpdateChecker::currentVersion())
    );
}

void UpdateMenuManager::onUpdateCheckFailed(const QString& error)
{
    // Fermer la boîte de dialogue de progression
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    
    // Réactiver le bouton
    m_checkUpdateAction->setEnabled(true);
    m_checkUpdateAction->setText(tr("&Vérifier les mises à jour"));
    
    qWarning() << "Échec de la vérification des mises à jour:" << error;
    
    // Afficher un message d'erreur
    QMessageBox::warning(
        qobject_cast<QWidget*>(parent()),
        tr("Erreur de vérification"),
        tr("Impossible de vérifier les mises à jour.\n\nErreur: %1\n\nVérifiez votre connexion internet et réessayez.").arg(error)
    );
}

void UpdateMenuManager::onDownloadProgress(int percentage)
{
    if (m_progressDialog) {
        m_progressDialog->setLabelText(tr("Téléchargement en cours... %1%").arg(percentage));
        m_progressDialog->setValue(percentage);
        m_progressDialog->setMaximum(100);
    }
}

void UpdateMenuManager::onUpdateCompleted()
{
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Mise à jour terminée"),
        tr("La mise à jour a été téléchargée avec succès.\nL'application va maintenant se fermer pour permettre l'installation.")
    );
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

// Ajoutez les méthodes qui manquent
void UpdateMenuManager::checkForUpdatesAuto()
{
    // Vérification silencieuse (sans UI)
    
    // Déconnecter temporairement les signaux pour éviter d'afficher l'UI
    disconnect(m_updateChecker, &UpdateChecker::updateAvailable, 
               this, &UpdateMenuManager::onUpdateAvailable);
    disconnect(m_updateChecker, &UpdateChecker::noUpdateAvailable, 
               this, &UpdateMenuManager::onNoUpdateAvailable);
    disconnect(m_updateChecker, &UpdateChecker::updateCheckFailed, 
               this, &UpdateMenuManager::onUpdateCheckFailed);
    
    // Connecter des gestionnaires temporaires
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this, 
        [this](const QString& version, const QString& downloadUrl) {
            // Reconnexion pour pouvoir utiliser le menu après
            reconnectSignals();
            
            // Afficher une notification non-bloquante
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
            // Simplement reconnecter les signaux sans notification
            reconnectSignals();
        }, Qt::SingleShotConnection);
    
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed, this,
        [this](const QString& error) {
            // Reconnecter les signaux
            reconnectSignals();
            // Log l'erreur silencieusement
            qWarning() << "Échec de la vérification automatique:" << error;
        }, Qt::SingleShotConnection);
    
    // Lancer la vérification silencieuse
    m_updateChecker->checkForUpdates();
}

void UpdateMenuManager::reconnectSignals()
{
    // Reconnecter les signaux standard
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &UpdateMenuManager::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::noUpdateAvailable,
            this, &UpdateMenuManager::onNoUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed,
            this, &UpdateMenuManager::onUpdateCheckFailed);
}