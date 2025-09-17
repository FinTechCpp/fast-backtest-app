#include "ui/views/Stats/ReportWidget.h"
#include "ui/app.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QDesktopServices>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

ReportWidget::ReportWidget(QWidget* parent)
    : StatsBaseWidget(parent)
    , m_hasValidStats(false)
    , m_hasValidConfig(false)
    , m_currentModelType(ai::ModelFactory::ModelType::ONNX)
{
    setupUI();
    
    // Initialize AI inference manager
    m_inferenceManager = std::make_unique<ai::InferenceManager>(this);
    
    // Connect AI inference signals
    connect(m_inferenceManager.get(), &ai::InferenceManager::inferenceCompleted,
            this, &ReportWidget::onInferenceCompleted);
    connect(m_inferenceManager.get(), &ai::InferenceManager::inferenceError,
            this, &ReportWidget::onInferenceError);
    connect(m_inferenceManager.get(), &ai::InferenceManager::inferenceProgress,
            this, &ReportWidget::onInferenceProgress);
    
    updateModelStatus();
}

ReportWidget::~ReportWidget()
{
    // InferenceManager will clean up automatically
}

void ReportWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    // Main report group
    m_reportGroup = new QGroupBox("🤖 AI Strategy Analysis Report");
    m_reportGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    border: 2px solid #3498db;"
        "    border-radius: 8px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 15px;"
        "    padding: 0 8px 0 8px;"
        "    background-color: white;"
        "    color: #3498db;"
        "}"
    );
    
    m_reportLayout = new QVBoxLayout(m_reportGroup);

    // Model selection controls
    m_modelControlsLayout = new QHBoxLayout();
    
    m_modelTypeLabel = new QLabel("AI Model Type:");
    m_modelTypeCombo = new QComboBox();
    m_modelTypeCombo->addItem("ONNX Runtime", static_cast<int>(ai::ModelFactory::ModelType::ONNX));
    m_modelTypeCombo->addItem("Llama.cpp (GGUF)", static_cast<int>(ai::ModelFactory::ModelType::LlamaCpp));
    m_modelTypeCombo->setCurrentIndex(0); // Start with ONNX

    m_loadModelButton = new QPushButton("🗂️ Load Model");
    m_loadModelButton->setToolTip("Load an AI model from file");
    m_loadModelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980b9;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #bdc3c7;"
        "}"
    );
    
    m_modelStatusLabel = new QLabel("");
    m_modelStatusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
    
    m_modelControlsLayout->addWidget(m_modelTypeLabel);
    m_modelControlsLayout->addWidget(m_modelTypeCombo);
    m_modelControlsLayout->addWidget(m_loadModelButton);
    m_modelControlsLayout->addStretch();
    m_modelControlsLayout->addWidget(m_modelStatusLabel);
    
    m_reportLayout->addLayout(m_modelControlsLayout);

    // Analysis controls
    m_analysisControlsLayout = new QHBoxLayout();
    
    m_generateButton = new QPushButton("🧠 Generate AI Analysis");
    m_generateButton->setToolTip("Generate comprehensive strategy analysis using AI");
    m_generateButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    padding: 12px 20px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #229954;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #bdc3c7;"
        "}"
    );
    
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximum(100);
    m_progressBar->setTextVisible(true);
    
    m_analysisControlsLayout->addWidget(m_generateButton);
    m_analysisControlsLayout->addStretch();
    m_analysisControlsLayout->addWidget(m_progressBar);
    
    m_reportLayout->addLayout(m_analysisControlsLayout);

    // Report display area
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setMinimumHeight(800);
    
    m_reportDisplay = new QTextEdit();
    m_reportDisplay->setReadOnly(true);
    m_reportDisplay->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #bdc3c7;"
        "    border-radius: 4px;"
        "    padding: 10px;"
        "    background-color: #f8f9fa;"
        "    font-family: 'Segoe UI', Arial, sans-serif;"
        "    font-size: 12px;"
        "    line-height: 1.5;"
        "}"
    );
    
    m_reportDisplay->setPlaceholderText(
        "AI-generated strategy analysis will appear here.\n\n"
        "The analysis will include:\n"
        "• Overall strategy performance assessment\n"
        "• Key strengths and weaknesses identification\n"
        "• Risk analysis and recommendations\n"
        "• Parameter optimization suggestions\n"
        "• Comparative insights and market conditions\n\n"
        "Choose your AI model type and click 'Generate AI Analysis' after running a backtest."
    );
    
    m_scrollArea->setWidget(m_reportDisplay);
    m_reportLayout->addWidget(m_scrollArea);

    m_mainLayout->addWidget(m_reportGroup);

    // Connect signals
    connect(m_generateButton, &QPushButton::clicked, this, &ReportWidget::onGenerateReportClicked);
    connect(m_loadModelButton, &QPushButton::clicked, this, &ReportWidget::onLoadModelClicked);
    connect(m_modelTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ReportWidget::onModelTypeChanged);
}

