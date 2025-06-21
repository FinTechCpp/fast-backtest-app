#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <optional>
#include <atomic>
#include <ctime>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// Forward declarations
namespace cpr {
    class Session;
    class Response;
}

namespace nlohmann {
    
    // Add serialization support for std::optional
    template <typename T>
    void to_json(nlohmann::json& j, const std::optional<T>& opt) {
        if (opt.has_value()) {
            j = opt.value();
        } else {
            j = nullptr;
        }
    }
    
    template <typename T>
    void from_json(const nlohmann::json& j, std::optional<T>& opt) {
        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();
        }
    }

}

namespace ig {

struct PositionCreateParams {
    std::string currencyCode;
    std::string direction;
    std::string epic;
    std::string expiry;
    bool forceOpen;
    bool guaranteedStop;
    double level;
    double limitDistance;
    double limitLevel;
    std::string orderType;
    std::string quoteId;
    double size;
    double stopDistance;
    double stopLevel;
    bool trailingStop;
    double trailingStopIncrement;
    std::string timeInForce;
    
    // Conversion en JSON
    nlohmann::json toJson() const {
        nlohmann::json params = {
            {"currencyCode", currencyCode},
            {"direction", direction},
            {"epic", epic},
            {"expiry", expiry},
            {"forceOpen", forceOpen},
            {"guaranteedStop", guaranteedStop},
            {"orderType", orderType},
            {"size", size},
            {"trailingStop", trailingStop}
        };
        
        // Ajouter uniquement les paramètres non vides/par défaut
        if (orderType != "MARKET" && level != 0.0) {
            params["level"] = level;
        }
        
        if (orderType == "QUOTE" && !quoteId.empty()) {
            params["quoteId"] = quoteId;
        }
        
        if (limitDistance > 0.0) {
            params["limitDistance"] = limitDistance;
        }
        
        if (limitLevel > 0.0) {
            params["limitLevel"] = limitLevel;
        }
        
        if (stopDistance > 0.0) {
            params["stopDistance"] = stopDistance;
        }
        
        if (stopLevel > 0.0) {
            params["stopLevel"] = stopLevel;
        }
        
        if (trailingStop && trailingStopIncrement > 0.0) {
            params["trailingStopIncrement"] = trailingStopIncrement;
        }
        
        if (!timeInForce.empty()) {
            params["timeInForce"] = timeInForce;
        }
        
        return params;
    }
};

struct PositionCloseParams {
    std::string dealId;           // ID de la position à fermer
    std::string direction;        // Direction (BUY ou SELL)
    std::string epic;             // Code du marché
    std::string expiry;           // Date d'expiration
    double level = 0.0;           // Niveau de prix
    std::string orderType;        // Type d'ordre (MARKET, LIMIT, etc.)
    std::string quoteId;          // ID de cotation (pour les ordres QUOTE)
    double size = 0.0;            // Taille de la position
    std::string timeInForce;      // Durée de validité
    
    // Conversion en JSON
    nlohmann::json toJson() const {
        nlohmann::json params = {
            {"dealId", dealId},
            {"direction", direction},
            {"epic", epic},
            {"expiry", expiry},
            {"level", level},
            {"orderType", orderType},
            {"size", size}
        };
        
        // Ajouter les paramètres optionnels seulement s'ils sont renseignés
        if (!quoteId.empty()) {
            params["quoteId"] = quoteId;
        }
        
        if (!timeInForce.empty()) {
            params["timeInForce"] = timeInForce;
        }
        
        return params;
    }
    
