#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <map>

/**
 * @brief OHLCV data container with time series access methods
 */
class Data {
public:
    // TODO : Creer un class pour la date plutot que de stocker des 
    // strings qui ne donnent pas de garantie sur le format 
    Data(const std::vector<std::string>& dates,
         const std::vector<double>& open,
         const std::vector<double>& high,
         const std::vector<double>& low,
         const std::vector<double>& close,
         const std::vector<double>& volume = {});
    
    // Data accessors
    size_t size() const;
    
    // Vector accessors
    const std::vector<double>& Open() const;
    const std::vector<double>& High() const;
    const std::vector<double>& Low() const;
    const std::vector<double>& Close() const;
    const std::vector<double>& Volume() const;

    size_t getActualIndex(int index) const;
    
    // Index-based access (to simulate Python's data.Close[-1])
    double Open(int index) const;
    double High(int index) const;
    double Low(int index) const;
    double Close(int index) const;
    double Volume(int index) const;
    std::string getDate(int index) const;
    
    // For slicing data in backtest execution
    void setLength(size_t length);
    
    // Custom column support
    void addColumn(const std::string& name, const std::vector<double>& values);
    const std::vector<double>& getColumn(const std::string& name) const;
    
private:
    std::vector<std::string> _dates;
    std::vector<double> _open;
    std::vector<double> _high;
    std::vector<double> _low;
    std::vector<double> _close;
    std::vector<double> _volume;
    std::map<std::string, std::vector<double>> _customColumns;
    size_t _currentLength;
};