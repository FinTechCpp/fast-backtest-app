#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace be {

class Duration;  // Forward declaration

// TODO : peut etre utiliser chrono pour les durées, et séparer dans un fichier date.cpp 

/**
 * @brief Représentation d'une date et heure
 *
 * Permet de stocker une date complète avec heure, minute et seconde.
 * Prend en charge les comparaisons et le calcul de durées entre dates.
 */
class Date {
public:
    // Constructeurs
    Date() : year(0), month(0), day(0), hour(0), minute(0), second(0) {}
    
    Date(double y, double m, double d, double h = 0, double min = 0, double s = 0)
        : year(y), month(m), day(d), hour(h), minute(min), second(s) {}
    
    // Getters
    double getYear() const { return year; }
    double getMonth() const { return month; }
    double getDay() const { return day; }
    double getHour() const { return hour; }
    double getMinute() const { return minute; }
    double getSecond() const { return second; }
    
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
    Duration operator-(const Date& other) const;
    
    /**
     * @brief Ajoute une durée à cette date
     * @param duration Durée à ajouter
     * @return Nouvelle date résultante
     */
    Date operator+(const Duration& duration) const;
    
    /**
     * @brief Soustrait une durée de cette date
     * @param duration Durée à soustraire
     * @return Nouvelle date résultante
     */
    Date operator-(const Duration& duration) const;

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
    
    // Convertit la date en timestamp (secondes depuis une époque arbitraire)
    double toTimestamp() const;
    
    // Crée une date à partir d'un timestamp
    static Date fromTimestamp(double timestamp);

private:
    double year;
    double month;
    double day;
    double hour;
    double minute;
    double second;
};

/**
 * @brief Représentation d'une durée en secondes
 *
 * Stocke une durée en secondes et offre des méthodes pour convertir 
 * et accéder à cette durée dans différentes unités (secondes, minutes, 
 * heures, jours, années).
 */
class Duration {
public:
    // Constructeur par défaut: durée nulle
    Duration() : seconds_(0) {}
    
    // Constructeur avec durée en secondes
    explicit Duration(double seconds) : seconds_(seconds) {}
    
    // Constructeurs statiques pour différentes unités
    static Duration fromSeconds(double s) { return Duration(s); }
    static Duration fromMinutes(double m) { return Duration(m * 60); }
    static Duration fromHours(double h) { return Duration(h * 3600); }
    static Duration fromDays(double d) { return Duration(d * 86400); }
    static Duration fromYears(double y) { return Duration(y * 31536000); }  // Approx. 365 jours
    
    // Getters pour différentes unités (tous retournent la durée totale dans l'unité spécifiée)
    double getTotalSeconds() const { return seconds_; }
    double getTotalMinutes() const { return seconds_ / 60.0; }
    double getTotalHours() const { return seconds_ / 3600.0; }
    double getTotalDays() const { return seconds_ / 86400.0; }
    double getTotalYears() const { return seconds_ / 31536000.0; }
    
    // Décomposition en unités (non cumulatives)
    int getYears() const { 
        return static_cast<int>(seconds_ / 31536000.0); 
    }
    
    int getDays() const { 
        double remainingSeconds = seconds_ - (getYears() * 31536000.0);
        return static_cast<int>(remainingSeconds / 86400.0);
    }
    
    int getHours() const {
        double remainingSeconds = seconds_ - (getYears() * 31536000.0) - (getDays() * 86400.0);
        return static_cast<int>(remainingSeconds / 3600.0);
    }
    
    int getMinutes() const {
        double remainingSeconds = seconds_ - (getYears() * 31536000.0) - 
                                  (getDays() * 86400.0) - (getHours() * 3600.0);
        return static_cast<int>(remainingSeconds / 60.0);
    }
    
    double getSeconds() const {
        return seconds_ - (getYears() * 31536000.0) - (getDays() * 86400.0) - 
               (getHours() * 3600.0) - (getMinutes() * 60.0);
    }
    
    // Opérateurs arithmétiques
    Duration operator+(const Duration& other) const { 
        return Duration(seconds_ + other.seconds_); 
    }
    
    Duration operator-(const Duration& other) const { 
        return Duration(seconds_ - other.seconds_); 
    }
    
    Duration& operator+=(const Duration& other) {
        seconds_ += other.seconds_;
        return *this;
    }
    
    Duration& operator-=(const Duration& other) {
        seconds_ -= other.seconds_;
        return *this;
    }
    
    Duration operator*(double factor) const {
        return Duration(seconds_ * factor);
    }
    
    Duration operator/(double divisor) const {
        if (divisor == 0) throw std::invalid_argument("Division by zero");
        return Duration(seconds_ / divisor);
    }
    
