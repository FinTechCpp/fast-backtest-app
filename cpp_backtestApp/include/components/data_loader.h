#pragma once

#include <QString>
#include <QDateTime>
#include <QTime>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QList>
#include <QPair>
#include <vector>
#include <map>
#include <memory>
#include <limits>

/**
 * @brief Structure pour stocker une barre OHLC
 */
struct OHLCBar {
    QDateTime timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume = 0.0;  
    
    OHLCBar() = default;
    OHLCBar(const QDateTime& dt, double o, double h, double l, double c, double v = 0.0)
        : timestamp(dt), open(o), high(h), low(l), close(c), volume(v) {}
};

Q_DECLARE_METATYPE(OHLCBar)
Q_DECLARE_METATYPE(std::vector<OHLCBar>)

/**
 * @brief Structure pour stocker les informations de validation d'un fichier de données
 */
struct DataFileInfo {
    // Basic file info
    QString filePath;
    QString fileName;
    qint64 fileSize;
    
    // Data metrics
    int totalRows;
    QDateTime startDate;
    QDateTime endDate;
    int durationDays;
    QString interval; // Detected interval between data points (e.g., "10secs")
    
    // Integrity checks
    bool hasHeader;
    bool isValid;
    int invalidRows;
    QStringList invalidRowDetails; // Contains reasons for invalid rows
    int gapsCount;
    QList<QPair<QDateTime, QDateTime>> largestGaps; // Stores significant gaps
    
    // Price stats
    double minPrice;
    double maxPrice;
    
    // Constructor with defaults
    DataFileInfo() : 
        fileSize(0), 
        totalRows(0), 
        durationDays(0), 
        hasHeader(false), 
        isValid(false), 
        invalidRows(0), 
        gapsCount(0),
        minPrice(std::numeric_limits<double>::max()),
        maxPrice(std::numeric_limits<double>::lowest()) {}
};

Q_DECLARE_METATYPE(DataFileInfo)

/**
 * @brief Classe pour charger et traiter les données de marché
 */
class DataLoader
{
public:
    DataLoader();
    ~DataLoader();
    
    /**
     * @brief Trouve le répertoire marketData
     * @return Chemin vers le répertoire marketData, QString vide si non trouvé
     */
    static QString findMarketDataDirectory();
    
    /**
     * @brief Charge les données OHLC depuis un fichier CSV
     * @param symbol Symbole du marché (ex: "NDX")
     * @param interval Intervalle des données (ex: "20secs")
     * @param period Période de données à charger (ex: "10d", "1m", "6m", "1y")
     * @param endDate Date de fin au format QDateTime
     * @return Vecteur de barres OHLC filtrées
     */
    static std::vector<OHLCBar> loadData(
        const QString& symbol = "NDX",
        const QString& interval = "10secs", 
        const QString& period = "1m",
        const QDateTime& endDate = QDateTime()
    );
    
    /**
     * @brief Trouve le fichier CSV correspondant au symbole et intervalle
     * @param symbol Symbole à rechercher
     * @param interval Intervalle à rechercher
     * @return Chemin vers le fichier trouvé, QString vide si non trouvé
     */
    static QString findDataFile(const QString& symbol, const QString& interval);
    
    /**
     * @brief Parse une ligne CSV en barre OHLC
     * @param line Ligne CSV à parser
     * @param hasHeader Indique si c'est la ligne d'en-tête
     * @return Barre OHLC parsée, nullptr si erreur
     */
    static std::unique_ptr<OHLCBar> parseCSVLine(const QString& line);
    
    /**
     * @brief Charge les données depuis un fichier CSV
     * @param filePath Chemin vers le fichier CSV
     * @param period Période à charger
     * @param endDate Date de fin
     * @return Vecteur de barres OHLC
     */
    static std::vector<OHLCBar> loadFromCSV(
        const QString& filePath,
        const QString& period,
        const QString& endDate
    );
    
    /**
     * @brief Filtre les données par période
     * @param data Données à filtrer
     * @param startDate Date de début
     * @param endDate Date de fin
     * @return Données filtrées
     */
    static std::vector<OHLCBar> filterByPeriod(
        const std::vector<OHLCBar>& data,
        const QDateTime& startDate,
        const QDateTime& endDate
    );
    
    /**
     * @brief Resample les données à un intervalle différent
     * @param data Données source
     * @param targetInterval Intervalle cible (ex: "1min", "5min")
     * @return Données resampleées
     */
    static std::vector<OHLCBar> resampleData(
        const std::vector<OHLCBar>& data,
        const QString& targetInterval
    );
    
    /**
     * @brief Calcule la date de début à partir de la période
     * @param endDate Date de fin
     * @param period Période (ex: "10d", "1m", "6m", "1y")
     * @return Date de début calculée
     */
    static QDateTime calculateStartDate(const QDateTime& endDate, const QString& period);
    
    /**
     * @brief Convertit un intervalle en secondes
     * @param interval Intervalle (ex: "10secs", "1min", "1h")
     * @return Nombre de secondes
     */
    static int intervalToSeconds(const QString& interval);
    
    /**
     * @brief Définit un répertoire personnalisé pour les données de marché
     * @param path Chemin vers le répertoire à utiliser
     * @return true si le répertoire est valide et a été défini, false sinon
     */
    static bool setCustomMarketDataDirectory(const QString& path);
    
    /**
     * @brief Vérifie l'intégrité des données d'un fichier CSV OHLC
     * @param filePath Chemin vers le fichier à vérifier
     * @return Structure contenant les informations sur le fichier et son intégrité
     */
    static DataFileInfo checkDataFile(const QString& filePath);

private:
    static QStringList getMarketDataPaths();
    
    // Ajout de la méthode helper pour la conversion en double
    static inline double parseDouble(const QChar* begin, const QChar* end);
};

