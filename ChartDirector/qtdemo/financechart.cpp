#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QObjectList>
#include <QDateTime>
#include <QPushButton>
#include <QFrame>
#include "financechart.h"
#include "FinanceChart.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cfloat>

// Contents of the combo boxes
static const char* chartTypes[] =
{
    "None", "None",
    "CandleStick", "CandleStick",
    "Close", "Closing Price",
    "HeikinAshi", "Heikin Ashi",
    "Median", "Median Price",
    "OHLC", "OHLC",
    "TP", "Typical Price",
    "WC", "Weighted Close"
};
static int chartTypeCount = (int)(sizeof(chartTypes) / sizeof(*chartTypes));

static const char* bandTypes[] =
{
    "None", "None",
    "BB", "Bollinger Band",
    "DC", "Donchain Channel",
    "Envelop", "Envelop (SMA 20 +/- 10%)"
};
static int bandTypeCount = (int)(sizeof(bandTypes) / sizeof(*bandTypes));

static const char* avgTypes[] =
{
    "None", "None",
    "SMA", "Simple",
    "EMA", "Exponential",
    "TMA", "Triangular",
    "WMA", "Weighted"
};
static int avgTypeCount = (int)(sizeof(avgTypes) / sizeof(*avgTypes));

static const char* indicatorTypes[] =
{
    "None", "None",
    "AccDist", "Accumulation/Distribution",
    "AroonOsc", "Aroon Oscillator",
    "Aroon", "Aroon Up/Down",
    "ADX", "Avg Directional Index",
    "ATR", "Avg True Range",
    "BBW", "Bollinger Band Width",
    "CMF", "Chaikin Money Flow",
    "COscillator", "Chaikin Oscillator",
    "CVolatility", "Chaikin Volatility",
    "CLV", "Close Location Value",
    "CCI", "Commodity Channel Index",
    "DPO", "Detrended Price Osc",
    "DCW", "Donchian Channel Width",
    "EMV", "Ease of Movement",
    "FStoch", "Fast Stochastic",
    "MACD", "MACD",
    "MDX", "Mass Index",
    "Momentum", "Momentum",
    "MFI", "Money Flow Index",
    "NVI", "Neg Volume Index",
    "OBV", "On Balance Volume",
    "Performance", "Performance",
    "PPO", "% Price Oscillator",
    "PVO", "% Volume Oscillator",
    "PVI", "Pos Volume Index",
    "PVT", "Price Volume Trend",
    "ROC", "Rate of Change",
    "RSI", "RSI",
    "SStoch", "Slow Stochastic",
    "StochRSI", "StochRSI",
    "TRIX", "TRIX",
    "UO", "Ultimate Oscillator",
    "Vol", "Volume",
    "WilliamR", "William's %R"
};
static int indicatorTypeCount = (int)(sizeof(indicatorTypes) / sizeof(*indicatorTypes));

// Helper utility to initialize a combo box from an array of text
void FinanceChartWindow::initComboBox(QComboBox* b, const char* list[], int count, const char* initial)
{
    b->clear();
    for (int i = 0; i < count; i += 2)
        b->addItem(list[i + 1], list[i]);
    
    // Set the initial selection
    for (int i = 0; i < count; i += 2)
    {
        if (0 == strcmp(list[i], initial))
        {
            b->setCurrentIndex(i / 2);
            break;
        }
    }
}