void ReportWidget::updateContent(const be::Stats& stats)
{
    m_currentStats = stats;
    m_hasValidStats = true;
    
    // Update button state
    updateGenerateButtonState();
}

void ReportWidget::clear()
{
    m_reportDisplay->clear();
    m_hasValidStats = false;
    m_hasValidConfig = false;
    updateGenerateButtonState();
}

void ReportWidget::setStrategyConfigurations(const StrategyBaseConfig& baseConfig, 
                                           const BuyHeikinGreenConfig& buyHeikinConfig)
{
    m_currentBaseConfig = baseConfig;
    m_currentBuyHeikinConfig = buyHeikinConfig;
    m_hasValidConfig = true;
    
    // Update button state
    updateGenerateButtonState();
}

void ReportWidget::onGenerateReportClicked()
{
    if (!m_hasValidStats || !m_hasValidConfig) {
        QMessageBox::warning(this, "Missing Data", 
                           "Please run a backtest and ensure strategy configuration is loaded before generating analysis.");
        return;
    }
    
    if (!isModelReady()) {
        QMessageBox::information(this, "Model Required", 
                                "No AI model is loaded. Please load a model first.");
        return;
    }
    
    if (m_inferenceManager->isRunning()) {
        QMessageBox::information(this, "Analysis in Progress", 
                                "An analysis is already running. Please wait for it to complete.");
        return;
    }
    
    // Disable button and show progress
    m_generateButton->setEnabled(false);
    m_generateButton->setText("🔄 Generating Analysis...");
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    // Create analysis prompt
    QString prompt = createAnalysisPrompt(m_currentStats, m_currentBaseConfig, m_currentBuyHeikinConfig);
    
    // Start inference
    m_inferenceManager->runInferenceAsync(prompt, 1024);
}

void ReportWidget::onLoadModelClicked()
{
    // Get current model type selection
    int selectedIndex = m_modelTypeCombo->currentIndex();
    auto modelType = static_cast<ai::ModelFactory::ModelType>(
        m_modelTypeCombo->itemData(selectedIndex).toInt());
    
    // Determine file filter based on model type
    QString filter;
    QString dialogTitle;
    
    switch (modelType) {
        case ai::ModelFactory::ModelType::ONNX:
            filter = "ONNX Models (*.onnx);;All Files (*)";
            dialogTitle = "Select ONNX Model File";
            break;
        case ai::ModelFactory::ModelType::LlamaCpp:
            filter = "GGUF Models (*.gguf);;Binary Models (*.bin);;All Files (*)";
            dialogTitle = "Select Llama.cpp Model File";
            break;
        default:
            filter = "All Model Files (*.onnx *.gguf *.bin);;All Files (*)";
            dialogTitle = "Select AI Model File";
            break;
    }
    
    QString modelPath = QFileDialog::getOpenFileName(
        this, dialogTitle, 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        filter);
    
    if (!modelPath.isEmpty()) {
        if (loadAIModel(modelPath, modelType)) {
            QMessageBox::information(this, "Model Loaded", 
                QString("Successfully loaded AI model:\n%1").arg(getCurrentModelInfo()));
        } else {
            QMessageBox::warning(this, "Load Failed", 
                "Failed to load the selected model. Check the model format and try again.");
        }
    }
}

