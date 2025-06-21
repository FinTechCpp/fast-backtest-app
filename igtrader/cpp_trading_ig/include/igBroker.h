#include <spdlog/spdlog.h>
#include "rest.h"
#include "lightstreamer.h"

#pragma once

#include <spdlog/spdlog.h>
#include "rest.h"
#include "lightstreamer.h"

namespace ig {

/**
 * @brief Broker class for handling IG trading operations
 * 
 * This class provides higher-level abstractions over the IG API
 * for common broker operations like checking open positions.
 */
class IGBroker {
public:
    /**
     * @brief Constructor
     * 
     * @param service Reference to an initialized IGService
     */
    explicit IGBroker(IGService& service);
    
    /**
     * @brief Check if there are any open positions
     * 
     * @return true if at least one position is open, false otherwise
     */
    bool hasOpenPosition();

    /**
     * @brief Move the stop loss of an open position
     * @param dealId The ID of the open position to modify
     * @param stopLevel The new stop level to set
     * @return true if the operation was successful, false otherwise
     */
    bool moveStopLoss(const std::string& dealId, double stopLevel);

private:
    IGService& service_;
};

} // namespace ig