FinanceChartWindow::FinanceChartWindow(QWidget *parent) :
    QMainWindow(parent)
{
    // Set window title
    setWindowTitle("Finance Chart Demonstration");
    setMinimumSize(800, 600); // Explicitly set a minimum size

    // Set up the window properties
    setWindowTitle("NDX Raw OHLC Data Chart - Dynamic Resampling");
    setStyleSheet("QMainWindow {background:#FFFFFF;}");
    
    // Initialize resampling configuration
    initializeResamplingConfig();
    
    // Set Parquet file path
    m_dataFilePath = QDir::homePath() + "/ig-trading-bot/marketData/NDX_10secs_20220214_to_20250502_TRADES.csv";
    
    // Create a central widget and layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *horizontalLayout = new QHBoxLayout(centralWidget);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    // The frame on the left side
    QWidget *leftPanel = new QWidget(centralWidget);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setStyleSheet("#leftPanel {background-color:#BADDFF}");
    leftPanel->setFixedWidth(155);
    horizontalLayout->addWidget(leftPanel);

    QFrame *separator = new QFrame(centralWidget);
    separator->setFrameStyle(QFrame::VLine | QFrame::Plain);
    horizontalLayout->addWidget(separator);

    QWidget *rightPanel = new QWidget(centralWidget);
    horizontalLayout->addWidget(rightPanel);
    rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    int yCursor = 8;

    // Pointer push button
    QPushButton *pointerPB = new QPushButton(QIcon(":/icons/scroll_icon.png"), "", leftPanel);
    pointerPB->setGeometry(8, yCursor, 32, 32);
    pointerPB->setCheckable(true);

    // Zoom In push button
    QPushButton *zoomInPB = new QPushButton(QIcon(":/icons/zoomin_icon.png"), "", leftPanel);
    zoomInPB->setGeometry(61, yCursor, 32, 32);
    zoomInPB->setCheckable(true);

    // Zoom Out push button
    QPushButton *zoomOutPB = new QPushButton(QIcon(":/icons/zoomout_icon.png"), "", leftPanel);
    zoomOutPB->setGeometry(114, yCursor, 32, 32);
    zoomOutPB->setCheckable(true);

    // The Pointer/Zoom In/Zoom Out buttons form a button group
    mouseUsage = new QButtonGroup(leftPanel);
    mouseUsage->addButton(pointerPB, Chart::MouseUsageScroll);
    mouseUsage->addButton(zoomInPB, Chart::MouseUsageZoomIn);
    mouseUsage->addButton(zoomOutPB, Chart::MouseUsageZoomOut);
    connect(mouseUsage, SIGNAL(buttonPressed(QAbstractButton*)),
        SLOT(onMouseUsageChanged(QAbstractButton*)));

    // Ticker Symbol
    (new QLabel("Ticker Symbol", leftPanel))->setGeometry(8, yCursor += 40, 140, 18);
    m_TickerSymbol = new QLineEdit("SPY", leftPanel);
    m_TickerSymbol->setGeometry(8, yCursor += 16, 140, 20);

    // Compare With
    (new QLabel("Compare With", leftPanel))->setGeometry(8, yCursor += 28, 140, 18);
    m_CompareWith = new QLineEdit(leftPanel);
    m_CompareWith->setGeometry(8, yCursor += 16, 140, 20);

    // Value bars/Log Scale/Grid Lines
    m_VolumeBars = new QCheckBox("Show Volume Bars", leftPanel);
    m_ParabolicSAR = new QCheckBox("Parabolic SAR", leftPanel);
    m_LogScale = new QCheckBox("Log Scale", leftPanel);
    m_PercentageScale = new QCheckBox("Percentage Grid", leftPanel);
    m_VolumeBars->setChecked(true);
    m_VolumeBars->setGeometry(8, yCursor += 28, 140, 20);
    m_ParabolicSAR->setGeometry(8, yCursor += 20, 140, 20);
    m_LogScale->setGeometry(8, yCursor += 20, 140, 20);
    m_PercentageScale->setGeometry(8, yCursor += 20, 140, 20);

    // Chart Type
    (new QLabel("Chart Type", leftPanel))->setGeometry(8, yCursor += 28, 140, 18);
    m_ChartType = new QComboBox(leftPanel);
    m_ChartType->setGeometry(8, yCursor += 16, 140, 20);

    // Price Bands
    (new QLabel("Price Bands", leftPanel))->setGeometry(8, yCursor += 28, 140, 18);
    m_PriceBand = new QComboBox(leftPanel);
    m_PriceBand->setGeometry(8, yCursor += 16, 140, 20);

    // Moving Averages
    (new QLabel("Moving Averages", leftPanel))->setGeometry(8, yCursor += 28, 140, 18);
    m_AvgType1 = new QComboBox(leftPanel);
    m_AvgType2 = new QComboBox(leftPanel);
    m_MovAvg1 = new QLineEdit("10", leftPanel);
    m_MovAvg2 = new QLineEdit("25", leftPanel);
    m_AvgType1->setGeometry(8, yCursor += 16, 105, 20);
    m_MovAvg1->setGeometry(113, yCursor, 35, 20);
    m_AvgType2->setGeometry(8, yCursor += 22, 105, 20);
    m_MovAvg2->setGeometry(113, yCursor, 35, 20);

    // Technical Indicators
    (new QLabel("Technical Indicators", leftPanel))->setGeometry(8, yCursor += 28, 140, 18);
    m_Indicator1 = new QComboBox(leftPanel);
    m_Indicator2 = new QComboBox(leftPanel);
    m_Indicator1->setGeometry(8, yCursor += 16, 140, 20);
    m_Indicator2->setGeometry(8, yCursor += 22, 140, 20);

    // Chart Viewer
    m_ChartViewer = new QChartViewer(rightPanel);
    // Remove fixed move/geometry
    // m_ChartViewer->move(8, 12);
    // Set Expanding size policy
    m_ChartViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Add to layout
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(m_ChartViewer);
    connect(m_ChartViewer, SIGNAL(viewPortChanged()), SLOT(onViewPortChanged()));
    connect(m_ChartViewer, SIGNAL(mouseMovePlotArea(QMouseEvent*)),
        SLOT(onMouseMovePlotArea(QMouseEvent*)));

    // Fill the contents of the combo boxes
    initComboBox(m_ChartType, chartTypes, chartTypeCount, "CandleStick");
    initComboBox(m_PriceBand, bandTypes, bandTypeCount, "None");  // Bollinger Band
    initComboBox(m_AvgType1, avgTypes, avgTypeCount, "None");    // Simple Moving Average
    initComboBox(m_AvgType2, avgTypes, avgTypeCount, "None");    // Simple Moving Average
    initComboBox(m_Indicator1, indicatorTypes, indicatorTypeCount, "RSI");
    initComboBox(m_Indicator2, indicatorTypes, indicatorTypeCount, "MACD");

    // Connect all controls to appropriate signal handlers
    const QObjectList &allControls = leftPanel->children();
    for (int i = 0; i < allControls.count(); ++i)
    {
        QObject *obj;
        if ((obj = qobject_cast<QComboBox *>(allControls[i])) != 0)
            connect(obj, SIGNAL(currentIndexChanged(int)), SLOT(onComboBoxChanged(int)));
        else if ((obj = qobject_cast<QCheckBox *>(allControls[i])) != 0)
            connect(obj, SIGNAL(clicked()), SLOT(onCheckBoxChanged()));
        else if ((obj = qobject_cast<QLineEdit *>(allControls[i])) != 0)
            connect(obj, SIGNAL(editingFinished()), SLOT(onLineEditChanged()));
    }

    // Enable mouse wheel zooming by setting the zoom ratio to 1.1 per wheel event
    // Dans le constructeur, modifiez la configuration du ratio de zoom de la roue de souris
    m_ChartViewer->setMouseWheelZoomRatio(1.1);  // Ratio significatif pour un zoom visible
    
    // Et ajoutez ces configurations supplémentaires après la ligne ci-dessus
    m_ChartViewer->setScrollDirection(Chart::DirectionHorizontalVertical);  // Autorise le défilement dans les deux directions
    m_ChartViewer->setZoomDirection(Chart::DirectionHorizontal);  // Limite le zoom à l'axe horizontal
    // Initially set the mouse to drag to scroll mode
    pointerPB->setChecked(true);
    m_ChartViewer->setMouseUsage(Chart::MouseUsageScroll);

    onLineEditChanged();

    // Update the chart
    drawChart(m_ChartViewer);
}

FinanceChartWindow::~FinanceChartWindow()
{
    delete m_ChartViewer->getChart();
}

// The Pointer, Zoom In or Zoom out button is pressed
void FinanceChartWindow::onMouseUsageChanged(QAbstractButton *b)
{
    m_ChartViewer->setMouseUsage(mouseUsage->id(b));
}

// View port has changed - update the chart if necessary
void FinanceChartWindow::onViewPortChanged()
{
    drawChart(m_ChartViewer);
}

// User changes checkbox settings - update the chart
void FinanceChartWindow::onCheckBoxChanged()
{
    drawChart(m_ChartViewer);
}

// User changes combo box settings - update the chart
void FinanceChartWindow::onComboBoxChanged(int)
{
    drawChart(m_ChartViewer);
}

