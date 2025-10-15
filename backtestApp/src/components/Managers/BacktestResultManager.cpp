#include "components/Managers/BacktestResultManager.h"
#include "components/serializerAdapters.h"
#include "stats.hpp"
#include "data.hpp"
#include "trade.hpp"
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QInputDialog>
#include <memory>

BacktestResultManager::BacktestResultManager(QObject *parent, SerializationUtils::FileFormat defaultFormat)
    : QObject(parent), m_defaultFormat(defaultFormat)
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
    // dir.setNameFilters(QStringList() << "*.json");
    dir.setFilter(QDir::Files);
    
    foreach (QString file, dir.entryList()) {
        QFileInfo fileInfo(m_resultsDir + "/" + file);
        QString baseName = fileInfo.completeBaseName(); // Nom sans extension
        
        results << baseName;
    }
    
    qDebug() << "Résultats trouvés:" << results;
    return results;
}

bool BacktestResultManager::saveResult(const QString& resultName, const BacktestResultConfig& config, SerializationUtils::FileFormat format)
{
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;
    QString resultPath = getResultPath(resultName, actualFormat);
    
    return SerializationUtils::saveToFile(resultPath, config, actualFormat);
}

bool BacktestResultManager::loadResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format)
{
    if (format == SerializationUtils::FileFormat::Auto) {
        // Essayer tous les formats possibles
        for (int i = 0; i < static_cast<int>(SerializationUtils::FileFormat::Auto); i++) {
            SerializationUtils::FileFormat currentFormat = static_cast<SerializationUtils::FileFormat>(i);
            QString resultPath = getResultPath(resultName, currentFormat);
            
            if (QFile::exists(resultPath)) {
                return SerializationUtils::loadFromFile(resultPath, config, currentFormat);
            }
        }
        return false;
    } else {
        QString resultPath = getResultPath(resultName, format);
        
        if (!QFile::exists(resultPath)) {
            return false;
        }
        
        return SerializationUtils::loadFromFile(resultPath, config, format);
    }
}

QString BacktestResultManager::getResultPath(const QString& resultName, SerializationUtils::FileFormat format) const
{
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;
    QString extension = SerializationUtils::getExtensionForFormat(actualFormat);
    
    return m_resultsDir + "/" + resultName + extension;
}

