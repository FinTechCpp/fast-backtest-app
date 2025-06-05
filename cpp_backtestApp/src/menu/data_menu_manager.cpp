#include "menu/data_menu_manager.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include "components/data_loader.h"

DataMenuManager::DataMenuManager(QObject* parent)
    : QObject(parent)
    , m_dataMenu(nullptr)
    , m_importCSVAction(nullptr)
    , m_importAPIAction(nullptr)
    , m_validateDataAction(nullptr)
    , m_cleanDataAction(nullptr)
    , m_dataInfoAction(nullptr)
    , m_setDirectoryAction(nullptr)
{
}

DataMenuManager::~DataMenuManager()
{
    qDebug() << "DataMenuManager destructor called";
}

void DataMenuManager::createDataMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar null passé à createDataMenu";
        return;
    }
    
    // Créer le menu Données (après le menu Profils, avant le menu Aide)
    m_dataMenu = menuBar->addMenu(tr("&Données"));
    
    createActions();
    
    // Ajouter les actions au menu
    m_dataMenu->addAction(m_importCSVAction);
    m_dataMenu->addAction(m_importAPIAction);
    m_dataMenu->addSeparator();
    m_dataMenu->addAction(m_validateDataAction);
    m_dataMenu->addAction(m_cleanDataAction);
    m_dataMenu->addSeparator();
    m_dataMenu->addAction(m_dataInfoAction);
    m_dataMenu->addSeparator();
    m_dataMenu->addAction(m_setDirectoryAction);
    
    qDebug() << "Menu Données créé";
}

void DataMenuManager::createActions()
{
    // Action Importer CSV
    m_importCSVAction = new QAction(tr("&Importer CSV..."), this);
    m_importCSVAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    m_importCSVAction->setStatusTip(tr("Importer des données OHLC depuis un fichier CSV"));
    connect(m_importCSVAction, &QAction::triggered, this, &DataMenuManager::onImportCSV);
    
    // Action Importer depuis API
    m_importAPIAction = new QAction(tr("Importer depuis &API..."), this);
    m_importAPIAction->setStatusTip(tr("Télécharger des données depuis une API"));
    connect(m_importAPIAction, &QAction::triggered, this, &DataMenuManager::onImportFromAPI);
    
    // Action Valider les données
    m_validateDataAction = new QAction(tr("&Valider les données"), this);
    m_validateDataAction->setStatusTip(tr("Vérifier l'intégrité des données importées"));
    connect(m_validateDataAction, &QAction::triggered, this, &DataMenuManager::onValidateData);
    
    // Action Nettoyer les données
    m_cleanDataAction = new QAction(tr("&Nettoyer les données"), this);
    m_cleanDataAction->setStatusTip(tr("Supprimer les données obsolètes"));
    connect(m_cleanDataAction, &QAction::triggered, this, &DataMenuManager::onCleanData);
    
    // Action Informations sur les données
    m_dataInfoAction = new QAction(tr("&Informations sur les données"), this);
    m_dataInfoAction->setStatusTip(tr("Afficher les informations sur les données disponibles"));
    connect(m_dataInfoAction, &QAction::triggered, this, &DataMenuManager::onShowDataInfo);
    
    // Action Définir répertoire personnalisé
    m_setDirectoryAction = new QAction(tr("&Définir répertoire de données..."), this);
    m_setDirectoryAction->setStatusTip(tr("Choisir un emplacement personnalisé pour les données"));
    connect(m_setDirectoryAction, &QAction::triggered, this, &DataMenuManager::onSetCustomDirectory);
}

void DataMenuManager::onImportCSV()
{
    qDebug() << "Import CSV demandé";
    
    // Trouver le répertoire marketData
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        // Créer le répertoire s'il n'existe pas
        QString projectRoot = QCoreApplication::applicationDirPath();
        QDir currentDir(projectRoot);
        while (currentDir.cdUp() && currentDir.dirName() != "ig-trading-bot") {}
        
        if (currentDir.dirName() == "ig-trading-bot") {
            marketDataDir = currentDir.absoluteFilePath("marketData");
            QDir().mkpath(marketDataDir);
        } else {
            marketDataDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
    }
    
    // Dialogue de sélection de fichier
    QStringList fileNames = QFileDialog::getOpenFileNames(
        qobject_cast<QWidget*>(parent()),
        tr("Importer des données CSV"),
        marketDataDir,
        tr("Fichiers CSV (*.csv);;Tous les fichiers (*)")
    );
    
    if (fileNames.isEmpty()) {
        return;
    }
    
    // Créer une boîte de dialogue de progression
    QProgressDialog progress(tr("Importation des fichiers..."), tr("Annuler"), 0, fileNames.size(), 
                           qobject_cast<QWidget*>(parent()));
    progress.setWindowModality(Qt::WindowModal);
    
    int importedCount = 0;
    
    for (int i = 0; i < fileNames.size(); ++i) {
        progress.setValue(i);
        
        if (progress.wasCanceled()) {
            break;
        }
        
        QString fileName = fileNames.at(i);
        progress.setLabelText(tr("Importation de %1...").arg(QFileInfo(fileName).fileName()));
        
        // Copier le fichier vers le répertoire marketData s'il n'y est pas déjà
        QFileInfo sourceInfo(fileName);
        QString destPath = QDir(marketDataDir).absoluteFilePath(sourceInfo.fileName());
        
        if (fileName != destPath) {
            if (QFile::exists(destPath)) {
                int ret = QMessageBox::question(
                    qobject_cast<QWidget*>(parent()),
                    tr("Fichier existant"),
                    tr("Le fichier %1 existe déjà. Voulez-vous le remplacer ?")
                        .arg(sourceInfo.fileName()),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
                );
                
                if (ret == QMessageBox::Cancel) {
                    break;
                } else if (ret == QMessageBox::No) {
                    continue;
                }
                
                QFile::remove(destPath);
            }
            
            if (QFile::copy(fileName, destPath)) {
                importedCount++;
                qDebug() << "Fichier copié:" << destPath;
            } else {
                qWarning() << "Échec de la copie:" << fileName << "vers" << destPath;
            }
        } else {
            importedCount++;
            qDebug() << "Fichier déjà dans le bon répertoire:" << fileName;
        }
        
        QApplication::processEvents();
    }
    
    progress.setValue(fileNames.size());
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Importation terminée"),
        tr("Importation terminée.\n%1 fichier(s) importé(s) avec succès.")
            .arg(importedCount)
    );
}

