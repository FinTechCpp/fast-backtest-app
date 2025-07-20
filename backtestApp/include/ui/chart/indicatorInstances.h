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

    virtual void setDefaults() = 0;

    virtual ~IndicatorBase() = default;

    bool operator==(const IndicatorBase& other) const {
        return id == other.id && type_ == other.type_;
    }

    bool operator!=(const IndicatorBase& other) const {
        return !(*this == other);
    }
};

struct RSIInstance : public IndicatorBase {
    RSIInstance() : IndicatorBase(IndicatorType::RSI) {
        setDefaults();
    }
    int period;             // Période du RSI
    int height;             // Hauteur du panneau
    int color;              // Couleur de la ligne principale (violet par défaut)
    int overboughtLevel;    // Niveau de surachat
    int oversoldLevel;      // Niveau de survente
    int upperColor;         // Couleur pour la zone de surachat
    int lowerColor;         // Couleur pour la zone de survente

    bool needsRecalculation(const IndicatorBase& other) const override {
        const RSIInstance* otherRSI = dynamic_cast<const RSIInstance*>(&other);
        if (!otherRSI) return true;
        return period != otherRSI->period;
    }
    
    QString getDisplayName() const override {
        return QString("RSI (%1)").arg(period);
    }

    void setDefaults() override {
        period = 14;
        height = 120;
        color = 0x800080; // Violet
        overboughtLevel = 80;
        oversoldLevel = 20;
        upperColor = 0xff6666; // Rouge clair
        lowerColor = 0x6666ff; // Bleu clair
    }
};

struct EMAInstance : public IndicatorBase {
    EMAInstance() : IndicatorBase(IndicatorType::EMA) {
        setDefaults();
    }
    int period;            // Période de l'EMA
    int color;             // Couleur de la ligne (bleu par défaut)

    bool needsRecalculation(const IndicatorBase& other) const override {
        const EMAInstance* otherEMA = dynamic_cast<const EMAInstance*>(&other);
        if (!otherEMA) return true;
        return period != otherEMA->period;
    }
    
    QString getDisplayName() const override {
        return QString("EMA (%1)").arg(period);
    }

    void setDefaults() override {
        period = 20;
        color = 0x0000FF; // Bleu par défaut
    }
};

struct SuperTrendInstance : public IndicatorBase {
    SuperTrendInstance() : IndicatorBase(IndicatorType::SUPERTREND) {
        setDefaults();
    }
    int period;            // Période pour le SuperTrend
    double multiplier;     // Multiplicateur pour le SuperTrend
    int upColor;           // Couleur de la ligne (vert par défaut)
    int downColor;         // Couleur de la ligne (rouge par défaut)

    bool needsRecalculation(const IndicatorBase& other) const override {
        const SuperTrendInstance* otherST = dynamic_cast<const SuperTrendInstance*>(&other);
        if (!otherST) return true;
        return period != otherST->period || multiplier != otherST->multiplier;
    }
    
    QString getDisplayName() const override {
        return QString("Supertrend (%1, %2)").arg(period).arg(multiplier, 0, 'f', 1);
    }

    void setDefaults() override {
        period = 10;
        multiplier = 3.0;
        upColor = 0xFFA500; // Orange doré
        downColor = 0x8B008B; // Rose extrêmememnt foncé
    }
};

struct StochasticInstance : public IndicatorBase {
    StochasticInstance() : IndicatorBase(IndicatorType::STOCHASTIC) {
        setDefaults();
    }
    int fastKPeriod;        // Période pour calculer le %K brut
    int slowKPeriod;        // Période de lissage pour %K
    int slowDPeriod;        // Période pour calculer %D
    int height;             // Hauteur du panneau
    int kColor;             // Couleur de la ligne %K (bleu par défaut)
    int dColor;             // Couleur de la ligne %D (rouge par défaut)
    int overboughtLevel;    // Niveau de surachat
    int oversoldLevel;      // Niveau de survente

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

    void setDefaults() override {
        fastKPeriod = 14;
        slowKPeriod = 3;
        slowDPeriod = 3;
        height = 120;
        kColor = 0x0000FF;
        dColor = 0xFF0000;
        overboughtLevel = 80;
        oversoldLevel = 20;
    }
};

struct ATRInstance : public IndicatorBase {
    ATRInstance() : IndicatorBase(IndicatorType::ATR) {
        setDefaults();
    }
    int period;            // Période de l'ATR
    int height;            // Hauteur du panneau
    int color;             // Couleur de la ligne (vert foncé par défaut)
    bool useLogScale;      // Indique si l'échelle logarithmique est utilisée

    bool needsRecalculation(const IndicatorBase& other) const override {
        const ATRInstance* otherATR = dynamic_cast<const ATRInstance*>(&other);
        if (!otherATR) return true;
        return period != otherATR->period || useLogScale != otherATR->useLogScale;
    }
    
    QString getDisplayName() const override {
        return QString("ATR (%1)").arg(period);
    }

    void setDefaults() override {
        period = 14;
        height = 120;
        color = 0x006400;
        useLogScale = false;
    }
};


struct PivotPointsInstance : public IndicatorBase {
    enum class PeriodType {
        FourHour,   // Points pivots toutes les 4 heures
        Daily,      // Points pivots quotidiens
        Weekly,     // Points pivots hebdomadaires
        Monthly,    // Points pivots mensuels
    };