// User changes line edit settings - update the chart
void FinanceChartWindow::onLineEditChanged()
{
    // Check if the line edit is for moving average periods
    m_avgPeriod1 = m_MovAvg1->text().toInt();
    if (m_avgPeriod1 < 1)
    {
        m_MovAvg1->setText("1");
        m_avgPeriod1 = 1;
    }
    if (m_avgPeriod1 > 300)
    {
        m_MovAvg1->setText("300");
        m_avgPeriod1 = 300;
    }

    m_avgPeriod2 = m_MovAvg2->text().toInt();
    if (m_avgPeriod2 < 1)
    {
        m_MovAvg2->setText("1");
        m_avgPeriod2 = 1;
    }
    if (m_avgPeriod2 > 300)
    {
        m_MovAvg2->setText("300");
        m_avgPeriod2 = 300;
    }

    // Also check if the user has changed the ticker symbol
    if (m_tickerKey != m_TickerSymbol->text() || m_compareKey != m_CompareWith->text())
    {
        loadData(m_TickerSymbol->text(), m_CompareWith->text());
        m_ChartViewer->setViewPortWidth(1);  // Show the entire data range
        m_ChartViewer->setViewPortLeft(0);
    }

    // Now draw the chart
    drawChart(m_ChartViewer);
}

// Load data from file only - no sample data generation
void FinanceChartWindow::loadData(const QString& ticker, const QString& compare)
{
    if (m_tickerKey != ticker)
    {
        m_tickerKey = ticker;
        
        // Check if there's a CSV file we can use
        QString csvFilePath = QDir::homePath() + "/ig-trading-bot/marketData/NDX_10secs_20220214_to_20250502_TRADES.csv";

        bool dataLoaded = false;
        
        // Try to load data from CSV file
        if (QFile::exists(csvFilePath)) {
            dataLoaded = loadCSVData(csvFilePath);
        }
        
        // If CSV loading failed or file doesn't exist, log error and return
        if (!dataLoaded) {
            qDebug() << "ERROR: Could not load data from file" << csvFilePath;
            // Clear all data to ensure nothing is displayed
            m_rawPrice.timeStamps.clear();
            m_rawPrice.openData.clear();
            m_rawPrice.highData.clear();
            m_rawPrice.lowData.clear();
            m_rawPrice.closeData.clear();
            m_rawPrice.volData.clear();
            return; // Exit early, no data to show
        }
        
        // Set up the viewport to show the entire data range
        m_ChartViewer->setFullRange("x", 0, (int)m_rawPrice.timeStamps.size() - 1);
        
        // Configure zoom/scroll properties
        m_ChartViewer->setZoomInWidthLimit(10.0 / m_rawPrice.timeStamps.size());  // Limite de zoom minimal
        // Ne pas écraser la valeur du ratio de zoom déjà configurée dans le constructeur
        
        // Start by showing the last 100 data points
        int totalPoints = (int)m_rawPrice.timeStamps.size();
        if (totalPoints > 100) {
            double visiblePortion = 100.0 / totalPoints;
            m_ChartViewer->setViewPortWidth(visiblePortion);
            m_ChartViewer->setViewPortLeft(1.0 - visiblePortion);
        } else {
            m_ChartViewer->setViewPortWidth(1.0);
            m_ChartViewer->setViewPortLeft(0);
        }
    }
    
    if (m_compareKey != compare)
    {
        m_compareKey = compare;
        m_rawPrice.compareData.clear();
        
        // Comparison functionality would be implemented here
        if (!m_compareKey.isEmpty()) {
            qDebug() << "Comparison functionality not implemented";
        }
    }
}



// Helper function to add a moving average to the chart
static LineLayer* addMovingAvg(FinanceChart *m, QString avgType, int avgPeriod, int color)
{
    if (avgType == "SMA")
        return m->addSimpleMovingAvg(avgPeriod, color);
    else if (avgType == "EMA")
        return m->addExpMovingAvg(avgPeriod, color);
    else if (avgType == "TMA")
        return m->addTriMovingAvg(avgPeriod, color);
    else if (avgType == "WMA")
        return m->addWeightedMovingAvg(avgPeriod, color);
    return 0;
}

// Helper function to add a technical indicator to the chart
// Helper function to add a technical indicator to the chart
static XYChart* addIndicator(FinanceChart *m, QString indicator, int height)
{
    if (indicator == "RSI")
        return m->addRSI(height, 14, 0x800080, 20, 0xff6666, 0x6666ff);
    else if (indicator == "MACD")
        return m->addMACD(height, 26, 12, 9, 0x0000ff, 0xff00ff, 0x008000);
    else if (indicator == "StochRSI")
        // Fix: added missing downColor parameter (requires 6 args, not 5)
        return m->addStochRSI(height, 14, 0x008000, 30.0, 0x666666, 0x999999);
    else if (indicator == "ATR")
        return m->addATR(height, 14, 0x008000, 0xff);
    else if (indicator == "ADX")
        return m->addADX(height, 14, 0x008000, 0x800000, 0x0000ff);
    else if (indicator == "DCW")
        return m->addDonchianWidth(height, 20, 0x0000ff);
    else if (indicator == "BBW")
        return m->addBollingerWidth(height, 20, 2, 0x0000ff);
    else if (indicator == "DPO")
        return m->addDPO(height, 20, 0x0000ff);
    else if (indicator == "PVT")
        return m->addPVT(height, 0x0000ff);
    else if (indicator == "Momentum")
        return m->addMomentum(height, 12, 0x0000ff);
    else if (indicator == "Performance")
        return m->addPerformance(height, 0x0000ff);
    else if (indicator == "ROC")
        return m->addROC(height, 12, 0x0000ff);
    else if (indicator == "OBV")
        return m->addOBV(height, 0x0000ff);
    else if (indicator == "AccDist")
        return m->addAccDist(height, 0x0000ff);
    else if (indicator == "CLV")
        return m->addCLV(height, 0x0000ff);
    else if (indicator == "WilliamR")
        return m->addWilliamR(height, 14, 0x800080, 30, 0xff6666, 0x6666ff);
    else if (indicator == "Aroon")
        return m->addAroon(height, 14, 0x339933, 0x333399);
    else if (indicator == "AroonOsc")
        return m->addAroonOsc(height, 14, 0x0000ff);
    else if (indicator == "CCI")
        return m->addCCI(height, 20, 0x800080, 100, 0xff6666, 0x6666ff);
    else if (indicator == "EMV")
        // Fix: added missing color2 parameter (requires 4 args, not 3)
        return m->addEaseOfMovement(height, 9, 0x006060, 0x999999);
    else if (indicator == "MDX")
        return m->addMassIndex(height, 0x800080, 0xff6666, 0x6666ff);
    // ... rest of function remains unchanged
    else if (indicator == "CVolatility")
        return m->addChaikinVolatility(height, 10, 10, 0x0000ff);
    else if (indicator == "COscillator")
        return m->addChaikinOscillator(height, 0x0000ff);
    else if (indicator == "CMF")
        return m->addChaikinMoneyFlow(height, 21, 0x008000);
    else if (indicator == "NVI")
        return m->addNVI(height, 255, 0x0000ff, 0x883333);
    else if (indicator == "PVI")
        return m->addPVI(height, 255, 0x0000ff, 0x883333);
    else if (indicator == "MFI")
        return m->addMFI(height, 14, 0x800080, 30, 0xff6666, 0x6666ff);
    else if (indicator == "PVO")
        return m->addPVO(height, 12, 26, 9, 0x0000ff, 0xff00ff, 0x008000);
    else if (indicator == "PPO")
        return m->addPPO(height, 12, 26, 9, 0x0000ff, 0xff00ff, 0x008000);
    else if (indicator == "UO")
        return m->addUltimateOscillator(height, 7, 14, 28, 0x800080, 20, 0xff6666, 0x6666ff);
    else if (indicator == "Vol")
        return m->addVolIndicator(height, 0x99ff99, 0xff9999, 0x808080);
    else if (indicator == "TRIX")
        return m->addTRIX(height, 12, 0x0000ff);
    else if (indicator == "FStoch")
        return m->addFastStochastic(height, 14, 3, 0x006060, 0x606000);
    else if (indicator == "SStoch")
        return m->addSlowStochastic(height, 14, 3, 0x006060, 0x606000);
    
    return 0;  // None
}

