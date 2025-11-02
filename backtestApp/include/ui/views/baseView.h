#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QString>
#include <memory>
#include "backtest.hpp" // Include definitions for be::Data and be::Stats
#include "data.hpp"
#include "stats.hpp"
#include "components/backtestResults.h" 

/**
 * @brief Abstract base class for all backtest result views
 */
class BaseView : public QWidget
{
    Q_OBJECT  

public:
    /**
     * @brief Constructor
     * @param parent Pointer to the parent object
     */
    explicit BaseView(QWidget* parent = nullptr);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~BaseView() = default;
    
    /**
     * @brief Updates the view with new backtest data
     * @param results Pointer to the backtest results
     */
    virtual void updateData(BacktestResults* results) = 0;
    
    /**
     * @brief Resets the view to its initial state
     */
    virtual void clear() = 0;

protected:
    BacktestResults* m_currentResults;  ///< Pointer to the current backtest results
    
    /** Dictionary of the view's widgets */
    QMap<QString, QWidget*> m_widgets;
    
    /** Main layout of the view */
    QVBoxLayout* m_mainLayout;
    
    /**
     * @brief Utility to completely clear a layout
     * @param layout Layout to clear
     */
    void clearLayout(QLayout* layout);
    
    /**
     * @brief Virtual method to build the interface
     * To be implemented in derived classes
     */
    virtual void setupUI() = 0;
};