    // Opérateurs de comparaison
    bool operator==(const Duration& other) const { return seconds_ == other.seconds_; }
    bool operator!=(const Duration& other) const { return seconds_ != other.seconds_; }
    bool operator<(const Duration& other) const { return seconds_ < other.seconds_; }
    bool operator<=(const Duration& other) const { return seconds_ <= other.seconds_; }
    bool operator>(const Duration& other) const { return seconds_ > other.seconds_; }
    bool operator>=(const Duration& other) const { return seconds_ >= other.seconds_; }
    
    /**
     * @brief Convertit la durée en chaîne de caractères
     * @return Représentation textuelle de la durée (ex: "1y 3d 5h 10m 30s")
     */
    std::string toString() const {
        std::stringstream ss;
        bool hasComponents = false;
        
        if (getYears() > 0) {
            ss << getYears() << "y ";
            hasComponents = true;
        }
        
        if (getDays() > 0 || hasComponents) {
            ss << getDays() << "d ";
            hasComponents = true;
        }
        
        if (getHours() > 0 || hasComponents) {
            ss << getHours() << "h ";
            hasComponents = true;
        }
        
        if (getMinutes() > 0 || hasComponents) {
            ss << getMinutes() << "m ";
            hasComponents = true;
        }
        
        ss << std::fixed << std::setprecision(2) << getSeconds() << "s";
        
        return ss.str();
    }
    
private:
    double seconds_;  // Stockage interne en secondes
};

// Implémentation des méthodes de Date qui dépendent de Duration
inline Duration Date::operator-(const Date& other) const {
    double diffSeconds = toTimestamp() - other.toTimestamp();
    return Duration(diffSeconds);
}

inline Date Date::operator+(const Duration& duration) const {
    return fromTimestamp(toTimestamp() + duration.getTotalSeconds());
}

inline Date Date::operator-(const Duration& duration) const {
    return fromTimestamp(toTimestamp() - duration.getTotalSeconds());
}

// Convertit une date en timestamp (secondes depuis une époque arbitraire)
inline double Date::toTimestamp() const {
    const double secondsInDay = 24 * 3600;
    const double secondsInMonth = 30 * secondsInDay;  // Approximation
    const double secondsInYear = 365.25 * secondsInDay;  // Approximation
    
    return second +
           minute * 60 +
           hour * 3600 +
           (day - 1) * secondsInDay +  // Jour 1 = index 0
           (month - 1) * secondsInMonth +  // Mois 1 = index 0
           year * secondsInYear;
}

// Crée une date à partir d'un timestamp
inline Date Date::fromTimestamp(double timestamp) {
    const double secondsInDay = 24 * 3600;
    const double secondsInMonth = 30 * secondsInDay;  // Approximation
    const double secondsInYear = 365.25 * secondsInDay;  // Approximation
    
    double remaining = timestamp;
    
    double y = std::floor(remaining / secondsInYear);
    remaining -= y * secondsInYear;
    
    double m = std::floor(remaining / secondsInMonth) + 1;  // +1 car mois 1 = index 0
    remaining -= (m - 1) * secondsInMonth;
    
    double d = std::floor(remaining / secondsInDay) + 1;  // +1 car jour 1 = index 0
    remaining -= (d - 1) * secondsInDay;
    
    double h = std::floor(remaining / 3600);
    remaining -= h * 3600;
    
    double min = std::floor(remaining / 60);
    remaining -= min * 60;
    
    double s = remaining;
    
    return Date(y, m, d, h, min, s);
}

}  // namespace be

/*
    // Créer des dates
    Date start(2023, 1, 1, 8, 30, 0);  // 1er janvier 2023, 8h30
    Date end(2023, 1, 3, 16, 45, 30);  // 3 janvier 2023, 16h45:30

    // Calculer l'intervalle entre deux dates
    Duration interval = end - start;
    std::cout << "Intervalle: " << interval.toString() << std::endl;
    // Affiche: "Intervalle: 2d 8h 15m 30s"

    // Accéder aux composantes de la durée
    std::cout << "Jours: " << interval.getDays() << std::endl;
    std::cout << "Heures: " << interval.getHours() << std::endl;
    std::cout << "Minutes: " << interval.getMinutes() << std::endl;
    std::cout << "Secondes: " << interval.getSeconds() << std::endl;
    std::cout << "Total en heures: " << interval.getTotalHours() << std::endl;

    // Créer une durée directement
    Duration workday = Duration::fromHours(8);
    std::cout << "Journée de travail: " << workday.toString() << std::endl;
    // Affiche: "Journée de travail: 8h 0m 0s"

    // Ajouter une durée à une date
    Date meeting = start + Duration::fromDays(1);
    std::cout << "Réunion prévue le: " << meeting.toString() << std::endl;
    // Affiche: "Réunion prévue le: 2023-01-02 08:30:00"

    return 0;  // Correction de la faute de frappe dans "return"
*/