#ifndef FINANCECHART_H
#define FINANCECHART_H

#include <QMainWindow>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QButtonGroup>
#include <QLabel>
#include <QScrollBar>
#include <vector>
#include <string>
#include <fstream>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>
#include "./qtdemo/qchartviewer.h"

class FinanceChartWindow : public QMainWindow
{
    Q_OBJECT
public:
    FinanceChartWindow(QWidget *parent = nullptr);
    ~FinanceChartWindow();

private:
    // The main ticker key and the compare to ticker key
    QString m_tickerKey;
    QString m_compareKey;

    // Data file path
    QString m_dataFilePath;

    struct PriceData
    {
        std::vector<double> timeStamps;
        std::vector<double> highData;
        std::vector<double> lowData;
        std::vector<double> openData;
        std::vector<double> closeData;
        std::vector<double> volData;
        std::vector<double> compareData;
    };

    // In this example, we use raw data without any resampling
    PriceData m_rawPrice;

    // Add method to load data from CSV or convert Parquet to CSV first
    bool loadCSVData(const QString& filePath);
    bool convertParquetToCSV(const QString& parquetPath, const QString& csvPath);

    // The moving average periods
    int m_avgPeriod1;
    int m_avgPeriod2;

    // Routines to get data into the data arrays
    void loadData(const QString& ticker, const QString& compare);

    // UI controls
    QButtonGroup *mouseUsage;
    QLineEdit *m_TickerSymbol;
    QLineEdit *m_CompareWith;
    QCheckBox *m_VolumeBars;
    QCheckBox *m_ParabolicSAR;
    QCheckBox *m_LogScale;
    QCheckBox *m_PercentageScale;
    QComboBox *m_ChartType;
    QComboBox *m_PriceBand;
    QComboBox *m_AvgType1;
    QComboBox *m_AvgType2;
    QLineEdit *m_MovAvg1;
    QLineEdit *m_MovAvg2;
    QComboBox *m_Indicator1;
    QComboBox *m_Indicator2;
    QChartViewer *m_ChartViewer;

    // Chart drawing functions
    void drawChart(QChartViewer *viewer);
    void trackFinance(MultiChart* m, int mouseX);

    // Helper function to initialize combo boxes
    void initComboBox(QComboBox* b, const char* list[], int count, const char* initial);
    
    // Helper function to calculate Heikin Ashi values
    void calculateHeikinAshi(const DoubleArray &open, const DoubleArray &high, 
                            const DoubleArray &low, const DoubleArray &close,
                            std::vector<double> &ha_open, std::vector<double> &ha_high, 
                            std::vector<double> &ha_low, std::vector<double> &ha_close);
    
    // Utility to convert between std::vector and DoubleArray
    DoubleArray vectorToArray(const std::vector<double>& v, int startIndex = 0, int length = -1);
    std::vector<double> arrayToVector(DoubleArray a);

    // Dynamic resampling configuration
    struct ResamplingConfig {
        double originalIntervalSeconds;  // Intervalle original des données en secondes
        int maxPointsForOriginalData;    // Nombre max de points avant resampling
        int targetPointsWhenResampled;   // Nombre cible de points après resampling
        bool autoDetectOriginalInterval; // Auto-détection de l'intervalle original
    };
    
    ResamplingConfig m_resamplingConfig;
    
    // Cache pour les données resamplees à différents niveaux
    struct ResampledData {
        int aggregationFactor;           // Facteur d'agrégation (combien d'intervalles originaux)
        double intervalSeconds;          // Intervalle en secondes après agrégation
        PriceData data;                  // Données agrégées
        bool isValid;                    // Validité du cache
    };
    
    std::vector<ResampledData> m_resampledCache;
    
    // Helper functions for dynamic resampling
    void initializeResamplingConfig();
    double detectOriginalTimeInterval(const std::vector<double>& timestamps);
    bool shouldUseResampling(int startIndex, int endIndex, int totalPoints) const;
    int calculateOptimalAggregationFactor(int pointsToDisplay, double viewPortWidth) const;
    void generateResampledData(int aggregationFactor);
    void aggregateOHLCData(const PriceData& source, PriceData& target, int aggregationFactor);
    PriceData* selectOptimalDataset(int startIndex, int endIndex, int& outStartIndex, int& outEndIndex);
    void invalidateResamplingCache();

private slots:
    void onMouseUsageChanged(QAbstractButton *b);
    void onComboBoxChanged(int);
    void onCheckBoxChanged();
    void onLineEditChanged();
    void onMouseMovePlotArea(QMouseEvent*);
    void onViewPortChanged();
};

#endif // FINANCECHART_H