    // Validation des paramètres
    void validate() const {
        if (dealId.empty()) {
            spdlog::error("Deal ID is required for closing a position");
        }
        
        if (direction.empty() || (direction != "BUY" && direction != "SELL")) {
            spdlog::error("Direction must be 'BUY' or 'SELL'");
        }
        
        if (epic.empty()) {
            spdlog::error("Epic is required for closing a position");
        }
        
        if (size <= 0.0) {
            spdlog::error("Size must be greater than zero");
        }
        
        if (orderType.empty()) {
            spdlog::error("Order type is required for closing a position");
        }
    }
};

struct PositionUpdateParams {
    std::string dealId;                      // ID de la position à modifier
    std::optional<double> limitLevel;        // Niveau de prix pour le take profit
    std::optional<double> stopLevel;         // Niveau de prix pour le stop loss
    bool guaranteedStop = false;             // Utiliser un stop garanti
    bool trailingStop = false;               // Utiliser un stop suiveur
    double trailingStopDistance = 0.0;       // Distance pour le stop suiveur
    double trailingStopIncrement = 0.0;      // Incrément pour le stop suiveur
    
    // Conversion en JSON
    nlohmann::json toJson() const {
        nlohmann::json params = nlohmann::json::object();
        
        // Ajouter uniquement les paramètres non nuls
        if (limitLevel.has_value()) {
            params["limitLevel"] = limitLevel.value();
        }
        
        if (stopLevel.has_value()) {
            params["stopLevel"] = stopLevel.value();
        }
        
        params["guaranteedStop"] = guaranteedStop;
        params["trailingStop"] = trailingStop;
        
        if (trailingStop) {
            if (trailingStopDistance > 0.0) {
                params["trailingStopDistance"] = trailingStopDistance;
            }
            
            if (trailingStopIncrement > 0.0) {
                params["trailingStopIncrement"] = trailingStopIncrement;
            }
        }
        
        return params;
    }
    
    // Validation des paramètres
    void validate() const {
        if (dealId.empty()) {
            spdlog::error("Deal ID is required for position update");
        }
        
        if (guaranteedStop && trailingStop) {
            spdlog::error("Cannot set both guaranteedStop and trailingStop to true");
        }
        
        if (trailingStop && (trailingStopDistance <= 0.0 || trailingStopIncrement <= 0.0)) {
            spdlog::error("When trailingStop is true, both trailingStopDistance and trailingStopIncrement must be provided with positive values");
        }
    }
};

struct Position {
    std::string deal_id;
    std::string direction;
    std::string epic;
    double size;
    double level;
    std::optional<double> stopLevel;
    std::optional<double> limitLevel;
    bool trailingStop;
    bool guaranteedStop;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, deal_id, direction, 
        epic, size, level, stopLevel, limitLevel, trailingStop, guaranteedStop);
};

/**
 * @brief Exception thrown when API request limit is reached
 */
class ApiExceededException : public std::runtime_error {
public:
    ApiExceededException() : std::runtime_error("API request limit exceeded") {}
};

/**
 * @brief Exception thrown when session token becomes invalid
 */
class TokenInvalidException : public std::runtime_error {
public:
    TokenInvalidException() : std::runtime_error("Session token is invalid or expired") {}
};

/**
 * @brief Exception for general IG API errors
 */
class IGException : public std::runtime_error {
public:
    explicit IGException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown when KYC verification is required
 */
class KycRequiredException : public std::runtime_error {
public:
    KycRequiredException() : std::runtime_error("KYC verification required") {}
};

/**
 * @brief Session class for CRUD operations with IG API
 */
class IGSessionCRUD {
public:
    /**
     * @brief Constructor for CRUD session
     * @param base_url Base URL for API
     * @param api_key API key for authentication
     * @param session HTTP session object
     */
    IGSessionCRUD(const std::string& base_url, const std::string& api_key,
                  std::shared_ptr<cpr::Session> session);

    /**
     * @brief Perform a create (POST) request
     * @param endpoint API endpoint
     * @param params Request parameters
     * @param version API version
     * @return HTTP response
     */
    cpr::Response create(const std::string& endpoint, const std::string& params, const std::string& version);

    /**
     * @brief Perform a read (GET) request
     * @param endpoint API endpoint
     * @param params Request parameters
     * @param version API version
     * @return HTTP response
     */
    cpr::Response read(const std::string& endpoint, const std::string& params, const std::string& version);

