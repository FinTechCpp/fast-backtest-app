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
#include <QInputDialog>
#include <QSettings>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

ReportWidget::ReportWidget(QWidget* parent)
    : StatsBaseWidget(parent)
    , m_hasValidStats(false)
    , m_hasValidConfig(false)
    , m_currentModelType(ai::ModelFactory::ModelType::MistralAI)
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
    m_modelTypeCombo->addItem("Mistral AI (API)", static_cast<int>(ai::ModelFactory::ModelType::MistralAI));
    m_modelTypeCombo->setCurrentIndex(0); // Start with Mistral AI

    m_loadModelButton = new QPushButton("� Configure API Key");
    m_loadModelButton->setToolTip("Configure Mistral AI API key");
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
        "• Profil génétique de la stratégie\n"
        "• Signature comportementale\n"
        "• Forces cachées et vulnérabilités\n"
        "• Projection comportementale\n"
        "• Philosophie sous-jacente\n\n"
        "Configure your Mistral AI API key and click 'Generate AI Analysis' after running a backtest."
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

void ReportWidget::setStrategyConfigurations(const StrategyConfig& Config)
{
    m_currentBaseConfig = Config;
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
    QString prompt = createAnalysisPrompt(m_currentStats, m_currentBaseConfig);
    
    // Start inference
    m_inferenceManager->runInferenceAsync(prompt, 1024);
}

