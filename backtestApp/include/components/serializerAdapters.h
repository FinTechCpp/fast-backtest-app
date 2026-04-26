#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>

#include <exception>

#include "ui/panels/generalParamsPanel.h"
#include "common.h"

#include "beTypes.h"
#include "ui/chart/chartTypes.h"

struct ProfileConfig {
    std::string name;
    std::string version;
    std::string createdAt;
    GeneralParamsConfig generalParams;
    std::vector<StrategyConfig> strategyConfigs;
    std::vector<std::unique_ptr<indicators::IndicatorBase>> chartIndicators;

    template<class Archive>
    void save(Archive & ar) const {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(generalParams),
           CEREAL_NVP(strategyConfigs),
           CEREAL_NVP(chartIndicators));
    }

    template<class Archive>
    void load(Archive & ar) {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(generalParams),
           CEREAL_NVP(strategyConfigs));
        try {
            ar(CEREAL_NVP(chartIndicators));
        } catch (const cereal::Exception&) {
            chartIndicators.clear();
        }
    }
};

// TODO finish full saving of backtest results
struct BacktestResultConfig {
    std::string name;
    std::string version;
    std::string createdAt;

    // Backtest Results: replace with BacktestResults structure directly?
    GeneralParamsConfig generalParams;
    std::vector<StrategyConfig> strategyConfigs;
    be::Stats stats;
    // This is really heavy; it would be better to reference data instead, and then verify that the loaded data matches the saved record
    std::vector<be::Candle> candles;
    std::vector<chart::ChartMarker> userMarkers;

    template<class Archive>
    void save(Archive & ar) const {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(generalParams),
           CEREAL_NVP(strategyConfigs),
           CEREAL_NVP(stats),
           CEREAL_NVP(candles),
           CEREAL_NVP(userMarkers));
    }

    template<class Archive>
    void load(Archive & ar) {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(generalParams),
           CEREAL_NVP(strategyConfigs),
           CEREAL_NVP(stats),
           CEREAL_NVP(candles));

        try {
            ar(CEREAL_NVP(userMarkers));
        } catch (const cereal::Exception&) {
            userMarkers.clear();
        }
    }
};

// Structure to import external results (without precomputed stats)
struct ExternalResultConfig {
    std::string name;
    std::string version;
    std::string createdAt;

    std::vector<be::TradeData> trades;  // Trades directly instead of stats
    std::vector<be::Candle> candles; 

    template<class Archive>
    void serialize(Archive & ar) {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(trades),
           CEREAL_NVP(candles));
    }
};

namespace cereal {
    template<class Archive>
    void save(Archive & ar, const chart::ChartMarker & marker) {
        const int markerTypeValue = static_cast<int>(marker.type);
        ar(cereal::make_nvp("barIndex", marker.barIndex),
           cereal::make_nvp("price", marker.price),
            cereal::make_nvp("type", markerTypeValue),
            cereal::make_nvp("color", marker.color));
    }

    template<class Archive>
    void load(Archive & ar, chart::ChartMarker & marker) {
        int markerTypeValue = static_cast<int>(chart::MarkerType::Check);
        ar(cereal::make_nvp("barIndex", marker.barIndex),
           cereal::make_nvp("price", marker.price),
           cereal::make_nvp("type", markerTypeValue));

        try {
            ar(cereal::make_nvp("color", marker.color));
        } catch (const cereal::Exception&) {
            marker.color = -1;
        }

        if (markerTypeValue == static_cast<int>(chart::MarkerType::Error)) {
            marker.type = chart::MarkerType::Error;
        } else if (markerTypeValue == static_cast<int>(chart::MarkerType::VerticalLine)) {
            marker.type = chart::MarkerType::VerticalLine;
        } else {
            marker.type = chart::MarkerType::Check;
        }
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
           cereal::make_nvp("close", candle.close));
        //    cereal::make_nvp("volume", candle.volume));
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
    void save(Archive & ar, const QDateTime & dt) {
        int year = dt.date().year();
        int month = dt.date().month();
        int day = dt.date().day();
        int hour = dt.time().hour();
        int minute = dt.time().minute();
        int second = dt.time().second();
        
        ar(cereal::make_nvp("year", year),
           cereal::make_nvp("month", month),
           cereal::make_nvp("day", day),
           cereal::make_nvp("hour", hour),
           cereal::make_nvp("minute", minute),
           cereal::make_nvp("second", second));
    }