void ReportWidget::onModelTypeChanged()
{
    int selectedIndex = m_modelTypeCombo->currentIndex();
    auto modelType = static_cast<ai::ModelFactory::ModelType>(
        m_modelTypeCombo->itemData(selectedIndex).toInt());
    
    m_currentModelType = modelType;
    
    updateModelStatus();
}

void ReportWidget::onInferenceCompleted(const QString& result)
{
    m_reportDisplay->setHtml(result);
    
    // Reset UI
    m_generateButton->setEnabled(true);
    m_generateButton->setText("🧠 Generate AI Analysis");
    m_progressBar->setVisible(false);
    
    // Update status
    m_modelStatusLabel->setText("Analysis completed successfully");
    m_modelStatusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
}

void ReportWidget::onInferenceError(const QString& error)
{
    m_reportDisplay->setPlainText("Error occurred during AI analysis:\n\n" + error);
    
    // Reset UI
    m_generateButton->setEnabled(true);
    m_generateButton->setText("🧠 Generate AI Analysis");
    m_progressBar->setVisible(false);
    
    // Update status
    m_modelStatusLabel->setText("Analysis failed");
    m_modelStatusLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
}

void ReportWidget::onInferenceProgress(int progress)
{
    m_progressBar->setValue(progress);
    m_progressBar->setFormat(QString("Analyzing... %1%").arg(progress));
}

QString ReportWidget::createAnalysisPrompt(const be::Stats& stats, 
                                          const StrategyBaseConfig& baseConfig,
                                          const BuyHeikinGreenConfig& buyHeikinConfig) const
{
    QString prompt = QString(
        "You are an expert quantitative analyst specializing in trading strategy evaluation. "
        "Analyze the following backtest results and strategy configuration to provide comprehensive insights.\n\n"
        
        "# TRADING STRATEGY BACKTEST ANALYSIS\n\n"
        
        "## Backtest Statistics:\n%1\n\n"
        
        "## Strategy Configuration:\n%2\n\n"
        
        "## Analysis Requirements:\n"
        "Please provide a detailed analysis covering:\n\n"
        
        "### 1. OVERALL PERFORMANCE ASSESSMENT\n"
        "- Overall strategy effectiveness and profitability\n"
        "- Risk-adjusted returns evaluation\n"
        "- Consistency of performance\n\n"
        
        "### 2. KEY STRENGTHS\n"
        "- What aspects of the strategy work well\n"
        "- Strong performance metrics\n"
        "- Robust risk management elements\n\n"
        
        "### 3. IDENTIFIED WEAKNESSES\n"
        "- Performance gaps and concerns\n"
        "- Risk management issues\n"
        "- Suboptimal parameters or settings\n\n"
        
        "### 4. RISK ANALYSIS\n"
        "- Drawdown patterns and recovery\n"
        "- Risk metrics evaluation (Sharpe, Sortino, etc.)\n"
        "- Position sizing and leverage assessment\n\n"
        
        "### 5. OPTIMIZATION RECOMMENDATIONS\n"
        "- Specific parameter adjustments\n"
        "- Risk management improvements\n"
        "- Strategy refinement suggestions\n\n"
        
        "### 6. MARKET CONDITIONS SUITABILITY\n"
        "- Market environments where strategy excels\n"
        "- Potential vulnerabilities in different conditions\n\n"
        
        "Format your response with clear headings and bullet points. "
        "Be specific with numbers and provide actionable insights. "
        "Keep the analysis professional but accessible.\n"
    )
    .arg(formatStatsForPrompt(stats))
    .arg(formatConfigForPrompt(baseConfig, buyHeikinConfig));
    
    return prompt;
}

