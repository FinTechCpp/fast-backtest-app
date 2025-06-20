#pragma once

#include "rest.h"
#include "lightstreamer.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace ig {

/**
 * @brief Ticker data structure for market updates
 */
struct Ticker {
    std::string epic;
    std::string timestamp;
    double bid = 0.0;
    double offer = 0.0;
    double last_traded_price = 0.0;
    int last_traded_volume = 0;
    int incr_volume = 0;
    double day_open_mid = 0.0;
    double day_net_change_mid = 0.0;
    double day_percent_change_mid = 0.0;
    double day_high = 0.0;
    double day_low = 0.0;
    
    Ticker(const std::string& epic_) : epic(epic_) {}
    
    void populate(const std::unordered_map<std::string, std::string>& values);
};

/**
 * @brief Ticker listener interface
 */
class TickerListener : public SubscriptionListener {
public:
    virtual void onTicker(const Ticker& ticker) = 0;
    
    void onItemUpdate(const std::string& item_name, 
                     const std::unordered_map<std::string, std::string>& values) override;
};

/**
 * @brief Ticker subscription
 */
class TickerSubscription : public Subscription {
public:
    static const std::vector<std::string> TICKER_FIELDS;
    
    explicit TickerSubscription(const std::string& epic);
};

/**
 * @brief IG Streaming service
 */
class IGStreamService {
public:
    /**
     * @brief Constructor
     * @param ig_service REST service to use for authentication
     */
    explicit IGStreamService(IGService& ig_service);
    
    /**
     * @brief Destructor
     */
    ~IGStreamService();
    
    /**
     * @brief Connect to the streaming service
     * @param account_id Account ID to use
     */
    void connect(const std::string& account_id = "");
    
    /**
     * @brief Disconnect from the streaming service
     */
    void disconnect();
    
    /**
     * @brief Subscribe to market ticks
     * @param epic Market epic to subscribe to
     * @param listener Listener for ticker updates
     * @return Subscription pointer
     */
    std::shared_ptr<TickerSubscription> subscribeToTicks(
        const std::string& epic, 
        std::shared_ptr<TickerListener> listener);
    
    /**
     * @brief Unsubscribe from all streams
     */
    void unsubscribeAll();
    
    /**
     * @brief Check if service is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Get a ticker by epic
     * @param epic Epic to get ticker for
     * @param timeout_seconds Timeout in seconds
     * @return Ticker data
     */
    Ticker getTicker(const std::string& epic, int timeout_seconds = 3);

private:
    IGService& ig_service_;
    std::string lightstreamer_endpoint_;
    std::string account_id_;
    std::shared_ptr<LightstreamerClient> ls_client_;
    std::unordered_map<std::string, std::shared_ptr<Ticker>> tickers_;
    std::unordered_map<std::string, std::shared_ptr<TickerSubscription>> subscriptions_;
    std::mutex tickers_mutex_;
    
    /**
     * @brief Create a session with the streaming service
     */
    void createSession();
};

} // namespace ig