    /**
     * @brief Perform an update (PUT) request
     * @param endpoint API endpoint
     * @param params Request parameters
     * @param version API version
     * @return HTTP response
     */
    cpr::Response update(const std::string& endpoint, const std::string& params, const std::string& version);

    /**
     * @brief Perform a delete request
     * @param endpoint API endpoint
     * @param params Request parameters
     * @param version API version
     * @return HTTP response
     */
    cpr::Response delete_req(const std::string& endpoint, const std::string& params, const std::string& version);

    /**
     * @brief Generic request method
     * @param action Request action (create, read, update, delete)
     * @param endpoint API endpoint
     * @param params Request parameters
     * @param version API version
     * @return HTTP response
     */
    cpr::Response req(const std::string& action, const std::string& endpoint, 
                     const std::string& params, const std::string& version);

    /**
     *  @brief Get current session headers
     *  @return Current session headers
     */
    const std::unordered_map<std::string, std::string>& GetHeaders() const {
        return current_headers_;
    }
    
public: // Make public
    std::unordered_map<std::string, std::string> current_headers_; // Make public
    
private:
    std::string base_url_;
    std::string api_key_;
    std::shared_ptr<cpr::Session> session_;
    
    /**
     * @brief Construct full URL from endpoint
     * @param endpoint API endpoint
     * @return Full URL
     */
    std::string buildUrl(const std::string& endpoint) const;
};


/**
 * @brief Main IG API service class
 * 
 * This class provides access to IG Markets REST API functionality
 */
class IGService {
public:
    /**
     * @brief Constructor
     * @param username IG username
     * @param password IG password
     * @param api_key API key
     * @param acc_type Account type (live or demo)
     * @param acc_number Account number (required for v3 authentication)
     * @param use_rate_limiter Whether to use rate limiting
     */
    IGService(const std::string& username, 
              const std::string& password,
              const std::string& api_key,
              const std::string& acc_type = "demo",
              const std::string& acc_number = "",
              bool use_rate_limiter = false);
    
    /**
     * @brief Destructor
     */
    ~IGService();

    // -------- SESSION MANAGEMENT --------
    
    /**
     * @brief Create a new session
     * @return Session details
     */
    nlohmann::json create_session();
    
    /**
     * @brief Refresh the current session
     * @return HTTP status code
     */
    int refresh_session();
    
    /**
     * @brief Switch to a different account
     * @param account_id Account ID to switch to
     * @param default_account Whether to set as default account
     * @return Response data
     */
    nlohmann::json switch_account(const std::string& account_id, bool default_account);
    

    /**
     * @brief Get list of client applications
     * @return List of client applications
     */
    nlohmann::json get_client_apps();

    /**
     * @brief Get current session details
     * @param fetch_session_tokens Whether to fetch session tokens
     * @return Session details
     */
    nlohmann::json read_session(bool fetch_session_tokens = false);
    
    /**
     * @brief Log out of the current session
     */
    void logout();

    // -------- ACCOUNT METHODS --------
    
    /**
     * @brief Get list of accounts
     * @return List of accounts
     */
    nlohmann::json fetch_accounts();
    
    /**
     * @brief Get account preferences
     * @return Account preferences
     */
    nlohmann::json fetch_account_preferences();
    
    /**
     * @brief Update account preferences
     * @param trailing_stops_enabled Whether to enable trailing stops
     * @return Update status
     */
    std::string update_account_preferences(bool trailing_stops_enabled = false);
    
    /**
     * @brief Get account activity for a period
     * @param milliseconds Period in milliseconds
     * @return Account activity
     */
    nlohmann::json fetch_account_activity_by_period(int64_t milliseconds);
    
    /**
     * @brief Get account activity for a date range
     * @param from_date Start date
     * @param to_date End date
     * @return Account activity
     */
    nlohmann::json fetch_account_activity_by_date(
        const std::chrono::system_clock::time_point& from_date,
        const std::chrono::system_clock::time_point& to_date);
    