// Draw the chart according to user selections
void FinanceChartWindow::drawChart(QChartViewer *viewer)
{
    // Get initial viewport bounds using raw data indices
    int rawStartIndex = (int)floor(viewer->getValueAtViewPort("x", viewer->getViewPortLeft()));
    int rawEndIndex = (int)ceil(viewer->getValueAtViewPort("x", viewer->getViewPortLeft() + 
        viewer->getViewPortWidth())) - 1;
    
    // Ensure the raw indices are within bounds
    int totalRawPoints = (int)m_rawPrice.timeStamps.size();
    if (rawStartIndex < 0) rawStartIndex = 0;
    if (rawStartIndex > totalRawPoints - 1) rawStartIndex = totalRawPoints - 1;
    if (rawEndIndex < 0) rawEndIndex = 0;
    if (rawEndIndex > totalRawPoints - 1) rawEndIndex = totalRawPoints - 1;
    if (rawEndIndex < rawStartIndex) rawEndIndex = rawStartIndex;

    // Select optimal dataset based on viewport and zoom level
    int actualStartIndex, actualEndIndex;
    PriceData* p = selectOptimalDataset(rawStartIndex, rawEndIndex, actualStartIndex, actualEndIndex);
    
    // Ensure the actual indices are within bounds of the selected dataset
    int noOfPoints = (int)p->timeStamps.size();
    if (actualStartIndex < 0) actualStartIndex = 0;
    if (actualStartIndex > noOfPoints - 1) actualStartIndex = noOfPoints - 1;
    if (actualEndIndex < 0) actualEndIndex = 0;
    if (actualEndIndex > noOfPoints - 1) actualEndIndex = noOfPoints - 1;

    // Ensure minimum number of visible points
    if (actualEndIndex - actualStartIndex < 3 && noOfPoints > 3) {
        int midPoint = (actualStartIndex + actualEndIndex) / 2;
        actualStartIndex = std::max(0, midPoint - 2);
        actualEndIndex = std::min(noOfPoints - 1, midPoint + 2);
    }
    
    if (actualEndIndex < actualStartIndex)
        actualEndIndex = actualStartIndex;
    
    // Extract the data to be viewed using the optimal dataset
    int noOfPointsToDisplay = actualEndIndex - actualStartIndex + 1;
    DoubleArray timeStamps = vectorToArray(p->timeStamps, actualStartIndex, noOfPointsToDisplay);
    DoubleArray highData = vectorToArray(p->highData, actualStartIndex, noOfPointsToDisplay);
    DoubleArray lowData = vectorToArray(p->lowData, actualStartIndex, noOfPointsToDisplay);
    DoubleArray openData = vectorToArray(p->openData, actualStartIndex, noOfPointsToDisplay);
    DoubleArray closeData = vectorToArray(p->closeData, actualStartIndex, noOfPointsToDisplay);
    DoubleArray volData = vectorToArray(p->volData, actualStartIndex, noOfPointsToDisplay);
    DoubleArray compareData;
    if (p->compareData.size() > 0)
        compareData = vectorToArray(p->compareData, actualStartIndex, noOfPointsToDisplay);
    
    // Create a FinanceChart object of width n pixels
    FinanceChart *c = new FinanceChart(2000);
    
    // Set the chart title with resampling information
    std::string title = m_tickerKey.toStdString();
    if (p != &m_rawPrice) {
        // Add resampling indicator to title
        int aggregationFactor = 1;
        double originalInterval = m_resamplingConfig.originalIntervalSeconds;
        double resampledInterval = originalInterval;
        for (const auto& cache : m_resampledCache) {
            if (cache.isValid && &cache.data == p) {
                aggregationFactor = cache.aggregationFactor;
                resampledInterval = cache.intervalSeconds;
                break;
            }
        }
        // Format interval display
        std::string intervalStr;
        if (resampledInterval >= 3600) {
            intervalStr = std::to_string((int)(resampledInterval / 3600)) + "h";
        } else if (resampledInterval >= 60) {
            intervalStr = std::to_string((int)(resampledInterval / 60)) + "m";
        } else {
            intervalStr = std::to_string((int)resampledInterval) + "s";
        }
        title += " [" + intervalStr + " aggregated - " + std::to_string(noOfPointsToDisplay) + " points]";
    } else {
        title += " [Original data - " + std::to_string(noOfPointsToDisplay) + " points]";
    }
    c->addTitle(title.c_str());
    
    // Set the data into the finance chart object
    c->setData(timeStamps, highData, lowData, openData, closeData, volData, 0);
    
    // Configure chart appearance for 10-second data
    // Set specific date/time formats for 10-second data
    c->setDateLabelFormat("yyyy", "<*font=bold*>{value|yyyy MMM d}", "{value|MMM d}",
                         "<*font=bold*>{value|d h:nn}", "{value|h:nn}",
                         "<*font=bold*>{value|h:nn:ss}", "{value|nn:ss}");
                         
    // Set the tool tip format to show seconds
    c->setToolTipDateFormat("[{xLabel|yyyy MMM d}]", "[{xLabel|yyyy MMM d}]", 
                           "[{xLabel|yyyy MMM d h:nn:ss}]");
    
    // Configure chart appearance
    if (m_LogScale->isChecked())
        c->setLogScale(true);
    if (m_PercentageScale->isChecked())
        c->setPercentageAxis();
    
    // Add the main chart
    c->addMainChart(650);
    
    // Add price line/band to the main chart depending on selected chart type
    QString selectedType = m_ChartType->itemData(m_ChartType->currentIndex()).toString();
    if (selectedType == "None")
        c->addCloseLine(0x000040);  // Default to closeline if none selected
    else if (selectedType == "CandleStick")
        c->addCandleStick(0x00ff00, 0xff0000);
    else if (selectedType == "OHLC")
        c->addHLOC(0x00ff00, 0xff0000);
    else if (selectedType == "HeikinAshi") {
        // Calculate Heikin Ashi values
        std::vector<double> ha_open, ha_high, ha_low, ha_close;
        calculateHeikinAshi(openData, highData, lowData, closeData, ha_open, ha_high, ha_low, ha_close);
        
        // Convert vectors to DoubleArray for ChartDirector
        DoubleArray haOpenArray = vectorToArray(ha_open);
        DoubleArray haHighArray = vectorToArray(ha_high);
        DoubleArray haLowArray = vectorToArray(ha_low);
        DoubleArray haCloseArray = vectorToArray(ha_close);
        
        // Save original OHLC data as copies
        DoubleArray originalOpen = openData;
        DoubleArray originalHigh = highData;
        DoubleArray originalLow = lowData;
        DoubleArray originalClose = closeData;
        
        // Set Heikin Ashi data for drawing
        c->setData(timeStamps, haHighArray, haLowArray, haOpenArray, haCloseArray, volData, 0);
        
        // Add candlestick chart with Heikin Ashi data
        c->addCandleStick(0x00dd00, 0xee0000);
        
        // Restore original OHLC data for other indicators
        c->setData(timeStamps, originalHigh, originalLow, originalOpen, originalClose, volData, 0);
    }
    else if (selectedType == "Close")
        c->addCloseLine(0x000040);
    else if (selectedType == "TP")
        c->addTypicalPrice(0x000040);
    else if (selectedType == "WC")
        c->addWeightedClose(0x000040);
    else if (selectedType == "Median")
        c->addMedianPrice(0x000040);
    
    // Add comparison line if there is data for comparison
    if (compareData.len > 0)
        c->addComparison(compareData, 0x0000ff, m_compareKey.toUtf8().data());
    
    // Add moving average lines
    QString avgType1 = m_AvgType1->itemData(m_AvgType1->currentIndex()).toString();
    if (avgType1 != "None")
        addMovingAvg(c, avgType1, m_avgPeriod1, 0x663300);  // Brown color
    
    QString avgType2 = m_AvgType2->itemData(m_AvgType2->currentIndex()).toString();
    if (avgType2 != "None")
        addMovingAvg(c, avgType2, m_avgPeriod2, 0x9900ff);  // Purple color
    
    // Add price band
    QString priceBand = m_PriceBand->itemData(m_PriceBand->currentIndex()).toString();
    if (priceBand == "BB")
        c->addBollingerBand(20, 2, 0x9999ff, 0xc06666ff);  // Blue band
    else if (priceBand == "DC")
        c->addDonchianChannel(20, 0x9999ff, 0xc06666ff);   // Blue band
    else if (priceBand == "Envelop")
        c->addEnvelop(20, 0.1, 0x9999ff, 0xc06666ff);      // Blue band
    
    // Add volume bars
    if (m_VolumeBars->isChecked())
        c->addVolBars(70, 0x99ff99, 0xff9999, 0x808080);   // Green/red/grey volume bars
    
    // Add parabolic SAR if selected
    if (m_ParabolicSAR->isChecked())
        c->addParabolicSAR(0.02, 0.02, 0.2, Chart::DiamondShape, 5, 0x008800, 0x000000);
    
    // Add technical indicators
    QString indicator1 = m_Indicator1->itemData(m_Indicator1->currentIndex()).toString();
    if (indicator1 != "None")
        addIndicator(c, indicator1, 120);  // 120 pixels height for each indicator
    
    QString indicator2 = m_Indicator2->itemData(m_Indicator2->currentIndex()).toString();
    if (indicator2 != "None")
        addIndicator(c, indicator2, 120);  // 120 pixels height for each indicator
    
    // Set the chart to the viewer
    delete viewer->getChart();
    viewer->setChart(c);
    
    // Apply track line with legend if mouse is in the plot area
    if (viewer->isMouseOnPlotArea())
        trackFinance(c, viewer->getPlotAreaMouseX());
    
    // Adjust the view port to show the latest data if necessary
    if (viewer->getViewPortLeft() + viewer->getViewPortWidth() < 1)
        viewer->updateViewPort(true, false);
}

