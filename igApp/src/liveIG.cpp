#include <spdlog/spdlog.h>
#include "Strategies/buy_heikin_green.hpp"
#include "Strategies/sell_heikin_red.hpp"
#include "common.h"
#include "strategy.h"
#include "igBroker.h"
#include "trading_ig_config.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
// #include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

int main() {
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::info("Starting IG Trading Application...");

    // Load configuration from .env file
    Config config;
    spdlog::info("Configuration loaded successfully for {} {}", config.username.empty() ? "[NON DÉFINI]" : config.username); 

    ig::IGService ig_service(config.username, 
                        config.password, 
                        config.api_key, 
                        config.acc_type, 
                        config.acc_number);

    ig::IGBroker broker(ig_service);

    StrategyBaseConfig base_config;

    // Time settings
    base_config.trading_from = {7, 0, 0};   // 7:00 AM
    base_config.trading_to = {23, 0, 0};    // 11:00 PM
    base_config.trading_days = {0, 1, 2, 3, 4};  // 0=Monday, 6=Sunday

    // Fixed SL/TP values
    base_config.take_profit_distance = 30.0;
    base_config.stop_loss_distance = 20.0;

    // ATR parameters for SL and TP
    base_config.use_atr_for_sl = false;     // Important: default value false
    base_config.use_atr_for_tp = false;     // Important: default value false
    base_config.atr_period = 14;
    base_config.stop_loss_atr_multiplier = 2.0; // sl_atr_multiple
    base_config.take_profit_atr_multiplier = 3.0; // tp_atr_multiple
    base_config.min_stop_loss_distance = 5.0;
    base_config.min_take_profit_distance = 5.0;

    // New Min/Max parameters for SL
    base_config.use_minmax_for_sl = false;
    base_config.sl_minmax_periods = 5;
    base_config.sl_minmax_delta = 5.0;

    // Risk management
    base_config.use_risk_based_sizing = false;
    base_config.risk_percentage = 1.0; // risk_per_trade_pct
    base_config.cash = 100000.0;
    base_config.max_position_percentage = 100.0;
    base_config.leverage_limit = 20.0;

    // Break-even parameters
    base_config.use_break_even = false;
    base_config.break_even_threshold = 0.7; 

    // Daily maximum loss
    base_config.use_daily_max_loss = false;
    base_config.daily_max_loss_percentage = 2.0;
    base_config.daily_max_loss_amount = 0.0; // Calculated from cash and daily_max_loss_percentage

    BuyHeikinGreenConfig buy_heikin_green_config;
    buy_heikin_green_config.ema_short_period = 20;
    buy_heikin_green_config.ema_long_period = 50;
    buy_heikin_green_config.stoch_fastk = 10;
    buy_heikin_green_config.stoch_slowk = 7;
    buy_heikin_green_config.stoch_slowd = 3;
    buy_heikin_green_config.stoch_threshold = 20;
    buy_heikin_green_config.rsi_period = 14;
    buy_heikin_green_config.rsi_threshold = 30;

    buy_heikin_green_config.use_ema_short_filter = true;
    buy_heikin_green_config.use_ema_long_filter = true;
    buy_heikin_green_config.use_stoch_filter = true;
    buy_heikin_green_config.use_rsi_filter = true;
    buy_heikin_green_config.use_previous_ha_candle_red_filter = true;

    BuyHeikinGreen Strategy(base_config, buy_heikin_green_config);
    Strategy.set_log_level(LogLevel::DEBUG);
    auto strategy_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
        "strategy_logger",       // Logger name
        "logs/strategy_log.log", // Log file path
        50 * 1024 * 1024,       // Max file size (50 MB)
        1                        // Max number of files to keep
    );
    strategy_logger->set_level(spdlog::level::debug);

    auto log_callback = [strategy_logger](const std::string& message, int level) {
        spdlog::level::level_enum spdlog_level = spdlog::level::info;
        switch (level) {
            case static_cast<int>(LogLevel::DEBUG):   spdlog_level = spdlog::level::debug; break;
            case static_cast<int>(LogLevel::INFO):    spdlog_level = spdlog::level::info; break;
            case static_cast<int>(LogLevel::WARNING): spdlog_level = spdlog::level::warn; break;
            case static_cast<int>(LogLevel::FATAL):   spdlog_level = spdlog::level::err; break;
        }

        strategy_logger->log(spdlog_level, "{}", message);
    };

    Strategy.set_log_callback(log_callback);


    spdlog::info("IG Broker initialized successfully");

    return 0;

}