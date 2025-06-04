#include "data.hpp"
#include <stdexcept>
#include <algorithm>

namespace be {

Data::Data(const std::vector<Date>& dates,
           const std::vector<double>& open,
           const std::vector<double>& high,
           const std::vector<double>& low,
           const std::vector<double>& close,
           const std::vector<double>& volume) {
    
    // Vérification que tous les vecteurs ont la même taille
    size_t size = dates.size();
    if (open.size() != size || high.size() != size || 
        low.size() != size || close.size() != size) {
        throw std::invalid_argument("Tous les vecteurs OHLC doivent avoir la même taille");
    }
    
    // Utiliser un vecteur vide pour le volume si non fourni
    std::vector<double> actualVolume = volume;
    if (actualVolume.empty()) {
        actualVolume.resize(size, 0.0);
    } else if (actualVolume.size() != size) {
        throw std::invalid_argument("Le vecteur de volume doit avoir la même taille");
    }
    
    // Construire les bougies
    _candles.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        _candles.push_back({
            dates[i],
            open[i],
            high[i],
            low[i],
            close[i],
            actualVolume[i]
        });
    }
}

Data::Data(std::vector<Candle> candles)
    : _candles(std::move(candles)) {
}

void Data::addColumn(const std::string& name, const std::vector<double>& values) {
    if (values.size() != _candles.size()) {
        throw std::invalid_argument(
            "Le vecteur de valeurs doit avoir la même taille que le nombre de bougies");
    }
    
    for (size_t i = 0; i < _candles.size(); ++i) {
        _candles[i].customValues[name] = values[i];
    }
}

const Candle& Data::at(size_t index) const {
    if (index >= _candles.size()) {
        throw std::out_of_range("Index de bougie hors limites");
    }
    return _candles[index];
}

std::vector<Date> Data::getDates() const {
    std::vector<Date> result;
    result.reserve(_candles.size());
    for (const auto& candle : _candles) {
        result.push_back(candle.date);
    }
    return result;
}

std::vector<double> Data::getOpen() const {
    std::vector<double> result;
    result.reserve(_candles.size());
    for (const auto& candle : _candles) {
        result.push_back(candle.open);
    }
    return result;
}

std::vector<double> Data::getHigh() const {
    std::vector<double> result;
    result.reserve(_candles.size());
    for (const auto& candle : _candles) {
        result.push_back(candle.high);
    }
    return result;
}

std::vector<double> Data::getLow() const {
    std::vector<double> result;
    result.reserve(_candles.size());
    for (const auto& candle : _candles) {
        result.push_back(candle.low);
    }
    return result;
}

std::vector<double> Data::getClose() const {
    std::vector<double> result;
    result.reserve(_candles.size());
    for (const auto& candle : _candles) {
        result.push_back(candle.close);
    }
    return result;
}

std::vector<double> Data::getVolume() const {
    std::vector<double> result;
    result.reserve(_candles.size());
    for (const auto& candle : _candles) {
        result.push_back(candle.volume);
    }
    return result;
}

std::vector<double> Data::getCustomColumn(const std::string& name) const {
    std::vector<double> result;
    result.reserve(_candles.size());
    for (const auto& candle : _candles) {
        auto it = candle.customValues.find(name);
        if (it == candle.customValues.end()) {
            // Utiliser NaN ou une valeur par défaut si la colonne n'existe pas
            result.push_back(std::numeric_limits<double>::quiet_NaN());
        } else {
            result.push_back(it->second);
        }
    }
    return result;
}

bool Data::moveNext() {
    if (_position < _candles.size()) {
        ++_position;
        return _position < _candles.size(); // True s'il reste des éléments
    }
    return false;
}

const Candle& Data::current() const {
    if (_position >= _candles.size()) {
        throw std::runtime_error("Tentative d'accès au-delà de la dernière bougie");
    }
    return _candles[_position];
}

double Data::currentCustomValue(const std::string& name) const {
    const auto& candle = current();
    auto it = candle.customValues.find(name);
    if (it == candle.customValues.end()) {
        throw std::out_of_range("Colonne personnalisée non trouvée: " + name);
    }
    return it->second;
}

std::vector<Candle> Data::lookback(size_t n) const {
    if (n == 0) {
        throw std::invalid_argument("Le nombre de bougies pour lookback doit être > 0");
    }
    
    if (n > _position + 1) {
        throw std::runtime_error(
            "Impossible de regarder " + std::to_string(n) + 
            " bougies en arrière à la position " + std::to_string(_position));
    }
    
    std::vector<Candle> result;
    result.reserve(n);
    
    // Extraire les n dernières bougies (incluant la courante)
    size_t start = _position + 1 - n;
    for (size_t i = 0; i < n; ++i) {
        result.push_back(_candles[start + i]);
    }
    
    return result;
}

} // namespace be