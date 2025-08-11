#include <QApplication>
#include <QGuiApplication>
#include <QStyleFactory>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QMetaType>
#include <iostream>
#include "ui/app.h"
#include "components/backtestRunner.h"
#include "components/Utils/dataLoader.h"
#include "components/updateChecker.h"
#include "ui/menu/updateMenuManager.h"
#include "backtest.hpp"   // For be::* types
#include "stats.hpp"      // For be::Stats

// Global variables for log management
static bool g_consoleOutput = false;
static QtMsgType g_logLevel = QtWarningMsg;
static QFile* g_logFile = nullptr;
static QTextStream* g_logStream = nullptr;

// Message handler for logs
void messageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString &msg)
{
    // Filtering on log level
    if (type < g_logLevel)
        return;
    
    // Write to file if available
    if (g_logStream) {
        QString formattedMsg = QString("[%1] [%2] %3")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(type == QtDebugMsg ? "DEBUG" : 
                 type == QtInfoMsg ? "INFO" : 
                 type == QtWarningMsg ? "WARNING" : 
                 type == QtCriticalMsg ? "CRITICAL" : "FATAL")
            .arg(msg);
        
        *g_logStream << formattedMsg << "\n";
        g_logStream->flush();
    }

    // Write to console if requested
    if (g_consoleOutput) {
        std::cout << msg.toStdString() << std::endl;
    }
}

// Find the project root directory
QString findProjectRoot()
{
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir currentDir(exeDir);
    
    // Traverse up the directory tree to find the fast-backtest-app folder
    do {
        QString currentPath = currentDir.absolutePath();
        
        // Check if the current directory is the project root
        if (currentDir.dirName() == "fast-backtest-app") {
            return currentPath;
        }

        // Check for a fast-backtest-app subdirectory
        QString igTradingBotPath = currentDir.absoluteFilePath("fast-backtest-app");
        if (QFileInfo(igTradingBotPath).isDir()) {
            return igTradingBotPath;
        }
        
    } while (currentDir.cdUp());
    
    return QString(); // Not found
}

// Function to set up logging
void setupLogging(QtMsgType logLevel, bool consoleOutput)
{
    // Save parameters to global variables
    g_logLevel = logLevel;
    g_consoleOutput = consoleOutput;

    // Find the project root directory
    QString projectRoot = findProjectRoot();
    QString logDir;
    
    if (!projectRoot.isEmpty()) {
        QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        logDir = QDir(projectRoot).absoluteFilePath("logs/backtestApp/" + currentDate);
        QDir().mkpath(logDir);
    } else {
        // Fallback to home directory
        logDir = QDir::homePath() + "/fast-backtest-app-logs/backtestApp/" +
                QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QDir().mkpath(logDir);
    }

    QString logTime = QDateTime::currentDateTime().toString("hh-mm-ss");
    QString logFile = logDir + "/backtestSession_" + logTime + ".log";
    
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss}] [%{type}] %{message}");

    // Initialize the log file
    g_logFile = new QFile(logFile);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        g_logStream = new QTextStream(g_logFile);
    } else {
        std::cerr << "Unable to open log file: " << logFile.toStdString() << std::endl;
        g_logFile = nullptr;
        g_logStream = nullptr;
    }

    // Install message handler
    qInstallMessageHandler(messageHandler);

    qInfo() << "Logs configured in file:" << logFile;
    qInfo() << "Log level:" << logLevel;
    qInfo() << "Console output:" << consoleOutput;
}

// Function to clean up logging resources
void cleanupLogging()
{
    if (g_logStream) {
        delete g_logStream;
        g_logStream = nullptr;
    }
    if (g_logFile) {
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}

int main(int argc, char *argv[])
{
    // Qt configuration 
#if QT_VERSION >= 0x050600 && QT_VERSION < 0x060000
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= 0x051400
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);

    // Force Fusion style for all OS
    app.setStyle(QStyleFactory::create("Fusion"));

    // Force a light palette
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(230, 230, 230));
    lightPalette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
    lightPalette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Text, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    lightPalette.setColor(QPalette::Link, QColor(0, 0, 255));
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    app.setPalette(lightPalette);

    // Command line configuration
    QCommandLineParser parser;
    parser.setApplicationDescription("Backtesting application with C++/Qt interface");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption logLevelOption(QStringList() << "l" << "log-level",
        "Sets the log level (DEBUG, INFO, WARNING, ERROR, CRITICAL)", "level", "DEBUG");  // Changed to DEBUG by default
    parser.addOption(logLevelOption);
    
    QCommandLineOption consoleOption(QStringList() << "c" << "console",
        "Also display logs in the console");
    parser.addOption(consoleOption);
    
    parser.process(app);

    // Configure logging BEFORE the first log calls
    QString logLevelStr = parser.value(logLevelOption).toUpper();
    bool consoleOutput = parser.isSet(consoleOption);

    QtMsgType logLevel = QtWarningMsg;  // Default log level
    if (logLevelStr == "DEBUG") logLevel = QtDebugMsg;
    else if (logLevelStr == "INFO") logLevel = QtInfoMsg;
    else if (logLevelStr == "WARNING") logLevel = QtWarningMsg;
    else if (logLevelStr == "ERROR" || logLevelStr == "CRITICAL") logLevel = QtCriticalMsg;

    // INSTALL THE HANDLER BEFORE THE FIRST LOGS
    setupLogging(logLevel, consoleOutput);

    // NOW we can use the logs
    qInfo() << "=== Application starting ===";
    qInfo() << "Arguments:" << QStringList(argv, argv + argc);

    // ==== Version check ====
    UpdateChecker updateChecker;
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable, 
        [&](const QString& newVersion, const QString& downloadUrl) {
            qInfo() << "Update available:" << newVersion;

            // Ask the user if they want to update
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText(QString("A new version (%1) is available.").arg(newVersion));
            msgBox.setInformativeText("Do you want to download and install it now?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            
            if (msgBox.exec() == QMessageBox::Yes) {
                // Download and install the update
                updateChecker.downloadAndInstallUpdate(downloadUrl);
            }
        });

    // Start the update check
    //updateChecker.checkForUpdates(); // Disabled for now
    //==== Wait for the update check to finish ====

    // Register custom types for Qt
    qRegisterMetaType<OHLCBar>("OHLCBar");
    qRegisterMetaType<std::vector<OHLCBar>>("std::vector<OHLCBar>");
    qRegisterMetaType<std::shared_ptr<be::Data>>("std::shared_ptr<be::Data>");
    qRegisterMetaType<be::Stats>("be::Stats");
    qRegisterMetaType<BacktestResults*>("BacktestResults*");
    qInfo() << "Custom types registered in Qt";

    try {
        // Create and show the main window
        qInfo() << "Creating main window...";
        App mainWindow;
        mainWindow.show();

        qInfo() << "Application ready";

        // Run the event loop
        int result = app.exec();

        // Clean up logs before exiting
        cleanupLogging();
        
        return result;
    }
    catch (const std::exception& e) {
        qCritical() << "Error initializing application:" << e.what();
        QMessageBox::critical(nullptr, "Error", 
            QString("An error occurred: %1").arg(e.what()));
        cleanupLogging();
        return -1;
    }
}