// Draw finance chart track line with legend
void FinanceChartWindow::trackFinance(MultiChart* m, int mouseX)
{
    // Clear the current dynamic layer and get the DrawArea object to draw on it
    DrawArea *d = m->initDynamicLayer();
    
    // It is possible for a FinanceChart to be empty, so we need to check for it
    if (m->getChartCount() == 0)
        return;
    
    // Get the data x-value that is nearest to the mouse
    int xValue = (int)(((XYChart *)m->getChart(0))->getNearestXValue(mouseX));
    
    // Iterate through all charts in the MultiChart
    XYChart *c = 0;
    for (int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart *)m->getChart(i);
        
        // Variables to hold the legend entries
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;
        
        // Iterate through all layers to find the data points
        for (int j = 0; j < c->getLayerCount(); ++j) {
            Layer *layer = c->getLayerByZ(j);
            
            // Skip empty layers
            if (!layer->getDataSetCount())
                continue;
            
            // Get the data set
            DataSet *dataSet = layer->getDataSetByZ(0);
            
            // Check if it's an OHLC layer
            int ohlcLayer = 0;
            const char *name = dataSet->getDataName();
            if (name && ((std::string)name == "Open" || (std::string)name == "High" ||
                (std::string)name == "Low" || (std::string)name == "Close")) {
                ohlcLayer = 1;
            }
            
            // Get the array index corresponding to the x-value
            int xIndex = layer->getXIndexOf(xValue);
            // Fix: Layer doesn't have getXData(), check dataset length instead
            if (xIndex < 0)
                continue;
            
            // Special handling for OHLC layers
            if (ohlcLayer) {
                // Fix: getDataSet can't take string names, must use indices
                // Assuming standard OHLC order: Open(0), High(1), Low(2), Close(3)
                double openValue = layer->getDataSetByZ(0)->getValue(xIndex);
                double highValue = layer->getDataSetByZ(1)->getValue(xIndex);
                double lowValue = layer->getDataSetByZ(2)->getValue(xIndex);
                double closeValue = layer->getDataSetByZ(3)->getValue(xIndex);
                
                if (openValue != Chart::NoValue) {
                    // Build the OHLC legend - Open, High, Low, Close values
                    ohlcLegend << "      <*block*>";
                    ohlcLegend << "Open: " << c->formatValue(openValue, "{value|P4}");
                    ohlcLegend << ", High: " << c->formatValue(highValue, "{value|P4}");
                    ohlcLegend << ", Low: " << c->formatValue(lowValue, "{value|P4}");
                    ohlcLegend << ", Close: " << c->formatValue(closeValue, "{value|P4}");
                    
                    // Add percentage change for the close value
                    // Fix: Use getDataSetByZ for Close data (index 3)
                    double lastCloseValue = layer->getDataSetByZ(3)->getValue(xIndex - 1);
                    if (lastCloseValue != Chart::NoValue) {
                        double change = closeValue - lastCloseValue;
                        double percent = change * 100 / closeValue;
                        
                        // Use appropriate colors and icons for up/down changes
                        std::string symbol = (change >= 0) ?
                            "<*font,color=008800*><*img=@triangle,width=8,color=008800*>" :
                            "<*font,color=CC0000*><*img=@invertedtriangle,width=8,color=CC0000*>";
                        
                        ohlcLegend << "  " << symbol << " " << c->formatValue(change, "{value|P4}");
                        ohlcLegend << " (" << c->formatValue(percent, "{value|2}") << "%)<*/font*>";
                    }
                    
                    ohlcLegend << "<*/*>";
                }
            }
            else {
                // Collect values for all data sets in the layer
                for (int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet *dataSet = layer->getDataSetByZ(k);
                    
                    // Get the name and value of the data set
                    const char *dataName = dataSet->getDataName();
                    int color = dataSet->getDataColor();
                    double value = dataSet->getValue(xIndex);
                    
                    // Skip empty data sets or the data point is missing
                    if (!dataName || !*dataName || (color == (int)Chart::Transparent) || 
                        (value == Chart::NoValue))
                        continue;
                    
                    // In a standard legend, the line or symbol is colored using the same color as the data
                    // However, we want the legend to look nicer, so we use a colored square instead
                    std::ostringstream legendEntry;
                    legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color="
                        << std::hex << color << "*> " << dataName << ": ";
                    
                    // Format the value (with unit if available)
                    std::string unitChar;
                    std::string formatString = "{value|P4}";
                    
                    // Extract the unit from the name if available (e.g., "RSI (14)" => "RSI")
                    int delimiterPosition = (int)std::string(dataName).find_first_of(" -(");
                    if ((int)std::string::npos != delimiterPosition) {
                        // Check if the delimiter is a space character
                        if (dataName[delimiterPosition] == ' ') {
                            // If the name contains the unit (e.g., "Volume (K)"), extract the unit
                            int lastDigitPos = (int)std::string(dataName).find_last_of("0123456789");
                            if (((int)std::string::npos != lastDigitPos) && (lastDigitPos + 1 < (int)std::string(dataName).size()) &&
                                (lastDigitPos > delimiterPosition))
                                unitChar = std::string(dataName).substr(lastDigitPos + 1);
                        }
                    }
                    
                    legendEntry << c->formatValue(value, formatString.c_str());
                    if (!unitChar.empty())
                        legendEntry << " " << unitChar;
                    legendEntry << "<*/*>";
                    
                    legendEntries.push_back(legendEntry.str());
                }
            }
        }
        
        // Get the plot area position relative to the entire FinanceChart
        PlotArea *plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();
        int plotAreaWidth = plotArea->getWidth();
        int plotAreaHeight = plotArea->getHeight();
        
        // Create the legend text
        std::ostringstream legendText;
        legendText << "<*block,valign=top,maxWidth=" << (plotAreaWidth - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "yyyy MMM d h:nn:ss")
            << "]<*/font*>" << ohlcLegend.str();
        for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
            legendText << "      " << legendEntries[i];
        }
        legendText << "<*/*>";
        
        // Draw a vertical track line at the x-position
        int xCoor = c->getXCoor(xValue) + c->getAbsOffsetX();
        d->vline(plotAreaTopY, plotAreaTopY + plotAreaHeight, xCoor, d->dashLineColor(0x000000, 0x0101));
        
        // Display the date at the bottom of the plot area
        std::ostringstream dateText;
        dateText << "<*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "yyyy MMM d h:nn:ss") << "]<*/font*>";
        TTFText *dateLabel = d->text(dateText.str().c_str(), "Arial", 8);
        dateLabel->draw(xCoor, plotAreaTopY + plotAreaHeight + 2, 0x000000, Chart::Top);
        dateLabel->destroy();
        
        // Position the tooltip based on mouse position to avoid overlap
        // If mouse is on right half of chart, put tooltip on left side
        TTFText *t = d->text(legendText.str().c_str(), "Arial", 8);
        if (mouseX > plotAreaLeftX + plotAreaWidth / 2) {
            // Mouse is on right side, put tooltip on left
            t->draw(plotAreaLeftX + 5, plotAreaTopY + 25, 0x000000, Chart::TopLeft);
        } else {
            // Mouse is on left side, put tooltip on right
            t->draw(plotAreaLeftX + plotAreaWidth - 5, plotAreaTopY + 25, 0x000000, Chart::TopRight);
        }
        t->destroy();
    }
}


