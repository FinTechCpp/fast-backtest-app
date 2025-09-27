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

static const QUrl GITHUB_PAGES_RELEASES_URL = QUrl(QStringLiteral("https://fintechcpp.github.io/fast-backtest-app-releases/"));

UpdateMenuManager::UpdateMenuManager(QObject* parent)
    : QObject(parent)
    , m_testDownloadAction(nullptr)
    , m_updateChecker(nullptr)
{
    qDebug() << "UpdateMenuManager constructor called";
    
    // Créer l'instance UpdateChecker
    m_updateChecker = new UpdateChecker(this);
    
    // Connecter les signaux
    connect(m_updateChecker, &UpdateChecker::downloadCompleted,
            this, &UpdateMenuManager::onDownloadCompleted);
    connect(m_updateChecker, &UpdateChecker::downloadError,
            this, &UpdateMenuManager::onDownloadError);
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
    
    // Créer l'action de test de téléchargement
    m_testDownloadAction = new QAction(tr("Test Téléchargement Release"), this);
    m_testDownloadAction->setStatusTip(tr("Tester le téléchargement de la dernière release"));
    
    // Connecter l'action
    connect(m_testDownloadAction, &QAction::triggered, 
            this, &UpdateMenuManager::onTestDownload);
    
    // Ajouter l'action au menu
    updateMenu->addAction(m_testDownloadAction);
    
    qDebug() << "Update menu created successfully";
}

void UpdateMenuManager::onTestDownload()
{
    qDebug() << "Test download action triggered";
    
    if (!m_updateChecker) {
        qCritical() << "UpdateChecker is null!";
        QMessageBox::critical(nullptr, tr("Erreur"), 
                             tr("Erreur interne: UpdateChecker non initialisé"));
        return;
    }
    
    // Informer l'utilisateur du début du téléchargement
    QMessageBox::information(nullptr, tr("Téléchargement"), 
                            tr("Début du test de téléchargement.\nVoir la console pour les logs détaillés."));
    
    // Démarrer le téléchargement
    qDebug() << "Starting download test from UpdateMenuManager...";
    m_updateChecker->downloadLatestRelease();
}

void UpdateMenuManager::onDownloadCompleted(bool success, const QString& filePath)
{
    qDebug() << "Download completed with success:" << success << "File path:" << filePath;
    
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
    
    QMessageBox::critical(nullptr, tr("Erreur de téléchargement"), 
                         tr("Erreur lors du téléchargement:\n%1").arg(errorMessage));
}