    /**
     * @brief Get account activity (v2)
     * @param from_date Start date
     * @param to_date End date
     * @param max_span_seconds Max timespan in seconds
     * @param page_size Page size
     * @return Account activity
     */
    nlohmann::json fetch_account_activity_v2(
        std::optional<std::chrono::system_clock::time_point> from_date = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> to_date = std::nullopt,
        std::optional<int> max_span_seconds = std::nullopt,
        int page_size = 20);
    
    /**
     * @brief Get account activity (v3)
     * @param from_date Start date
     * @param to_date End date
     * @param detailed Whether to include details
     * @param deal_id Deal ID filter
     * @param fiql_filter FIQL filter
     * @param page_size Page size
     * @return Account activity
     */
    nlohmann::json fetch_account_activity(
        std::optional<std::chrono::system_clock::time_point> from_date = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> to_date = std::nullopt,
        bool detailed = false,
        const std::string& deal_id = "",
        const std::string& fiql_filter = "",
        int page_size = 50);

    // -------- DEALING METHODS --------
    
    /**
     * @brief Get deal information by reference
     * @param deal_reference Deal reference
     * @return Deal information
     */
    nlohmann::json fetch_deal_by_deal_reference(const std::string& deal_reference);
    
    /**
     * @brief Get open position by deal ID
     * @param deal_id Deal ID
     * @return Position information
     */
    nlohmann::json fetch_open_position_by_deal_id(const std::string& deal_id);
    
    /**
     * @brief Get all open positions
     * @return Open positions
     */
    ig::Position fetch_open_positions();
    
    /**
     * @brief Close an open position
    * @param params Parameters for closing the position
     * @return Deal confirmation
     */
    nlohmann::json close_open_position(const PositionCloseParams& params);
    
    /**
     * @brief Create an open position
     * @param currency_code string - Currency code (e.g., "EUR")
     * @param direction string - Trade direction (e.g., "BUY")
     * @param epic string - Market epic (e.g., "IX.D.FTSE.DAILY.IP")
     * @param expiry string - Expiry date (e.g., "-" for no expiry)
     * @param force_open bool - Force open flag (e.g., true)
     * @param guaranteed_stop bool - Use guaranteed stop (e.g., false)
     * @param level double - Price level (e.g., 0.0 for MARKET orders)
     * @param limit_distance double - Limit distance (e.g., 0.0)
     * @param limit_level double - Limit level (e.g., 0.0)
     * @param order_type string - Order type (e.g., "MARKET")
     * @param quote_id string - Quote ID (e.g., "")
     * @param size double - Position size (e.g., 1.0)
     * @param stop_distance double - Stop distance (e.g., 0.0)
     * @param stop_level double - Stop level (e.g., 0.0)
     * @param trailing_stop bool - Use trailing stop (e.g., false)
     * @param trailing_stop_increment double - Trailing stop increment (e.g., 0.0)
     * @param time_in_force string - Time in force (e.g., "")
     * @return Deal confirmation - bool
     */
    nlohmann::json create_open_position(const PositionCreateParams& params);
    

    /**
     * @brief Update an open position
     * 
     * Updates the parameters of an existing open position such as 
     * stop levels, limit levels, trailing stops, etc.
     * 
     * @param params Structure containing all the update parameters
     * @return JSON response containing the update result
     */
    nlohmann::json update_open_position(const PositionUpdateParams& params);

    // -------- MARKET METHODS --------
    
    /**
     * @brief Get market details by epic
     * @param epic Market epic
     * @return Market details
     */
    nlohmann::json fetch_market_by_epic(const std::string& epic);
    
    /**
     * @brief Search markets by term
     * @param search_term Search term
     * @return Matching markets
     */
    nlohmann::json search_markets(const std::string& search_term);
    
