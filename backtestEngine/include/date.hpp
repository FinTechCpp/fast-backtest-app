#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace be {

/**
 * @brief Représentation d'une durée en secondes
 *
 * Stocke une durée en secondes et offre des méthodes pour convertir 
 * et accéder à cette durée dans différentes unités (secondes, minutes, 
 * heures, jours, années).
 */
struct Duration {
    Duration() : seconds(0) {}
    explicit Duration(double seconds) : seconds(seconds) {}
    
    // Constructeurs statiques pour différentes unités
    static Duration fromMinutes(double m) { return Duration(m * 60); }
    static Duration fromHours(double h) { return Duration(h * 3600); }
    static Duration fromDays(double d) { return Duration(d * 86400); }
    static Duration fromYears(double y) { return Duration(y * 31536000); }
    
    // Getters pour différentes unités
    double toMinutes() const { return seconds / 60.0; }
    double toHours() const { return seconds / 3600.0; }
    double toDays() const { return seconds / 86400.0; }
    double toYears() const { return seconds / 31536000.0; }
    
    /**
     * @brief Convertit la durée en chaîne de caractères
     * @return Représentation textuelle de la durée (ex: "1y 3d 5h 10m 30s")
     */
    std::string toString() const {
        std::stringstream ss;
        bool hasComponents = false;

        int years = static_cast<int>(toYears());
        int days = static_cast<int>(toDays()) % 365;
        int hours = static_cast<int>(toHours()) % 24;
        int minutes = static_cast<int>(toMinutes()) % 60;
        int secs = static_cast<int>(seconds) % 60;

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
    
    // Getters
    // double getYear() const { return year; }
    // double getMonth() const { return month; }
    // double getDay() const { return day; }
    // double getHour() const { return hour; }
    // double getMinute() const { return minute; }
    // double getSecond() const { return second; }
    
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
        const double secondsInDay = 24 * 3600;
        const double secondsInMonth = 30 * secondsInDay;  // Approximation
        const double secondsInYear = 365.25 * secondsInDay;  // Approximation
        
        return second +
            minute * 60 +
            hour * 3600 +
            (day - 1) * secondsInDay +  // Jour 1 = index 0
            (month - 1) * secondsInMonth +  // Mois 1 = index 0
            (year - 1970) * secondsInYear;
    }

    double year;
    double month;
    double day;
    double hour;
    double minute;
    double second;
};

}  // namespace be