// Convert std::vector to a DoubleArray
DoubleArray FinanceChartWindow::vectorToArray(const std::vector<double>& v, int startIndex, int length)
{
    if (v.empty())
        return DoubleArray();
    
    // Default startIndex and length
    if (startIndex < 0)
        startIndex = 0;
    if (length < 0)
        length = (int)v.size() - startIndex;
    
    // Ensure startIndex and length are valid
    if (startIndex >= (int)v.size())
        startIndex = (int)v.size() - 1;
    if (length > (int)v.size() - startIndex)
        length = (int)v.size() - startIndex;
    
    return DoubleArray(&v[startIndex], length);
}

// Convert DoubleArray to std::vector
std::vector<double> FinanceChartWindow::arrayToVector(DoubleArray a)
{
    std::vector<double> ret;
    if (a.len > 0) {
        ret.resize(a.len);
        for (int i = 0; i < a.len; ++i)
            ret[i] = a[i];
    }
    return ret;
}

// User is moving the mouse on the plot area
void FinanceChartWindow::onMouseMovePlotArea(QMouseEvent*)
{
    trackFinance((MultiChart*)m_ChartViewer->getChart(), m_ChartViewer->getPlotAreaMouseX());
    m_ChartViewer->updateDisplay();
}

// Loading data via CSV
bool FinanceChartWindow::loadCSVData(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open file" << filePath;
        return false;
    }

    QTextStream in(&file);
    
    // Clear previous data
    m_rawPrice.timeStamps.clear();
    m_rawPrice.openData.clear();
    m_rawPrice.highData.clear();
    m_rawPrice.lowData.clear();
    m_rawPrice.closeData.clear();
    m_rawPrice.volData.clear();
    
    // Skip header
    QString header = in.readLine();
    
    // Read data rows
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(',');
        
        if (parts.size() >= 5)  // At least date, open, high, low, close
            {
            // Parse date time - expected format: "2022-02-14 14:30:10+00:00"
            QString dateStr = parts[0];
            QDateTime dt = QDateTime::fromString(dateStr, "yyyy-MM-dd HH:mm:ss+00:00");
            if (!dt.isValid()) {
                // Try an alternative format without timezone
                dt = QDateTime::fromString(dateStr.split('+')[0].trimmed(), "yyyy-MM-dd HH:mm:ss");
            }
            
            // Si toujours invalide, essayez d'autres formats possibles
            if (!dt.isValid()) {
                dt = QDateTime::fromString(dateStr.split('+')[0].trimmed(), "yyyy-MM-dd HH:mm");
            }
            
            if (dt.isValid()) {
                // Convert to Unix timestamp (seconds since Jan 1, 1970)
                qint64 unixTime = dt.toSecsSinceEpoch();
                
                // Convert Unix timestamp to ChartDirector time format
                double chartTime = Chart::chartTime2(unixTime);
                
                // Debug: print the date and its chartTime
                qDebug() << "Date from CSV:" << dt.toString("yyyy-MM-dd HH:mm:ss") 
                         << "Unix time:" << unixTime 
                         << "ChartDir time:" << chartTime;
                
                m_rawPrice.timeStamps.push_back(chartTime);
                m_rawPrice.openData.push_back(parts[1].toDouble());
                m_rawPrice.highData.push_back(parts[2].toDouble());
                m_rawPrice.lowData.push_back(parts[3].toDouble());
                m_rawPrice.closeData.push_back(parts[4].toDouble());
                
                // Add volume if available, otherwise set to 0
                double volume = (parts.size() > 5) ? parts[5].toDouble() : 0.0;
                m_rawPrice.volData.push_back(volume);
            }
        }
    }
    
    file.close();
    
    // Check if we loaded any data
    return !m_rawPrice.timeStamps.empty();
}