    template<class Archive>
    void load(Archive & ar, QDateTime & dt) {
        int year, month, day, hour, minute, second;
        
        ar(cereal::make_nvp("year", year),
           cereal::make_nvp("month", month),
           cereal::make_nvp("day", day),
           cereal::make_nvp("hour", hour),
           cereal::make_nvp("minute", minute),
           cereal::make_nvp("second", second));
        
        dt = QDateTime(QDate(year, month, day), QTime(hour, minute, second));
    }

    // For QString (generic adapter)
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
           cereal::make_nvp("tradesPerDay", stats.tradesPerDay),
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
           cereal::make_nvp("grossProfit", stats.grossProfit),
           cereal::make_nvp("grossLoss", stats.grossLoss),
           cereal::make_nvp("expectancyPct", stats.expectancyPct),
           cereal::make_nvp("sqn", stats.sqn),
           cereal::make_nvp("kellyCriterion", stats.kellyCriterion),
           cereal::make_nvp("avgMAE", stats.avgMAE),
           cereal::make_nvp("maxMAE", stats.maxMAE),
           cereal::make_nvp("ulcerIndex", stats.ulcerIndex),
           cereal::make_nvp("ulcerPerformanceIndex", stats.ulcerPerformanceIndex),
           cereal::make_nvp("skewness", stats.skewness),
           cereal::make_nvp("kurtosis", stats.kurtosis),
           cereal::make_nvp("omegaRatio", stats.omegaRatio));
    }

    template<class Archive>
    void serialize(Archive & ar, be::EquityPoint & point) {
        ar(cereal::make_nvp("index", point.index),
           cereal::make_nvp("value", point.value));
    }
    
    // Serialization for StrategyConfig
    // We separate save and load to handle potential retrocompatibility issues with former config file versions
    template<class Archive>
    void save(Archive & ar, const StrategyConfig & config) {
        ar(cereal::make_nvp("name", config.name),
           cereal::make_nvp("enable_logging", config.enable_logging),
           cereal::make_nvp("logLevel", config.logLevel),
           cereal::make_nvp("buyFilters", config.buyFilters),
           cereal::make_nvp("sellFilters", config.sellFilters),
           cereal::make_nvp("resaleFilters", config.resaleFilters),
           cereal::make_nvp("rebuyFilters", config.rebuyFilters),
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
           cereal::make_nvp("rl_model_path", config.rl_model_path),
           cereal::make_nvp("rl_lookback_periods", config.rl_lookback_periods),
           cereal::make_nvp("rl_tp_max_multiplier", config.rl_tp_max_multiplier),
           cereal::make_nvp("rl_tp_min_multiplier", config.rl_tp_min_multiplier),
           cereal::make_nvp("use_risk_based_sizing", config.use_risk_based_sizing),
           cereal::make_nvp("risk_percentage", config.risk_percentage),
           cereal::make_nvp("leverage_limit", config.leverage_limit),
           cereal::make_nvp("cash_allocation_percentage", config.cash_allocation_percentage),
           cereal::make_nvp("use_break_even", config.use_break_even),
           cereal::make_nvp("break_even_threshold", config.break_even_threshold),
           cereal::make_nvp("break_even_offset_per_mille", config.break_even_offset_per_mille),
           cereal::make_nvp("use_daily_max_loss", config.use_daily_max_loss),
           cereal::make_nvp("daily_max_loss_percentage", config.daily_max_loss_percentage),
           cereal::make_nvp("use_daily_max_profit", config.use_daily_max_profit),
           cereal::make_nvp("daily_max_profit_percentage", config.daily_max_profit_percentage),
           cereal::make_nvp("use_daily_max_drawdown", config.use_daily_max_drawdown),
           cereal::make_nvp("daily_max_drawdown_percentage", config.daily_max_drawdown_percentage),
           cereal::make_nvp("use_lua_script", config.use_lua_script),
           cereal::make_nvp("lua_script", config.lua_script));
    }

    template<class Archive>
    void load(Archive & ar, StrategyConfig & config) {
        ar(cereal::make_nvp("name", config.name),
           cereal::make_nvp("enable_logging", config.enable_logging),
           cereal::make_nvp("logLevel", config.logLevel),
           cereal::make_nvp("buyFilters", config.buyFilters),
           cereal::make_nvp("sellFilters", config.sellFilters),
           cereal::make_nvp("resaleFilters", config.resaleFilters),
           cereal::make_nvp("rebuyFilters", config.rebuyFilters),
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
           cereal::make_nvp("rl_model_path", config.rl_model_path),
           cereal::make_nvp("rl_lookback_periods", config.rl_lookback_periods),
           cereal::make_nvp("rl_tp_max_multiplier", config.rl_tp_max_multiplier),
           cereal::make_nvp("rl_tp_min_multiplier", config.rl_tp_min_multiplier),
           cereal::make_nvp("use_risk_based_sizing", config.use_risk_based_sizing),
           cereal::make_nvp("risk_percentage", config.risk_percentage),
           cereal::make_nvp("leverage_limit", config.leverage_limit),
           cereal::make_nvp("cash_allocation_percentage", config.cash_allocation_percentage),
           cereal::make_nvp("use_break_even", config.use_break_even),
           cereal::make_nvp("break_even_threshold", config.break_even_threshold),
           cereal::make_nvp("break_even_offset_per_mille", config.break_even_offset_per_mille),
           cereal::make_nvp("use_daily_max_loss", config.use_daily_max_loss),
           cereal::make_nvp("daily_max_loss_percentage", config.daily_max_loss_percentage),
           cereal::make_nvp("use_daily_max_profit", config.use_daily_max_profit),
           cereal::make_nvp("daily_max_profit_percentage", config.daily_max_profit_percentage),
           cereal::make_nvp("use_daily_max_drawdown", config.use_daily_max_drawdown),
           cereal::make_nvp("daily_max_drawdown_percentage", config.daily_max_drawdown_percentage));

        config.use_lua_script = false;
        config.lua_script.clear();

        try {
            ar(cereal::make_nvp("use_lua_script", config.use_lua_script));
        } catch (const std::exception&) {
            config.use_lua_script = false;
        }

        try {
            ar(cereal::make_nvp("lua_script", config.lua_script));
        } catch (const std::exception&) {
            config.lua_script.clear();
        }
    }
    
    // Serialization for GeneralParamsConfig
    template<class Archive>
    void serialize(Archive & ar, GeneralParamsConfig & config) {
        ar(cereal::make_nvp("symbol", config.symbol),
           cereal::make_nvp("interval", config.interval),
           cereal::make_nvp("period", config.period),
           cereal::make_nvp("endDate", config.endDate),
           cereal::make_nvp("cash", config.cash),
           cereal::make_nvp("spread", config.spread),
           cereal::make_nvp("commission", config.commission),
           cereal::make_nvp("leverage_limit", config.leverage_limit),
           cereal::make_nvp("tradeOnClose", config.tradeOnClose),
           cereal::make_nvp("positionMode", config.positionMode),
           cereal::make_nvp("executeLimitOnLimitPrice", config.executeLimitOnLimitPrice),
           cereal::make_nvp("executeStopOnOpen", config.executeStopOnOpen),
           cereal::make_nvp("spreadEntryRatio", config.spreadEntryRatio),
           cereal::make_nvp("minPositionStep", config.minPositionStep),
           cereal::make_nvp("finalizeTrades", config.finalizeTrades),
           cereal::make_nvp("resetIndicatorsOnNewDay", config.resetIndicatorsOnNewDay));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::GenericFilter & filter) {
        ar(cereal::make_nvp("leftValue", filter.leftValue),
           cereal::make_nvp("rightValue", filter.rightValue),
           cereal::make_nvp("comparisonOperator", filter.op),
           cereal::make_nvp("offset", filter.offset),
           cereal::make_nvp("temporalLogic", filter.temporalLogic),
           cereal::make_nvp("lookbackPeriods", filter.lookbackPeriods),
           cereal::make_nvp("enabled", filter.enabled), 
           cereal::make_nvp("description", filter.description));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::ValueSource & valueSource)
    {
        // Always serialize the category first
        ar(cereal::make_nvp("category", valueSource.category));
        
        // Serialize the members of the first union according to the category
        switch (valueSource.category) {
            case filter::ValueCategory::PRICE:
                ar(cereal::make_nvp("priceType", valueSource.priceType));
                break;
                
            case filter::ValueCategory::INDICATOR:
                ar(cereal::make_nvp("indicatorType", valueSource.indicatorType));
                
                // For indicators, serialize the correct parameter structure
                switch (valueSource.indicatorType) {
                    case filter::IndicatorType::EMA:
                        ar(cereal::make_nvp("emaParams", valueSource.emaParams));
                        break;
                        
                    case filter::IndicatorType::RSI:
                        ar(cereal::make_nvp("rsiParams", valueSource.rsiParams));
                        break;
                        
                    case filter::IndicatorType::STOCHASTIC_K:
                    case filter::IndicatorType::STOCHASTIC_D:
                        ar(cereal::make_nvp("stochParams", valueSource.stochParams));
                        break;
                        
                    case filter::IndicatorType::ATR:
                        ar(cereal::make_nvp("atrParams", valueSource.atrParams));
                        break;
                        
                    case filter::IndicatorType::SUPERTREND_VALUE:
                    case filter::IndicatorType::SUPERTREND_DIRECTION:
                        ar(cereal::make_nvp("supertrendParams", valueSource.supertrendParams));
                        break;
                    case filter::IndicatorType::CCI:
                        ar(cereal::make_nvp("cciParams", valueSource.cciParams));
                        break;
                    case filter::IndicatorType::MACD_HISTOGRAM:
                        ar(cereal::make_nvp("macdParams", valueSource.macdParams));
                        break;
                    case filter::IndicatorType::MACD_SIGNAL:
                        ar(cereal::make_nvp("macdParams", valueSource.macdParams));
                        break;
                    case filter::IndicatorType::MACD_LINE:
                        ar(cereal::make_nvp("macdParams", valueSource.macdParams));
                        break;
                    case filter::IndicatorType::BB_LOWER:
                        ar(cereal::make_nvp("bbParams", valueSource.bbParams));
                        break;
                    case filter::IndicatorType::BB_UPPER:
                        ar(cereal::make_nvp("bbParams", valueSource.bbParams));
                        break;
                    case filter::IndicatorType::BB_PERCENT_B:
                        ar(cereal::make_nvp("bbParams", valueSource.bbParams));
                        break;
                    case filter::IndicatorType::PIVOT_POINT:
                        // No specific parameter for this type
                        break;
                }
                break;
                
            case filter::ValueCategory::CANDLE_PROPERTY:
                ar(cereal::make_nvp("candlePropertyType", valueSource.candlePropertyType));
                break;
                
            case filter::ValueCategory::CONSTANT:
                // No member of the first union for constants
                break;
        }
        
        // Serialize members outside of the unions
        if (valueSource.category == filter::ValueCategory::CONSTANT) {
            ar(cereal::make_nvp("constantValue", valueSource.constantValue));
        }
        
        // Historical offset applies to all categories
        ar(cereal::make_nvp("historicalOffset", valueSource.historicalOffset));
        // ar(cereal::make_nvp("description", valueSource.description));
    }

    // Parameter structures
    template<class Archive>
    void serialize(Archive & ar, filter::EMAParams & params) {
        ar(cereal::make_nvp("period", params.period));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::RSIParams & params) {
        ar(cereal::make_nvp("period", params.period));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::StochasticParams & params) {
        ar(cereal::make_nvp("fastK", params.fastK),
           cereal::make_nvp("slowK", params.slowK),
           cereal::make_nvp("slowD", params.slowD));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::ATRParams & params) {
        ar(cereal::make_nvp("period", params.period),
           cereal::make_nvp("useLog", params.useLog));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::SuperTrendParams & params) {
        ar(cereal::make_nvp("atrPeriod", params.atrPeriod),
           cereal::make_nvp("multiplier", params.multiplier));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::CCIParams & params) {
        ar(cereal::make_nvp("period", params.period));
    }

    template<class Archive>
    void serialize(Archive & ar, filter::MACDParams & params) {
        ar(cereal::make_nvp("fast", params.fast),
           cereal::make_nvp("slow", params.slow),
           cereal::make_nvp("signal", params.signal),
           cereal::make_nvp("source", params.source),
           cereal::make_nvp("osc_ma_type", params.osc_ma_type),
           cereal::make_nvp("signal_ma_type", params.signal_ma_type),
           cereal::make_nvp("signal_smoothing", params.signal_smoothing));
    }
    
    template<class Archive>
    void serialize(Archive & ar, filter::BBParams & params) {
        ar(cereal::make_nvp("period", params.period),
           cereal::make_nvp("stdDevMultiplier", params.stddev_multiplier),
           cereal::make_nvp("source", params.source),
           cereal::make_nvp("osc_ma_type", params.ma_type));
   }

    // Indicator Base and Subclasses Serialization
    template<class Archive>
    void serialize(Archive & ar, indicators::IndicatorBase & ind) {
        ar(cereal::make_nvp("id", ind.id),
           cereal::make_nvp("visible", ind.visible),
           cereal::make_nvp("resetOnNewDay", ind.resetOnNewDay));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::RSIInstance & rsi) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&rsi)),
           cereal::make_nvp("period", rsi.period),
           cereal::make_nvp("height", rsi.height),
           cereal::make_nvp("color", rsi.color),
           cereal::make_nvp("overboughtLevel", rsi.overboughtLevel),
           cereal::make_nvp("oversoldLevel", rsi.oversoldLevel),
           cereal::make_nvp("upperColor", rsi.upperColor),
           cereal::make_nvp("lowerColor", rsi.lowerColor));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::EMAInstance & ema) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&ema)),
           cereal::make_nvp("period", ema.period),
           cereal::make_nvp("color", ema.color));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::SuperTrendInstance & st) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&st)),
           cereal::make_nvp("period", st.period),
           cereal::make_nvp("multiplier", st.multiplier),
           cereal::make_nvp("upColor", st.upColor),
           cereal::make_nvp("downColor", st.downColor));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::StochasticInstance & stoch) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&stoch)),
           cereal::make_nvp("fastKPeriod", stoch.fastKPeriod),
           cereal::make_nvp("slowKPeriod", stoch.slowKPeriod),
           cereal::make_nvp("slowDPeriod", stoch.slowDPeriod),
           cereal::make_nvp("height", stoch.height),
           cereal::make_nvp("kColor", stoch.kColor),
           cereal::make_nvp("dColor", stoch.dColor),
           cereal::make_nvp("overboughtLevel", stoch.overboughtLevel),
           cereal::make_nvp("oversoldLevel", stoch.oversoldLevel));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::ATRInstance & atr) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&atr)),
           cereal::make_nvp("period", atr.period),
           cereal::make_nvp("height", atr.height),
           cereal::make_nvp("color", atr.color),
           cereal::make_nvp("useLogScale", atr.useLogScale));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::PivotPointsInstance::LevelStyle & style) {
        ar(cereal::make_nvp("color", style.color),
           cereal::make_nvp("thickness", style.thickness),
           cereal::make_nvp("lineStyle", style.lineStyle),
           cereal::make_nvp("visible", style.visible),
           cereal::make_nvp("labelFormat", style.labelFormat));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::PivotPointsInstance & pp) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&pp)),
           cereal::make_nvp("periodType", pp.periodType),
           cereal::make_nvp("calculationMethod", pp.calculationMethod),
           cereal::make_nvp("levelStyles", pp.levelStyles),
           cereal::make_nvp("showLabels", pp.showLabels));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::CCIInstance & cci) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&cci)),
           cereal::make_nvp("period", cci.period),
           cereal::make_nvp("height", cci.height),
           cereal::make_nvp("color", cci.color),
           cereal::make_nvp("upperLevel", cci.upperLevel),
           cereal::make_nvp("lowerLevel", cci.lowerLevel),
           cereal::make_nvp("upperColor", cci.upperColor),
           cereal::make_nvp("lowerColor", cci.lowerColor));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::MACDInstance & macd) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&macd)),
           cereal::make_nvp("fastPeriod", macd.fastPeriod),
           cereal::make_nvp("slowPeriod", macd.slowPeriod),
           cereal::make_nvp("signalPeriod", macd.signalPeriod),
           cereal::make_nvp("source", macd.source),
           cereal::make_nvp("osc_ma_type", macd.osc_ma_type),
           cereal::make_nvp("signal_ma_type", macd.signal_ma_type),
           cereal::make_nvp("signal_smoothing", macd.signal_smoothing),
           cereal::make_nvp("height", macd.height),
           cereal::make_nvp("macdColor", macd.macdColor),
           cereal::make_nvp("signalColor", macd.signalColor),
           cereal::make_nvp("histogramColor", macd.histogramColor));
    }

    template<class Archive>
    void serialize(Archive & ar, indicators::BBInstance & bb) {
        ar(cereal::make_nvp("base", cereal::base_class<indicators::IndicatorBase>(&bb)),
           cereal::make_nvp("period", bb.period),
           cereal::make_nvp("stddev_multiplier", bb.stddev_multiplier),
           cereal::make_nvp("source", bb.source),
           cereal::make_nvp("ma_type", bb.ma_type),
           cereal::make_nvp("height", bb.height),
           cereal::make_nvp("middleBandColor", bb.middleBandColor),
           cereal::make_nvp("upperBandColor", bb.upperBandColor),
           cereal::make_nvp("lowerBandColor", bb.lowerBandColor),
           cereal::make_nvp("fillColor", bb.fillColor));
    }
}