#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <QDateTime>

// Inclure les définitions de structures (sans les fonctions de sérialisation)
#include "Strategies/buy_heikin_green.hpp"
#include "Strategies/sell_heikin_red.hpp"
#include "Strategies/generic_strategy.hpp"
#include "ui/panels/generalParamsPanel.h"
#include "common.h"

#include "beTypes.h"
#include "ui/chart/chartTypes.h"

/**
 * @brief Configuration structures for generic strategy
 */

// enum class ComparisonType : int {
//     THRESHOLD_ABOVE = 0,    // valeur1 > seuil
//     THRESHOLD_BELOW = 1,    // valeur1 < seuil
//     CROSSOVER_ABOVE = 2,    // valeur1 croise au-dessus valeur2
//     CROSSOVER_BELOW = 3     // valeur1 croise en-dessous valeur2
// };

// enum class ValueType : int {
//     PRICE = 0,              // Prix actuel
//     INDICATOR = 1,          // Valeur d'un indicateur
//     CONSTANT = 2            // Valeur constante
// };

// struct ValueSource {
//     ValueType type = ValueType::PRICE;
//     std::string identifier = "";  // nom de l'indicateur ou "price" ou valeur constante
//     double constantValue = 0.0;
//     int historicalOffset = 0;  // pour accéder aux valeurs précédentes (0 = actuelle, 1 = précédente, etc.)
    
//     ValueSource() = default;
//     ValueSource(ValueType t, const std::string& id, int offset = 0) 
//         : type(t), identifier(id), historicalOffset(offset) {}
//     ValueSource(double value) 
//         : type(ValueType::CONSTANT), constantValue(value) {}
// };

// struct FilterConfig {
//     std::string name = "";
//     ValueSource value1;
//     ValueSource value2;
//     ComparisonType comparison = ComparisonType::THRESHOLD_ABOVE;
//     int lookbackPeriods = 1;  // sur combien de périodes chercher la condition
//     bool enabled = true;
// };

// struct IndicatorConfig {
//     std::string name = "";
//     std::string type = "";  // "EMA", "RSI", "STOCH", "ATR", "SUPERTREND"
//     std::map<std::string, double> parameters;  // paramètres spécifiques à l'indicateur
//     bool enabled = true;
// };



struct ProfileConfig {
    std::string name;
    std::string version;
    std::string createdAt;
    GeneralParamsConfig generalParams;
    StrategyBaseConfig baseConfig;
    BuyHeikinGreenConfig buyConfig;
    SellHeikinRedConfig sellConfig;
    GenericStrategyConfig genericConfig;

    template<class Archive>
    void serialize(Archive & ar) {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(generalParams),
           CEREAL_NVP(baseConfig),
           CEREAL_NVP(buyConfig),
           CEREAL_NVP(sellConfig));
        //    CEREAL_NVP(genericConfig));
    }
};

struct BacktestResultConfig {
    std::string name;
    std::string version;
    std::string createdAt;
    be::Stats stats;
    // c'est vraiment lourd, il faudrait plutot une reference vers des données, ensuite en verifie que les données chargées étaient bien celles de l'enregistrement
    std::vector<be::Candle> candles;
    // std::vector<std::unique_ptr<indicators::IndicatorBase>> indicatorInstances; // Instances of indicators calculated during backtest

    // peut etre ajouter la configuration du profile utiliser pour le backtest en question ?

    template<class Archive>
    void serialize(Archive & ar) {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(stats),
           CEREAL_NVP(candles));
        //    CEREAL_NVP(indicatorInstances));
    }
};

