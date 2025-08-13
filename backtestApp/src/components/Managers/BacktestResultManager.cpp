#include "components/Managers/BacktestResultManager.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QInputDialog>

BacktestResultManager::BacktestResultManager(QObject *parent)
    : QObject(parent)
{
    // Configuration du répertoire des résultats
    initializeDirectories();
    
    qInfo() << "BacktestResultManager initialisé avec succès. Répertoire:" << m_resultsDir;
}

BacktestResultManager::~BacktestResultManager()
{
    // Nettoyage si nécessaire
}

void BacktestResultManager::initializeDirectories()
{
    // Utilisation d'emplacements standard pour les données d'application
    m_resultsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backtest_results";

    // Création du répertoire s'il n'existe pas
    QDir dir;
    if (!dir.exists(m_resultsDir)) {
        dir.mkpath(m_resultsDir);
        qInfo() << "Répertoire des résultats créé:" << m_resultsDir;
    }
}

QStringList BacktestResultManager::listBacktestResults() const
{
    QStringList results;
    
    // Scan du répertoire pour les fichiers JSON
    QDir dir(m_resultsDir);
    dir.setNameFilters(QStringList() << "*.json");
    dir.setFilter(QDir::Files);
    
    foreach (QString file, dir.entryList()) {
        // Suppression de l'extension .json du nom de fichier
        QString resultName = file;
        resultName.chop(5);  // Suppression de ".json"
        results << resultName;
    }
    
    qDebug() << "Résultats trouvés:" << results;
    return results;
}

bool BacktestResultManager::saveResult(const QString& resultName, const BacktestResultConfig& config)
{
    QString resultPath = getResultPath(resultName);
    return SerializationUtils::saveToJsonFile(resultPath, config);
}

bool BacktestResultManager::loadResultFromJson(const QString& resultName, BacktestResultConfig& config)
{
    QString resultPath = getResultPath(resultName);
    
    if (!QFile::exists(resultPath)) {
        return false;
    }
    
    return SerializationUtils::loadFromJsonFile(resultPath, config);
}

QString BacktestResultManager::getResultPath(const QString& resultName) const
{
    return m_resultsDir + "/" + resultName + ".json";
}

bool BacktestResultManager::resultExists(const QString& resultName) const
{
    QString resultPath = getResultPath(resultName);
    return QFile::exists(resultPath);
}

bool BacktestResultManager::openBacktestResultsDirectory() const
{
    QDir dir(m_resultsDir);
    if (!dir.exists()) {
        qWarning() << "Le répertoire des résultats n'existe pas:" << m_resultsDir;
        return false;
    }
    
    bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(m_resultsDir));
    if (success) {
        qInfo() << "Répertoire des résultats ouvert:" << m_resultsDir;
    } else {
        qWarning() << "Échec de l'ouverture du répertoire des résultats:" << m_resultsDir;
    }
    
    return success;
}

bool BacktestResultManager::saveBacktestResult(const BacktestResultConfig& config, QWidget* parentWidget)
{
    QString resultName = QString::fromStdString(config.name);
    
    // Vérification si un résultat avec ce nom existe déjà
    if (resultExists(resultName)) {
        if (parentWidget) {
            QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
                                                                      "Résultat existant", 
                                                                      QString("Un résultat nommé '%1' existe déjà. Voulez-vous le remplacer?").arg(resultName),
                                                                      QMessageBox::Yes | QMessageBox::No,
                                                                      QMessageBox::No);
            if (choice != QMessageBox::Yes) {
                return false;
            }
        }
    }
    
    bool success = saveResult(resultName, config);
    
    if (success) {
        emit resultListUpdated();
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Succès", 
                                    QString("Résultat '%1' sauvegardé.").arg(resultName));
        }
    } else if (parentWidget) {
        QMessageBox::warning(parentWidget, "Erreur", 
                            QString("Échec de la sauvegarde du résultat '%1'.").arg(resultName));
    }
    
    return success;
}

bool BacktestResultManager::loadBacktestResult(const QString& resultName, BacktestResultConfig& config)
{
    bool success = loadResultFromJson(resultName, config);
    
    if (!success) {
        qWarning() << "Échec du chargement du résultat:" << resultName;
    }
    
    return success;
}

