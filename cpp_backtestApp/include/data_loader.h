#pragma once

#include <QString>
#include <QDateTime>
#include <QTime>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <vector>
#include <map>
#include <memory>

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
 * @brief Classe pour charger et traiter les données de marché
 */
class DataLoader
{
public:
    DataLoader();
    ~DataLoader();
    
    /**
     * @brief Charge les données OHLC depuis un fichier CSV
     * @param symbol Symbole du marché (ex: "NDX")
     * @param interval Intervalle des données (ex: "20secs")
     * @param period Période de données à charger (ex: "10d", "1m", "6m", "1y")
     * @param endDate Date de fin au format QDateTime
     * @param tradingFrom Heure de début du trading
     * @param tradingTo Heure de fin du trading
     * @param tradingDays Liste des jours de trading (0=Lundi, 6=Dimanche)
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

private:
    static const QString MARKET_DATA_PATH;
    static QStringList getMarketDataPaths();
    static QString findMarketDataDirectory();
};