void DataMenuManager::onImportFromAPI()
{
    qDebug() << "Import depuis API demandé";
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Fonctionnalité à venir"),
        tr("L'importation depuis une API sera disponible dans une future version.")
    );
}

void DataMenuManager::onValidateData()
{
    qDebug() << "Validation des données demandée";
    
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("Aucune donnée"),
            tr("Aucun répertoire de données trouvé.")
        );
        return;
    }
    
    QDir dir(marketDataDir);
    QStringList csvFiles = dir.entryList(QStringList() << "*.csv", QDir::Files);
    
    if (csvFiles.isEmpty()) {
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Aucune donnée"),
            tr("Aucun fichier CSV trouvé dans %1").arg(marketDataDir)
        );
        return;
    }
    
    QProgressDialog progress(tr("Validation des données..."), tr("Annuler"), 0, csvFiles.size(), 
                           qobject_cast<QWidget*>(parent()));
    progress.setWindowModality(Qt::WindowModal);
    
    QStringList validFiles;
    QStringList invalidFiles;
    
    for (int i = 0; i < csvFiles.size(); ++i) {
        progress.setValue(i);
        
        if (progress.wasCanceled()) {
            break;
        }
        
        QString filePath = dir.absoluteFilePath(csvFiles.at(i));
        progress.setLabelText(tr("Validation de %1...").arg(csvFiles.at(i)));
        
        // Tester de charger quelques lignes du fichier
        auto testData = DataLoader::loadFromCSV(filePath, "1d", "");
        
        if (!testData.empty()) {
            validFiles << csvFiles.at(i);
        } else {
            invalidFiles << csvFiles.at(i);
        }
        
        QApplication::processEvents();
    }
    
    progress.setValue(csvFiles.size());
    
    QString message = tr("Validation terminée:\n");
    message += tr("- %1 fichier(s) valide(s)\n").arg(validFiles.size());
    message += tr("- %1 fichier(s) invalide(s)").arg(invalidFiles.size());
    
    if (!invalidFiles.isEmpty()) {
        message += tr("\n\nFichiers invalides:\n");
        for (const QString& file : invalidFiles) {
            message += "- " + file + "\n";
        }
    }
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Validation des données"),
        message
    );
}

void DataMenuManager::onCleanData()
{
    qDebug() << "Nettoyage des données demandé";
    
    int ret = QMessageBox::question(
        qobject_cast<QWidget*>(parent()),
        tr("Nettoyer les données"),
        tr("Cette action supprimera tous les fichiers de données obsolètes.\nÊtes-vous sûr de vouloir continuer ?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret == QMessageBox::Yes) {
        // TODO: Implémenter la logique de nettoyage
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Nettoyage terminé"),
            tr("Le nettoyage des données a été effectué.")
        );
    }
}

void DataMenuManager::onShowDataInfo()
{
    qDebug() << "Informations sur les données demandées";
    
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("Aucune donnée"),
            tr("Aucun répertoire de données trouvé.")
        );
        return;
    }
    
    QDir dir(marketDataDir);
    QStringList csvFiles = dir.entryList(QStringList() << "*.csv", QDir::Files);
    
    QString info = tr("Répertoire des données: %1\n\n").arg(marketDataDir);
    info += tr("Nombre de fichiers CSV: %1\n\n").arg(csvFiles.size());
    
    if (!csvFiles.isEmpty()) {
        info += tr("Fichiers disponibles:\n");
        for (const QString& file : csvFiles) {
            QFileInfo fileInfo(dir.absoluteFilePath(file));
            info += tr("- %1 (%2 MB)\n")
                       .arg(file)
                       .arg(fileInfo.size() / (1024.0 * 1024.0), 0, 'f', 2);
        }
    }
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Informations sur les données"),
        info
    );
}

void DataMenuManager::onSetCustomDirectory()
{
    QString currentDir = DataLoader::findMarketDataDirectory();
    
    QString dir = QFileDialog::getExistingDirectory(
        qobject_cast<QWidget*>(parent()),
        tr("Choisir un répertoire pour les données de marché"),
        currentDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (dir.isEmpty()) {
        return;
    }
    
    if (DataLoader::setCustomMarketDataDirectory(dir)) {
        QSettings settings("IG-Trading-Bot", "BacktestApp");
        QString savedPath = settings.value("marketDataPath").toString();
        qDebug() << "Chemin sauvegardé dans QSettings:" << savedPath;
        QString actualPath = DataLoader::findMarketDataDirectory();
        qDebug() << "Chemin retourné par findMarketDataDirectory:" << actualPath;
        
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Répertoire défini"),
            tr("Le répertoire des données a été défini avec succès.\n\n%1").arg(dir)
        );
    } else {
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("Erreur"),
            tr("Impossible de définir ce répertoire pour les données.\nVérifiez les permissions d'accès.")
        );
    }
}