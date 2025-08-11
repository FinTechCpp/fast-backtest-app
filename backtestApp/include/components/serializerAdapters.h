#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <QDateTime>

// Inclure les définitions de structures (sans les fonctions de sérialisation)
#include "Strategies/buy_heikin_green.hpp"
#include "Strategies/sell_heikin_red.hpp"
#include "ui/panels/generalParamsPanel.h"
#include "common.h"

namespace cereal {
    // template<class Archive>
    // void serialize(Archive & ar, be::Date & date) {
    //     ar(make_nvp("year", date.year),
    //        make_nvp("month", date.month),
    //        make_nvp("day", date.day),
    //        make_nvp("hour", date.hour),
    //        make_nvp("minute", date.minute),
    //        make_nvp("second", date.second));
    // }
    
    template<class Archive>
    void serialize(Archive & ar, Time & time) {
        ar(cereal::make_nvp("hour", time.hour),
           cereal::make_nvp("minute", time.minute),
           cereal::make_nvp("second", time.second));
    }

    template<class Archive>
    void serialize(Archive & ar, QDateTime & dt) {
        ar(cereal::make_nvp("year", dt.date().year()),
           cereal::make_nvp("month", dt.date().month()),
           cereal::make_nvp("day", dt.date().day()),
           cereal::make_nvp("hour", dt.time().hour()),
           cereal::make_nvp("minute", dt.time().minute()),
           cereal::make_nvp("second", dt.time().second()));
    }

    // Pour QString (adaptateur générique)
    template<class Archive>
    void save(Archive & ar, const QString & str) {
        std::string stdStr = str.toStdString();
        ar(stdStr);
    }

    template<class Archive>
    void load(Archive & ar, QString & str) {
        std::string stdStr;
        ar(stdStr);
        str = QString::fromStdString(stdStr);
    }
    
    // Sérialisation pour BuyHeikinGreenConfig
    template<class Archive>
    void serialize(Archive & ar, BuyHeikinGreenConfig & config) {
        ar(cereal::make_nvp("ema_short_period", config.ema_short_period),
           cereal::make_nvp("ema_long_period", config.ema_long_period),
           cereal::make_nvp("stoch_fastk", config.stoch_fastk),
           cereal::make_nvp("stoch_slowk", config.stoch_slowk),
           cereal::make_nvp("stoch_slowd", config.stoch_slowd),
           cereal::make_nvp("stoch_threshold", config.stoch_threshold),
           cereal::make_nvp("rsi_period", config.rsi_period),
           cereal::make_nvp("rsi_threshold", config.rsi_threshold),
           cereal::make_nvp("supertrend_atr_period", config.supertrend_atr_period),
           cereal::make_nvp("supertrend_multiplier", config.supertrend_multiplier),
           cereal::make_nvp("previous_ha_candle_red_filter_n", config.previous_ha_candle_red_filter_n),
           cereal::make_nvp("rsi_history_periods", config.rsi_history_periods),
           cereal::make_nvp("stoch_history_periods", config.stoch_history_periods),
           cereal::make_nvp("use_ema_short_filter", config.use_ema_short_filter),
           cereal::make_nvp("use_ema_long_filter", config.use_ema_long_filter),
           cereal::make_nvp("use_stoch_filter", config.use_stoch_filter),
           cereal::make_nvp("use_rsi_filter", config.use_rsi_filter),
           cereal::make_nvp("use_supertrend_filter", config.use_supertrend_filter),
           cereal::make_nvp("use_previous_ha_candle_red_filter", config.use_previous_ha_candle_red_filter));
    }
    
    // Sérialisation pour SellHeikinRedConfig
    template<class Archive>
    void serialize(Archive & ar, SellHeikinRedConfig & config) {
        ar(cereal::make_nvp("ema_short_period", config.ema_short_period),
           cereal::make_nvp("ema_long_period", config.ema_long_period),
           cereal::make_nvp("stoch_fastk", config.stoch_fastk),
           cereal::make_nvp("stoch_slowk", config.stoch_slowk),
           cereal::make_nvp("stoch_slowd", config.stoch_slowd),
           cereal::make_nvp("stoch_threshold", config.stoch_threshold),
           cereal::make_nvp("rsi_period", config.rsi_period),
           cereal::make_nvp("rsi_threshold", config.rsi_threshold),
           cereal::make_nvp("supertrend_atr_period", config.supertrend_atr_period),
           cereal::make_nvp("supertrend_multiplier", config.supertrend_multiplier),
           cereal::make_nvp("previous_ha_candle_green_filter_n", config.previous_ha_candle_green_filter_n),
           cereal::make_nvp("rsi_history_periods", config.rsi_history_periods),
           cereal::make_nvp("stoch_history_periods", config.stoch_history_periods),
           cereal::make_nvp("use_ema_short_filter", config.use_ema_short_filter),
           cereal::make_nvp("use_ema_long_filter", config.use_ema_long_filter),
           cereal::make_nvp("use_stoch_filter", config.use_stoch_filter),
           cereal::make_nvp("use_rsi_filter", config.use_rsi_filter),
           cereal::make_nvp("use_supertrend_filter", config.use_supertrend_filter),
           cereal::make_nvp("use_previous_ha_candle_green_filter", config.use_previous_ha_candle_green_filter));
    }
    
