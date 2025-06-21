#include "spdlog/spdlog.h"
#include "igBroker.h"
#include <nlohmann/json.hpp>

namespace ig {

IGBroker::IGBroker(IGService& service) 
    : service_(service) {
    spdlog::debug("IGBroker initialized");
}

bool IGBroker::hasOpenPosition() {
    try {
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
    } catch (const std::exception& e) {
        spdlog::error("Error checking for open positions: {}", e.what());
        return false;
    }
}

} // namespace ig