    /**
     * @brief Get historical prices
     * @param epic Market epic
     * @param resolution Price resolution
     * @param start_date Start date
     * @param end_date End date
     * @param numpoints Number of data points
     * @param pagesize Page size
     * @param wait Wait time between requests
     * @return Historical prices
     */
    nlohmann::json fetch_historical_prices_by_epic(
        const std::string& epic,
        const std::string& resolution = "",
        const std::string& start_date = "",
        const std::string& end_date = "",
        int numpoints = 0,
        int pagesize = 20,
        int wait = 1);

    /**
     * @brief Get account number
     * @return Account number
     */
    const std::string& getAccountNumber() const {
        return acc_number_;
    }

private:
    // Base URLs for API environments
    static const std::unordered_map<std::string, std::string> D_BASE_URL;
    
    // Authentication properties
    std::string api_key_;
    std::string username_;
    std::string password_;
    std::string acc_number_;
    std::string base_url_;
    
    // Session state
    std::shared_ptr<cpr::Session> session_;
    std::unique_ptr<IGSessionCRUD> crud_session_;
    std::string refresh_token_;
    std::chrono::system_clock::time_point valid_until_;
    
    // Rate limiting
    bool use_rate_limiter_;
    std::atomic<bool> bucket_threads_run_;
    int trading_requests_per_minute_;
    int non_trading_requests_per_minute_;
    std::queue<bool> trading_requests_queue_;
    std::queue<bool> non_trading_requests_queue_;
    std::vector<std::chrono::system_clock::time_point> trading_times_;
    std::vector<std::chrono::system_clock::time_point> non_trading_times_;
    std::mutex trading_queue_mutex_;
    std::mutex non_trading_queue_mutex_;
    std::unique_ptr<std::thread> token_bucket_trading_thread_;
    std::unique_ptr<std::thread> token_bucket_non_trading_thread_;
    
    std::string authorization_header_;

    /**
     * @brief Setup rate limiter
     */
    void setup_rate_limiter();
    
    /**
     * @brief Token bucket thread for trading requests
     */
    void token_bucket_trading();
    
    /**
     * @brief Token bucket thread for non-trading requests
     */
    void token_bucket_non_trading();
    
    /**
     * @brief Apply rate limiting for trading requests
     */
    void trading_rate_limit_pause_or_pass();
    
    /**
     * @brief Apply rate limiting for non-trading requests
     */
    void non_trading_rate_limit_pause_or_pass();
    
    /**
     * @brief Stop bucket threads
     */
    void exit_bucket_threads();
    
    /**
     * @brief Check if session is valid
     */
    void check_session();
    
    /**
     * @brief Handle response headers for authentication
     * @param response HTTP response
     */
    void manage_headers(const cpr::Response& response);
    
    /**
     * @brief Handle OAuth tokens
     * @param oauth OAuth data
     */
    void handle_oauth(const nlohmann::json& oauth);
    
    /**
     * @brief Get encryption key for password
     * @return Encryption key and timestamp
     */
    std::pair<std::string, std::string> get_encryption_key();
    
    /**
     * @brief Encrypt password for login
     * @return Encrypted password
     */
    std::string encrypted_password();
    
    /**
     * @brief Parse JSON response
     * @param response_text Response text
     * @return Parsed JSON
     */
    static nlohmann::json parse_response(const std::string& response_text);
    
    /**
     * @brief Send a request to the API
     * @param action Request action
     * @param endpoint API endpoint
     * @param params Request parameters
     * @param version API version
     * @param check Whether to check session
     * @return HTTP response
     */
    cpr::Response request(const std::string& action, const std::string& endpoint,
                          const std::string& params, const std::string& version = "1",
                          bool check = true);
    
public:
    /**
     * @brief Get current HTTP headers
     * @return Reference to the current headers map
     */
    const std::unordered_map<std::string, std::string>& GetHeaders() const {
        return crud_session_->GetHeaders();
    }
};

// Convert user-friendly resolution format to IG API format
std::string conv_resol(const std::string& resolution);

} // namespace ig