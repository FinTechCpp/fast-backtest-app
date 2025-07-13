#pragma once

#include <QString>
#include <vector>
#include <memory>
#include <map>

enum class IndicatorType {
    RSI,
    EMA,
    STOCHASTIC,
    ATR,
    SUPERTREND,
    PivotPoints,
};

struct IndicatorBase {
    IndicatorBase(IndicatorType type) : type_(type) {}
    int id = -1; // Identifiant unique de l'indicateur
    IndicatorType type_; // Type d'indicateur
    bool visible = true; // Si l'indicateur est visible

    virtual bool needsRecalculation(const IndicatorBase& other) const = 0;
    
    // Nouvelle méthode pour obtenir le nom d'affichage de l'indicateur
    virtual QString getDisplayName() const = 0;

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
    
    QString getDisplayName() const override {
        return QString("RSI (%1)").arg(period);
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
    
    QString getDisplayName() const override {
        return QString("EMA (%1)").arg(period);
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
    
    QString getDisplayName() const override {
        return QString("Supertrend (%1, %2)").arg(period).arg(multiplier, 0, 'f', 1);
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
    
    QString getDisplayName() const override {
        return QString("Stochastic (%1,%2,%3)").arg(fastKPeriod).arg(slowKPeriod).arg(slowDPeriod);
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
    
    QString getDisplayName() const override {
        return QString("ATR (%1)").arg(period);
    }
};


struct PivotPointsInstance : public IndicatorBase {
    enum class PeriodType {
        Daily,      // Points pivots quotidiens
        Weekly,     // Points pivots hebdomadaires
        Monthly,    // Points pivots mensuels
        Quarterly,  // Points pivots trimestriels
        Yearly      // Points pivots annuels
    };

    enum class LevelType {
        Pivot,      // Point pivot principal (PP)
        R1, R2, R3, // Niveaux de résistance
        S1, S2, S3, // Niveaux de support
        M_PR1,      // Milieu entre PP et R1
        M_R1R2,     // Milieu entre R1 et R2
        M_R2R3,     // Milieu entre R2 et R3
        M_PS1,      // Milieu entre PP et S1
        M_S1S2,     // Milieu entre S1 et S2
        M_S2S3      // Milieu entre S2 et S3
    };

    struct LevelStyle {
        int color = 0x006400;     // Couleur de la ligne
        int thickness;    // Épaisseur (1-3)
        Qt::PenStyle lineStyle; // Style (solid, dash, dot, etc.)
        bool visible;     // Visibilité du niveau
        
        QString labelFormat; // Format d'affichage optionnel (ex: "PP: %.2f")
    };

    PivotPointsInstance() : IndicatorBase(IndicatorType::PivotPoints) {}
    PeriodType periodType;
    std::map<LevelType, LevelStyle> levelStyles;
    bool showMidLevels;  // Afficher les niveaux milieux
    bool showLabels;     // Afficher les étiquettes des niveaux
    // peut etre ajouter la configuration de l'affichage des niveaux 3, 4, 5, etc. (activable desactivable)


    bool needsRecalculation(const IndicatorBase& other) const override {
        const PivotPointsInstance* otherPP = dynamic_cast<const PivotPointsInstance*>(&other);
        if (!otherPP) return true;
        return true;
        // return periodType != otherPP->periodType || 
        //        showMidLevels != otherPP->showMidLevels;
    }
    
    QString getDisplayName() const override {
        QString periodStr;
        switch (periodType) {
            case PeriodType::Daily: periodStr = "Daily"; break;
            case PeriodType::Weekly: periodStr = "Weekly"; break;
            case PeriodType::Monthly: periodStr = "Monthly"; break;
            case PeriodType::Quarterly: periodStr = "Quarterly"; break;
            case PeriodType::Yearly: periodStr = "Yearly"; break;
        }
        return QString("Pivot Points (%1)").arg(periodStr);
    }

    bool isLevelVisible(LevelType level) const {
        auto it = levelStyles.find(level);
        if (it == levelStyles.end()) return false;
        
        // Vérifier si c'est un niveau milieu et si les niveaux milieux sont activés
        bool isMidLevel = (level >= LevelType::M_PR1);
        return it->second.visible && (!isMidLevel || showMidLevels);
    }
};