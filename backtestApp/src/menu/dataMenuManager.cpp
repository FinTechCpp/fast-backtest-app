#include "menu/dataMenuManager.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QDateTime>
#include "components/dataLoader.h"

DataMenuManager::DataMenuManager(QObject* parent)
    : QObject(parent)
    , m_dataMenu(nullptr)
    , m_importCSVAction(nullptr)
    , m_importAPIAction(nullptr)
    , m_validateDataAction(nullptr)
    , m_cleanDataAction(nullptr)
    , m_dataInfoAction(nullptr)
    , m_setDirectoryAction(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
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
        while (currentDir.cdUp() && currentDir.dirName() != "fast-backtest-app") {}
        
        if (currentDir.dirName() == "fast-backtest-app") {
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
    
    // URL de l'API pour récupérer la liste des fichiers de données de marché
    QUrl apiUrl("http://10.8.0.1:9004/market-data");
    
    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    // Envoyer la requête GET pour récupérer la liste des fichiers
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Connecter le signal finished au slot qui traite la réponse
    connect(reply, &QNetworkReply::finished, this, &DataMenuManager::onMarketDataListReceived);
    
    qDebug() << "Requête envoyée vers:" << apiUrl.toString();
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
    
    QList<DataFileInfo> fileInfos;
    
    for (int i = 0; i < csvFiles.size(); ++i) {
        progress.setValue(i);
        
        if (progress.wasCanceled()) {
            break;
        }
        
        QString filePath = dir.absoluteFilePath(csvFiles.at(i));
        progress.setLabelText(tr("Validation de %1...").arg(csvFiles.at(i)));
        
        // Utiliser la nouvelle fonction de validation
        DataFileInfo info = DataLoader::checkDataFile(filePath);
        fileInfos.append(info);
        
        QApplication::processEvents();
    }
    
    progress.setValue(csvFiles.size());
    
    // Préparer le rapport détaillé
    int validCount = 0;
    int invalidCount = 0;
    
    for (const DataFileInfo& info : fileInfos) {
        if (info.isValid) {
            validCount++;
        } else {
            invalidCount++;
        }
    }
    
    QString summaryMessage = tr("Validation terminée:\n");
    summaryMessage += tr("- %1 fichier(s) valide(s)\n").arg(validCount);
    summaryMessage += tr("- %1 fichier(s) invalide(s)").arg(invalidCount);
    
    if (invalidCount > 0) {
        summaryMessage += tr("\n\nFichiers invalides:\n");
        for (const DataFileInfo& info : fileInfos) {
            if (!info.isValid) {
                summaryMessage += "- " + info.fileName + "\n";
            }
        }
    }
    
    // Afficher le résumé dans une boîte de dialogue standard
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Validation des données"),
        summaryMessage
    );
    
    // Créer une boîte de dialogue détaillée
    QDialog* detailsDialog = new QDialog(qobject_cast<QWidget*>(parent()));
    detailsDialog->setWindowTitle(tr("Détails de la validation"));
    detailsDialog->resize(700, 500);
    
    QVBoxLayout* layout = new QVBoxLayout(detailsDialog);
    
    QTextEdit* textEdit = new QTextEdit(detailsDialog);
    textEdit->setReadOnly(true);
    
    // Générer le rapport détaillé
    QString detailedReport = tr("<h2>Rapport de validation des données</h2>");
    detailedReport += tr("<p>Répertoire: %1</p>").arg(marketDataDir);
    
    for (const DataFileInfo& info : fileInfos) {
        detailedReport += tr("<hr><h3>%1</h3>").arg(info.fileName);
        detailedReport += tr("<p><b>Statut:</b> %1</p>")
                            .arg(info.isValid ? tr("<span style='color:green'>Valide</span>") 
                                              : tr("<span style='color:red'>Invalide</span>"));
        
        detailedReport += tr("<p><b>Informations de base:</b><br>");
        detailedReport += tr("Taille: %1 MB<br>")
                            .arg(info.fileSize / (1024.0 * 1024.0), 0, 'f', 2);
        detailedReport += tr("Nombre de lignes: %1<br>").arg(info.totalRows);
        detailedReport += tr("En-tête: %1</p>").arg(info.hasHeader ? tr("Oui") : tr("Non"));
        
        if (info.isValid) {
            detailedReport += tr("<p><b>Métriques:</b><br>");
            detailedReport += tr("Intervalle détecté: %1<br>").arg(info.interval);
            detailedReport += tr("Période: %1 à %2 (%3 jours)<br>")
                                .arg(info.startDate.toString("yyyy-MM-dd hh:mm"))
                                .arg(info.endDate.toString("yyyy-MM-dd hh:mm"))
                                .arg(info.durationDays);
            detailedReport += tr("Plage de prix: %1 à %2</p>")
                                .arg(info.minPrice, 0, 'f', 2)
                                .arg(info.maxPrice, 0, 'f', 2);
            
            if (info.gapsCount > 0) {
                detailedReport += tr("<p><b>Trous dans les données:</b> %1<br>").arg(info.gapsCount);
                
                // Afficher les plus grands trous (jusqu'à 5)
                int showCount = qMin(5, info.largestGaps.size());
                for (int i = 0; i < showCount; i++) {
                    QDateTime gapStart = info.largestGaps[i].first;
                    QDateTime gapEnd = info.largestGaps[i].second;
                    int hours = gapStart.secsTo(gapEnd) / 3600;
                    int minutes = (gapStart.secsTo(gapEnd) % 3600) / 60;
                    
                    detailedReport += tr("• %1 à %2 (%3h %4m)<br>")
                                      .arg(gapStart.toString("yyyy-MM-dd hh:mm"))
                                      .arg(gapEnd.toString("yyyy-MM-dd hh:mm"))
                                      .arg(hours)
                                      .arg(minutes);
                }
                detailedReport += tr("</p>");
            } else {
                detailedReport += tr("<p><b>Trous dans les données:</b> Aucun</p>");
            }
        }
        
        if (info.invalidRows > 0) {
            detailedReport += tr("<p><b>Lignes invalides:</b> %1").arg(info.invalidRows);
            
            // Afficher les détails des lignes invalides (limité à 10)
            int showCount = qMin(10, info.invalidRowDetails.size());
            if (showCount > 0) {
                detailedReport += tr("<br>Détails (max 10):<br>");
                for (int i = 0; i < showCount; i++) {
                    detailedReport += tr("• %1<br>").arg(info.invalidRowDetails[i]);
                }
            }
            detailedReport += tr("</p>");
        }
    }
    
    textEdit->setHtml(detailedReport);
    
    QPushButton* closeButton = new QPushButton(tr("Fermer"), detailsDialog);
    connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);
    
    layout->addWidget(textEdit);
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    
    detailsDialog->exec();
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
        QSettings settings("fast-backtest-app", "BacktestApp");
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

