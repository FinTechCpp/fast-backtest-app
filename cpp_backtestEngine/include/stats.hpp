#pragma once

#include <vector>
#include <map>
#include <string>
#include "trade.hpp"
#include "data.hpp"

// TODO: remplacer par une structure pour contenir les données et non des strings
/**
 * @brief Calculate performance statistics
 */
std::map<std::string, double> computeStats(
    const std::vector<std::shared_ptr<Trade>>& trades,
    const std::vector<double>& equity,
    const Data& data);

/**
 * @brief Get empty stats with default values
 */
std::map<std::string, double> dummyStats();