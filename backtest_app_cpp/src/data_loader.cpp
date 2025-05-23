#include "data_loader.h"
#include <QApplication>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <algorithm>
#include <cmath>

// CORRECTION: Ajouter les includes manquants pour PyBindingManager et pybind11
#include "binding/pybinding.h"

// And add the pybind11 namespace
namespace py = pybind11;

const QString DataLoader::MARKET_DATA_PATH = "/home/max/ig-trading-bot/marketData";

DataLoader::DataLoader()
{
}

DataLoader::~DataLoader()
{
}

QStringList DataLoader::getMarketDataPaths()
{
    QStringList paths;
    
    // Chemin principal fixe
    paths << MARKET_DATA_PATH;
    
    // Chemin relatif au répertoire courant
    QString currentDir = QDir::currentPath();
    paths << QDir(currentDir).absoluteFilePath("../marketData");
    paths << QDir(currentDir).absoluteFilePath("marketData");
    
    // Chemin depuis variable d'environnement
    QString envPath = qgetenv("BOT_REPO_PATH");
    if (!envPath.isEmpty()) {
        paths << QDir(envPath).absoluteFilePath("marketData");
    }
    
    return paths;
}

QString DataLoader::findMarketDataDirectory()
{
    QStringList possiblePaths = getMarketDataPaths();
    
    for (const QString& path : possiblePaths) {
        if (QDir(path).exists()) {
            qDebug() << "Répertoire marketData trouvé:" << path;
            return path;
        }
    }
    
    qWarning() << "Aucun répertoire marketData trouvé dans les chemins:" << possiblePaths;
    return QString();
}

QString DataLoader::findDataFile(const QString& symbol, const QString& interval)
{
    QString marketDataDir = findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        return QString();
    }
    
    // Rechercher un fichier correspondant au pattern symbol_interval_*.parquet
    QDir dir(marketDataDir);
    QStringList filters;
    filters << QString("%1_%2_*.parquet").arg(symbol, interval);
    
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Time);
    if (!files.isEmpty()) {
        QString filePath = dir.absoluteFilePath(files.first());
        qDebug() << "Fichier de données trouvé:" << filePath;
        return filePath;
    }
    
    qWarning() << "Aucun fichier trouvé pour" << symbol << interval;
    return QString();
}

std::unique_ptr<OHLCBar> DataLoader::parseCSVLine(const QString& line, bool hasHeader)
{
    if (hasHeader) return nullptr;
    
    QStringList parts = line.split(',');
    if (parts.size() < 5) return nullptr;
    
    auto bar = std::make_unique<OHLCBar>();
    
    // Parse timestamp (assume first column is timestamp)
    bar->timestamp = QDateTime::fromString(parts[0], Qt::ISODate);
    
    // Parse OHLC values
    bar->open = parts[1].toDouble();
    bar->high = parts[2].toDouble();
    bar->low = parts[3].toDouble();
    bar->close = parts[4].toDouble();
    
    // Parse volume if available
    if (parts.size() > 5) {
        bar->volume = parts[5].toDouble();
    }
    
    return bar;
}

QDateTime DataLoader::calculateStartDate(const QDateTime& endDate, const QString& period)
{
    QDateTime startDate = endDate;
    
    if (period.endsWith("d")) {
        int days = period.left(period.length() - 1).toInt();
        startDate = endDate.addDays(-days);
    } else if (period.endsWith("m")) {
        int months = period.left(period.length() - 1).toInt();
        startDate = endDate.addMonths(-months);
    } else if (period.endsWith("y")) {
        int years = period.left(period.length() - 1).toInt();
        startDate = endDate.addYears(-years);
    }
    
    return startDate;
}

std::vector<OHLCBar> DataLoader::filterByPeriod(
    const std::vector<OHLCBar>& data,
    const QDateTime& startDate,
    const QDateTime& endDate)
{
    std::vector<OHLCBar> filtered;
    
    for (const auto& bar : data) {
        if (bar.timestamp >= startDate && bar.timestamp <= endDate) {
            filtered.push_back(bar);
        }
    }
    
    return filtered;
}

std::vector<OHLCBar> DataLoader::filterByTradingDays(
    const std::vector<OHLCBar>& data,
    const std::vector<int>& tradingDays)
{
    if (tradingDays.empty()) return data;
    
    std::vector<OHLCBar> filtered;
    
    for (const auto& bar : data) {
        int dayOfWeek = bar.timestamp.date().dayOfWeek() - 1; // Qt: 1=Monday -> 0=Monday
        if (std::find(tradingDays.begin(), tradingDays.end(), dayOfWeek) != tradingDays.end()) {
            filtered.push_back(bar);
        }
    }
    
    return filtered;
}

std::vector<OHLCBar> DataLoader::filterByTradingHours(
    const std::vector<OHLCBar>& data,
    const QTime& tradingFrom,
    const QTime& tradingTo)
{
    if (tradingFrom.isNull() || tradingTo.isNull()) return data;
    
    std::vector<OHLCBar> filtered;
    
    for (const auto& bar : data) {
        QTime barTime = bar.timestamp.time();
        if (barTime >= tradingFrom && barTime <= tradingTo) {
            filtered.push_back(bar);
        }
    }
    
    return filtered;
}

int DataLoader::intervalToSeconds(const QString& interval)
{
    if (interval.endsWith("secs")) {
        return interval.left(interval.length() - 4).toInt();
    } else if (interval.endsWith("min")) {
        return interval.left(interval.length() - 3).toInt() * 60;
    } else if (interval.endsWith("h")) {
        return interval.left(interval.length() - 1).toInt() * 3600;
    } else if (interval.endsWith("d")) {
        return interval.left(interval.length() - 1).toInt() * 86400;
    }
    return 60; // Défaut: 1 minute
}

