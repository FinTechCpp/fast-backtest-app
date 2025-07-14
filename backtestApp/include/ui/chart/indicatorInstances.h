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
        FourHour,   // Points pivots toutes les 4 heures
        Daily,      // Points pivots quotidiens
        Weekly,     // Points pivots hebdomadaires
        Monthly,    // Points pivots mensuels
    };

    enum class LevelType {
        R3,         // Résistance 3
        R2,         // Résistance 2
        R1,         // Résistance 1
        Pivot,      // Point pivot principal (PP)
        S1,         // Support 1
        S2,         // Support 2
        S3,         // Support 3
        M_R2R3,     // Milieu entre R2 et R3
        M_R1R2,     // Milieu entre R1 et R2
        M_PR1,      // Milieu entre PP et R1
        M_PS1,      // Milieu entre PP et S1
        M_S1S2,     // Milieu entre S1 et S2
        M_S2S3,     // Milieu entre S2 et S3

        NumLevels   // Nombre total de niveaux
    };

    enum class LineStyle {
        Solid,
        Dash,
        Dot
    };

    struct LevelStyle {
        int color = 0x000000;     // Couleur de la ligne
        int thickness = 2;    // Épaisseur (1-3)
        LineStyle lineStyle = LineStyle::Solid; // Style (solid, dash, dot, etc.)
        bool visible = false;     // Visibilité du niveau

        QString labelFormat = QString(); // Format d'affichage optionnel (ex: "PP: %.2f")
    };

    PivotPointsInstance() : IndicatorBase(IndicatorType::PivotPoints) {
        initializeDefaultStyles();
    }
    PeriodType periodType;
    std::map<LevelType, LevelStyle> levelStyles;
    // bool showMidLevels = false;  // Afficher les niveaux milieux
    bool showLabels = true;     // Afficher les étiquettes des niveaux
    // peut etre ajouter la configuration de l'affichage des niveaux 3, 4, 5, etc. (activable desactivable)


    bool needsRecalculation(const IndicatorBase& other) const override {
        const PivotPointsInstance* otherPP = dynamic_cast<const PivotPointsInstance*>(&other);
        if (!otherPP) return true;
        return periodType != otherPP->periodType;
    }
    
    QString getDisplayName() const override {
        QString periodStr;
        switch (periodType) {
            case PeriodType::FourHour: periodStr = "4H"; break;
            case PeriodType::Daily: periodStr = "Daily"; break;
            case PeriodType::Weekly: periodStr = "Weekly"; break;
            case PeriodType::Monthly: periodStr = "Monthly"; break;
        }
        return QString("Pivot Points (%1)").arg(periodStr);
    }

    bool isLevelVisible(LevelType level) const {
        auto it = levelStyles.find(level);
        if (it == levelStyles.end()) return false;
        return it->second.visible;
    }

    void initializeDefaultStyles() {
        // Point pivot central (noir, trait plein, visible)
        LevelStyle pivotStyle;
        pivotStyle.color = 0x000000;  // Noir
        pivotStyle.thickness = 2;
        pivotStyle.lineStyle = LineStyle::Solid;
        pivotStyle.visible = true;
        pivotStyle.labelFormat = "PP: %.2f";
        levelStyles[LevelType::Pivot] = pivotStyle;
        
        // Résistances (rouge, trait plein, visibles)
        LevelStyle resistanceStyle;
        resistanceStyle.color = 0xFF0000;  // Rouge
        resistanceStyle.thickness = 2;
        resistanceStyle.lineStyle = LineStyle::Solid;
        resistanceStyle.visible = true;
        
        resistanceStyle.labelFormat = "R1: %.2f";
        levelStyles[LevelType::R1] = resistanceStyle;
        
        resistanceStyle.labelFormat = "R2: %.2f";
        levelStyles[LevelType::R2] = resistanceStyle;
        
        resistanceStyle.labelFormat = "R3: %.2f";
        levelStyles[LevelType::R3] = resistanceStyle;
        
        // Supports (vert, trait plein, visibles)
        LevelStyle supportStyle;
        supportStyle.color = 0x008000;  // Vert
        supportStyle.thickness = 2;
        supportStyle.lineStyle = LineStyle::Solid;
        supportStyle.visible = true;
        
        supportStyle.labelFormat = "S1: %.2f";
        levelStyles[LevelType::S1] = supportStyle;
        
        supportStyle.labelFormat = "S2: %.2f";
        levelStyles[LevelType::S2] = supportStyle;
        
        supportStyle.labelFormat = "S3: %.2f";
        levelStyles[LevelType::S3] = supportStyle;
        
        // Niveaux milieux résistance (rouge, trait pointillé, non visibles par défaut)
        LevelStyle midResistanceStyle;
        midResistanceStyle.color = 0xFF0000;  // Rouge
        midResistanceStyle.thickness = 1;
        midResistanceStyle.lineStyle = LineStyle::Dash;
        midResistanceStyle.visible = false;  // Visible si showMidLevels est true
        
        midResistanceStyle.labelFormat = "M(R2-R3): %.2f";
        levelStyles[LevelType::M_R2R3] = midResistanceStyle;
        
        midResistanceStyle.labelFormat = "M(R1-R2): %.2f";
        levelStyles[LevelType::M_R1R2] = midResistanceStyle;
        
        midResistanceStyle.labelFormat = "M(P-R1): %.2f";
        levelStyles[LevelType::M_PR1] = midResistanceStyle;
        
        // Niveaux milieux support (vert, trait pointillé, non visibles par défaut)
        LevelStyle midSupportStyle;
        midSupportStyle.color = 0x008000;  // Vert
        midSupportStyle.thickness = 1;
        midSupportStyle.lineStyle = LineStyle::Dash;
        midSupportStyle.visible = false;  // Visible si showMidLevels est true
        
        midSupportStyle.labelFormat = "M(P-S1): %.2f";
        levelStyles[LevelType::M_PS1] = midSupportStyle;
        
        midSupportStyle.labelFormat = "M(S1-S2): %.2f";
        levelStyles[LevelType::M_S1S2] = midSupportStyle;
        
        midSupportStyle.labelFormat = "M(S2-S3): %.2f";
        levelStyles[LevelType::M_S2S3] = midSupportStyle;
        
        // Activer les étiquettes par défaut
        showLabels = true;
        
        // Type de période par défaut
        periodType = PeriodType::Daily;
    }
};