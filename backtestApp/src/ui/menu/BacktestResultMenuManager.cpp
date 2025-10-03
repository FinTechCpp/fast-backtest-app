#include "ui/menu/BacktestResultMenuManager.h"
#include "ui/app.h"
#include "components/Managers/BacktestResultManager.h"
#include <QDebug>
#include <QMessageBox>
#include <QCoreApplication>
#include <QInputDialog>
#include <QTimer>

BacktestResultMenuManager::BacktestResultMenuManager(App* parent)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_resultManager(nullptr)
    , m_resultMenu(nullptr)
    , m_loadResultSubmenu(nullptr)
    , m_saveResultAction(nullptr)
    , m_deleteResultAction(nullptr)
    , m_importAction(nullptr)
    , m_exportAction(nullptr)
    , m_openDirectoryAction(nullptr)
    , m_viewDetailsAction(nullptr)
{
    qDebug() << "BacktestResultMenuManager créé";
}

void BacktestResultMenuManager::createResultMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar null passé à createResultMenu";
        return;
    }
    
    // Créer le menu Résultats (avant le menu Aide)
    m_resultMenu = menuBar->addMenu(tr("&Résultats"));
    
    createActions();
    
    // Ajouter les actions au menu
    m_resultMenu->addAction(m_saveResultAction);
    m_resultMenu->addAction(m_viewDetailsAction);
    m_resultMenu->addAction(m_deleteResultAction);
    m_resultMenu->addSeparator();
    
    // Créer le sous-menu pour charger les résultats
    m_loadResultSubmenu = m_resultMenu->addMenu(tr("&Charger un résultat"));
    
    m_resultMenu->addSeparator();
    m_resultMenu->addAction(m_importAction);
    m_resultMenu->addAction(m_exportAction);

    // Ajouter un séparateur puis l'action pour ouvrir le dossier
    m_resultMenu->addSeparator();
    m_resultMenu->addAction(m_openDirectoryAction);
    
    qDebug() << "Menu Résultats créé";
}

void BacktestResultMenuManager::createActions()
{
    // Action Sauvegarder
    m_saveResultAction = new QAction(tr("&Sauvegarder le résultat actuel"), this);
    m_saveResultAction->setStatusTip(tr("Sauvegarder le résultat de backtest actuel"));
    connect(m_saveResultAction, &QAction::triggered, this, &BacktestResultMenuManager::onSaveCurrentResult);
    
    // Action Voir détails
    m_viewDetailsAction = new QAction(tr("&Voir les détails..."), this);
    m_viewDetailsAction->setStatusTip(tr("Afficher les détails du résultat"));
    connect(m_viewDetailsAction, &QAction::triggered, this, &BacktestResultMenuManager::onViewResultDetails);
    
    // Action Supprimer résultat
    m_deleteResultAction = new QAction(tr("&Supprimer le résultat"), this);
    m_deleteResultAction->setStatusTip(tr("Supprimer le résultat sélectionné"));
    connect(m_deleteResultAction, &QAction::triggered, this, &BacktestResultMenuManager::onDeleteResult);
    
    // Action Importer
    m_importAction = new QAction(tr("&Importer..."), this);
    m_importAction->setStatusTip(tr("Importer un résultat de backtest"));
    connect(m_importAction, &QAction::triggered, this, &BacktestResultMenuManager::onImportResult);
    
    // Action Exporter
    m_exportAction = new QAction(tr("&Exporter..."), this);
    m_exportAction->setStatusTip(tr("Exporter le résultat actuel"));
    connect(m_exportAction, &QAction::triggered, this, &BacktestResultMenuManager::onExportResult);
    
    // Action - Ouvrir le dossier des résultats
    m_openDirectoryAction = new QAction(tr("&Ouvrir le dossier des résultats"), this);
    m_openDirectoryAction->setStatusTip(tr("Ouvrir le dossier contenant les fichiers de résultats"));
    connect(m_openDirectoryAction, &QAction::triggered, this, &BacktestResultMenuManager::onOpenResultsDirectory);
    
    // Désactiver les actions qui nécessitent un résultat chargé
    m_viewDetailsAction->setEnabled(false);
    m_deleteResultAction->setEnabled(false);
    m_exportAction->setEnabled(false);
}

