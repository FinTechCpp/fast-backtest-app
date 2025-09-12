#pragma once 

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

#define BE_SECONDS_IN_A_MINUTE 60.0
#define BE_MINUTES_IN_AN_HOUR 60.0
#define BE_HOURS_IN_A_DAY 24.0
#define BE_DAYS_IN_A_YEAR 365.0

#define BE_SECONDS_IN_AN_HOUR (BE_SECONDS_IN_A_MINUTE * BE_MINUTES_IN_AN_HOUR)
#define BE_SECONDS_IN_A_DAY (BE_SECONDS_IN_AN_HOUR * BE_HOURS_IN_A_DAY)
#define BE_SECONDS_IN_A_YEAR (BE_SECONDS_IN_A_DAY * BE_DAYS_IN_A_YEAR)

namespace be {
    /**
     * @brief Représentation d'une durée en secondes
     *
     * Stocke une durée en secondes et offre des méthodes pour convertir 
     * cette durée dans différentes unités (secondes, minutes, 
     * heures, jours, années).
     */
    struct Duration {
        Duration() : seconds(0) {}
        explicit Duration(double seconds) : seconds(seconds) {}
        
        // Constructeurs statiques pour différentes unités
        static Duration fromMinutes(double m) { return Duration(m * BE_SECONDS_IN_A_MINUTE); }
        static Duration fromHours(double h) { return Duration(h * BE_SECONDS_IN_AN_HOUR); }
        static Duration fromDays(double d) { return Duration(d * BE_SECONDS_IN_A_DAY); }
        static Duration fromYears(double y) { return Duration(y * BE_SECONDS_IN_A_YEAR); }

        // Getters pour différentes unités
        double toMinutes() const { return seconds / BE_SECONDS_IN_A_MINUTE; }
        double toHours() const { return seconds / BE_SECONDS_IN_AN_HOUR; }
        double toDays() const { return seconds / BE_SECONDS_IN_A_DAY; }
        double toYears() const { return seconds / BE_SECONDS_IN_A_YEAR; }

        /**
         * @brief Convertit la durée en chaîne de caractères
         * @return Représentation textuelle de la durée (ex: "1y 3d 5h 10m 30s")
         */
        std::string toString() const {
            std::stringstream ss;
            bool hasComponents = false;

            int years = static_cast<int>(toYears());
            int days = static_cast<int>(toDays()) % static_cast<int>(BE_DAYS_IN_A_YEAR);
            int hours = static_cast<int>(toHours()) % static_cast<int>(BE_HOURS_IN_A_DAY);
            int minutes = static_cast<int>(toMinutes()) % static_cast<int>(BE_MINUTES_IN_AN_HOUR);
            int secs = static_cast<int>(seconds) % static_cast<int>(BE_SECONDS_IN_A_MINUTE);

            if (years > 0) {
                ss << years << "y ";
                hasComponents = true;
            }
            
            if (days > 0 || hasComponents) {
                ss << days << "d ";
                hasComponents = true;
            }

            if (hours > 0 || hasComponents) {
                ss << hours << "h ";
                hasComponents = true;
            }

            if (minutes > 0 || hasComponents) {
                ss << minutes << "m ";
            }


            ss << std::fixed << std::setprecision(2) << secs << "s";

            return ss.str();
        }
        
        double seconds;  // Stockage interne en secondes

        // operateurs de comparaison
        bool operator==(const Duration& other) const { return seconds == other.seconds; }
        bool operator!=(const Duration& other) const { return !(*this == other); }
        bool operator<(const Duration& other) const { return seconds < other.seconds; }
        bool operator<=(const Duration& other) const { return seconds <= other.seconds; }
        bool operator>(const Duration& other) const { return seconds > other.seconds; }
        bool operator>=(const Duration& other) const { return seconds >= other.seconds; }
    };

    /**
     * @brief Représentation d'une date et heure
     *
     * Permet de stocker une date complète avec heure, minute et seconde.
     * Prend en charge les comparaisons et le calcul de durées entre dates.
     */
    struct Date {
        Date() : year(0), month(0), day(0), hour(0), minute(0), second(0) {}
        Date(double y, double m, double d, double h = 0, double min = 0, double s = 0)
            : year(y), month(m), day(d), hour(h), minute(min), second(s) {}
        
        // Opérateurs de comparaison
        bool operator==(const Date& other) const {
            return year == other.year && month == other.month && day == other.day &&
                hour == other.hour && minute == other.minute && second == other.second;
        }
        