// Helper function to calculate Heikin Ashi values from OHLC data
void FinanceChartWindow::calculateHeikinAshi(const DoubleArray &open, const DoubleArray &high, 
                                            const DoubleArray &low, const DoubleArray &close,
                                            std::vector<double> &ha_open, std::vector<double> &ha_high, 
                                            std::vector<double> &ha_low, std::vector<double> &ha_close)
{
    int size = open.len;
    ha_open.resize(size);
    ha_high.resize(size);
    ha_low.resize(size);
    ha_close.resize(size);
    
    // Calculate HA_Close values: (Open + High + Low + Close) / 4
    for (int i = 0; i < size; ++i) {
        ha_close[i] = (open[i] + high[i] + low[i] + close[i]) / 4.0;
    }
    
    // Calculate HA_Open values: (HA_Open_previous + HA_Close_previous) / 2
    ha_open[0] = open[0]; // For the first candle, HA_Open = Open
    for (int i = 1; i < size; ++i) {
        ha_open[i] = (ha_open[i-1] + ha_close[i-1]) / 2.0;
    }
    
    // Calculate HA_High and HA_Low values
    for (int i = 0; i < size; ++i) {
        ha_high[i] = std::max(high[i], std::max(ha_open[i], ha_close[i]));
        ha_low[i] = std::min(low[i], std::min(ha_open[i], ha_close[i]));
    }
}

// Initialize resampling configuration
void FinanceChartWindow::initializeResamplingConfig()
{
    m_resamplingConfig.originalIntervalSeconds = 0; // Will be auto-detected
    m_resamplingConfig.maxPointsForOriginalData = 5000; // Seuil pour déclencher le resampling
    m_resamplingConfig.targetPointsWhenResampled = 2000; // Nombre cible de points après resampling
    m_resamplingConfig.autoDetectOriginalInterval = true;
}

// Detect the original time interval of the data
double FinanceChartWindow::detectOriginalTimeInterval(const std::vector<double>& timestamps)
{
    if (timestamps.size() < 2) {
        return 60.0; // Default to 1 minute if we can't detect
    }
    
    // Calculate intervals for the first 100 points (or all if less)
    std::vector<double> intervals;
    int sampleSize = std::min(100, (int)timestamps.size() - 1);
    
    for (int i = 0; i < sampleSize; ++i) {
        double interval = timestamps[i + 1] - timestamps[i];
        intervals.push_back(interval);
    }
    
    // Sort intervals and find the most common one
    std::sort(intervals.begin(), intervals.end());
    
    // Find the median interval as the most likely original interval
    size_t medianIndex = intervals.size() / 2;
    double detectedInterval = intervals[medianIndex];
    
    // Convert from ChartDirector time to seconds
    // ChartDirector time is in days, so multiply by 86400 to get seconds
    return detectedInterval * 86400.0;
}

// Determine if resampling should be used based on current view
bool FinanceChartWindow::shouldUseResampling(int startIndex, int endIndex, int totalPoints) const
{
    int pointsToDisplay = endIndex - startIndex + 1;
    return pointsToDisplay > m_resamplingConfig.maxPointsForOriginalData;
}

// Calculate optimal aggregation factor based on data points and viewport
int FinanceChartWindow::calculateOptimalAggregationFactor(int pointsToDisplay, double viewPortWidth) const
{
    if (pointsToDisplay <= m_resamplingConfig.targetPointsWhenResampled) {
        return 1; // No aggregation needed
    }
    
    // Calculate the minimum aggregation factor needed
    int minFactor = (int)std::ceil((double)pointsToDisplay / m_resamplingConfig.targetPointsWhenResampled);
    
    // Round to sensible aggregation factors based on the original interval
    double originalInterval = m_resamplingConfig.originalIntervalSeconds;
    
    // Common time intervals for aggregation
    std::vector<int> commonFactors;
    if (originalInterval <= 1) { // Sub-second data
        commonFactors = {2, 5, 10, 15, 30, 60, 120, 300, 600, 1800, 3600};
    } else if (originalInterval <= 10) { // Seconds
        commonFactors = {2, 3, 5, 6, 10, 12, 15, 30, 60, 120, 180, 360};
    } else if (originalInterval <= 60) { // Up to minute
        commonFactors = {2, 3, 5, 10, 15, 30, 60, 120};
    } else { // Minutes or more
        commonFactors = {2, 3, 5, 10, 15, 30, 60};
    }
    
    // Find the smallest common factor that meets our needs
    for (int factor : commonFactors) {
        if (factor >= minFactor) {
            return factor;
        }
    }
    
    // If no common factor works, use the minimum factor
    return minFactor;
}

