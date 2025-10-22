#include "ui/views/reportView.h"
#include "ui/app.h"
#include <QDebug>

ReportView::ReportView(QWidget* parent)
    : BaseView(parent)
    , m_reportWidget(nullptr)
{
    setupUI();
}

ReportView::~ReportView()
{
    // Qt will automatically clean up child widgets
}

void ReportView::setupUI()
{
    // Create the ReportWidget
    m_reportWidget = new ReportWidget(this);
    
    // Add it directly to the main layout
    m_mainLayout->addWidget(m_reportWidget);
    
    qDebug() << "ReportView configured";
}

void ReportView::updateData(BacktestResults* results)
{
    m_currentResults = results;
    
    if (!m_currentResults) {
        qWarning() << "ReportView: Null results received";
        clear();
        return;
    }
    
    // Update the ReportWidget with stats
    if (m_reportWidget) {
        m_reportWidget->updateContent(m_currentResults->stats);
        
        // Get strategy configurations from the App
        QWidget* widget = parentWidget();
        App* app = nullptr;
        while (widget && !app) {
            app = qobject_cast<App*>(widget);
            widget = widget->parentWidget();
        }
        
        if (app) {
            // Get all strategy configs and pass them to ReportWidget
            std::vector<StrategyConfig> strategyConfigs = app->getStrategyConfigs();
            if (!strategyConfigs.empty()) {
                // Pass the first strategy config (you could modify this to handle multiple)
                m_reportWidget->setStrategyConfigurations(strategyConfigs[0]);
            }
        }
    }
    
    qDebug() << "ReportView updated";
}

void ReportView::clear()
{
    qDebug() << "ReportView::clear() called";
    
    if (m_reportWidget) {
        m_reportWidget->clear();
    }
    
    m_currentResults = nullptr;
}
