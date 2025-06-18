#pragma once

#include <QObject>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QProgressDialog>

// Forward declaration
class UpdateChecker;

/**
 * @brief Gestionnaire de menu des mises à jour
 * 
 * Cette classe gère le menu "Mise à jour" dans la barre de menu principale
 * avec les actions pour vérifier et gérer les mises à jour.
 */
class UpdateMenuManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateMenuManager(QObject* parent = nullptr);
    ~UpdateMenuManager();

    /**
     * @brief Crée le menu Mise à jour dans la barre de menu
     * @param menuBar La barre de menu où ajouter le menu
     */
    void createUpdateMenu(QMenuBar* menuBar);

    // Ajouter cette méthode
    void checkForUpdatesAuto();

private slots:
    void onCheckForUpdates();
    void onUpdateAvailable(const QString& version, const QString& downloadUrl);
    void onNoUpdateAvailable();
    void onUpdateCheckFailed(const QString& error);
    void onDownloadProgress(int percentage);
    void onUpdateCompleted();
    void onUpdateFailed(const QString& error);

private:
    void createActions();
    void reconnectSignals();
    
    // Menu et actions
    QMenu* m_updateMenu;
    QAction* m_checkUpdateAction;
    QAction* m_aboutVersionAction;
    
    // Composants de mise à jour
    UpdateChecker* m_updateChecker;
    QProgressDialog* m_progressDialog;
    
    // Variables d'état
    QString m_latestVersion;
    QString m_downloadUrl;
};