void BacktestResultMenuManager::setResultManager(BacktestResultManager* resultManager)
{
    m_resultManager = resultManager;
    if (m_resultManager) {
        qDebug() << "Connexion du BacktestResultManager au BacktestResultMenuManager";
        
        // Connecter les signaux
        connect(m_resultManager, &BacktestResultManager::resultListUpdated,
                this, &BacktestResultMenuManager::updateResultList);
        
        qDebug() << "Signaux connectés, mise à jour initiale de la liste des résultats";
        
        // Utiliser un timer pour s'assurer que tout est initialisé
        QTimer::singleShot(100, this, [this]() {
            qDebug() << "Mise à jour différée de la liste des résultats";
            updateResultList();
        });
        
        qDebug() << "BacktestResultManager connecté au BacktestResultMenuManager";
    } else {
        qWarning() << "BacktestResultManager null passé à setResultManager";
    }
}

void BacktestResultMenuManager::updateResultList()
{
    qDebug() << "updateResultList() appelé";
    
    if (!m_resultManager || !m_loadResultSubmenu) {
        qWarning() << "BacktestResultManager ou LoadResultSubmenu manquant";
        return;
    }
    
    // Nettoyer les actions existantes
    for (auto action : m_resultActions.values()) {
        m_loadResultSubmenu->removeAction(action);
        delete action;
    }
    m_resultActions.clear();
    
    // Ajouter les résultats disponibles
    QStringList results = m_resultManager->listBacktestResults();
    
    qDebug() << "Résultats récupérés:" << results;
    
    if (results.isEmpty()) {
        QAction* noResultsAction = new QAction(tr("(Aucun résultat)"), this);
        noResultsAction->setEnabled(false);
        m_loadResultSubmenu->addAction(noResultsAction);
    } else {
        for (const QString& result : results) {
            QAction* action = new QAction(result, this);
            action->setCheckable(true);
            action->setChecked(result == m_currentResult);
            action->setData(result);
            
            connect(action, &QAction::triggered, this, &BacktestResultMenuManager::onLoadResult);
            
            m_loadResultSubmenu->addAction(action);
            m_resultActions[result] = action;
            
            qDebug() << "Action créée pour le résultat:" << result;
        }
    }
    
    qDebug() << "Liste des résultats mise à jour avec" << results.size() << "résultats";
}

void BacktestResultMenuManager::onResultLoaded(const QString& resultName)
{
    // Mettre à jour les coches dans le menu
    for (auto it = m_resultActions.begin(); it != m_resultActions.end(); ++it) {
        it.value()->setChecked(it.key() == resultName);
    }
    
    // Mettre à jour le nom du résultat courant
    m_currentResult = resultName;
    
    // Activer les actions qui nécessitent un résultat chargé
    bool hasResult = !resultName.isEmpty();
    m_viewDetailsAction->setEnabled(hasResult);
    m_deleteResultAction->setEnabled(hasResult);
    m_exportAction->setEnabled(hasResult);
    
    qDebug() << "Résultat actuel mis à jour vers:" << resultName;
}

void BacktestResultMenuManager::onSaveCurrentResult()
{
    if (!m_resultManager || !m_mainWindow) {
        return;
    }
    
    // Créer un nouveau résultat à partir des statistiques actuelles
    // Cela suppose que l'App a une méthode pour récupérer les statistiques actuelles
    // Exemple : be::Stats currentStats = m_mainWindow->getCurrentStats();

    // Pour cet exemple, nous allons supposer que tu as une méthode dans App pour créer une config
    // BacktestResultConfig config = m_mainWindow->createBacktestResultConfig();
    
    bool ok;
    QString name = QInputDialog::getText(m_mainWindow, tr("Nom du résultat"),
                                        tr("Entrez un nom pour ce résultat:"),
                                        QLineEdit::Normal, "", &ok);
    
    if (ok && !name.isEmpty()) {
        // Créer une configuration de résultat
        BacktestResultConfig config;
        config.name = name.toStdString();
        config.version = QCoreApplication::applicationVersion().toStdString();
        config.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();

        BacktestResults currentResults = m_mainWindow->getBacktestResults();

        config.generalParams = currentResults.generalConfig;
        config.strategyConfig = currentResults.strategyConfig;
        config.candles = currentResults.candles;
        config.stats = currentResults.stats;

        // Sauvegarder le résultat
        m_resultManager->saveBacktestResult(config, m_mainWindow);
        // m_resultManager->saveBacktestResult(config, m_mainWindow, SerializationUtils::FileFormat::JSON);
    }
}