namespace cereal {
    template<class Archive>
    void serialize(Archive & ar, indicators::IndicatorBase & base) {
        ar(cereal::make_nvp("id", base.id),
           cereal::make_nvp("visible", base.visible));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::RSIInstance & config) {
        ar(cereal::base_class<indicators::IndicatorBase>(config),
           cereal::make_nvp("period", config.period),
           cereal::make_nvp("height", config.height),
           cereal::make_nvp("color", config.color),
           cereal::make_nvp("overboughtLevel", config.overboughtLevel),
           cereal::make_nvp("oversoldLevel", config.oversoldLevel),
           cereal::make_nvp("upperColor", config.upperColor),
           cereal::make_nvp("lowerColor", config.lowerColor));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::EMAInstance & config) {
        ar(cereal::base_class<indicators::IndicatorBase>(config),
           cereal::make_nvp("period", config.period),
           cereal::make_nvp("color", config.color));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::StochasticInstance & config) {
        ar(cereal::base_class<indicators::IndicatorBase>(config),
           cereal::make_nvp("fastKPeriod", config.fastKPeriod),
           cereal::make_nvp("slowKPeriod", config.slowKPeriod),
           cereal::make_nvp("slowDPeriod", config.slowDPeriod),
           cereal::make_nvp("height", config.height),
           cereal::make_nvp("kColor", config.kColor),
           cereal::make_nvp("dColor", config.dColor),
           cereal::make_nvp("overboughtLevel", config.overboughtLevel),
           cereal::make_nvp("oversoldLevel", config.oversoldLevel));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::ATRInstance & config) {
        ar(cereal::base_class<indicators::IndicatorBase>(config),
           cereal::make_nvp("period", config.period),
           cereal::make_nvp("height", config.height),
           cereal::make_nvp("color", config.color),
           cereal::make_nvp("useLogScale", config.useLogScale));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::SuperTrendInstance & config) {
        ar(cereal::base_class<indicators::IndicatorBase>(config),
           cereal::make_nvp("period", config.period),
           cereal::make_nvp("multiplier", config.multiplier),
           cereal::make_nvp("upColor", config.upColor),
           cereal::make_nvp("downColor", config.downColor));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::PivotPointsInstance & config) {
        ar(cereal::base_class<indicators::IndicatorBase>(config),
           cereal::make_nvp("periodType", config.periodType),
           cereal::make_nvp("calculationMethod", config.calculationMethod),
           cereal::make_nvp("levelStyles", config.levelStyles),
           cereal::make_nvp("showLabels", config.showLabels));
    }

    template<class Archive>
    void serialize(Archive & ar, be::Duration & duration) {
        ar(cereal::make_nvp("seconds", duration.seconds));
    }

    template<class Archive>
    void serialize(Archive & ar, be::Date & date) {
        ar(cereal::make_nvp("year", date.year),
           cereal::make_nvp("month", date.month),
           cereal::make_nvp("day", date.day),
           cereal::make_nvp("hour", date.hour),
           cereal::make_nvp("minute", date.minute),
           cereal::make_nvp("second", date.second));
    }

    template<class Archive>
    void serialize(Archive & ar, be::Candle & candle) {
        ar(cereal::make_nvp("date", candle.date),
           cereal::make_nvp("open", candle.open),
           cereal::make_nvp("high", candle.high),
           cereal::make_nvp("low", candle.low),
           cereal::make_nvp("close", candle.close),
           cereal::make_nvp("volume", candle.volume));
    }

    template<class Archive>
    void serialize(Archive & ar, be::TradeData & trade) {
        ar(cereal::make_nvp("id", trade.id),
           cereal::make_nvp("size", trade.size),
           cereal::make_nvp("entryPrice", trade.entryPrice),
           cereal::make_nvp("exitPrice", trade.exitPrice),
           cereal::make_nvp("entryBar", trade.entryBar),
           cereal::make_nvp("exitBar", trade.exitBar),
           cereal::make_nvp("entryDate", trade.entryDate),
           cereal::make_nvp("exitDate", trade.exitDate),
           cereal::make_nvp("closeReason", trade.closeReason),
           cereal::make_nvp("tag", trade.tag),
           cereal::make_nvp("commissions", trade.commissions),
           cereal::make_nvp("isBreakEven", trade.isBreakEven),
           cereal::make_nvp("tpPrice", trade.tpPrice),
           cereal::make_nvp("initialSLPrice", trade.initialSlPrice),
           cereal::make_nvp("lastSLPrice", trade.lastSlPrice),
           cereal::make_nvp("breakEvenTriggerPrice", trade.breakEvenTriggerPrice),
           cereal::make_nvp("pl", trade.pl),
           cereal::make_nvp("plPercent", trade.plPercent));
    }
    
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

