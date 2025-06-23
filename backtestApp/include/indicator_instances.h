#pragma once


enum class IndicatorType {
    RSI,
    EMA,
    STOCHASTIC,
    ATR,
    SUPERTREND,
    // autres types futurs
};

struct IndicatorBase {
    IndicatorBase(IndicatorType type) : type_(type) {}
    int id = -1; // Identifiant unique de l'indicateur
    IndicatorType type_; // Type d'indicateur
    bool visible = true; // Si l'indicateur est visible

    virtual bool needsRecalculation(const IndicatorBase& other) const = 0;

    bool operator==(const IndicatorBase& other) const {
        return id == other.id && type_ == other.type_;
    }

    bool operator!=(const IndicatorBase& other) const {
        return !(*this == other);
    }
};

struct RSIInstance : public IndicatorBase {
    RSIInstance() : IndicatorBase(IndicatorType::RSI) {}
    int period;             // Période du RSI
    int height = 120;       // Hauteur du panneau
    int color = 0x800080;   // Couleur de la ligne principale (violet par défaut)
    int overboughtLevel = 80; // Niveau de surachat
    int oversoldLevel = 20;   // Niveau de survente
    int upperColor = 0xff6666; // Couleur pour la zone de surachat
    int lowerColor = 0x6666ff; // Couleur pour la zone de survente

    bool needsRecalculation(const IndicatorBase& other) const override {
        const RSIInstance* otherRSI = dynamic_cast<const RSIInstance*>(&other);
        if (!otherRSI) return true;
        return period != otherRSI->period;
    }
};

struct EMAInstance : public IndicatorBase {
    EMAInstance() : IndicatorBase(IndicatorType::EMA) {}
    int period;            // Période de l'EMA
    int color = 0x0000FF;  // Couleur de la ligne (bleu par défaut)

    bool needsRecalculation(const IndicatorBase& other) const override {
        const EMAInstance* otherEMA = dynamic_cast<const EMAInstance*>(&other);
        if (!otherEMA) return true;
        return period != otherEMA->period;
    }
};

struct SuperTrendInstance : public IndicatorBase {
    SuperTrendInstance() : IndicatorBase(IndicatorType::SUPERTREND) {}
    int period;            // Période pour le SuperTrend
    double multiplier;     // Multiplicateur pour le SuperTrend
    int upColor = 0x00AA00;  // Couleur de la ligne (vert par défaut)
    int downColor = 0xFF0000; // Couleur de la ligne (rouge par défaut)

    bool needsRecalculation(const IndicatorBase& other) const override {
        const SuperTrendInstance* otherST = dynamic_cast<const SuperTrendInstance*>(&other);
        if (!otherST) return true;
        return period != otherST->period || multiplier != otherST->multiplier;
    }
};

struct StochasticInstance : public IndicatorBase {
    StochasticInstance() : IndicatorBase(IndicatorType::STOCHASTIC) {}
    int fastKPeriod;        // Période pour calculer le %K brut
    int slowKPeriod;        // Période de lissage pour %K
    int slowDPeriod;        // Période pour calculer %D
    int height = 120;       // Hauteur du panneau
    int kColor = 0x0000FF;  // Couleur de la ligne %K (bleu par défaut)
    int dColor = 0xFF0000;  // Couleur de la ligne %D (rouge par défaut)
    int overboughtLevel = 80; // Niveau de surachat
    int oversoldLevel = 20;   // Niveau de survente

    bool needsRecalculation(const IndicatorBase& other) const override {
        const StochasticInstance* otherStochastic = dynamic_cast<const StochasticInstance*>(&other);
        if (!otherStochastic) return true;
        return fastKPeriod != otherStochastic->fastKPeriod ||
               slowKPeriod != otherStochastic->slowKPeriod ||
               slowDPeriod != otherStochastic->slowDPeriod;
    }
};

struct ATRInstance : public IndicatorBase {
    ATRInstance() : IndicatorBase(IndicatorType::ATR) {}
    int period;            // Période de l'ATR
    int height = 120;      // Hauteur du panneau
    int color = 0x006400;  // Couleur de la ligne (vert foncé par défaut)
    bool useLogScale = false; // Indique si l'échelle logarithmique est utilisée

    bool needsRecalculation(const IndicatorBase& other) const override {
        const ATRInstance* otherATR = dynamic_cast<const ATRInstance*>(&other);
        if (!otherATR) return true;
        return period != otherATR->period || useLogScale != otherATR->useLogScale;
    }
};