QString ReportWidget::formatStatsForPrompt(const be::Stats& stats) const
{
    QString formatted;
    QTextStream stream(&formatted);
    
    // Performance metrics
    stream << "**Performance Metrics:**\n";
    stream << QString("- Total Return: %1%\n").arg(stats.returnPct, 0, 'f', 2);
    stream << QString("- Annualized Return: %1%\n").arg(stats.returnAnnPct, 0, 'f', 2);
    stream << QString("- CAGR: %1%\n").arg(stats.cagrPct, 0, 'f', 2);
    stream << QString("- Volatility (Ann.): %1%\n").arg(stats.volatilityAnnPct, 0, 'f', 2);
    stream << QString("- Buy & Hold Return: %1%\n").arg(stats.buyHoldReturnPct, 0, 'f', 2);
    
    // Risk metrics
    stream << "\n**Risk Metrics:**\n";
    stream << QString("- Sharpe Ratio: %1\n").arg(stats.sharpeRatio, 0, 'f', 3);
    stream << QString("- Sortino Ratio: %1\n").arg(stats.sortinoRatio, 0, 'f', 3);
    stream << QString("- Calmar Ratio: %1\n").arg(stats.calmarRatio, 0, 'f', 3);
    stream << QString("- Maximum Drawdown: %1%\n").arg(stats.maxDrawdownPct, 0, 'f', 2);
    stream << QString("- Average Drawdown: %1%\n").arg(stats.avgDrawdownPct, 0, 'f', 2);
    
    // Trade statistics
    stream << "\n**Trade Statistics:**\n";
    stream << QString("- Total Trades: %1\n").arg(stats.numTrades);
    stream << QString("- Winning Trades: %1 (%2%)\n").arg(stats.numTPTrades).arg(stats.pctTPTrades, 0, 'f', 1);
    stream << QString("- Losing Trades: %1 (%2%)\n").arg(stats.numSLTrades).arg(stats.pctSLTrades, 0, 'f', 1);
    stream << QString("- Break-even Trades: %1 (%2%)\n").arg(stats.numBETrades).arg(stats.pctBETrades, 0, 'f', 1);
    stream << QString("- Best Trade: %1%\n").arg(stats.bestTradePct, 0, 'f', 2);
    stream << QString("- Worst Trade: %1%\n").arg(stats.worstTradePct, 0, 'f', 2);
    stream << QString("- Average Trade: %1%\n").arg(stats.avgTradePct, 0, 'f', 2);
    stream << QString("- Profit Factor: %1\n").arg(stats.profitFactor, 0, 'f', 2);
    stream << QString("- Expectancy: %1%\n").arg(stats.expectancyPct, 0, 'f', 2);
    
    // Additional metrics
    stream << "\n**Additional Metrics:**\n";
    stream << QString("- Exposure Time: %1%\n").arg(stats.exposureTimePct, 0, 'f', 1);
    stream << QString("- System Quality Number: %1\n").arg(stats.sqn, 0, 'f', 2);
    stream << QString("- Kelly Criterion: %1\n").arg(stats.kellyCriterion, 0, 'f', 3);
    
    return formatted;
}