    template<class Archive>
    void serialize(Archive & ar, be::Stats & stats) {
        ar(cereal::make_nvp("equityCurve", stats.equityCurve),
           cereal::make_nvp("trades", stats.trades),
           cereal::make_nvp("start", stats.start),
           cereal::make_nvp("end", stats.end),
           cereal::make_nvp("duration", stats.duration),
           cereal::make_nvp("exposureTimePct", stats.exposureTimePct),
           cereal::make_nvp("equityFinal", stats.equityFinal),
           cereal::make_nvp("equityPeak", stats.equityPeak),
           cereal::make_nvp("equityInitial", stats.equityInitial),
           cereal::make_nvp("returnPct", stats.returnPct),
           cereal::make_nvp("buyHoldReturnPct", stats.buyHoldReturnPct),
           cereal::make_nvp("buyHoldCagrPct", stats.buyHoldCagrPct),
           cereal::make_nvp("returnAnnPct", stats.returnAnnPct),
           cereal::make_nvp("volatilityAnnPct", stats.volatilityAnnPct),
           cereal::make_nvp("cagrPct", stats.cagrPct),
           cereal::make_nvp("sharpeRatio", stats.sharpeRatio),
           cereal::make_nvp("sortinoRatio", stats.sortinoRatio),
           cereal::make_nvp("calmarRatio", stats.calmarRatio),
           cereal::make_nvp("alphaPct", stats.alphaPct),
           cereal::make_nvp("beta", stats.beta),
           cereal::make_nvp("maxDrawdownPct", stats.maxDrawdownPct),
           cereal::make_nvp("avgDrawdownPct", stats.avgDrawdownPct),
           cereal::make_nvp("maxDrawdownDuration", stats.maxDrawdownDuration),
           cereal::make_nvp("avgDrawdownDuration", stats.avgDrawdownDuration),
           cereal::make_nvp("numTrades", stats.numTrades),
           cereal::make_nvp("numTPTrades", stats.numTPTrades),
           cereal::make_nvp("pctTPTrades", stats.pctTPTrades),
           cereal::make_nvp("numSLTrades", stats.numSLTrades),
           cereal::make_nvp("pctSLTrades", stats.pctSLTrades),
           cereal::make_nvp("numBETrades", stats.numBETrades),
           cereal::make_nvp("pctBETrades", stats.pctBETrades),
           cereal::make_nvp("numManualTrades", stats.numManualTrades),
           cereal::make_nvp("pctManualTrades", stats.pctManualTrades),
           cereal::make_nvp("numUnknownTrades", stats.numUnknownTrades),
           cereal::make_nvp("pctUnknownTrades", stats.pctUnknownTrades),
           cereal::make_nvp("bestTradePct", stats.bestTradePct),
           cereal::make_nvp("worstTradePct", stats.worstTradePct),
           cereal::make_nvp("avgTradePct", stats.avgTradePct),
           cereal::make_nvp("maxTradeDuration", stats.maxTradeDuration),
           cereal::make_nvp("avgTradeDuration", stats.avgTradeDuration),
           cereal::make_nvp("profitFactor", stats.profitFactor),
           cereal::make_nvp("expectancyPct", stats.expectancyPct),
           cereal::make_nvp("sqn", stats.sqn),
           cereal::make_nvp("kellyCriterion", stats.kellyCriterion));
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
           cereal::make_nvp("sl_minmax_delta_coef_atr", config.sl_minmax_delta_coef_atr),
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

    // template<class Archive>
    // void serialize(Archive & ar, GenericStrategyConfig & config) {
    //     ar(cereal::make_nvp("name", config.name),
    //        cereal::make_nvp("direction", config.direction),
    //        cereal::make_nvp("indicators", config.indicators),
    //        cereal::make_nvp("filters", config.filters));
    // }

    // Sérialisation pour ValueSource
    // template<class Archive>
    // void serialize(Archive & ar, ValueSource & config) {
    //     int typeInt = static_cast<int>(config.type);
    //     ar(cereal::make_nvp("type", typeInt),
    //        cereal::make_nvp("identifier", config.identifier),
    //        cereal::make_nvp("constantValue", config.constantValue),
    //        cereal::make_nvp("historicalOffset", config.historicalOffset));
    //     config.type = static_cast<ValueType>(typeInt);
    // }

    // Sérialisation pour FilterConfig
    // template<class Archive>
    // void serialize(Archive & ar, FilterConfig & config) {
    //     int comparisonInt = static_cast<int>(config.comparison);
    //     ar(cereal::make_nvp("name", config.name),
    //        cereal::make_nvp("value1", config.value1),
    //        cereal::make_nvp("value2", config.value2),
    //        cereal::make_nvp("comparison", comparisonInt),
    //        cereal::make_nvp("lookbackPeriods", config.lookbackPeriods),
    //        cereal::make_nvp("enabled", config.enabled));
    //     config.comparison = static_cast<ComparisonType>(comparisonInt);
    // }

    // Sérialisation pour IndicatorConfig
    // template<class Archive>
    // void serialize(Archive & ar, IndicatorConfig & config) {
    //     ar(cereal::make_nvp("name", config.name),
    //        cereal::make_nvp("type", config.type),
    //        cereal::make_nvp("parameters", config.parameters),
    //        cereal::make_nvp("enabled", config.enabled));
    // }
    
    // Sérialisation pour GenericStrategyConfig
    // template<class Archive>
    // void serialize(Archive & ar, GenericStrategyConfig & config) {
    //     ar(cereal::make_nvp("name", config.name),
    //        cereal::make_nvp("direction", config.direction),
    //        cereal::make_nvp("indicators", config.indicators),
    //        cereal::make_nvp("filters", config.filters));
    // }
}