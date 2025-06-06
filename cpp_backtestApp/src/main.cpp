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
#include "app.h"
#include "components/backtest_runner.h"
#include "components/data_loader.h"
#include "components/UpdateChecker.h"
#include "menu/update_menu_manager.h"
#include "backtest.hpp"   // Pour be::* types
#include "stats.hpp"      // Pour be::Stats

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
    
    // Écrire aussi dans la console si demandé
    if (g_consoleOutput) {
        std::cout << msg.toStdString() << std::endl;
    }
}

// Fonction pour trouver le répertoire racine du projet
QString findProjectRoot()
{
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir currentDir(exeDir);
    
    // Remonte dans l'arborescence pour trouver le dossier ig-trading-bot
    do {
        QString currentPath = currentDir.absolutePath();
        
        // Vérifie si c'est le dossier ig-trading-bot
        if (currentDir.dirName() == "ig-trading-bot") {
            return currentPath;
        }
        
        // Cherche un sous-dossier ig-trading-bot
        QString igTradingBotPath = currentDir.absoluteFilePath("ig-trading-bot");
        if (QFileInfo(igTradingBotPath).isDir()) {
            return igTradingBotPath;
        }
        
    } while (currentDir.cdUp());
    
    return QString(); // Not found
}

// Fonction pour configurer les logs
void setupLogging(QtMsgType logLevel, bool consoleOutput)
{
    // Sauvegarder les paramètres dans les variables globales
    g_logLevel = logLevel;
    g_consoleOutput = consoleOutput;
    
    // Trouver le répertoire racine du projet
    QString projectRoot = findProjectRoot();
    QString logDir;
    
    if (!projectRoot.isEmpty()) {
        QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        logDir = QDir(projectRoot).absoluteFilePath("logs/backtest/" + currentDate);
        QDir().mkpath(logDir);
    } else {
        // Fallback vers le répertoire home
        logDir = QDir::homePath() + "/ig-trading-bot-logs/backtest/" + 
                QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QDir().mkpath(logDir);
    }
    
    QString logTime = QDateTime::currentDateTime().toString("hh-mm-ss");
    QString logFile = logDir + "/backtest_" + logTime + ".log";
    
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss}] [%{type}] %{message}");
    
    // Initialiser le fichier de log
    g_logFile = new QFile(logFile);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        g_logStream = new QTextStream(g_logFile);
    } else {
        std::cerr << "Impossible d'ouvrir le fichier de log: " << logFile.toStdString() << std::endl;
        g_logFile = nullptr;
        g_logStream = nullptr;
    }
    
    // Installer le handler de messages
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
    // Configuration Qt
#if QT_VERSION >= 0x050600 && QT_VERSION < 0x060000
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= 0x051400
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Configuration de la ligne de commande
    QCommandLineParser parser;
    parser.setApplicationDescription("Application de backtesting avec interface C++/Qt");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption logLevelOption(QStringList() << "l" << "log-level",
        "Définit le niveau de log (DEBUG, INFO, WARNING, ERROR, CRITICAL)", "level", "DEBUG");  // Changé en DEBUG par défaut
    parser.addOption(logLevelOption);
    
    QCommandLineOption consoleOption(QStringList() << "c" << "console",
        "Affiche aussi les logs dans la console");
    parser.addOption(consoleOption);
    
    parser.process(app);
    
    // Configuration du logging AVANT les premiers appels de log
    QString logLevelStr = parser.value(logLevelOption).toUpper();
    bool consoleOutput = parser.isSet(consoleOption);
    
    QtMsgType logLevel = QtWarningMsg;  // Par défaut
    if (logLevelStr == "DEBUG") logLevel = QtDebugMsg;
    else if (logLevelStr == "INFO") logLevel = QtInfoMsg;
    else if (logLevelStr == "WARNING") logLevel = QtWarningMsg;
    else if (logLevelStr == "ERROR" || logLevelStr == "CRITICAL") logLevel = QtCriticalMsg;
    
    // INSTALLER LE HANDLER AVANT LES PREMIERS LOGS
    setupLogging(logLevel, consoleOutput);
    
    // MAINTENANT on peut utiliser les logs
    qInfo() << "=== Démarrage de l'application ===";
    qInfo() << "Arguments:" << QStringList(argv, argv + argc);
    
    // Vérification des mises à jour
    UpdateChecker updateChecker;
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable, 
        [&](const QString& newVersion, const QString& downloadUrl) {
            qInfo() << "Mise à jour disponible:" << newVersion;
            
            // Demander à l'utilisateur s'il souhaite mettre à jour
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText(QString("Une nouvelle version (%1) est disponible.").arg(newVersion));
            msgBox.setInformativeText("Voulez-vous la télécharger et l'installer maintenant ?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            
            if (msgBox.exec() == QMessageBox::Yes) {
                // Télécharger et installer la mise à jour
                updateChecker.downloadAndInstallUpdate();
            }
        });

    // Lancer la vérification des mises à jour
    updateChecker.checkForUpdates();
    
    // Enregistrer les types personnalisés pour Qt
    qRegisterMetaType<OHLCBar>("OHLCBar");
    qRegisterMetaType<std::vector<OHLCBar>>("std::vector<OHLCBar>");
    qRegisterMetaType<std::shared_ptr<be::Data>>("std::shared_ptr<be::Data>");
    qRegisterMetaType<be::Stats>("be::Stats");
    qRegisterMetaType<BacktestResults*>("BacktestResults*");
    qInfo() << "Types personnalisés enregistrés dans Qt";
    
    try {
        // Créer et afficher la fenêtre principale
        qInfo() << "Création de la fenêtre principale...";
        App mainWindow;
        mainWindow.show();
        
        qInfo() << "Application prête";
        
        // Exécuter la boucle d'événements
        int result = app.exec();
        
        // Nettoyer les logs avant de quitter
        cleanupLogging();
        
        return result;
    }
    catch (const std::exception& e) {
        qCritical() << "Erreur lors de l'initialisation de l'application:" << e.what();
        QMessageBox::critical(nullptr, "Erreur", 
            QString("Une erreur est survenue: %1").arg(e.what()));
        cleanupLogging();
        return -1;
    }
}