void BacktestResultMenuManager::onDeleteResult()
{
    if (m_resultManager && !m_currentResult.isEmpty()) {
        m_resultManager->deleteBacktestResult(m_currentResult, m_mainWindow);
        m_currentResult.clear();
        
        // Mettre à jour l'interface
        onResultLoaded(QString());
    }
}

void BacktestResultMenuManager::onImportResult()
{
    if (m_resultManager) {
        if (m_resultManager->importBacktestResult(m_mainWindow)) {
            updateResultList();
        }
    }
}

void BacktestResultMenuManager::onExportResult()
{
    if (m_resultManager && !m_currentResult.isEmpty()) {
        m_resultManager->exportBacktestResult(m_currentResult, m_mainWindow);
    }
}

void BacktestResultMenuManager::onLoadResult()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && m_resultManager) {
        QString resultName = action->data().toString();
        if (!resultName.isEmpty()) {
            // Charger le résultat
            BacktestResultConfig config;
            if (m_resultManager->loadBacktestResult(resultName, config)) {
                // Mettre à jour l'interface avec les statistiques chargées
                // m_mainWindow->setCurrentStats(config.stats);

                std::unique_ptr<BacktestResults> results = std::make_unique<BacktestResults>();

                results->generalConfig = config.generalParams;
                results->strategyConfig = config.strategyConfig;
                results->candles = config.candles;
                results->stats = config.stats;

                m_mainWindow->setGeneralParamsConfig(config.generalParams);
                m_mainWindow->setStrategyConfig(config.strategyConfig);
                m_mainWindow->setBacktestResults(std::move(results));

                // Mettre à jour l'état du menu
                onResultLoaded(resultName);
                
                QMessageBox::information(m_mainWindow, tr("Résultat chargé"),
                                        tr("Le résultat '%1' a été chargé.").arg(resultName));
            }
        }
    }
}

void BacktestResultMenuManager::onOpenResultsDirectory()
{
    if (m_resultManager) {
        if (!m_resultManager->openBacktestResultsDirectory()) {
            // Afficher un message d'erreur en cas d'échec
            QMessageBox::warning(m_mainWindow, tr("Erreur"),
                                tr("Impossible d'ouvrir le dossier des résultats.\n"
                                   "Chemin: %1").arg(m_resultManager->getBacktestResultsDirectory()));
        }
    }
}

void BacktestResultMenuManager::onViewResultDetails()
{
    if (!m_resultManager || m_currentResult.isEmpty()) {
        return;
    }
    
    // Charger les détails du résultat
    BacktestResultConfig config;
    if (m_resultManager->loadBacktestResult(m_currentResult, config)) {
        // Afficher les détails du résultat dans une boîte de dialogue
        QString details = tr("Nom: %1\n"
                           "Version: %2\n"
                           "Date de création: %3\n\n"
                           "Statistiques:\n"
                           "- Rendement: %4%\n"
                           "- Transactions: %5\n"
                           "- Sharpe: %6\n"
                           "- Drawdown max: %7%")
                         .arg(QString::fromStdString(config.name))
                         .arg(QString::fromStdString(config.version))
                         .arg(QString::fromStdString(config.createdAt))
                         .arg(config.stats.returnPct, 0, 'f', 2)
                         .arg(config.stats.numTrades)
                         .arg(config.stats.sharpeRatio, 0, 'f', 2)
                         .arg(config.stats.maxDrawdownPct, 0, 'f', 2);
                         
        QMessageBox::information(m_mainWindow, tr("Détails du résultat"), details);
    }
}