#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include "components/Utils/SerializationUtils.hpp"

/**
 * @brief Gestionnaire pour sauvegarder et charger les résultats de backtest
 */
class BacktestResultManager : public QObject
{
    Q_OBJECT

public:
    BacktestResultManager(QObject* parent = nullptr);
    ~BacktestResultManager();
    
    // Liste tous les résultats de backtest disponibles
    QStringList listBacktestResults() const;

    // Actions utilisateur
    bool saveBacktestResult(const BacktestResultConfig& config, QWidget* parentWidget = nullptr);
    bool loadBacktestResult(const QString& resultName, BacktestResultConfig& config);
    bool deleteBacktestResult(const QString& resultName, QWidget* parentWidget = nullptr);
    bool importBacktestResult(QWidget* parentWidget = nullptr);
    bool exportBacktestResult(const QString& resultName, QWidget* parentWidget = nullptr);

    // Méthode pour ouvrir le dossier des résultats
    bool openBacktestResultsDirectory() const;
    
    // Getter pour le chemin du dossier des résultats
    QString getBacktestResultsDirectory() const { return m_resultsDir; }

signals:
    void resultListUpdated();

private:
    QString m_resultsDir;         // Répertoire où sont stockés les résultats
    
    // Méthodes de gestion des fichiers
    bool resultExists(const QString& resultName) const;
    bool saveResult(const QString& resultName, const BacktestResultConfig& config);
    bool loadResultFromJson(const QString& resultName, BacktestResultConfig& config);
    QString getResultPath(const QString& resultName) const;
    
    // Initialiser les répertoires de l'application
    void initializeDirectories();
};