    // Sérialisation pour StrategyBaseConfig
    template<class Archive>
    void serialize(Archive & ar, StrategyBaseConfig & config) {
        ar(cereal::make_nvp("enable_logging", config.enable_logging),
           cereal::make_nvp("logLevel", config.logLevel),
           cereal::make_nvp("sl_method", config.sl_method),
           cereal::make_nvp("tp_method", config.tp_method),
           cereal::make_nvp("trading_from", config.trading_from),
           cereal::make_nvp("trading_to", config.trading_to),
           cereal::make_nvp("trading_days_array", config.trading_days_array),
           cereal::make_nvp("stop_loss_distance", config.stop_loss_distance),
           cereal::make_nvp("take_profit_distance", config.take_profit_distance),
           cereal::make_nvp("atr_period", config.atr_period),
           cereal::make_nvp("stop_loss_atr_multiplier", config.stop_loss_atr_multiplier),
           cereal::make_nvp("take_profit_atr_multiplier", config.take_profit_atr_multiplier),
           cereal::make_nvp("min_stop_loss_distance", config.min_stop_loss_distance),
           cereal::make_nvp("min_take_profit_distance", config.min_take_profit_distance),
           cereal::make_nvp("sl_minmax_periods", config.sl_minmax_periods),
           cereal::make_nvp("sl_minmax_delta", config.sl_minmax_delta),
           cereal::make_nvp("tp_sl_ratio", config.tp_sl_ratio),
           cereal::make_nvp("tp_supertrend_atr_period", config.tp_supertrend_atr_period),
           cereal::make_nvp("tp_supertrend_multiplier", config.tp_supertrend_multiplier),
           cereal::make_nvp("rl_model_path", config.rl_model_path),
           cereal::make_nvp("rl_lookback_periods", config.rl_lookback_periods),
           cereal::make_nvp("rl_tp_max_multiplier", config.rl_tp_max_multiplier),
           cereal::make_nvp("rl_tp_min_multiplier", config.rl_tp_min_multiplier),
           cereal::make_nvp("nth_heikin_ashi_count", config.nth_heikin_ashi_count),
           cereal::make_nvp("use_risk_based_sizing", config.use_risk_based_sizing),
           cereal::make_nvp("risk_percentage", config.risk_percentage),
           cereal::make_nvp("leverage_limit", config.leverage_limit),
           cereal::make_nvp("use_break_even", config.use_break_even),
           cereal::make_nvp("break_even_threshold", config.break_even_threshold),
           cereal::make_nvp("break_even_offset_per_mille", config.break_even_offset_per_mille),
           cereal::make_nvp("use_daily_max_loss", config.use_daily_max_loss),
           cereal::make_nvp("daily_max_loss_percentage", config.daily_max_loss_percentage),
           cereal::make_nvp("use_daily_max_profit", config.use_daily_max_profit),
           cereal::make_nvp("daily_max_profit_percentage", config.daily_max_profit_percentage),
           cereal::make_nvp("use_daily_max_drawdown", config.use_daily_max_drawdown),
           cereal::make_nvp("daily_max_drawdown_percentage", config.daily_max_drawdown_percentage));
    }
    
    // Sérialisation pour GeneralParamsConfig
    template<class Archive>
    void serialize(Archive & ar, GeneralParamsConfig & config) {
        ar(cereal::make_nvp("strategyName", config.strategyName),
           cereal::make_nvp("symbol", config.symbol),
           cereal::make_nvp("interval", config.interval),
           cereal::make_nvp("period", config.period),
           cereal::make_nvp("endDate", config.endDate),
           cereal::make_nvp("cash", config.cash),
           cereal::make_nvp("spread", config.spread),
           cereal::make_nvp("commission", config.commission),
           cereal::make_nvp("leverage_limit", config.leverage_limit),
           cereal::make_nvp("tradeOnClose", config.tradeOnClose),
           cereal::make_nvp("hedging", config.hedging),
           cereal::make_nvp("exclusiveOrders", config.exclusiveOrders),
           cereal::make_nvp("finalizeTrades", config.finalizeTrades));
    }
}