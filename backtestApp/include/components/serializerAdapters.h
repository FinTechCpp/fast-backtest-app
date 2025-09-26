#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>

#include "ui/panels/generalParamsPanel.h"
#include "common.h"

#include "beTypes.h"
#include "ui/chart/chartTypes.h"

struct ProfileConfig {
    std::string name;
    std::string version;
    std::string createdAt;
    GeneralParamsConfig generalParams;
    StrategyConfig strategyConfig;

    template<class Archive>
    void serialize(Archive & ar) {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(version),
           CEREAL_NVP(createdAt),
           CEREAL_NVP(generalParams),
           CEREAL_NVP(strategyConfig));
    }
};

// TODO finir l'enregistrement total des resultats de backtest
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
    
    // Sérialisation pour StrategyConfig
    // TODO il faudra ajouter les filtres et la direction
    template<class Archive>
    void serialize(Archive & ar, StrategyConfig & config) {
        ar(cereal::make_nvp("name", config.name),
           cereal::make_nvp("tradeDirection", config.tradeDirection),
           cereal::make_nvp("enable_logging", config.enable_logging),
           cereal::make_nvp("logLevel", config.logLevel),
           cereal::make_nvp("filters", config.filters),
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

    template<class Archive>
    void serialize(Archive & ar, GenericFilter & filter) {
        ar(cereal::make_nvp("leftValue", filter.leftValue),
           cereal::make_nvp("rightValue", filter.rightValue),
           cereal::make_nvp("comparisonOperator", filter.op),
           cereal::make_nvp("temporalLogic", filter.temporalLogic),
           cereal::make_nvp("lookbackPeriods", filter.lookbackPeriods),
           cereal::make_nvp("enabled", filter.enabled), 
           cereal::make_nvp("description", filter.description));
    }

    template<class Archive>
    void serialize(Archive & ar, ValueSource & valueSource)
    {
        // Toujours sérialiser la catégorie en premier
        ar(cereal::make_nvp("category", valueSource.category));
        
        // Sérialiser les membres de la première union selon la catégorie
        switch (valueSource.category) {
            case ValueCategory::PRICE:
                ar(cereal::make_nvp("priceType", valueSource.priceType));
                break;
                
            case ValueCategory::INDICATOR:
                ar(cereal::make_nvp("indicatorType", valueSource.indicatorType));
                
                // Pour les indicateurs, sérialiser la bonne structure de paramètres
                switch (valueSource.indicatorType) {
                    case IndicatorType::EMA:
                        ar(cereal::make_nvp("emaParams", valueSource.emaParams));
                        break;
                        
                    case IndicatorType::RSI:
                        ar(cereal::make_nvp("rsiParams", valueSource.rsiParams));
                        break;
                        
                    case IndicatorType::STOCHASTIC_K:
                    case IndicatorType::STOCHASTIC_D:
                        ar(cereal::make_nvp("stochParams", valueSource.stochParams));
                        break;
                        
                    case IndicatorType::ATR:
                        ar(cereal::make_nvp("atrParams", valueSource.atrParams));
                        break;
                        
                    case IndicatorType::SUPERTREND_VALUE:
                    case IndicatorType::SUPERTREND_DIRECTION:
                        ar(cereal::make_nvp("supertrendParams", valueSource.supertrendParams));
                        break;
                        
                    case IndicatorType::PIVOT_POINT:
                        // Pas de paramètre spécifique pour ce type
                        break;
                }
                break;
                
            case ValueCategory::CANDLE_PROPERTY:
                ar(cereal::make_nvp("candlePropertyType", valueSource.candlePropertyType));
                break;
                
            case ValueCategory::CONSTANT:
                // Pas de membre de la première union pour les constantes
                break;
        }
        
        // Sérialiser les membres hors des unions
        if (valueSource.category == ValueCategory::CONSTANT) {
            ar(cereal::make_nvp("constantValue", valueSource.constantValue));
        }
        
        // Le décalage historique s'applique à toutes les catégories
        ar(cereal::make_nvp("historicalOffset", valueSource.historicalOffset));
    }

    // Parameter structures
    template<class Archive>
    void serialize(Archive & ar, EMAParams & params) {
        ar(cereal::make_nvp("period", params.period));
    }

    template<class Archive>
    void serialize(Archive & ar, RSIParams & params) {
        ar(cereal::make_nvp("period", params.period));
    }

    template<class Archive>
    void serialize(Archive & ar, StochasticParams & params) {
        ar(cereal::make_nvp("fastK", params.fastK),
           cereal::make_nvp("slowK", params.slowK),
           cereal::make_nvp("slowD", params.slowD));
    }

    template<class Archive>
    void serialize(Archive & ar, ATRParams & params) {
        ar(cereal::make_nvp("period", params.period),
           cereal::make_nvp("useLog", params.useLog));
    }

    template<class Archive>
    void serialize(Archive & ar, SuperTrendParams & params) {
        ar(cereal::make_nvp("atrPeriod", params.atrPeriod),
           cereal::make_nvp("multiplier", params.multiplier));
    }
}