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
    
    // Initialize gap detection
    _hasGapAfter.resize(_dates.size(), false);
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
    
    // Initialize gap detection
    _hasGapAfter.resize(_dates.size(), false);
}

Data::Data(const std::vector<Candle>& candles) {
    _dates.reserve(candles.size());
    _open.reserve(candles.size());
    _high.reserve(candles.size());
    _low.reserve(candles.size());
    _close.reserve(candles.size());
    _volume.reserve(candles.size());

    for (const auto& candle : candles) {
        _dates.push_back(candle.date);
        _open.push_back(candle.open);
        _high.push_back(candle.high);
        _low.push_back(candle.low);
        _close.push_back(candle.close);
        _volume.push_back(candle.volume);
    }

    // Initialize gap detection
    _hasGapAfter.resize(_dates.size(), false);
}

Candle Data::at(size_t index) const {
    if (index >= _dates.size()) {
        throw std::out_of_range("Index de bougie hors limites");
    }

    Candle candle;

    // Réutiliser une Candle temporaire
    candle.date = _dates[index];
    candle.open = _open[index];
    candle.high = _high[index];
    candle.low = _low[index];
    candle.close = _close[index];
    candle.volume = _volume[index];

    return candle;
}

bool Data::moveNext() {
    if (_position < _dates.size()) {
        ++_position;
        return _position < _dates.size(); // True s'il reste des éléments
    }
    return false;
}

Candle Data::current() const {
    return at(_position);
}

Date Data::currentDate() const {
    return _dates[_position];
}

double Data::currentOpen() const {
    return _open[_position];
}

double Data::currentHigh() const {
    return _high[_position];
}

double Data::currentLow() const {
    return _low[_position];
}

double Data::currentClose() const {
    return _close[_position];
}

double Data::currentVolume() const {
    return _volume[_position];
}

void Data::setGapIndices(const std::vector<size_t>& gapIndices) {
    // Initialize all to false first
    _hasGapAfter.resize(_dates.size(), false);
    
    // Mark the specified indices as having gaps
    for (size_t index : gapIndices) {
        if (index < _dates.size()) {
            _hasGapAfter[index] = true;
        }
    }
}

bool Data::hasGapAfterCurrent() const {
    return hasGapAfterIndex(_position);
}

bool Data::hasGapAfterIndex(size_t index) const {
    if (index >= _dates.size()) {
        throw std::out_of_range("Index de bougie hors limites");
    }
    
    return _hasGapAfter[index];
}

} // namespace be