bool BacktestResultManager::deleteBacktestResult(const QString& resultName, QWidget* parentWidget)
{
    if (!resultExists(resultName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                QString("Le résultat '%1' n'existe pas.").arg(resultName));
        }
        return false;
    }
    
    QMessageBox::StandardButton confirm = QMessageBox::Yes;
    
    if (parentWidget) {
        confirm = QMessageBox::question(parentWidget,
                                      "Confirmation",
                                      QString("Êtes-vous sûr de vouloir supprimer le résultat '%1' ?").arg(resultName),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
    }
    
    if (confirm == QMessageBox::Yes) {
        QString resultPath = getResultPath(resultName);
        bool success = QFile::remove(resultPath);
        
        if (success) {
            emit resultListUpdated();
            if (parentWidget) {
                QMessageBox::information(parentWidget, "Succès", 
                                        "Résultat supprimé.");
            }
        } else if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                "Échec de la suppression du résultat.");
        }
        
        return success;
    }
    
    return false;
}

bool BacktestResultManager::importBacktestResult(QWidget* parentWidget)
{
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
                                                  "Importer un résultat de backtest",
                                                  QDir::homePath(),
                                                  "Fichiers JSON (*.json)");
    
    if (fileName.isEmpty()) {
        return false; // Utilisateur a annulé
    }
    
    if (!QFile::exists(fileName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", "Le fichier sélectionné n'existe pas.");
        }
        return false;
    }
    
    // Essai de chargement du résultat depuis le fichier
    BacktestResultConfig importedConfig;
    bool loadSuccess = SerializationUtils::loadFromJsonFile(fileName, importedConfig);
    
    if (!loadSuccess) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur d'importation", 
                                "Le fichier n'est pas un résultat de backtest valide.");
        }
        return false;
    }
    
    // Demander le nom du résultat
    bool ok;
    QString resultName = QString::fromStdString(importedConfig.name);
    
    if (parentWidget) {
        resultName = QInputDialog::getText(parentWidget, 
                                         "Nom du résultat", 
                                         "Entrez un nom pour ce résultat de backtest:",
                                         QLineEdit::Normal,
                                         resultName, &ok);
        
        if (!ok || resultName.isEmpty()) {
            return false;
        }
    }
    
    // Mettre à jour le nom dans la configuration
    importedConfig.name = resultName.toStdString();
    
    // Vérifier si ce nom existe déjà
    if (resultExists(resultName)) {
        QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
                                                                  "Résultat existant", 
                                                                  QString("Un résultat nommé '%1' existe déjà. Voulez-vous le remplacer?").arg(resultName),
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return false;
        }
    }
    
    // Sauvegarder le résultat importé
    bool success = saveResult(resultName, importedConfig);
    
    if (success) {
        emit resultListUpdated();
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Import réussi", 
                                    QString("Résultat importé avec succès sous le nom '%1'.").arg(resultName));
        }
    } else if (parentWidget) {
        QMessageBox::warning(parentWidget, "Erreur d'importation", 
                            "Échec de l'importation du résultat.");
    }
    
    return success;
}

bool BacktestResultManager::exportBacktestResult(const QString& resultName, QWidget* parentWidget)
{
    if (!resultExists(resultName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                QString("Le résultat '%1' n'existe pas.").arg(resultName));
        }
        return false;
    }
    
    // Chargement du résultat
    BacktestResultConfig config;
    if (!loadResultFromJson(resultName, config)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                                QString("Impossible de charger le résultat '%1'.").arg(resultName));
        }
        return false;
    }
    
    // Nom de fichier par défaut avec nom du résultat et horodatage
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString defaultFileName = QString("backtest_result_%1_%2.json").arg(resultName).arg(timestamp);
    
    QString fileName = QFileDialog::getSaveFileName(parentWidget,
                                                  QString("Exporter le résultat - %1").arg(resultName),
                                                  QDir::homePath() + "/" + defaultFileName,
                                                  "Fichiers JSON (*.json)");
    
    if (fileName.isEmpty()) {
        return false; // Utilisateur a annulé
    }
    
    // Sauvegarde du résultat vers le fichier sélectionné
    bool success = SerializationUtils::saveToJsonFile(fileName, config);
    
    if (success) {
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Export réussi", 
                                    QString("Résultat '%1' exporté vers:\n%2").arg(resultName).arg(fileName));
        }
    } else {
        if (parentWidget) {
            QMessageBox::critical(parentWidget, "Erreur d'exportation", 
                                "Erreur lors de l'écriture du fichier de résultat.");
        }
    }
    
    return success;
}