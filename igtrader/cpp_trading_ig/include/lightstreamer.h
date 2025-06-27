#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <map>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// Forward declarations
namespace cpr {
    class Session;
    class Response;
}

namespace ig {
    class IGService;
}

namespace lightstreamer {

// Forward declarations
class Subscription;
class LightstreamerClient;

/**
 * @brief Structure representing an item update from Lightstreamer
 */
struct ItemUpdate {
    int pos;                                           // Position of the item
    std::string item_name;                            // Name of the item
    std::map<std::string, std::string> values;        // Field values
};

/**
 * @brief Subscription class for Lightstreamer items
 * 
 * Represents a subscription to be submitted to a Lightstreamer Server.
 * Handles item updates and notifies registered listeners.
 */
class Subscription {
public:
    /**
     * @brief Constructor
     * @param mode Subscription mode (MERGE, DISTINCT, COMMAND)
     * @param items List of item names to subscribe to
     * @param fields List of field names to subscribe to
     * @param adapter Data adapter name (optional)
     */
    Subscription(const std::string& mode, 
                const std::vector<std::string>& items,
                const std::vector<std::string>& fields,
                const std::string& adapter = "");

    /**
     * @brief Add a listener for item updates
     * @param listener Function to call when an update is received
     */
    void add_listener(std::function<void(const ItemUpdate&)> listener);

    /**
     * @brief Notify all listeners of an item update
     * @param item_line Raw item line from Lightstreamer
     */
    void notify_update(const std::string& item_line);

    /**
     * @brief Decode a field value according to Lightstreamer protocol
     * @param value Raw value from server
     * @param last_value Previous value for this field
     * @return Decoded value
     */
    std::string decode_value(const std::string& value, const std::string& last_value);

    // Getters
    const std::vector<std::string>& get_item_names() const { return item_names_; }
    const std::vector<std::string>& get_field_names() const { return field_names_; }
    const std::string& get_adapter() const { return adapter_; }
    const std::string& get_mode() const { return mode_; }

private:
    std::vector<std::string> item_names_;
    std::vector<std::string> field_names_;
    std::string adapter_;
    std::string mode_;
    std::string snapshot_;
    
    // Map of item position to current field values
    std::map<int, std::map<std::string, std::string>> items_map_;
    
    // List of update listeners
    std::vector<std::function<void(const ItemUpdate&)>> listeners_;
    
    // Mutex for thread safety
    std::mutex listeners_mutex_;
    std::mutex items_mutex_;
};

/**
 * @brief Lightstreamer client for IG Markets
 * 
 * Manages the communication with Lightstreamer Server using IG's specific
 * HTTP long-polling protocol.
 */
class LightstreamerClient {
public:
    /**
     * @brief Constructor
     * @param base_url Base URL for Lightstreamer server
     * @param adapter_set Adapter set name (empty for IG)
     * @param user Username (account number for IG)
     * @param password Password in IG format (CST-xxx|XST-xxx)
     */
    LightstreamerClient(const std::string& base_url, 
                       const std::string& adapter_set = "",
                       const std::string& user = "", 
                       const std::string& password = "");

    /**
     * @brief Destructor
     */
    ~LightstreamerClient();

    /**
     * @brief Connect to Lightstreamer server
     */
    void connect();

    /**
     * @brief Bind to existing session (after LOOP)
     */
    void bind();

    /**
     * @brief Disconnect from server
     */
    void disconnect();

    /**
     * @brief Subscribe to items
     * @param subscription Subscription object
     * @return Subscription key for unsubscribing
     */
    int subscribe(std::shared_ptr<Subscription> subscription);

    /**
     * @brief Unsubscribe from items
     * @param subscription_key Key returned by subscribe()
     */
    void unsubscribe(int subscription_key);

    /**
     * @brief Set connection parameters
     * @param user Username
     * @param password Password
     */
    void set_credentials(const std::string& user, const std::string& password);

    /**
     * @brief Check if client is connected
     * @return True if connected
     */
    bool is_connected() const { return active_connection_.load(); }

private:
    // Connection parameters
    std::string base_url_;
    std::string control_url_;
    std::string adapter_set_;
    std::string user_;
    std::string password_;
    
    // Session state
    std::string session_id_;
    std::map<std::string, std::string> session_params_;
    
    // Subscriptions management
    std::map<int, std::shared_ptr<Subscription>> subscriptions_;
    int current_subscription_key_;
    std::mutex subscriptions_mutex_;
    
    // Threading
    std::thread stream_thread_;
    std::atomic<bool> active_connection_;
    std::atomic<bool> should_stop_;
    
    // HTTP session for streaming
    std::unique_ptr<cpr::Session> stream_session_;
    std::unique_ptr<cpr::Session> control_session_;
    
    // Content length for streaming
    int64_t content_length_;
    
    // Bind counter for thread naming
    int bind_counter_;

    /**
     * @brief Handle the streaming connection
     */
    void handle_stream();

    /**
     * @brief Receive and process messages from stream
     */
    void receive_messages();

    /**
     * @brief Read a single line from stream
     * @return Line content
     */
    std::string read_stream_line();

    /**
     * @brief Forward update message to appropriate subscription
     * @param message Update message
     */
    void forward_update_message(const std::string& message);

    /**
     * @brief Make a control request
     * @param params Request parameters
     * @return HTTP response
     */
    cpr::Response make_control_request(const std::map<std::string, std::string>& params);

    /**
     * @brief Set control link URL
     * @param custom_address Custom address from session (optional)
     */
    void set_control_link_url(const std::string& custom_address = "");

    /**
     * @brief Join streaming thread
     */
    void join_stream_thread();

    /**
     * @brief URL encode parameters
     * @param params Parameters to encode
     * @return Encoded string
     */
    std::string url_encode_params(const std::map<std::string, std::string>& params);
};

} // namespace lightstreamer