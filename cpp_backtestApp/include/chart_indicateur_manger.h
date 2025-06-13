// #pragma once


// class ChartIndicatorManager {
// public:
//     // RSI
//     int addRSI(int period);
//     bool setRSIConfig(int id, const RSIInstance &config);
//     bool removeRSI(int id);
    
//     // EMA
//     int addEMA(int period);
//     bool setEMAConfig(int id, const EMAInstance& config);
//     bool removeEMA(int id);
    
//     // Stochastic
//     int addStochastic(int fastKPeriod, int slowKPeriod, int slowDPeriod);
//     bool setStochasticConfig(int id, const StochasticInstance& config);
//     bool removeStochastic(int id);
    
//     // ATR
//     int addATR(int period);
//     bool setATRConfig(int id, const ATRInstance& config);
//     bool removeATR(int id);
    
//     void updateIndicatorCache(const std::shared_ptr<const be::Data>& data);
    
// private:
//     // Méthodes de calcul
//     void ensureRSICached(int period);
//     void ensureEMACached(int period);
//     void ensureStochasticCached(int fastKPeriod, int slowKPeriod, int slowDPeriod);
//     void ensureATRCached(int period);
    
//     // Données
//     IndicatorCache m_indicatorCache;
//     std::vector<RSIInstance> m_rsiInstances;
//     std::vector<EMAInstance> m_emaInstances;
//     std::vector<StochasticInstance> m_stochasticInstances;
//     std::vector<ATRInstance> m_atrInstances;
    
//     // Compteurs pour les IDs
//     int m_nextRSIId = 0;
//     int m_nextEMAId = 0;
//     int m_nextStochasticId = 0;
//     int m_nextATRId = 0;
// };