QString ReportWidget::formatConfigForPrompt(const StrategyBaseConfig& baseConfig,
                                          const BuyHeikinGreenConfig& buyHeikinConfig) const
{
    QString formatted;
    QTextStream stream(&formatted);
    
    // Base configuration
    stream << "**Base Strategy Configuration:**\n";
    stream << QString("- Take Profit Distance: %1\n").arg(baseConfig.take_profit_distance);
    stream << QString("- Stop Loss Distance: %1\n").arg(baseConfig.stop_loss_distance);
    stream << QString("- Risk-based Sizing: %1\n").arg(baseConfig.use_risk_based_sizing ? "Yes" : "No");
    stream << QString("- Risk Percentage: %1%\n").arg(baseConfig.risk_percentage);
    stream << QString("- Use Break-even: %1\n").arg(baseConfig.use_break_even ? "Yes" : "No");
    if (baseConfig.use_break_even) {
        stream << QString("- Break-even Threshold: %1\n").arg(baseConfig.break_even_threshold);
    }
    
    // Buy Heikin Green specific configuration
    stream << "\n**Buy Heikin Green Strategy Configuration:**\n";
    
    if (buyHeikinConfig.use_ema_short_filter) {
        stream << QString("- EMA Short Period: %1 (ACTIVE)\n").arg(buyHeikinConfig.ema_short_period);
    }
    if (buyHeikinConfig.use_ema_long_filter) {
        stream << QString("- EMA Long Period: %1 (ACTIVE)\n").arg(buyHeikinConfig.ema_long_period);
    }
    if (buyHeikinConfig.use_stoch_filter) {
        stream << QString("- Stochastic Filter (ACTIVE): FastK=%1, SlowK=%2, SlowD=%3, Threshold=%4\n")
                  .arg(buyHeikinConfig.stoch_fastk)
                  .arg(buyHeikinConfig.stoch_slowk)
                  .arg(buyHeikinConfig.stoch_slowd)
                  .arg(buyHeikinConfig.stoch_threshold);
    }
    if (buyHeikinConfig.use_rsi_filter) {
        stream << QString("- RSI Filter (ACTIVE): Period=%1, Threshold=%2\n")
                  .arg(buyHeikinConfig.rsi_period)
                  .arg(buyHeikinConfig.rsi_threshold);
    }
    if (buyHeikinConfig.use_supertrend_filter) {
        stream << QString("- SuperTrend Filter (ACTIVE): ATR Period=%1, Multiplier=%2\n")
                  .arg(buyHeikinConfig.supertrend_atr_period)
                  .arg(buyHeikinConfig.supertrend_multiplier);
    }
    if (buyHeikinConfig.use_atr_filter) {
        stream << QString("- ATR Filter (ACTIVE): Period=%1, Threshold=%2\n")
                  .arg(buyHeikinConfig.atr_filter_period)
                  .arg(buyHeikinConfig.atr_threshold);
    }
    if (buyHeikinConfig.use_previous_ha_candle_red_filter) {
        stream << QString("- Previous HA Candle Red Filter (ACTIVE): N=%1\n")
                  .arg(buyHeikinConfig.previous_ha_candle_red_filter_n);
    }
    
    return formatted;
}

bool ReportWidget::isModelReady() const
{
    return m_inferenceManager && m_inferenceManager.get() != nullptr;
}

bool ReportWidget::loadAIModel(const QString& modelPath, ai::ModelFactory::ModelType modelType)
{
    try {
        auto model = ai::ModelFactory::createModel(modelType);
        
        if (!model->loadModel(modelPath)) {
            qWarning() << "Failed to load model:" << model->getLastError();
            return false;
        }
        
        m_inferenceManager->setModel(std::move(model));
        m_currentModelPath = modelPath;
        m_currentModelType = modelType;
        
        updateModelStatus();
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "Exception loading model:" << e.what();
        return false;
    }
}

QString ReportWidget::getCurrentModelInfo() const
{
    if (!m_inferenceManager) {
        return "No model loaded";
    }
    
    return QString("Model: %1\nPath: %2\nType: %3")
        .arg("Generic AI Model")  // Would get from model interface in real implementation
        .arg(m_currentModelPath)
        .arg(static_cast<int>(m_currentModelType));
}

void ReportWidget::updateModelStatus()
{
    if (isModelReady()) {
        m_modelStatusLabel->setText("Model Ready");
        m_modelStatusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
        updateGenerateButtonState();
    } else {
        m_modelStatusLabel->setText("No Model Loaded");
        m_modelStatusLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
        m_generateButton->setEnabled(false);
    }
}

void ReportWidget::updateGenerateButtonState()
{
    bool canGenerate = m_hasValidStats && m_hasValidConfig && isModelReady();
    m_generateButton->setEnabled(canGenerate);
}