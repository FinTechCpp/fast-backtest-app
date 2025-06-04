#pragma once

#include <memory>

namespace be {

// Forward declarations
class Broker;

/**
 * @brief Current market position representation
 */
class Position {
public:
    Position(std::shared_ptr<Broker> broker);
    
    // Check if position exists
    explicit operator bool() const;
    
    // Close position
    void close(double portion = 1.0);
    
    // Position properties
    double size() const;
    double pl() const;
    double plPercent() const;
    bool isLong() const;
    bool isShort() const;
    
private:
    std::shared_ptr<Broker> _broker;
};

} // namespace be