bool BacktestResultManager::resultExists(const QString& resultName, SerializationUtils::FileFormat format) const
{
    if (format == SerializationUtils::FileFormat::Auto) {
        // Vérifier tous les formats possibles
        for (int i = 0; i < static_cast<int>(SerializationUtils::FileFormat::Auto); i++) {
            SerializationUtils::FileFormat currentFormat = static_cast<SerializationUtils::FileFormat>(i);
            QString resultPath = getResultPath(resultName, currentFormat);
            if (QFile::exists(resultPath)) {
                return true;
            }
        }
        return false;
    } else {
        QString resultPath = getResultPath(resultName, format);
        return QFile::exists(resultPath);
    }
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

bool BacktestResultManager::saveBacktestResult(const BacktestResultConfig& config, QWidget* parentWidget, SerializationUtils::FileFormat format)
{
    QString resultName = QString::fromStdString(config.name);
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;

    // Vérification si un résultat avec ce nom existe déjà
    if (resultExists(resultName, actualFormat)) {
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
    
    bool success = saveResult(resultName, config, actualFormat);
    
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

bool BacktestResultManager::loadBacktestResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format)
{
    bool success = loadResult(resultName, config, format);
    
    if (!success) {
        qWarning() << "Échec du chargement du résultat:" << resultName;
    }
    
    return success;
}

bool BacktestResultManager::deleteBacktestResult(const QString& resultName, QWidget* parentWidget)
{
    // Vérifier si le résultat existe dans n'importe quel format
    if (!resultExists(resultName, SerializationUtils::FileFormat::Auto)) {
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
        bool success = true;
        
        // Supprimer le résultat dans tous les formats où il existe
        for (int i = 0; i < static_cast<int>(SerializationUtils::FileFormat::Auto); i++) {
            SerializationUtils::FileFormat currentFormat = static_cast<SerializationUtils::FileFormat>(i);
            QString resultPath = getResultPath(resultName, currentFormat);
            
            if (QFile::exists(resultPath)) {
                bool deleteResult = QFile::remove(resultPath);
                success = success && deleteResult;
            }
        }
        
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
    // Construire le filtre pour tous les formats supportés
    QString filterString = "Tous les formats supportés (*.json *.bin);;";
    filterString += "Fichiers JSON (*.json);;";
    filterString += "Fichiers binaires (*.bin)";
    
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
        "Importer un résultat de backtest",
        QDir::homePath(),
        filterString);
    
    if (fileName.isEmpty()) {
        return false; // Utilisateur a annulé
    }
    
    if (!QFile::exists(fileName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", "Le fichier sélectionné n'existe pas.");
        }
        return false;
    }
    
    // Détecter le format du fichier à partir de son extension
    SerializationUtils::FileFormat detectedFormat = SerializationUtils::detectFormatFromExtension(fileName);
    
    // Essai de chargement du résultat depuis le fichier
    BacktestResultConfig importedConfig;
    bool loadSuccess = SerializationUtils::loadFromFile(fileName, importedConfig, detectedFormat);
    
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
    if (resultExists(resultName, m_defaultFormat)) {
        QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
            "Résultat existant", 
            QString("Un résultat nommé '%1' existe déjà. Voulez-vous le remplacer?").arg(resultName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return false;
        }
    }
    
    // Sauvegarder le résultat importé dans le format par défaut
    bool success = saveResult(resultName, importedConfig, m_defaultFormat);
    
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

bool BacktestResultManager::importExternalResult(QWidget* parentWidget)
{
    // Construire le filtre pour tous les formats supportés
    QString filterString = "Tous les formats supportés (*.json *.bin);;";
    filterString += "Fichiers JSON (*.json);;";
    filterString += "Fichiers binaires (*.bin)";
    
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
        "Importer un résultat externe (sans stats pré-calculées)",
        QDir::homePath(),
        filterString);
    
    if (fileName.isEmpty()) {
        return false; // Utilisateur a annulé
    }
    
    if (!QFile::exists(fileName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", "Le fichier sélectionné n'existe pas.");
        }
        return false;
    }
    
    // Charger le résultat externe et calculer les stats
    BacktestResultConfig importedConfig;
    bool loadSuccess = loadExternalResult(fileName, importedConfig);
    
    if (!loadSuccess) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur d'importation", 
                "Le fichier n'est pas un résultat externe valide (format ExternalResultConfig attendu).");
        }
        return false;
    }
    
    // Demander le nom du résultat
    bool ok;
    QString resultName = QString::fromStdString(importedConfig.name);
    
    if (parentWidget) {
        resultName = QInputDialog::getText(parentWidget, 
            "Nom du résultat", 
            "Entrez un nom pour ce résultat externe importé:",
            QLineEdit::Normal,
            resultName, &ok);
        
        if (!ok || resultName.isEmpty()) {
            return false;
        }
    }
    
    // Mettre à jour le nom dans la configuration
    importedConfig.name = resultName.toStdString();
    
    // Vérifier si ce nom existe déjà
    if (resultExists(resultName, m_defaultFormat)) {
        QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
            "Résultat existant", 
            QString("Un résultat nommé '%1' existe déjà. Voulez-vous le remplacer?").arg(resultName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return false;
        }
    }
    
    // Sauvegarder le résultat importé dans le format par défaut
    bool success = saveResult(resultName, importedConfig, m_defaultFormat);
    
    if (success) {
        emit resultListUpdated();
        if (parentWidget) {
            QString statsInfo = QString(
                "Résultat externe importé avec succès sous le nom '%1'.\n\n"
                "Statistiques calculées:\n"
                "- Nombre de trades: %2\n"
                "- Net Profit: %3 (%4%)\n"
                "- Profit Factor: %5")
                .arg(resultName)
                .arg(importedConfig.stats.numTrades)
                .arg(importedConfig.stats.equityFinal - importedConfig.stats.equityInitial, 0, 'f', 2)
                .arg(importedConfig.stats.returnPct, 0, 'f', 2)
                .arg(importedConfig.stats.profitFactor, 0, 'f', 2);
            
            QMessageBox::information(parentWidget, "Import réussi", statsInfo);
        }
    } else if (parentWidget) {
        QMessageBox::warning(parentWidget, "Erreur d'importation", 
            "Échec de l'importation du résultat externe.");
    }
    
    return success;
}