void ReportWidget::onLoadModelClicked()
{
    // Get current model type selection
    int selectedIndex = m_modelTypeCombo->currentIndex();
    auto modelType = static_cast<ai::ModelFactory::ModelType>(
        m_modelTypeCombo->itemData(selectedIndex).toInt());
    
    // For Mistral AI, we need to configure the API key
    bool ok;
    QString apiKey = QInputDialog::getText(this, "Mistral AI Configuration",
                                         "Enter your Mistral AI API key:", QLineEdit::Password,
                                         "", &ok);
    
    if (ok && !apiKey.isEmpty()) {
        if (loadAIModel(apiKey, modelType)) {
            QMessageBox::information(this, "API Configured", 
                "Successfully configured Mistral AI API connection.");
        } else {
            QMessageBox::warning(this, "Configuration Failed", 
                "Failed to configure Mistral AI API. Please check your API key.");
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
                                          const StrategyConfig& Config) const
{
    QString prompt = QString(
        "Tu es un analyste quantitatif expert spécialisé dans l'évaluation de stratégies de trading. "
        "Analyse les statistiques de backtest suivantes pour révéler les caractéristiques profondes de cette stratégie.\n\n"
        
        "# ANALYSE SYNTHÉTIQUE DE STRATÉGIE DE TRADING\n\n"
        
        "## Données de Performance:\n%1\n\n"
        
        "## Configuration Stratégique:\n%2\n\n"
        
        "## Mission d'Analyse:\n"
        "NE PAS répéter les statistiques - elles sont déjà visibles à l'utilisateur. "
        "Ton rôle est d'INTERPRÉTER ces chiffres pour révéler la PERSONNALITÉ de cette stratégie.\n\n"
        
        "### 🧬 PROFIL GÉNÉTIQUE DE LA STRATÉGIE\n"
        "À partir des ratios de performance, détermine quel TYPE de stratégie c'est :\n"
        "- Stratégie agressive vs conservatrice (indices : Sharpe, Sortino, Calmar)\n"
        "- Stratégie haute fréquence vs position (indices : nombre de trades, exposure time)\n"
        "- Stratégie momentum vs contrarian (indices : win rate, profit factor)\n"
        "- Stratégie risk-on vs défensive (indices : drawdown, volatilité)\n\n"
        
        "### 🔍 SIGNATURE COMPORTEMENTALE\n"
        "Que révèlent les patterns cachés :\n"
        "- Pourquoi cette combinaison win rate / profit factor ? Qu'est-ce que ça dit sur la logique ?\n"
        "- Le ratio SQN révèle-t-il une stratégie robuste ou chanceux ?\n"
        "- L'exposure time indique-t-il une stratégie selective ou opportuniste ?\n"
        "- Les drawdowns sont-ils cohérents avec le style ou cachent-ils un problème ?\n\n"
        
        "### ⚡ FORCES CACHÉES\n"
        "Qu'est-ce qui rend cette stratégie UNIQUE et EFFICACE :\n"
        "- Quel est son 'superpouvoir' principal révélé par les métriques ?\n"
        "- Pourquoi elle surperforme (ou sous-performe) vs buy & hold ?\n"
        "- Quelle compétence spécifique démontre-t-elle ?\n\n"
        
        "### 🎯 VULNÉRABILITÉS CRITIQUES\n"
        "Où réside sa FAIBLESSE fondamentale :\n"
        "- Quel ratio révèle sa talon d'Achille ?\n"
        "- Dans quelles conditions de marché elle s'effondrerait ?\n"
        "- Quel est le piège caché dans ses bonnes performances ?\n\n"
        
        "### 🔮 PROJECTION COMPORTEMENTALE\n"
        "Comment elle se comporterait en conditions réelles :\n"
        "- Sa robustesse psychologique (supporterait-elle la pression réelle ?)\n"
        "- Sa capacité d'adaptation aux changements de marché\n"
        "- Ses chances de survie à long terme\n\n"
        
        "### 🧠 PHILOSOPHIE SOUS-JACENTE\n"
        "Quelle vision du marché cette stratégie incarne-t-elle :\n"
        "- Croit-elle aux tendances ou aux retournements ?\n"
        "- Privilégie-t-elle la régularité ou les gros coups ?\n"
        "- Quelle est sa théorie implicite sur les inefficiences de marché ?\n\n"
        
        "IMPORTANT : Sois perspicace, utilise une approche de profiling psychologique de la stratégie. "
        "Révèle ce que les chiffres ne disent pas explicitement. Adopte un ton analytique mais accessible.\n"
    )
    .arg(formatStatsForPrompt(stats))
    .arg(formatConfigForPrompt(Config));
    
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

QString ReportWidget::formatConfigForPrompt(const StrategyConfig& Config) const
{
    QString formatted;
    QTextStream stream(&formatted);
    
    // Base configuration
    stream << "**Base Strategy Configuration:**\n";
    stream << QString("- Take Profit Distance: %1\n").arg(Config.take_profit_distance);
    stream << QString("- Stop Loss Distance: %1\n").arg(Config.stop_loss_distance);
    stream << QString("- Risk-based Sizing: %1\n").arg(Config.use_risk_based_sizing ? "Yes" : "No");
    stream << QString("- Risk Percentage: %1%\n").arg(Config.risk_percentage);
    stream << QString("- Use Break-even: %1\n").arg(Config.use_break_even ? "Yes" : "No");
    if (Config.use_break_even) {
        stream << QString("- Break-even Threshold: %1\n").arg(Config.break_even_threshold);
    }
    
    
    
    return formatted;
}

bool ReportWidget::isModelReady() const
{
    return m_inferenceManager && m_inferenceManager.get() != nullptr;
}

bool ReportWidget::loadAIModel(const QString& apiKey, ai::ModelFactory::ModelType modelType)
{
    try {
        auto model = ai::ModelFactory::createModel(modelType);
        
        // For Mistral AI, set the API key and test connection
        if (modelType == ai::ModelFactory::ModelType::MistralAI) {
            if (!model->setApiKey(apiKey)) {
                qWarning() << "Failed to set API key";
                return false;
            }
            
            if (!model->testApiConnection()) {
                qWarning() << "Failed to connect to Mistral AI API:" << model->getLastError();
                return false;
            }
        }
        
        m_inferenceManager->setModel(std::move(model));
        m_currentModelPath = "Mistral AI API";
        m_currentModelType = modelType;
        
        updateModelStatus();
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "Exception configuring model:" << e.what();
        return false;
    }
}

QString ReportWidget::getCurrentModelInfo() const
{
    if (!m_inferenceManager) {
        return "Aucun modèle configuré";
    }
    
    if (m_currentModelType == ai::ModelFactory::ModelType::MistralAI) {
        return QString("Modèle: Mistral AI\nAPI: %1\nAnalyse: Profil psychologique en français")
            .arg(m_currentModelPath);
    }
    
    return QString("Modèle: %1\nChemin: %2\nType: %3")
        .arg("IA Générique")
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