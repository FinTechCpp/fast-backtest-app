#include "data.hpp"
#include <stdexcept>
#include <algorithm>

namespace be {

Data::Data(const std::vector<Date>& dates,
           const std::vector<double>& open,
           const std::vector<double>& high,
           const std::vector<double>& low,
           const std::vector<double>& close,
           const std::vector<double>& volume) 
           : _dates(dates), _open(open), _high(high), _low(low), _close(close), _volume(volume) {

    // Validation des dimensions
    if (_dates.size() != _open.size() || _dates.size() != _high.size() ||
        _dates.size() != _low.size() || _dates.size() != _close.size() ||
        (!_volume.empty() && _dates.size() != _volume.size())) {
        throw std::invalid_argument("Tous les vecteurs doivent avoir la même taille");
    }
    
    // Si volume est vide, l'initialiser avec des zéros
    if (_volume.empty()) {
        _volume.resize(_dates.size(), 0.0);
    }
}

Data::Data(std::vector<Date>&& dates,
           std::vector<double>&& open,
           std::vector<double>&& high,
           std::vector<double>&& low,
           std::vector<double>&& close,
           std::vector<double>&& volume) 
           : _dates(std::move(dates)), _open(std::move(open)), _high(std::move(high)),
             _low(std::move(low)), _close(std::move(close)), _volume(std::move(volume)) {

    // Validation des dimensions
    if (_dates.size() != _open.size() || _dates.size() != _high.size() ||
        _dates.size() != _low.size() || _dates.size() != _close.size() ||
        (!_volume.empty() && _dates.size() != _volume.size())) {
        throw std::invalid_argument("Tous les vecteurs doivent avoir la même taille");
    }
    
    // Si volume est vide, l'initialiser avec des zéros
    if (_volume.empty()) {
        _volume.resize(_dates.size(), 0.0);
    }
}

void Data::addColumn(const std::string& name, const std::vector<double>& values) {
    if (values.size() != _dates.size()) {
        throw std::invalid_argument(
            "Le vecteur de valeurs doit avoir la même taille que le nombre de bougies");
    }

    _customColumns[name] = values;
}

const Candle& Data::at(size_t index) const {
    if (index >= _dates.size()) {
        throw std::out_of_range("Index de bougie hors limites");
    }

    // Réutiliser une Candle temporaire
    _tempCandle.date = _dates[index];
    _tempCandle.open = _open[index];
    _tempCandle.high = _high[index];
    _tempCandle.low = _low[index];
    _tempCandle.close = _close[index];
    _tempCandle.volume = _volume[index];
    
    // Ajouter les valeurs personnalisées
    _tempCandle.customValues.clear();
    for (const auto& [name, values] : _customColumns)
        if (index < values.size())
            _tempCandle.customValues[name] = values[index];
    
    return _tempCandle;
}

bool Data::moveNext() {
    if (_position < _dates.size()) {
        ++_position;
        return _position < _dates.size(); // True s'il reste des éléments
    }
    return false;
}

const Candle& Data::current() const {
    return at(_position);
}

Date Data::currentDate() const {
    if (_position >= _dates.size())
        throw std::runtime_error("Position actuelle invalide");
    return _dates[_position];
}

double Data::currentOpen() const {
    if (_position >= _open.size())
        throw std::runtime_error("Position actuelle invalide");
    return _open[_position];
}

double Data::currentHigh() const {
    if (_position >= _high.size())
        throw std::runtime_error("Position actuelle invalide");
    return _high[_position];
}

double Data::currentLow() const {
    if (_position >= _low.size())
        throw std::runtime_error("Position actuelle invalide");
    return _low[_position];
}

double Data::currentClose() const {
    if (_position >= _close.size())
        throw std::runtime_error("Position actuelle invalide");
    return _close[_position];
}

double Data::currentVolume() const {
    if (_position >= _volume.size())
        throw std::runtime_error("Position actuelle invalide");
    return _volume[_position];
}

// std::vector<Candle> Data::lookback(size_t n) const
// {
//     if (n == 0) {
//         throw std::invalid_argument("Le nombre de bougies pour lookback doit être > 0");
//     }
    
//     if (n > _position + 1) {
//         throw std::runtime_error(
//             "Impossible de regarder " + std::to_string(n) + 
//             " bougies en arrière à la position " + std::to_string(_position));
//     }
    
//     std::vector<Candle> result;
//     result.reserve(n);
    
//     // Extraire les n dernières bougies (incluant la courante)
//     size_t start = _position + 1 - n;
//     for (size_t i = 0; i < n; ++i) {
//         result.push_back(at(start + i));
//     }
    
//     return result;
// }

} // namespace be