// Generate resampled data with the given aggregation factor
void FinanceChartWindow::generateResampledData(int aggregationFactor)
{
    // Check if we already have this aggregation factor cached
    for (auto& cached : m_resampledCache) {
        if (cached.aggregationFactor == aggregationFactor && cached.isValid) {
            return; // Already cached
        }
    }
    
    // Remove old cache entry for this factor if it exists
    m_resampledCache.erase(
        std::remove_if(m_resampledCache.begin(), m_resampledCache.end(),
            [aggregationFactor](const ResampledData& item) {
                return item.aggregationFactor == aggregationFactor;
            }),
        m_resampledCache.end()
    );
    
    // Create new resampled data
    ResampledData newData;
    newData.aggregationFactor = aggregationFactor;
    newData.intervalSeconds = m_resamplingConfig.originalIntervalSeconds * aggregationFactor;
    newData.isValid = true;
    
    // Aggregate the raw data
    aggregateOHLCData(m_rawPrice, newData.data, aggregationFactor);
    
    // Add to cache
    m_resampledCache.push_back(newData);
}

// Aggregate OHLC data with the given factor
void FinanceChartWindow::aggregateOHLCData(const PriceData& source, PriceData& target, int aggregationFactor)
{
    if (source.timeStamps.empty() || aggregationFactor <= 1) {
        target = source;
        return;
    }
    
    int sourceSize = source.timeStamps.size();
    int targetSize = (sourceSize + aggregationFactor - 1) / aggregationFactor;
    
    target.timeStamps.clear();
    target.openData.clear();
    target.highData.clear();
    target.lowData.clear();
    target.closeData.clear();
    target.volData.clear();
    target.compareData.clear();
    
    target.timeStamps.reserve(targetSize);
    target.openData.reserve(targetSize);
    target.highData.reserve(targetSize);
    target.lowData.reserve(targetSize);
    target.closeData.reserve(targetSize);
    target.volData.reserve(targetSize);
    
    for (int i = 0; i < sourceSize; i += aggregationFactor) {
        int endIdx = std::min(i + aggregationFactor, sourceSize);
        
        // Timestamp: use the timestamp of the first point in the group
        target.timeStamps.push_back(source.timeStamps[i]);
        
        // Open: first open in the group
        target.openData.push_back(source.openData[i]);
        
        // Close: last close in the group
        target.closeData.push_back(source.closeData[endIdx - 1]);
        
        // High: maximum high in the group
        double maxHigh = source.highData[i];
        for (int j = i + 1; j < endIdx; ++j) {
            maxHigh = std::max(maxHigh, source.highData[j]);
        }
        target.highData.push_back(maxHigh);
        
        // Low: minimum low in the group
        double minLow = source.lowData[i];
        for (int j = i + 1; j < endIdx; ++j) {
            minLow = std::min(minLow, source.lowData[j]);
        }
        target.lowData.push_back(minLow);
        
        // Volume: sum of volumes in the group
        double totalVol = 0;
        for (int j = i; j < endIdx; ++j) {
            totalVol += source.volData[j];
        }
        target.volData.push_back(totalVol);
    }
    
    // Handle compare data if it exists
    if (!source.compareData.empty()) {
        target.compareData.reserve(targetSize);
        for (int i = 0; i < sourceSize; i += aggregationFactor) {
            int endIdx = std::min(i + aggregationFactor, sourceSize);
            target.compareData.push_back(source.compareData[endIdx - 1]);
        }
    }
}

// Select the optimal dataset based on the current view
FinanceChartWindow::PriceData* FinanceChartWindow::selectOptimalDataset(int startIndex, int endIndex, int& outStartIndex, int& outEndIndex)
{
    int pointsToDisplay = endIndex - startIndex + 1;
    int totalPoints = m_rawPrice.timeStamps.size();
    
    // Auto-detect original interval if not set
    if (m_resamplingConfig.autoDetectOriginalInterval && m_resamplingConfig.originalIntervalSeconds == 0) {
        m_resamplingConfig.originalIntervalSeconds = detectOriginalTimeInterval(m_rawPrice.timeStamps);
        qDebug() << "Auto-detected original interval:" << m_resamplingConfig.originalIntervalSeconds << "seconds";
    }
    
    // Check if resampling is needed
    if (!shouldUseResampling(startIndex, endIndex, totalPoints)) {
        outStartIndex = startIndex;
        outEndIndex = endIndex;
        return &m_rawPrice; // Use original data
    }
    
    // Calculate optimal aggregation factor
    double viewPortWidth = m_ChartViewer->getViewPortWidth();
    int aggregationFactor = calculateOptimalAggregationFactor(pointsToDisplay, viewPortWidth);
    
    // Generate resampled data if needed
    generateResampledData(aggregationFactor);
    
    // Find the cached resampled data
    for (auto& cached : m_resampledCache) {
        if (cached.aggregationFactor == aggregationFactor && cached.isValid) {
            // Adjust indices for the resampled data
            outStartIndex = startIndex / aggregationFactor;
            outEndIndex = endIndex / aggregationFactor;
            
            // Ensure indices are within bounds
            int resampledSize = cached.data.timeStamps.size();
            outStartIndex = std::max(0, std::min(outStartIndex, resampledSize - 1));
            outEndIndex = std::max(outStartIndex, std::min(outEndIndex, resampledSize - 1));
            
            qDebug() << "Using resampled data with factor" << aggregationFactor 
                     << "- Points reduced from" << pointsToDisplay 
                     << "to" << (outEndIndex - outStartIndex + 1);
            
            return &cached.data;
        }
    }
    
    // Fallback to original data if resampling failed
    outStartIndex = startIndex;
    outEndIndex = endIndex;
    return &m_rawPrice;
}

// Invalidate resampling cache (call when raw data changes)
void FinanceChartWindow::invalidateResamplingCache()
{
    for (auto& cached : m_resampledCache) {
        cached.isValid = false;
    }
    m_resamplingConfig.originalIntervalSeconds = 0; // Force re-detection
}