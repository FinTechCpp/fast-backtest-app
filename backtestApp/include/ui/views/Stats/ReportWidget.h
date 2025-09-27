#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QGroupBox>
#include <QComboBox>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <memory>
#include <string>
#include <vector>

#include "ui/views/Stats/StatsBaseWidget.h"
#include "ai/ModelInterface.h"
#include "stats.hpp"
#include "common.h"
#include "components/backtestResults.h"

/**
 * @brief AI-powered analysis widget for backtest results
 * 
 * This widget uses a local Llama model to analyze backtest statistics and strategy configurations,
 * generating insightful reports about strategy performance, strengths, weaknesses, and recommendations.
 */
class ReportWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit ReportWidget(QWidget* parent = nullptr);
    ~ReportWidget();

    void updateContent(const be::Stats& stats) override;
    void clear() override;

    /**
     * @brief Set the strategy configuration for analysis
     * @param baseConfig Base strategy configuration
     */
    void setStrategyConfigurations(const StrategyConfig& Config);

signals:
    void reportGenerated(const QString& report);
    void analysisStarted();
    void analysisFinished();
    void modelDownloadProgress(int percentage);
    void modelDownloadFinished();
    void errorOccurred(const QString& error);

private slots:
    void onGenerateReportClicked();
    void onLoadModelClicked();
    void onModelTypeChanged();
    void onInferenceCompleted(const QString& result);
    void onInferenceError(const QString& error);
    void onInferenceProgress(int progress);

private:
    /**
     * @brief Initialize the user interface
     */
    void setupUI();

    /**
     * @brief Create the optimized prompt for AI analysis
     * @param stats Backtest statistics
     * @param Config Base strategy configuration
     * @return Formatted prompt string
     */
    QString createAnalysisPrompt(const be::Stats& stats, 
                                const StrategyConfig& Config) const;

    /**
     * @brief Format statistics into a readable string for the AI model
     * @param stats Backtest statistics
     * @return Formatted statistics string
     */
    QString formatStatsForPrompt(const be::Stats& stats) const;

    /**
     * @brief Format strategy configuration into a readable string
     * @param Config Base configuration
     * @return Formatted configuration string
     */
    QString formatConfigForPrompt(const StrategyConfig& Config) const;

    /**
     * @brief Check if an AI model is loaded and ready
     * @return True if model exists and is ready
     */
    bool isModelReady() const;

    /**
     * @brief Load an AI model from file
     * @param modelPath Path to the model file
     * @param modelType Type of model to load (auto-detected if not specified)
     */
    bool loadAIModel(const QString& modelPath, ai::ModelFactory::ModelType modelType = ai::ModelFactory::ModelType::Auto);

    /**
     * @brief Get information about the currently loaded model
     */
    QString getCurrentModelInfo() const;

    /**
     * @brief Update the model status display
     */
    void updateModelStatus();

    /**
     * @brief Update the generate button state based on data availability
     */
    void updateGenerateButtonState();

    // UI Components
    QVBoxLayout* m_mainLayout;
    QGroupBox* m_reportGroup;
    QVBoxLayout* m_reportLayout;
    
    // Model selection controls
    QHBoxLayout* m_modelControlsLayout;
    QLabel* m_modelTypeLabel;
    QComboBox* m_modelTypeCombo;
    QPushButton* m_loadModelButton;
    QLabel* m_modelStatusLabel;
    
    // Analysis controls
    QHBoxLayout* m_analysisControlsLayout;
    QPushButton* m_generateButton;
    QProgressBar* m_progressBar;
    
    QScrollArea* m_scrollArea;
    QTextEdit* m_reportDisplay;
    
    // Data
    be::Stats m_currentStats;
    StrategyConfig m_currentBaseConfig;
    bool m_hasValidStats;
    bool m_hasValidConfig;
    
    // AI Model Management
    std::unique_ptr<ai::InferenceManager> m_inferenceManager;
    QString m_currentModelPath;
    ai::ModelFactory::ModelType m_currentModelType;
};