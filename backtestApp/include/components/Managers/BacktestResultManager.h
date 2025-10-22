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
    BacktestResultManager(QObject* parent = nullptr, SerializationUtils::FileFormat defaultFormat = SerializationUtils::FileFormat::JSON);
    ~BacktestResultManager();
    
    // Liste tous les résultats de backtest disponibles
    QStringList listBacktestResults() const;

    // Actions utilisateur avec format optionnel
    bool saveBacktestResult(const BacktestResultConfig& config, QWidget* parentWidget = nullptr, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    bool loadBacktestResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    bool deleteBacktestResult(const QString& resultName, QWidget* parentWidget = nullptr);
    bool importBacktestResult(QWidget* parentWidget = nullptr);
    bool importExternalResult(QWidget* parentWidget = nullptr);  // Import de résultats externes
    bool exportBacktestResult(const QString& resultName, QWidget* parentWidget = nullptr, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    
    // Méthode pour charger un résultat externe et calculer les stats
    bool loadExternalResult(const QString& filePath, BacktestResultConfig& config);

    // Méthode pour définir le format par défaut
    void setDefaultFormat(SerializationUtils::FileFormat format) { m_defaultFormat = format; }
    SerializationUtils::FileFormat getDefaultFormat() const { return m_defaultFormat; }

    // Méthode pour ouvrir le dossier des résultats
    bool openBacktestResultsDirectory() const;
    
    // Getter pour le chemin du dossier des résultats
    QString getBacktestResultsDirectory() const { return m_resultsDir; }

signals:
    void resultListUpdated();

private:
    QString m_resultsDir;         // Répertoire où sont stockés les résultats
    SerializationUtils::FileFormat m_defaultFormat;

    // Méthodes de gestion des fichiers
    bool resultExists(const QString& resultName, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto) const;
    bool saveResult(const QString& resultName, const BacktestResultConfig& config, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    bool loadResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    QString getResultPath(const QString& resultName, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto) const;

    // Initialiser les répertoires de l'application
    void initializeDirectories();
};