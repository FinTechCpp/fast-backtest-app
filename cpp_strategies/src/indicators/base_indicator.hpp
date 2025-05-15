#pragma once

/**
 * Base class for all incremental indicators
 */
class IncrementalIndicator {
protected:
    bool is_initialized = false;
    
public:
    virtual ~IncrementalIndicator() = default;
    bool requires_initialization() const { return !is_initialized; }
    bool initialized() const { return is_initialized; }
};