        bool operator!=(const Date& other) const {
            return !(*this == other);
        }
        
        bool operator<(const Date& other) const {
            if (year != other.year) return year < other.year;
            if (month != other.month) return month < other.month;
            if (day != other.day) return day < other.day;
            if (hour != other.hour) return hour < other.hour;
            if (minute != other.minute) return minute < other.minute;
            return second < other.second;
        }
        
        bool operator<=(const Date& other) const {
            return *this < other || *this == other;
        }
        
        bool operator>(const Date& other) const {
            return other < *this;
        }
        
        bool operator>=(const Date& other) const {
            return other <= *this;
        }
        
        /**
         * @brief Calcule la différence entre deux dates
         * @param other Date à soustraire de la date actuelle
         * @return Objet Duration représentant le temps écoulé entre les deux dates
         */
        Duration operator-(const Date& other) const {
            // Implémentation des méthodes de Date qui dépendent de Duration
            double diffSeconds = toSecondsSince1970() - other.toSecondsSince1970();
            return Duration(diffSeconds);
        }

        /**
         * @brief Opérateur de flux pour afficher une date
         * @param os Flux de sortie
         * @param date Date à afficher
         * @return Référence au flux de sortie
         */
        friend std::ostream& operator<<(std::ostream& os, const Date& date) {
            os << date.toString();
            return os;
        }
        
        /**
         * @brief Convertit la date en chaîne de caractères
         * @return Représentation textuelle de la date (ex: "2023-05-15 14:30:00")
         */
        std::string toString() const {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0)
            << year << "-" 
            << std::setw(2) << std::setfill('0') << month << "-"
            << std::setw(2) << std::setfill('0') << day << " "
            << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second;
            return ss.str();
        }
        
        double toSecondsSince1970() const {
            return second +
                minute * BE_SECONDS_IN_A_MINUTE +
                hour * BE_SECONDS_IN_AN_HOUR +
                (day - 1) * BE_SECONDS_IN_A_DAY +  // Jour 1 = index 0
                (month - 1) * 30 * BE_SECONDS_IN_A_DAY +  // Mois 1 = index 0
                (year - 1970) * BE_SECONDS_IN_A_YEAR;
        }

