#pragma once

#include <string>
#include <memory>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>
#include "lightstreamer.h"

// Forward declarations
namespace ig {
    class IGService;
}

namespace ig {

/**
 * @brief Structure representing ticker data
 */
struct TickerData {
    std::string epic;
    std::string utm;        // Update time
    double bid = 0.0;
    double offer = 0.0;
    double last_traded_price = 0.0;
    int total_traded_volume = 0;
    std::string last_update_time;
    
    // Default constructor
    TickerData() = default;
    
    // Constructor with epic
    TickerData(const std::string& epic_name) : epic(epic_name) {}
};

/**
 * @brief Structure representing candle data
 */
struct CandleData {
    std::string epic;
    std::string utm;        // Update time
    double bid = 0.0;
    double offer = 0.0;
    double last_traded_price = 0.0;
    int total_traded_volume = 0;
    std::string last_update_time;
    std::string consolidation_level;
    
    // Default constructor
    CandleData() = default;
    
    // Constructor with epic
    CandleData(const std::string& epic_name) : epic(epic_name) {}
};

/**
 * @brief IG Stream Service
 * 
 * Main service class for IG Markets streaming API.
 * Provides high-level interface for streaming market data.
 */
class IGStreamService {
public:
    /**
     * @brief Constructor
     * @param ig_service Shared pointer to IG REST service
     */
    explicit IGStreamService(std::shared_ptr<IGService> ig_service);

    /**
     * @brief Destructor
     */
    ~IGStreamService();

    /**
     * @brief Create streaming session
     * @param encryption Whether to use encryption (default: false)
     * @param version API version (default: "2")
     */
    void create_session(bool encryption = false, const std::string& version = "2");

    /**
     * @brief Subscribe to items
     * @param subscription Subscription object
     */
    void subscribe(const lightstreamer::Subscription& subscription);

    /**
     * @brief Unsubscribe from items
     * @param subscription Subscription object
     */
    void unsubscribe(const lightstreamer::Subscription& subscription);

    /**
     * @brief Unsubscribe from all items
     */
    void unsubscribe_all();

    /**
     * @brief Disconnect from streaming service
     */
    void disconnect();

    /**
     * @brief Check if service is connected
     * @return True if connected
     */
    bool is_connected() const;

    /**
     * @brief Get the Lightstreamer client
     * @return Shared pointer to client
     */
    std::shared_ptr<lightstreamer::LightstreamerClient> get_client() const {
        return ls_client_;
    }

private:
    std::shared_ptr<IGService> ig_service_;
    std::string lightstreamer_endpoint_;
    std::string acc_number_;
    std::shared_ptr<lightstreamer::LightstreamerClient> ls_client_;
    
    // Track subscriptions for unsubscribe_all
    std::map<const lightstreamer::Subscription*, int> subscription_keys_;
    std::mutex subscriptions_mutex_;
};

/**
 * @brief Streaming Manager
 * 
 * High-level manager for streaming operations with built-in data handling
 * and consumer thread for processing updates.
 */
class StreamingManager {
public:
    /**
     * @brief Constructor
     * @param service Shared pointer to IG stream service
     */
    explicit StreamingManager(std::shared_ptr<IGStreamService> service);

    /**
     * @brief Destructor
     */
    ~StreamingManager();

    /**
     * @brief Start tick subscription for an epic
     * @param epic Market epic to subscribe to
     */
    void start_tick_subscription(const std::string& epic);

    /**
     * @brief Start candle subscription for an epic
     * @param epic Market epic to subscribe to
     * @param resolution Candle resolution (default: "SECOND")
     */
    void start_candle_subscription(const std::string& epic, const std::string& resolution = "SECOND");

    /**
     * @brief Get ticker data for an epic
     * @param epic Market epic
     * @param timeout_seconds Timeout in seconds (default: 3)
     * @return Ticker data
     */
    TickerData get_ticker(const std::string& epic, int timeout_seconds = 3);

    /**
     * @brief Get candle data for an epic
     * @param epic Market epic
     * @param timeout_seconds Timeout in seconds (default: 3)
     * @return Candle data
     */
    CandleData get_candle(const std::string& epic, int timeout_seconds = 3);

    /**
     * @brief Stop all subscriptions
     */
    void stop_all_subscriptions();

    /**
     * @brief Check if manager is running
     * @return True if running
     */
    bool is_running() const { return consumer_running_.load(); }

private:
    std::shared_ptr<IGStreamService> service_;
    
    // Subscription management
    std::map<std::string, std::shared_ptr<lightstreamer::Subscription>> subscriptions_;
    std::mutex subscriptions_mutex_;
    
    // Data storage
    std::map<std::string, TickerData> tickers_;
    std::map<std::string, CandleData> candles_;
    std::mutex data_mutex_;
    
    // Consumer thread for processing updates
    std::queue<lightstreamer::ItemUpdate> update_queue_;
    std::thread consumer_thread_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> consumer_running_;
    
    /**
     * @brief Consumer thread function
     */
    void consumer_worker();
    
    /**
     * @brief Process an item update
     * @param update Item update to process
     */
    void process_update(const lightstreamer::ItemUpdate& update);
    
    /**
     * @brief Update listener for subscriptions
     * @param update Item update
     */
    void on_item_update(const lightstreamer::ItemUpdate& update);
    
    /**
     * @brief Parse ticker update
     * @param epic Market epic
     * @param values Field values
     */
    void parse_ticker_update(const std::string& epic, const std::map<std::string, std::string>& values);
    
    /**
     * @brief Parse candle update
     * @param epic Market epic
     * @param values Field values
     */
    void parse_candle_update(const std::string& epic, const std::map<std::string, std::string>& values);
    
    /**
     * @brief Extract epic from item name
     * @param item_name Full item name
     * @return Epic name
     */
    std::string extract_epic_from_item(const std::string& item_name);
};

} // namespace ig