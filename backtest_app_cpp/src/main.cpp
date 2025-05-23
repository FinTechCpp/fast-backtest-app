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
#include <QDebug>
#include <QMessageBox>
#include <iostream>
#include "app.h"
#include "binding/pybinding.h"  // AJOUT OBLIGATOIRE

// Variables globales pour la configuration du logging
static bool g_consoleOutput = false;
static QtMsgType g_logLevel = QtWarningMsg;
static QFile* g_logFile = nullptr;
static QTextStream* g_logStream = nullptr;

// Handler de messages pour les logs
void messageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString &msg)
{
    // Filtrer selon le niveau de log
    if (type < g_logLevel)
        return;
    
    // Écrire dans le fichier si disponible
    if (g_logStream) {
        QString formattedMsg = QString("[%1] [%2] %3\n")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(type == QtDebugMsg ? "debug" : 
                 type == QtInfoMsg ? "info" : 
                 type == QtWarningMsg ? "warning" : 
                 type == QtCriticalMsg ? "error" : "critical")
            .arg(msg);
        *g_logStream << formattedMsg;
        g_logStream->flush();
    }
    
    // Écrire aussi dans la console si demandé
    if (g_consoleOutput) {
        std::cout << msg.toStdString() << std::endl;
    }
}

// Fonction pour configurer les logs
void setupLogging(QtMsgType logLevel, bool consoleOutput)
{
    // Sauvegarder les paramètres dans les variables globales
    g_logLevel = logLevel;
    g_consoleOutput = consoleOutput;
    
    QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString logDir = QDir::homePath() + "/ig-trading-bot/logs/backtest/" + currentDate;
    QDir().mkpath(logDir);
    
    QString logTime = QDateTime::currentDateTime().toString("hh-mm-ss");
    QString logFile = logDir + "/backtest_" + logTime + ".log";
    
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss}] [%{type}] %{message}");
    
    // Initialiser le fichier de log
    g_logFile = new QFile(logFile);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        g_logStream = new QTextStream(g_logFile);
        g_logStream->setCodec("UTF-8");
    } else {
        qWarning() << "Impossible d'ouvrir le fichier de log:" << logFile;
        delete g_logFile;
        g_logFile = nullptr;
    }
    
    // CORRECTION: Installer le handler de messages
    qInstallMessageHandler(messageHandler);
    
    qInfo() << "Logs configurés dans le fichier:" << logFile;
    qInfo() << "Niveau de log:" << logLevel;
    qInfo() << "Sortie console:" << consoleOutput;
}

// Fonction pour nettoyer les ressources de logging
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
    qInfo() << "=== Démarrage de l'application ===";
    qInfo() << "Arguments:" << QStringList(argv, argv + argc);
    
    // Configuration Qt
#if QT_VERSION >= 0x050600 && QT_VERSION < 0x060000
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= 0x051400
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    qInfo() << "Création de QApplication...";
    QApplication app(argc, argv);
    
    // Configuration du style pour une apparence cohérente
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet("QWidget {font-size:12px}");
    
    // Configuration des paramètres de l'application
    app.setApplicationName("Backtest App C++");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("IG Trading Bot");
    
    // Parser les arguments de ligne de commande
    QCommandLineParser parser;
    parser.setApplicationDescription("Application de backtesting en C++ avec Qt");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption logLevelOption(QStringList() << "log-level",
        "Niveau de log (debug, info, warning, error, critical)", "level", "warning");
    parser.addOption(logLevelOption);
    
    QCommandLineOption consoleOption(QStringList() << "console",
        "Afficher les logs dans la console aussi");
    parser.addOption(consoleOption);
    
    // Analyser les arguments
    parser.process(app);
    
    // Déterminer le niveau de log
    QString logLevelStr = parser.value(logLevelOption).toLower();
    QtMsgType logLevel = QtWarningMsg; // Par défaut
    
    if (logLevelStr == "debug") logLevel = QtDebugMsg;
    else if (logLevelStr == "info") logLevel = QtInfoMsg;
    else if (logLevelStr == "warning") logLevel = QtWarningMsg;
    else if (logLevelStr == "error" || logLevelStr == "critical") logLevel = QtCriticalMsg;
    
    bool consoleOutput = parser.isSet(consoleOption);
    
    // Configurer le système de logging
    qInfo() << "Configuration du système de logging...";
    setupLogging(logLevel, consoleOutput);
    
    // Initialiser Python et pybind11
    qInfo() << "Initialisation du système Python...";
    PyBindingManager* pyManager = PyBindingManager::getInstance();
    if (!pyManager->initialize()) {
        qCritical() << "Échec de l'initialisation de Python";
        QMessageBox::critical(nullptr, "Erreur", 
            "Impossible d'initialiser le système Python.\n"
            "Vérifiez que Python 3.10 et pybind11 sont installés.");
        return 1;
    }
    qInfo() << "Système Python initialisé avec succès";
    
    // Créer et afficher la fenêtre principale
    qInfo() << "Création de la fenêtre principale...";
    App mainWindow;
    
    qInfo() << "Affichage de la fenêtre...";
    mainWindow.show();
    
    qInfo() << "Application prête, démarrage de la boucle d'événements...";
    
    // Démarrer la boucle d'événements
    int result = app.exec();
    
    qInfo() << "Application fermée avec code:" << result;
    
    // Nettoyage
    qInfo() << "Finalisation du système Python...";
    pyManager->finalize();
    
    qInfo() << "Nettoyage du système de logging...";
    cleanupLogging();
    
    qInfo() << "=== Fin de l'application ===";
    
    return result;
}