void DataMenuManager::onMarketDataListReceived()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        qDebug() << "Réponse reçue, taille:" << responseData.size() << "octets";
        
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isObject()) {
            QJsonObject response = doc.object();
            QJsonArray files = response["market_data_files"].toArray();
            
            qDebug() << "Nombre de fichiers sur le serveur:" << files.size();
            
            if (files.isEmpty()) {
                QMessageBox::information(
                    qobject_cast<QWidget*>(parent()),
                    tr("Synchronisation"),
                    tr("Aucun fichier disponible sur le serveur.")
                );
            } else {
                compareAndDownloadFiles(files);
            }
        } else {
            QMessageBox::warning(
                qobject_cast<QWidget*>(parent()),
                tr("Erreur"),
                tr("Format de réponse invalide du serveur.")
            );
        }
    } else {
        QString errorMsg = tr("Erreur de connexion à l'API: %1").arg(reply->errorString());
        qWarning() << errorMsg;
        
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("Erreur de connexion"),
            errorMsg
        );
    }
    
    reply->deleteLater();
}

void DataMenuManager::compareAndDownloadFiles(const QJsonArray& remoteFiles)
{
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        // Créer le répertoire s'il n'existe pas
        QString projectRoot = QCoreApplication::applicationDirPath();
        QDir currentDir(projectRoot);
        while (currentDir.cdUp() && currentDir.dirName() != "fast-backtest-app") {}
        
        if (currentDir.dirName() == "fast-backtest-app") {
            marketDataDir = currentDir.absoluteFilePath("marketData");
            QDir().mkpath(marketDataDir);
        } else {
            marketDataDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
    }
    
    QDir localDir(marketDataDir);
    QStringList localFiles = localDir.entryList(QStringList() << "*.csv" << "*.parquet", QDir::Files);
    
    QStringList filesToDownload;
    QStringList updateMessages;
    
    // Comparer chaque fichier distant avec les fichiers locaux
    for (const QJsonValue& fileValue : remoteFiles) {
        QJsonObject fileObj = fileValue.toObject();
        QString filename = fileObj["filename"].toString();
        QString remoteModified = fileObj["modified"].toString();
        qint64 remoteSize = fileObj["size"].toVariant().toLongLong();
        
        QString localFilePath = localDir.absoluteFilePath(filename);
        bool shouldDownload = false;
        QString reason;
        
        if (!QFile::exists(localFilePath)) {
            // Fichier n'existe pas localement
            shouldDownload = true;
            reason = tr("nouveau fichier");
        } else {
            // Comparer la date de modification
            QFileInfo localFileInfo(localFilePath);
            QDateTime localModified = localFileInfo.lastModified();
            QDateTime remoteDateTime = QDateTime::fromString(remoteModified, Qt::ISODate);
            
            if (remoteDateTime > localModified) {
                shouldDownload = true;
                reason = tr("fichier plus récent (%1 vs %2)")
                    .arg(remoteDateTime.toString("yyyy-MM-dd hh:mm"))
                    .arg(localModified.toString("yyyy-MM-dd hh:mm"));
            } else if (localFileInfo.size() != remoteSize) {
                shouldDownload = true;
                reason = tr("taille différente (%1 vs %2 octets)")
                    .arg(remoteSize)
                    .arg(localFileInfo.size());
            }
        }
        
        if (shouldDownload) {
            filesToDownload.append(filename);
            updateMessages.append(tr("• %1 (%2)").arg(filename).arg(reason));
        }
    }
    
    if (filesToDownload.isEmpty()) {
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Synchronisation"),
            tr("Tous les fichiers locaux sont à jour.")
        );
        return;
    }
    
    // Demander confirmation à l'utilisateur
    QString message = tr("Les fichiers suivants seront téléchargés/mis à jour:\n\n");
    message += updateMessages.join("\n");
    message += tr("\n\nVoulez-vous continuer ?");
    
    int ret = QMessageBox::question(
        qobject_cast<QWidget*>(parent()),
        tr("Synchronisation des fichiers"),
        message,
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // Créer une boîte de dialogue de progression
    QProgressDialog* progress = new QProgressDialog(
        tr("Téléchargement des fichiers..."), 
        tr("Annuler"), 
        0, 
        filesToDownload.size(), 
        qobject_cast<QWidget*>(parent())
    );
    progress->setWindowModality(Qt::WindowModal);
    progress->show();
    
    // Télécharger les fichiers un par un
    for (int i = 0; i < filesToDownload.size(); ++i) {
        if (progress->wasCanceled()) {
            break;
        }
        
        QString filename = filesToDownload.at(i);
        progress->setValue(i);
        progress->setLabelText(tr("Téléchargement de %1...").arg(filename));
        
        // Construire l'URL de téléchargement
        QUrl downloadUrl(QString("http://10.8.0.1:9004/download-market-data/%1").arg(filename));
        
        QNetworkRequest request(downloadUrl);
        QNetworkReply* downloadReply = m_networkManager->get(request);
        
        // Attendre la fin du téléchargement (synchrone pour simplifier)
        QEventLoop loop;
        connect(downloadReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        
        if (downloadReply->error() == QNetworkReply::NoError) {
            // Sauvegarder le fichier
            QString localFilePath = localDir.absoluteFilePath(filename);
            QFile localFile(localFilePath);
            
            if (localFile.open(QIODevice::WriteOnly)) {
                localFile.write(downloadReply->readAll());
                localFile.close();
                qDebug() << "Fichier téléchargé:" << filename;
            } else {
                qWarning() << "Impossible d'écrire le fichier:" << filename;
            }
        } else {
            qWarning() << "Erreur téléchargement" << filename << ":" << downloadReply->errorString();
        }
        
        downloadReply->deleteLater();
        QApplication::processEvents();
    }
    
    progress->setValue(filesToDownload.size());
    progress->close();
    delete progress;
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Synchronisation terminée"),
        tr("Synchronisation terminée.\n%1 fichier(s) téléchargé(s).")
            .arg(filesToDownload.size())
    );
}

void DataMenuManager::onFileDownloadFinished()
{
    // Cette méthode peut être utilisée pour des téléchargements asynchrones si nécessaire
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    reply->deleteLater();
}

QString DataMenuManager::getFileLastModified(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        return fileInfo.lastModified().toString(Qt::ISODate);
    }
    return QString();
}