    enum class CalculationMethod {
        HLC,     // High, Low, Close (méthode standard)
        OHLC,    // Open, High, Low, Close
        HL0       // High, Low, Open
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
        Dot,
        DotDash,
        AltDash
    };

    struct LevelStyle {
        int color = 0x000000;     // Couleur de la ligne
        int thickness = 2;    // Épaisseur (1-3)
        LineStyle lineStyle = LineStyle::Solid; // Style (solid, dash, dot, etc.)
        bool visible = false;     // Visibilité du niveau

        QString labelFormat = QString(); // Format d'affichage optionnel (ex: "PP: %.2f")
    };

    PivotPointsInstance() : IndicatorBase(IndicatorType::PivotPoints) {
        setDefaults();
    }
    PeriodType periodType;                          // Type de période (4H, journalier, hebdomadaire, mensuel)
    CalculationMethod calculationMethod;            // Méthode de calcul des points pivots
    std::map<LevelType, LevelStyle> levelStyles;    // Styles pour chaque niveau de pivot
    bool showLabels = true;                         // Afficher les étiquettes des niveaux


    bool needsRecalculation(const IndicatorBase& other) const override {
        const PivotPointsInstance* otherPP = dynamic_cast<const PivotPointsInstance*>(&other);
        if (!otherPP) return true;
        return periodType != otherPP->periodType || calculationMethod != otherPP->calculationMethod;
    }
    
    QString getDisplayName() const override {
        QString periodStr;
        switch (periodType) {
            case PeriodType::FourHour: periodStr = "4H"; break;
            case PeriodType::Daily: periodStr = "Jour"; break;
            case PeriodType::Weekly: periodStr = "Hebdomadaire"; break;
            case PeriodType::Monthly: periodStr = "Mensuel"; break;
        }
        return QString("Pivot Points (%1)").arg(periodStr);
    }

    bool isLevelVisible(LevelType level) const {
        auto it = levelStyles.find(level);
        if (it == levelStyles.end()) return false;
        return it->second.visible;
    }

    void setDefaults() override {
        // Point pivot central (noir, trait plein, visible)
        LevelStyle pivotStyle;
        pivotStyle.color = 0x000000;  // Noir
        pivotStyle.thickness = 2;
        pivotStyle.lineStyle = LineStyle::Solid;
        pivotStyle.visible = true;
        pivotStyle.labelFormat = "Piv %1";
        levelStyles[LevelType::Pivot] = pivotStyle;
        
        // Résistances (rouge, trait plein, visibles)
        LevelStyle resistanceStyle;
        resistanceStyle.color = 0xFF0000;  // Rouge
        resistanceStyle.thickness = 2;
        resistanceStyle.lineStyle = LineStyle::Solid;
        resistanceStyle.visible = true;
        
        resistanceStyle.labelFormat = "R1 %1";
        levelStyles[LevelType::R1] = resistanceStyle;

        resistanceStyle.labelFormat = "R2 %1";
        levelStyles[LevelType::R2] = resistanceStyle;

        resistanceStyle.labelFormat = "R3 %1";
        levelStyles[LevelType::R3] = resistanceStyle;
        
        // Supports (vert, trait plein, visibles)
        LevelStyle supportStyle;
        supportStyle.color = 0x008000;  // Vert
        supportStyle.thickness = 2;
        supportStyle.lineStyle = LineStyle::Solid;
        supportStyle.visible = true;

        supportStyle.labelFormat = "S1 %1";
        levelStyles[LevelType::S1] = supportStyle;

        supportStyle.labelFormat = "S2 %1";
        levelStyles[LevelType::S2] = supportStyle;

        supportStyle.labelFormat = "S3 %1";
        levelStyles[LevelType::S3] = supportStyle;
        
        // Niveaux milieux résistance (rouge, trait pointillé, non visibles par défaut)
        LevelStyle midResistanceStyle;
        midResistanceStyle.color = 0xFF0000;  // Rouge
        midResistanceStyle.thickness = 1;
        midResistanceStyle.lineStyle = LineStyle::Dash;
        midResistanceStyle.visible = false;  // Visible si showMidLevels est true
        
        midResistanceStyle.labelFormat = "mR3 %1";
        levelStyles[LevelType::M_R2R3] = midResistanceStyle;

        midResistanceStyle.labelFormat = "mR2 %1";
        levelStyles[LevelType::M_R1R2] = midResistanceStyle;

        midResistanceStyle.labelFormat = "mR1 %1";
        levelStyles[LevelType::M_PR1] = midResistanceStyle;
        
        // Niveaux milieux support (vert, trait pointillé, non visibles par défaut)
        LevelStyle midSupportStyle;
        midSupportStyle.color = 0x008000;  // Vert
        midSupportStyle.thickness = 1;
        midSupportStyle.lineStyle = LineStyle::Dash;
        midSupportStyle.visible = false;  // Visible si showMidLevels est true

        midSupportStyle.labelFormat = "mS1 %1";
        levelStyles[LevelType::M_PS1] = midSupportStyle;

        midSupportStyle.labelFormat = "mS2 %1";
        levelStyles[LevelType::M_S1S2] = midSupportStyle;

        midSupportStyle.labelFormat = "mS3 %1";
        levelStyles[LevelType::M_S2S3] = midSupportStyle;
        
        // Activer les étiquettes par défaut
        showLabels = true;
        
        // Type de période par défaut
        periodType = PeriodType::Daily;

        calculationMethod = CalculationMethod::HLC;
    }
};