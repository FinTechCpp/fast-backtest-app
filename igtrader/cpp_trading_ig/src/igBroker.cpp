#include "spdlog/spdlog.h"
#include "igBroker.h"
#include <nlohmann/json.hpp>

namespace ig {

IGBroker::IGBroker(IGService& service) 
    : service_(service) {
    spdlog::debug("IGBroker initialized");
}

bool IGBroker::hasOpenPosition() {
    spdlog::debug("Checking for open positions...");
    
    // Fetch open positions
    nlohmann::json positions_data = service_.fetch_open_positions();
    
    // The API returns a 'positions' array containing all open positions
    if (positions_data.contains("positions") && positions_data["positions"].is_array()) {
        const auto& positions = positions_data["positions"];
        bool has_positions = !positions.empty();
        
        spdlog::debug("Found {} open positions", positions.size());
        return has_positions;
    } else {
        spdlog::warn("Unexpected response format from fetch_open_positions()");
        return false;
    }
}

bool IGBroker::moveStopLoss(const std::string& dealId, double stopLevel) {
    spdlog::debug("Moving stop loss for position {} to level {}", dealId, stopLevel);
    
    auto position = service_.fetch_open_position_by_deal_id(dealId);
    
    // Create parameters for position update
    PositionUpdateParams params;
    params.dealId = dealId;
    params.stopLevel = stopLevel;
    
    // Keep existing settings for guaranteed and trailing stops
    if (position.contains("position")) {
        params.guaranteedStop = position["position"]["guaranteedStop"].get<bool>();
        params.trailingStop = position["position"]["trailingStop"].get<bool>();
        
        // If it's a trailing stop, preserve the increment if available
        if (params.trailingStop && position["position"].contains("trailingStopIncrement")) {
            params.trailingStopIncrement = position["position"]["trailingStopIncrement"].get<double>();
        }
    }
    
    // Update the position
    service_.update_open_position(params);
    spdlog::info("Successfully moved stop loss for position {} to level {}", dealId, stopLevel);
    return true;
}

} // namespace ig