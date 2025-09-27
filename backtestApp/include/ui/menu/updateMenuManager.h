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

private slots:
    void onTestDownload();
    void onDownloadCompleted(bool success, const QString& filePath);
    void onDownloadError(const QString& errorMessage);

private:
    QAction* m_testDownloadAction;
    UpdateChecker* m_updateChecker;

};