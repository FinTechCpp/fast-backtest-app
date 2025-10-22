#include "ui/dialogs/TradingHoursDialog.h"

TradingHoursDialog::TradingHoursDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Configuration des heures de trading");
    setMinimumWidth(400);
    setupUI();
}

void TradingHoursDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* tradingHoursGroup = new QGroupBox("Heures de trading", this);
    QFormLayout* tradingHoursLayout = new QFormLayout();
    
    m_tradingFromTime = new QTimeEdit(this);
    m_tradingFromTime->setTime(QTime(15, 30));
    m_tradingFromTime->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de début:", this), m_tradingFromTime);
    
    m_tradingToTime = new QTimeEdit(this);
    m_tradingToTime->setTime(QTime(22, 0));
    m_tradingToTime->setDisplayFormat("hh:mm");
    tradingHoursLayout->addRow(new QLabel("Heure de fin:", this), m_tradingToTime);
    
    // Jours de trading
    QWidget* tradingDaysWidget = new QWidget(this);
    QHBoxLayout* tradingDaysLayout = new QHBoxLayout(tradingDaysWidget);
    
    QStringList dayNames = {"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
    m_tradingDayCheckboxes.clear();
    
    for (int i = 0; i < 7; ++i) {
        QCheckBox* dayCheckbox = new QCheckBox(dayNames[i], this);
        dayCheckbox->setChecked(i < 5);
        tradingDaysLayout->addWidget(dayCheckbox);
        m_tradingDayCheckboxes.push_back(dayCheckbox);
    }
    
    tradingHoursLayout->addRow(new QLabel("Jours de trading:", this), tradingDaysWidget);
    
    tradingHoursGroup->setLayout(tradingHoursLayout);
    mainLayout->addWidget(tradingHoursGroup);
    
    // Boutons OK/Cancel
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void TradingHoursDialog::setConfig(const StrategyConfig& config) {
    m_tradingFromTime->setTime(QTime(config.trading_from.hour, config.trading_from.minute));
    m_tradingToTime->setTime(QTime(config.trading_to.hour, config.trading_to.minute));
    for (int i = 0; i < 7; ++i) {
        m_tradingDayCheckboxes[i]->setChecked(config.trading_days_array[i]);
    }
}

void TradingHoursDialog::updateConfig(StrategyConfig& config) const {
    QTime fromTime = m_tradingFromTime->time();
    config.trading_from = Time{fromTime.hour(), fromTime.minute(), 0};
    QTime toTime = m_tradingToTime->time();
    config.trading_to = Time{toTime.hour(), toTime.minute(), 0};
    for (int i = 0; i < 7; ++i) {
        config.trading_days_array[i] = m_tradingDayCheckboxes[i]->isChecked();
    }
}
