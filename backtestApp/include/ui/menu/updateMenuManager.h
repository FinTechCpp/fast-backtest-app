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
 * @brief Update menu manager
 *
 * This class manages the "Update" menu in the main menu bar
 * with actions to check for and manage updates.
 */
class UpdateMenuManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateMenuManager(QObject* parent = nullptr);
    ~UpdateMenuManager();

    /**
     * @brief Creates the Update menu in the menu bar
     * @param menuBar The menu bar to add the menu to
     */
    void createUpdateMenu(QMenuBar* menuBar);

private slots:
    void onCheckForUpdates();
    void onUpdateCheckCompleted(bool updateAvailable, const QString& latestVersion, const QString& currentVersion);
    void onUpdateCheckError(const QString& errorMessage);
    void onDownloadCompleted(bool success, const QString& filePath);
    void onDownloadError(const QString& errorMessage);
    void onInstallationProgress(const QString& message);
    void onInstallationCompleted(bool success, const QString& message);
    void onInstallationError(const QString& errorMessage);

private:
    QAction* m_checkForUpdatesAction;
    UpdateChecker* m_updateChecker;
    QString m_pendingDownloadVersion;  // Version to download if user agrees
    QProgressDialog* m_progressDialog; // Dialogue de progression pour l'installation

};