bool BacktestResultManager::exportBacktestResult(const QString& resultName, QWidget* parentWidget, SerializationUtils::FileFormat format)
{
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;
    
    // Vérifier si le résultat existe dans n'importe quel format
    if (!resultExists(resultName, SerializationUtils::FileFormat::Auto)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                QString("Le résultat '%1' n'existe pas.").arg(resultName));
        }
        return false;
    }
    
    // Chargement du résultat
    BacktestResultConfig config;
    if (!loadBacktestResult(resultName, config, SerializationUtils::FileFormat::Auto)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Erreur", 
                QString("Impossible de charger le résultat '%1'.").arg(resultName));
        }
        return false;
    }
    
    // Nom de fichier par défaut avec nom du résultat et horodatage
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString extension = SerializationUtils::getExtensionForFormat(actualFormat);
    QString defaultFileName = QString("backtest_result_%1_%2%3").arg(resultName).arg(timestamp).arg(extension);
    
    // Construire le filtre pour le format spécifié
    QString filterString;
    if (actualFormat == SerializationUtils::FileFormat::JSON) {
        filterString = "Fichiers JSON (*.json)";
    } else if (actualFormat == SerializationUtils::FileFormat::Binary) {
        filterString = "Fichiers binaires (*.bin)";
    } else {
        filterString = "Tous les formats supportés (*.json *.bin);;Fichiers JSON (*.json);;Fichiers binaires (*.bin)";
    }
    
    QString fileName = QFileDialog::getSaveFileName(parentWidget,
        QString("Exporter le résultat - %1").arg(resultName),
        QDir::homePath() + "/" + defaultFileName,
        filterString);
    
    if (fileName.isEmpty()) {
        return false; // Utilisateur a annulé
    }
    
    // Si l'utilisateur a changé l'extension, détecter le nouveau format
    SerializationUtils::FileFormat selectedFormat = actualFormat;
    if (format == SerializationUtils::FileFormat::Auto) {
        selectedFormat = SerializationUtils::detectFormatFromExtension(fileName);
    }
    
    // Sauvegarde du résultat vers le fichier sélectionné
    bool success = SerializationUtils::saveToFile(fileName, config, selectedFormat);
    
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

bool BacktestResultManager::loadExternalResult(const QString& filePath, BacktestResultConfig& config)
{
    qDebug() << "Tentative de chargement d'un résultat externe depuis:" << filePath;
    
    ExternalResultConfig externalConfig;
    
    // Détecter le format du fichier
    SerializationUtils::FileFormat format = SerializationUtils::FileFormat::JSON;
    if (filePath.endsWith(".bin")) 
        format = SerializationUtils::FileFormat::Binary;
    
    // Charger le fichier externe
    if (!SerializationUtils::loadFromFile(filePath, externalConfig, format)) {
        qCritical() << "Erreur lors du chargement du résultat externe";
        return false;
    }
    
    qInfo() << "Résultat externe chargé avec succès. Calcul des stats...";
    
    // Convertir ExternalResultConfig vers BacktestResultConfig
    config.name = externalConfig.name;
    config.version = externalConfig.version;
    config.createdAt = externalConfig.createdAt;
    config.generalParams = externalConfig.generalParams;
    config.strategyConfig = externalConfig.strategyConfig;
    config.candles = externalConfig.candles;
    
    // Créer une structure Data à partir des candles
    be::Data data(externalConfig.candles);
    
    // Créer l'equity curve à partir des trades
    std::vector<be::EquityPoint> equityCurve;
    double currentEquity = config.generalParams.cash;
    equityCurve.push_back({0, currentEquity});
    
    return true;
}