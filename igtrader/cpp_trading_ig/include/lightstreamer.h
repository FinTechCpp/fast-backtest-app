#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

namespace ig {

// Forward declarations
class LightstreamerClient;

/**
 * @brief Listener interface for item updates
 */
class SubscriptionListener {
public:
    virtual ~SubscriptionListener() = default;

    /**
     * @brief Called when new data for an item arrives
     * @param item_name Name of the item
     * @param values Map of field values for the item
     */
    virtual void onItemUpdate(const std::string& item_name, 
                             const std::unordered_map<std::string, std::string>& values) = 0;
                             
    /**
     * @brief Called when subscription is successful
     */
    virtual void onSubscription() {}
    
    /**
     * @brief Called when unsubscription is successful
     */
    virtual void onUnsubscription() {}
    
    /**
     * @brief Called when subscription fails
     */
    virtual void onSubscriptionError(int code, const std::string& message) {}
};

/**
 * @brief Represents a subscription to Lightstreamer data
 */
class Subscription {
public:
    /**
     * @brief Constructor
     * @param mode Subscription mode (MERGE, DISTINCT, etc.)
     * @param items List of items to subscribe to
     * @param fields List of fields to retrieve
     * @param adapter Data adapter name
     */
    Subscription(const std::string& mode, 
                const std::vector<std::string>& items, 
                const std::vector<std::string>& fields,
                const std::string& adapter = "");

    /**
     * @brief Add a listener for updates
     * @param listener Listener to add
     */
    void addListener(std::shared_ptr<SubscriptionListener> listener);

    /**
     * @brief Remove a listener
     * @param listener Listener to remove
     */
    void removeListener(std::shared_ptr<SubscriptionListener> listener);

    /**
     * @brief Process an update from the server
     * @param item_position Item position in the subscription
     * @param values Raw values from the server
     */
    void notifyUpdate(int item_position, const std::vector<std::string>& values);

    // Getters
    const std::string& getMode() const { return mode_; }
    const std::vector<std::string>& getItems() const { return items_; }
    const std::vector<std::string>& getFields() const { return fields_; }
    const std::string& getAdapter() const { return adapter_; }

    // Nouvelles méthodes d'accès
    std::vector<std::shared_ptr<SubscriptionListener>> getListeners() {
        std::lock_guard<std::mutex> lock(listeners_mutex_);
        return listeners_;
    }
    
    std::mutex& getListenersMutex() {
        return listeners_mutex_;
    }

private:
    std::string mode_;
    std::vector<std::string> items_;
    std::vector<std::string> fields_;
    std::string adapter_;
    std::vector<std::shared_ptr<SubscriptionListener>> listeners_;
    std::unordered_map<int, std::unordered_map<std::string, std::string>> items_cache_;
    std::mutex listeners_mutex_;

    /**
     * @brief Decode a field value according to Lightstreamer protocol
     * @param value Encoded value
     * @param last Previous value
     * @return Decoded value
     */
    std::string decodeValue(const std::string& value, const std::string& last);
};

/**
 * @brief Connection listener interface
 */
class ConnectionListener {
public:
    virtual ~ConnectionListener() = default;
    
    /**
     * @brief Called when connection status changes
     * @param status New status
     */
    virtual void onStatusChange(const std::string& status) {}
    
    /**
     * @brief Called when connection starts
     */
    virtual void onListenStart() {}
    
    /**
     * @brief Called when connection ends
     */
    virtual void onListenEnd() {}
    
    /**
     * @brief Called on connection failure
     * @param code Error code
     * @param message Error message
     */
    virtual void onServerError(int code, const std::string& message) {}
};

/**
 * @brief Client for Lightstreamer connection
 */
class LightstreamerClient {
public:
    /**
     * @brief Constructor
     * @param server_url Lightstreamer server URL
     * @param adapter_set Adapter set name
     */
    LightstreamerClient(const std::string& server_url, const std::string& adapter_set = "");
    
    /**
     * @brief Destructor
     */
    ~LightstreamerClient();
    
    /**
     * @brief Set credentials for connection
     * @param username Username
     * @param password Password
     */
    void setCredentials(const std::string& username, const std::string& password);
    
    /**
     * @brief Add a connection listener
     * @param listener Listener to add
     */
    void addListener(std::shared_ptr<ConnectionListener> listener);
    
    /**
     * @brief Remove a connection listener
     * @param listener Listener to remove
     */
    void removeListener(std::shared_ptr<ConnectionListener> listener);
    
    /**
     * @brief Connect to the Lightstreamer server
     */
    void connect();
    
    /**
     * @brief Disconnect from the Lightstreamer server
     */
    void disconnect();
    
    /**
     * @brief Subscribe to a data stream
     * @param subscription Subscription to add
     */
    void subscribe(std::shared_ptr<Subscription> subscription);
    
    /**
     * @brief Unsubscribe from a data stream
     * @param subscription Subscription to remove
     */
    void unsubscribe(std::shared_ptr<Subscription> subscription);
    
    /**
     * @brief Get all active subscriptions
     * @return Vector of active subscriptions
     */
    std::vector<std::shared_ptr<Subscription>> getSubscriptions() const;
    
    /**
     * @brief Check if client is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

private:
    std::string server_url_;
    std::string adapter_set_;
    std::string username_;
    std::string password_;
    std::string session_id_;
    std::string control_address_;
    
    std::atomic<bool> connected_;
    std::atomic<bool> stop_requested_;
    
    std::vector<std::shared_ptr<ConnectionListener>> listeners_;
    std::unordered_map<int, std::shared_ptr<Subscription>> subscriptions_;
    int subscription_counter_;
    
    std::unique_ptr<std::thread> stream_thread_;
    mutable std::mutex listeners_mutex_;
    mutable std::mutex subscriptions_mutex_;
    
    /**
     * @brief Send a control request to the server
     * @param params Request parameters
     * @return Server response
     */
    std::string sendControlRequest(const std::unordered_map<std::string, std::string>& params);
    
    /**
     * @brief Create a new session
     */
    void createSession();
    
    /**
     * @brief Bind to an existing session
     */
    void bindSession();
    
    /**
     * @brief Stream thread function
     */
    void streamThread();
    
    /**
     * @brief Process a server message
     * @param message Message to process
     */
    void processMessage(const std::string& message);
    
    /**
     * @brief Notify listeners of status change
     * @param status New status
     */
    void notifyStatusChange(const std::string& status);
    
    /**
     * @brief Perform HTTP request
     * @param url URL to request
     * @param params Request parameters
     * @return Response data
     */
    std::string httpRequest(const std::string& url, const std::unordered_map<std::string, std::string>& params);
};

} // namespace ig