        double year;
        double month;
        double day;
        double hour;
        double minute;
        double second;
    };


    /**
     * @brief Structure qui représente une bougie (OHLCV)
     */
    struct Candle {
        Date date;
        double open;
        double high;
        double low;
        double close;
        double volume;
    };

    /**
     * @brief enumération des raisons de fermeture d'un trade
     */
    enum class CloseReason {
        Unknown,      // Pas encore déterminé
        ManualClose,  // Fermeture manuelle (liquidation)
        StopLoss,     // Fermé par Stop Loss
        TakeProfit,   // Fermé par Take Profit
        BreakEven     // Fermé par Stop Loss en Break Even
    };

    /**
     * @brief Structure contenant les données d'un trade
     * 
     * Cette structure est utilisée pour stocker les informations essentielles
     * d'un trade, telles que la taille, les prix d'entrée et de sortie,
     * les dates, les raisons de fermeture, etc.
     */
    struct TradeData {
        int id = -1; // Identifiant unique du trade
        double size = 0.0;
        double entryPrice = 0.0;
        double exitPrice = 0.0;
        size_t entryBar = 0;
        size_t exitBar = 0;
        be::Date entryDate;
        be::Date exitDate;
        be::CloseReason closeReason = be::CloseReason::Unknown;
        std::string tag;
        double commissions = 0.0;
        bool isBreakEven = false;
        double tpPrice = 0.0; // Prix du Take Profit
        double initialSlPrice = 0.0; // Prix du Stop Loss initialement défini
        double lastSlPrice = 0.0; // Prix du dernier Stop Loss
        double breakEvenTriggerPrice = 0.0;
        double pl = 0.0;
        double plPercent = 0.0;


        // Methode utilitaires
        bool wasLong() const { return size > 0; }
        bool wasShort() const { return size < 0; }
        bool hasBeenClosed() const { return exitPrice > 0; }
    };

    /**
     * @brief Structure contenant toutes les statistiques de performance du backtest
     * 
     * Elle inclut les données brutes (courbe d'équité et trades)
     * pour analyses et visualisations supplémentaires.
     */
    struct Stats {
        // Données brutes pour analyse et visualisation
        std::vector<double> equityCurve;                ///< Courbe d'équité complète
        std::vector<TradeData> trades;     ///< Liste des trades fermés
        
        // Statistiques temporelles
        Date start = Date();                 ///< Indice de la première barre
        Date end = Date();                   ///< Indice de la dernière barre
        Duration duration = Duration();      ///< Durée du backtest
        double exposureTimePct = 0;          ///< Pourcentage du temps avec positions ouvertes

        // Statistiques d'équité
        double equityFinal = 0;              ///< Équité finale en unités monétaires
        double equityPeak = 0;               ///< Équité maximale atteinte
        double equityInitial = 0;            ///< Équité initiale au début du backtest
        
        // Statistiques de rendement
        double returnPct = 0;                ///< Rendement total en pourcentage
        double buyHoldReturnPct = 0;         ///< Rendement d'une stratégie buy & hold
        double buyHoldCagrPct = 0;           ///< CAGR d'une stratégie buy & hold
        double returnAnnPct = 0;             ///< Rendement annualisé en pourcentage
        double volatilityAnnPct = 0;         ///< Volatilité annualisée en pourcentage
        double cagrPct = 0;                  ///< Taux de croissance annuel composé
        
        // Ratios de risque
        double sharpeRatio = 0;              ///< Ratio de Sharpe (rendement ajusté au risque)
        double sortinoRatio = 0;             ///< Ratio de Sortino (risque négatif uniquement)
        double calmarRatio = 0;              ///< Ratio de Calmar (rendement / drawdown max)
        double alphaPct = 0;                 ///< Alpha en pourcentage (surperformance)
        double beta = 0;                     ///< Beta (corrélation avec le marché)
        
        // Statistiques de drawdown
        double maxDrawdownPct = 0;           ///< Drawdown maximal en pourcentage
        double avgDrawdownPct = 0;           ///< Drawdown moyen en pourcentage
        // TODO : a changer pour utiliser une structure Time
        Duration maxDrawdownDuration = Duration();   ///< Durée maximale d'un drawdown en barres
        Duration avgDrawdownDuration = Duration();   ///< Durée moyenne des drawdowns

        // Statistiques des trades
        unsigned int numTrades = 0;          ///< Nombre total de trades
        unsigned int numTPTrades = 0;         ///< Nombre de trades gagnants
        double pctTPTrades = 0;          ///< Pourcentage de trades gagnants
        unsigned int numSLTrades = 0;          ///< Nombre de trades perdants
        double pctSLTrades = 0;          ///< Pourcentage de trades perdants
        unsigned int numBETrades = 0;         ///< Nombre de trades neutres
        double pctBETrades = 0;          ///< Pourcentage de trades en break-even
        unsigned int numManualTrades = 0;          ///< Nombre de trades manuels
        double pctManualTrades = 0;     ///< Pourcentage de trades manuels
        unsigned int numUnknownTrades = 0;         ///< Nombre de trades avec raison de fermeture inconnue
        double pctUnknownTrades = 0;    ///< Pourcentage de trades avec raison de fermeture
        double bestTradePct = 0;             ///< Meilleur trade en pourcentage
        double worstTradePct = 0;            ///< Pire trade en pourcentage
        double avgTradePct = 0;              ///< Trade moyen en pourcentage
        Duration maxTradeDuration = Duration();      ///< Durée maximale d'un trade en barres
        Duration avgTradeDuration = Duration();      ///< Durée moyenne des trades
        double profitFactor = 0;             ///< Facteur de profit (gains/pertes)
        double expectancyPct = 0;            ///< Espérance mathématique par trade
        double sqn = 0;                      ///< System Quality Number
        double kellyCriterion = 0;           ///< Critère de Kelly

        double avgMAE = 0;                   ///< Moyenne des erreurs absolues (MAE)
        double maxMAE = 0;                   ///< Maximum des erreurs absolues (MAE)
        double ulcerIndex = 0;               ///< Ulcer Index (mesure de la douleur du drawdown)
        double ulcerPerformanceIndex = 0;    ///< Ratio de performance sur Ulcer Index
        double skewness = 0;                  ///< Skewness des rendements journaliers
        double kurtosis = 0;                  ///< Kurtosis des rendements journaliers
        double omegaRatio = 0;                ///< Omega Ratio (rendement positif/négatif)


        /**
         * @brief Convertit la structure en map pour compatibilité
         * @return Map associant les noms des statistiques à leurs valeurs
         */
        std::map<std::string, double> toMap() const;
    };
}