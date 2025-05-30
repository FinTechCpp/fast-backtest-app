#include "data.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

Data::Data(const std::vector<Date>& dates,
           const std::vector<double>& open,
           const std::vector<double>& high,
           const std::vector<double>& low,
           const std::vector<double>& close,
           const std::vector<double>& volume)
    : _dates(dates),
      _open(open),
      _high(high),
      _low(low),
      _close(close),
      _currentLength(dates.size()) 
{
    // Validate data
    if (_dates.empty()) {
        throw std::invalid_argument("Empty data provided");
    }
    
    // Check if all arrays have the same size
    const size_t dataSize = _dates.size();
    if (_open.size() != dataSize || _high.size() != dataSize ||
        _low.size() != dataSize || _close.size() != dataSize) {
        throw std::invalid_argument("All data arrays must have the same size");
    }
    
    // Handle volume data - if empty, initialize with zeros
    if (volume.empty()) {
        _volume.resize(dataSize, 0.0);
    } else {
        if (volume.size() != dataSize) {
            throw std::invalid_argument("Volume array size must match other data arrays");
        }
        _volume = volume;
    }
}

size_t Data::size() const {
    return _currentLength;
}

// Vector accessors return the entire vector up to current length
const std::vector<double>& Data::Open() const {
    return _open;
}

const std::vector<double>& Data::High() const {
    return _high;
}

const std::vector<double>& Data::Low() const {
    return _low;
}

const std::vector<double>& Data::Close() const {
    return _close;
}

const std::vector<double>& Data::Volume() const {
    return _volume;
}

// Private helper method for index calculation and bounds checking
size_t Data::getActualIndex(int index) const {
    size_t actualIndex = (index >= 0) ? 
        static_cast<size_t>(index) : 
        _currentLength + static_cast<size_t>(index);
        
    if (actualIndex >= _currentLength) {
        throw std::out_of_range("Index out of range");
    }
    return actualIndex;
}

// Index-based accessors with Python-like negative indexing
double Data::Open(int index) const {
    return _open[getActualIndex(index)];
}

double Data::High(int index) const {
    return _high[getActualIndex(index)];
}

double Data::Low(int index) const {
    return _low[getActualIndex(index)];
}

double Data::Close(int index) const {
    return _close[getActualIndex(index)];
}

double Data::Volume(int index) const {
    return _volume[getActualIndex(index)];
}

Date Data::getDate(int index) const {
    return _dates[getActualIndex(index)];
}

void Data::setLength(size_t length) {
    if (length > _dates.size()) {
        throw std::invalid_argument("Length cannot exceed original data size");
    }
    _currentLength = length;
}

void Data::addColumn(const std::string& name, const std::vector<double>& values) {
    if (values.size() != _dates.size()) {
        throw std::invalid_argument("Column size must match existing data size");
    }
    _customColumns[name] = values;
}

const std::vector<double>& Data::getColumn(const std::string& name) const {
    auto it = _customColumns.find(name);
    if (it == _customColumns.end()) {
        throw std::out_of_range("Column not found: " + name);
    }
    return it->second;
}