std::vector<OHLCBar> DataLoader::resampleData(
    const std::vector<OHLCBar>& data,
    const QString& targetInterval)
{
    if (data.empty()) return data;
    
    int targetSeconds = intervalToSeconds(targetInterval);
    std::vector<OHLCBar> resampled;
    
    // Implémentation simplifiée du resampling
    // Dans un cas réel, vous voudriez une logique plus sophistiquée
    
    auto currentBar = data[0];
    QDateTime nextBoundary = currentBar.timestamp.addSecs(targetSeconds);
    
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i].timestamp >= nextBoundary) {
            resampled.push_back(currentBar);
            currentBar = data[i];
            nextBoundary = currentBar.timestamp.addSecs(targetSeconds);
        } else {
            // Agréger les données
            currentBar.high = std::max(currentBar.high, data[i].high);
            currentBar.low = std::min(currentBar.low, data[i].low);
            currentBar.close = data[i].close;
            currentBar.volume += data[i].volume;
        }
    }
    
    resampled.push_back(currentBar);
    return resampled;
}

// CORRECTION: Nouvelle implémentation de loadData qui correspond à la signature du header
std::vector<OHLCBar> DataLoader::loadData(
    const QString& symbol,
    const QString& interval,
    const QString& period,
    const QDateTime& endDate,
    const QTime& tradingFrom,
    const QTime& tradingTo,
    const std::vector<int>& tradingDays)
{
    std::vector<OHLCBar> result;
    
    try {
        PyBindingManager* pyManager = PyBindingManager::getInstance();
        if (!pyManager || !pyManager->isInitialized()) {
            qWarning() << "PyBindingManager non initialisé";
            return result;
        }
        
        // Utiliser la fonction Python helper pour charger les données
        void* pyData = pyManager->loadData(symbol, interval, period, 
                                          endDate.toString("dd/MM/yyyy"),
                                          tradingFrom, tradingTo);
        
        if (!pyData) {
            qWarning() << "Échec du chargement des données Python";
            return result;
        }
        
        // CORRECTION: Conversion correcte des données Python
        py::object data_obj = *static_cast<py::object*>(pyData);
        
        if (data_obj.is_none()) {
            qWarning() << "Données Python vides";
            return result;
        }
        
        // Obtenir les colonnes avec la syntaxe correcte
        py::object open_col = data_obj["open"];
        py::object high_col = data_obj["high"];
        py::object low_col = data_obj["low"];
        py::object close_col = data_obj["close"];
        py::object index = data_obj.attr("index");
        
        // Obtenir la taille
        int data_len = py::len(data_obj);
        
        // CORRECTION: Utilisation correcte de l'indexation iloc
        for (int i = 0; i < data_len; ++i) {
            try {
                // Utiliser py::int_(i) pour créer un objet entier Python
                py::int_ py_index(i);
                
                double open = py::float_(open_col.attr("iloc")[py_index]);
                double high = py::float_(high_col.attr("iloc")[py_index]);
                double low = py::float_(low_col.attr("iloc")[py_index]);
                double close = py::float_(close_col.attr("iloc")[py_index]);
                
                // Pour l'index (timestamp)
                py::object timestamp = index[py_index];
                
                // Conversion du timestamp Python vers QDateTime
                QString timestamp_str = py::str(timestamp).cast<std::string>().c_str();
                QDateTime dt = QDateTime::fromString(timestamp_str, Qt::ISODate);
                
                if (dt.isValid()) {
                    result.emplace_back(dt, open, high, low, close);
                }
            } catch (const std::exception& e) {
                qWarning() << "Erreur lors de la conversion de la ligne" << i << ":" << e.what();
                continue;
            }
        }
        
        qInfo() << "Chargé" << result.size() << "barres depuis Python";
        
    } catch (const std::exception& e) {
        qWarning() << "Erreur lors du chargement des données:" << e.what();
    }
    
    // Appliquer les filtres si spécifiés
    if (!tradingDays.empty()) {
        result = filterByTradingDays(result, tradingDays);
    }
    
    if (!tradingFrom.isNull() && !tradingTo.isNull()) {
        result = filterByTradingHours(result, tradingFrom, tradingTo);
    }
    
    return result;
}

// CORRECTION: Supprimer la méthode en doublon qui retourne void*
// Cette méthode entre en conflit avec celle du header

// CORRECTION: Nouvelle implémentation de loadFromCSV avec les bons types
std::vector<OHLCBar> DataLoader::loadFromCSV(
    const QString& filePath,
    const QString& period,
    const QString& endDate,
    const QTime& tradingFrom,
    const QTime& tradingTo)
{
    std::vector<OHLCBar> result;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Impossible d'ouvrir le fichier:" << filePath;
        return result;
    }
    
    QTextStream in(&file);
    bool isFirstLine = true;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        
        auto bar = parseCSVLine(line, isFirstLine);
        if (bar) {
            result.push_back(*bar);
        }
        
        isFirstLine = false;
    }
    
    // Appliquer les filtres de période et d'heures de trading
    QDateTime actualEndDate = QDateTime::fromString(endDate, "dd/MM/yyyy");
    if (actualEndDate.isValid()) {
        QDateTime startDate = calculateStartDate(actualEndDate, period);
        result = filterByPeriod(result, startDate, actualEndDate);
    }
    
    if (!tradingFrom.isNull() && !tradingTo.isNull()) {
        result = filterByTradingHours(result, tradingFrom, tradingTo);
    }
    
    qInfo() << "Chargé" << result.size() << "barres depuis CSV:" << filePath;
    return result;
}