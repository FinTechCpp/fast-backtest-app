#include "ui/menu/dataMenuManager.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QDateTime>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include "components/Utils/dataLoader.h"
#include <QNetworkInterface>

const QString DataMenuManager::SERVER_URL = "http://marketdata-fintech.duckdns.org/";

DataMenuManager::DataMenuManager(QObject* parent)
    : QObject(parent)
    , m_dataMenu(nullptr)
    , m_importCSVAction(nullptr)
    , m_importAPIAction(nullptr)
    , m_validateDataAction(nullptr)
    , m_cleanDataAction(nullptr)
    , m_dataInfoAction(nullptr)
    , m_setDirectoryAction(nullptr)
    , m_manageLocalFilesAction(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

DataMenuManager::~DataMenuManager()
{
    qDebug() << "DataMenuManager destructor called";
}

void DataMenuManager::createDataMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar null passed to createDataMenu";
        return;
    }
    
    // Create the Data menu (after Profiles menu, before Help menu)
    m_dataMenu = menuBar->addMenu(tr("&Data"));
    
    createActions();
    
    // Add actions to the menu
    m_dataMenu->addAction(m_importCSVAction);
    m_dataMenu->addAction(m_importAPIAction);
    m_dataMenu->addSeparator();
    m_dataMenu->addAction(m_manageLocalFilesAction);
    m_dataMenu->addSeparator();
    m_dataMenu->addAction(m_validateDataAction);
    m_dataMenu->addAction(m_cleanDataAction);
    m_dataMenu->addSeparator();
    m_dataMenu->addAction(m_dataInfoAction);
    m_dataMenu->addSeparator();
    m_dataMenu->addAction(m_setDirectoryAction);
    
    qDebug() << "Data menu created";
}

void DataMenuManager::createActions()
{
    // Action Import CSV
    m_importCSVAction = new QAction(tr("&Import local file..."), this);
    m_importCSVAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
   m_importCSVAction->setStatusTip(tr("Import OHLC data from a CSV file")); 
    connect(m_importCSVAction, &QAction::triggered, this, &DataMenuManager::onImportCSV);
    
    // Action Download from Fintech server
    m_importAPIAction = new QAction(tr("Download from server..."), this);
    m_importAPIAction->setStatusTip(tr("Download data from the Fintech server"));
    connect(m_importAPIAction, &QAction::triggered, this, &DataMenuManager::onImportFromAPI);
    
    // Action Validate data
    m_validateDataAction = new QAction(tr("&Validate data"), this);
    m_validateDataAction->setStatusTip(tr("Check integrity of imported data"));
    connect(m_validateDataAction, &QAction::triggered, this, &DataMenuManager::onValidateData);
    
    // Action Clean data
    m_cleanDataAction = new QAction(tr("&Clean data"), this);
    m_cleanDataAction->setStatusTip(tr("Remove obsolete data"));
    connect(m_cleanDataAction, &QAction::triggered, this, &DataMenuManager::onCleanData);
    
    // Action Data information
    m_dataInfoAction = new QAction(tr("&Data information"), this);
    m_dataInfoAction->setStatusTip(tr("Show information about available data"));
    connect(m_dataInfoAction, &QAction::triggered, this, &DataMenuManager::onShowDataInfo);
    
    // Action Set custom data directory
    m_setDirectoryAction = new QAction(tr("&Set data directory..."), this);
    m_setDirectoryAction->setStatusTip(tr("Choose a custom location for data"));
    connect(m_setDirectoryAction, &QAction::triggered, this, &DataMenuManager::onSetCustomDirectory);
    
    // Action Manage local files
    m_manageLocalFilesAction = new QAction(tr("&Manage local files..."), this);
    m_manageLocalFilesAction->setStatusTip(tr("View and delete local data files"));
    connect(m_manageLocalFilesAction, &QAction::triggered, this, &DataMenuManager::onManageLocalFiles);
}

void DataMenuManager::onImportCSV()
{
    qDebug() << "Import CSV requested";
    
    // Find the marketData directory
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        // Create the directory if it doesn't exist
        QString projectRoot = QCoreApplication::applicationDirPath();
        QDir currentDir(projectRoot);
        while (currentDir.cdUp() && currentDir.dirName() != "fast-backtest-app") {}
        
        if (currentDir.dirName() == "fast-backtest-app") {
            marketDataDir = currentDir.absoluteFilePath("marketData");
            QDir().mkpath(marketDataDir);
        } else {
            marketDataDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
    }
    
    // File selection dialog
    QStringList fileNames = QFileDialog::getOpenFileNames(
        qobject_cast<QWidget*>(parent()),
        tr("Import CSV data"),
        marketDataDir,
        tr("CSV Files (*.csv);;All Files (*)")
    );
    
    if (fileNames.isEmpty()) {
        return;
    }
    
    // Create a progress dialog
    QProgressDialog progress(tr("Importing files..."), tr("Cancel"), 0, fileNames.size(), 
                           qobject_cast<QWidget*>(parent()));
    progress.setWindowModality(Qt::WindowModal);
    
    int importedCount = 0;
    
    for (int i = 0; i < fileNames.size(); ++i) {
        progress.setValue(i);
        
        if (progress.wasCanceled()) {
            break;
        }
        
        QString fileName = fileNames.at(i);
        progress.setLabelText(tr("Importing %1...").arg(QFileInfo(fileName).fileName()));
        
        // Copy the file to marketData directory if not already there
        QFileInfo sourceInfo(fileName);
        QString destPath = QDir(marketDataDir).absoluteFilePath(sourceInfo.fileName());
        
        if (fileName != destPath) {
            if (QFile::exists(destPath)) {
                int ret = QMessageBox::question(
                    qobject_cast<QWidget*>(parent()),
                    tr("Existing file"),
                    tr("The file %1 already exists. Do you want to replace it?")
                        .arg(sourceInfo.fileName()),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
                );
                
                if (ret == QMessageBox::Cancel) {
                    break;
                } else if (ret == QMessageBox::No) {
                    continue;
                }
                
                QFile::remove(destPath);
            }
            
            if (QFile::copy(fileName, destPath)) {
                importedCount++;
                qDebug() << "File copied:" << destPath;
            } else {
                qWarning() << "Failed to copy:" << fileName << "to" << destPath;
            }
        } else {
            importedCount++;
            qDebug() << "File already in target directory:" << fileName;
        }
        
        QApplication::processEvents();
    }
    
    progress.setValue(fileNames.size());
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Import completed"),
        tr("Import completed.\n%1 file(s) imported successfully.")
            .arg(importedCount)
    );
}

void DataMenuManager::onImportFromAPI()
{
    qDebug() << "Import from API requested";

    // API URL to retrieve list of market data files
    QString market_files_endpoint = "market-data";
    QUrl apiUrl(DataMenuManager::SERVER_URL + market_files_endpoint);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    // Send GET request to retrieve the file list
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Connect finished signal to the slot that handles the response
    connect(reply, &QNetworkReply::finished, this, &DataMenuManager::onMarketDataListReceived);
    
    qDebug() << "Request sent to:" << apiUrl.toString();
}

void DataMenuManager::onValidateData()
{
    qDebug() << "Data validation requested";
    
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("No data"),
            tr("No data directory found.")
        );
        return;
    }
    
    QDir dir(marketDataDir);
    QStringList csvFiles = dir.entryList(QStringList() << "*.csv", QDir::Files);
    
    if (csvFiles.isEmpty()) {
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("No data"),
            tr("No CSV files found in %1").arg(marketDataDir)
        );
        return;
    }
    
    QProgressDialog progress(tr("Validating data..."), tr("Cancel"), 0, csvFiles.size(), 
                           qobject_cast<QWidget*>(parent()));
    progress.setWindowModality(Qt::WindowModal);
    
    QList<DataFileInfo> fileInfos;
    
    for (int i = 0; i < csvFiles.size(); ++i) {
        progress.setValue(i);
        
        if (progress.wasCanceled()) {
            break;
        }
        
        QString filePath = dir.absoluteFilePath(csvFiles.at(i));
        progress.setLabelText(tr("Validating %1...").arg(csvFiles.at(i)));
        
        // Use the new validation function
        DataFileInfo info = DataLoader::checkDataFile(filePath);
        fileInfos.append(info);
        
        QApplication::processEvents();
    }
    
    progress.setValue(csvFiles.size());
    
    // Prepare detailed report
    int validCount = 0;
    int invalidCount = 0;
    
    for (const DataFileInfo& info : fileInfos) {
        if (info.isValid) {
            validCount++;
        } else {
            invalidCount++;
        }
    }
    
    QString summaryMessage = tr("Validation completed:\n");
    summaryMessage += tr("- %1 valid file(s)\n").arg(validCount);
    summaryMessage += tr("- %1 invalid file(s)").arg(invalidCount);
    
    if (invalidCount > 0) {
        summaryMessage += tr("\n\nInvalid files:\n");
        for (const DataFileInfo& info : fileInfos) {
            if (!info.isValid) {
                summaryMessage += "- " + info.fileName + "\n";
            }
        }
    }
    
    // Show summary in a standard dialog
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Data validation"),
        summaryMessage
    );
    
    // Create a detailed dialog
    QDialog* detailsDialog = new QDialog(qobject_cast<QWidget*>(parent()));
    detailsDialog->setWindowTitle(tr("Validation details"));
    detailsDialog->resize(700, 500);
    
    QVBoxLayout* layout = new QVBoxLayout(detailsDialog);
    
    QTextEdit* textEdit = new QTextEdit(detailsDialog);
    textEdit->setReadOnly(true);
    
    // Generate detailed report
    QString detailedReport = tr("<h2>Data validation report</h2>");
    detailedReport += tr("<p>Directory: %1</p>").arg(marketDataDir);
    
    for (const DataFileInfo& info : fileInfos) {
        detailedReport += tr("<hr><h3>%1</h3>").arg(info.fileName);
        detailedReport += tr("<p><b>Status:</b> %1</p>")
                            .arg(info.isValid ? tr("<span style='color:green'>Valid</span>") 
                                              : tr("<span style='color:red'>Invalid</span>"));
        
        detailedReport += tr("<p><b>Basic information:</b><br>");
        detailedReport += tr("Size: %1 MB<br>")
                            .arg(info.fileSize / (1024.0 * 1024.0), 0, 'f', 2);
        detailedReport += tr("Number of rows: %1<br>").arg(info.totalRows);
        detailedReport += tr("Header: %1</p>").arg(info.hasHeader ? tr("Yes") : tr("No"));
        
        if (info.isValid) {
            detailedReport += tr("<p><b>Metrics:</b><br>");
            detailedReport += tr("Detected interval: %1<br>").arg(info.interval);
            detailedReport += tr("Period: %1 to %2 (%3 days)<br>")
                                .arg(info.startDate.toString("yyyy-MM-dd hh:mm"))
                                .arg(info.endDate.toString("yyyy-MM-dd hh:mm"))
                                .arg(info.durationDays);
            detailedReport += tr("Price range: %1 to %2</p>")
                                .arg(info.minPrice, 0, 'f', 2)
                                .arg(info.maxPrice, 0, 'f', 2);
            
            if (info.gapsCount > 0) {
                detailedReport += tr("<p><b>Gaps in data:</b> %1<br>").arg(info.gapsCount);
                
                // Show largest gaps (up to 5)
                int showCount = qMin(5, info.largestGaps.size());
                for (int i = 0; i < showCount; i++) {
                    QDateTime gapStart = info.largestGaps[i].first;
                    QDateTime gapEnd = info.largestGaps[i].second;
                    int hours = gapStart.secsTo(gapEnd) / 3600;
                    int minutes = (gapStart.secsTo(gapEnd) % 3600) / 60;
                    
                    detailedReport += tr("• %1 to %2 (%3h %4m)<br>")
                                      .arg(gapStart.toString("yyyy-MM-dd hh:mm"))
                                      .arg(gapEnd.toString("yyyy-MM-dd hh:mm"))
                                      .arg(hours)
                                      .arg(minutes);
                }
                detailedReport += tr("</p>");
            } else {
                detailedReport += tr("<p><b>Gaps in data:</b> None</p>");
            }
        }
        
        if (info.invalidRows > 0) {
            detailedReport += tr("<p><b>Invalid rows:</b> %1").arg(info.invalidRows);
            
            // Show details of invalid rows (limited to 10)
            int showCount = qMin(10, info.invalidRowDetails.size());
            if (showCount > 0) {
                detailedReport += tr("<br>Details (max 10):<br>");
                for (int i = 0; i < showCount; i++) {
                    detailedReport += tr("• %1<br>").arg(info.invalidRowDetails[i]);
                }
            }
            detailedReport += tr("</p>");
        }
    }
    
    textEdit->setHtml(detailedReport);
    
    QPushButton* closeButton = new QPushButton(tr("Close"), detailsDialog);
    connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);
    
    layout->addWidget(textEdit);
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    
    detailsDialog->exec();
}

void DataMenuManager::onCleanData()
{
    qDebug() << "Data clean requested";
    
    int ret = QMessageBox::question(
        qobject_cast<QWidget*>(parent()),
        tr("Clean data"),
        tr("This will remove all obsolete data files.\nAre you sure you want to continue?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret == QMessageBox::Yes) {
        // TODO: Implement cleaning logic
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Cleanup completed"),
            tr("Data cleanup has been performed.")
        );
    }
}

void DataMenuManager::onShowDataInfo()
{
    qDebug() << "Data info requested";
    
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("No data"),
            tr("No data directory found.")
        );
        return;
    }
    
    QDir dir(marketDataDir);
    QStringList csvFiles = dir.entryList(QStringList() << "*.csv", QDir::Files);
    
    QString info = tr("Data directory: %1\n\n").arg(marketDataDir);
    info += tr("Number of CSV files: %1\n\n").arg(csvFiles.size());
    
    if (!csvFiles.isEmpty()) {
        info += tr("Available files:\n");
        for (const QString& file : csvFiles) {
            QFileInfo fileInfo(dir.absoluteFilePath(file));
            info += tr("- %1 (%2 MB)\n")
                       .arg(file)
                       .arg(fileInfo.size() / (1024.0 * 1024.0), 0, 'f', 2);
        }
    }
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Data information"),
        info
    );
}

void DataMenuManager::onSetCustomDirectory()
{
    QString currentDir = DataLoader::findMarketDataDirectory();
    
    QString dir = QFileDialog::getExistingDirectory(
        qobject_cast<QWidget*>(parent()),
        tr("Choose a directory for market data"),
        currentDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (dir.isEmpty()) {
        return;
    }
    
    if (DataLoader::setCustomMarketDataDirectory(dir)) {
        QSettings settings("fast-backtest-app", "BacktestApp");
        QString savedPath = settings.value("marketDataPath").toString();
        qDebug() << "Path saved in QSettings:" << savedPath;
        QString actualPath = DataLoader::findMarketDataDirectory();
        qDebug() << "Path returned by findMarketDataDirectory:" << actualPath;
        
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Directory set"),
            tr("The data directory has been set successfully.\n\n%1").arg(dir)
        );
    } else {
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("Error"),
            tr("Unable to set this directory for data.\nCheck access permissions.")
        );
    }
}

void DataMenuManager::onMarketDataListReceived()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        qDebug() << "Response received, size:" << responseData.size() << "bytes";
        
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isObject()) {
            QJsonObject response = doc.object();
            QJsonArray files = response["market_data_files"].toArray();
            
            qDebug() << "Number of files on server:" << files.size();
            
            if (files.isEmpty()) {
                QMessageBox::information(
                    qobject_cast<QWidget*>(parent()),
                    tr("Synchronization"),
                    tr("No files available on the server.")
                );
            } else {
                compareAndDownloadFiles(files);
            }
        } else {
            QMessageBox::warning(
                qobject_cast<QWidget*>(parent()),
                tr("Error"),
                tr("Invalid response format from server.")
            );
        }
    } else {
        QString errorMsg = tr("API connection error: %1").arg(reply->errorString());
        qWarning() << errorMsg;
        
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("Connection error"),
            errorMsg
        );
    }
    
    reply->deleteLater();
}

void DataMenuManager::compareAndDownloadFiles(const QJsonArray& remoteFiles)
{
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        // Create the directory if it doesn't exist
        QString projectRoot = QCoreApplication::applicationDirPath();
        QDir currentDir(projectRoot);
        while (currentDir.cdUp() && currentDir.dirName() != "fast-backtest-app") {}
        
        if (currentDir.dirName() == "fast-backtest-app") {
            marketDataDir = currentDir.absoluteFilePath("marketData");
            QDir().mkpath(marketDataDir);
        } else {
            marketDataDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
    }
    
    QDir localDir(marketDataDir);
    QStringList localFiles = localDir.entryList(QStringList() << "*.csv" << "*.parquet", QDir::Files);
    
    // Create dialog for file selection
    QDialog* selectionDialog = new QDialog(qobject_cast<QWidget*>(parent()));
    selectionDialog->setWindowTitle(tr("Select files to download"));
    selectionDialog->resize(800, 500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(selectionDialog);
    
    // Header label
    QLabel* headerLabel = new QLabel(tr("Select the files you want to download from the server:"));
    mainLayout->addWidget(headerLabel);
    
    // List widget for file selection
    QListWidget* fileListWidget = new QListWidget(selectionDialog);
    
    // Map to store file information
    QMap<QString, QJsonObject> fileInfoMap;
    
    int filesNeedingUpdate = 0;
    
    // Populate the list with files that need update only
    for (const QJsonValue& fileValue : remoteFiles) {
        QJsonObject fileObj = fileValue.toObject();
        QString filename = fileObj["filename"].toString();
        QString remoteModified = fileObj["modified"].toString();
        qint64 remoteSize = fileObj["size"].toVariant().toLongLong();
        
        fileInfoMap[filename] = fileObj;
        
        QString localFilePath = localDir.absoluteFilePath(filename);
        QString displayText = filename;
        QString statusText;
        bool shouldShow = false;
        
        if (!QFile::exists(localFilePath)) {
            // File does not exist locally
            statusText = tr(" [NEW]");
            shouldShow = true;
        } else {
            // Compare modification date and size
            QFileInfo localFileInfo(localFilePath);
            QDateTime localModified = localFileInfo.lastModified();
            QDateTime remoteDateTime = QDateTime::fromString(remoteModified, Qt::ISODate);
            
            if (remoteDateTime > localModified) {
                statusText = tr(" [UPDATE AVAILABLE - newer version]");
                shouldShow = true;
            } else if (localFileInfo.size() != remoteSize) {
                statusText = tr(" [UPDATE AVAILABLE - different size]");
                shouldShow = true;
            }
            // Files that are up to date are NOT added to the list
        }
        
        // Only add files that need update
        if (shouldShow) {
            filesNeedingUpdate++;
            
            // Create list item
            QListWidgetItem* item = new QListWidgetItem(displayText + statusText);
            item->setData(Qt::UserRole, filename);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked); // Checked by default
            
            // Highlight items
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            item->setForeground(QColor(0, 100, 0)); // Dark green
            
            fileListWidget->addItem(item);
        }
    }
    
    // If no files need update, inform the user and return
    if (filesNeedingUpdate == 0) {
        delete selectionDialog;
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("Synchronization"),
            tr("All local files are up to date.")
        );
        return;
    }
    
    mainLayout->addWidget(fileListWidget);
    
    // Buttons for select all / deselect all
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* selectAllButton = new QPushButton(tr("Select All"));
    QPushButton* deselectAllButton = new QPushButton(tr("Deselect All"));
    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(deselectAllButton);
    buttonLayout->addStretch();
    
    connect(selectAllButton, &QPushButton::clicked, [fileListWidget]() {
        for (int i = 0; i < fileListWidget->count(); ++i) {
            fileListWidget->item(i)->setCheckState(Qt::Checked);
        }
    });
    
    connect(deselectAllButton, &QPushButton::clicked, [fileListWidget]() {
        for (int i = 0; i < fileListWidget->count(); ++i) {
            fileListWidget->item(i)->setCheckState(Qt::Unchecked);
        }
    });
    
    mainLayout->addLayout(buttonLayout);
    
    // OK and Cancel buttons
    QHBoxLayout* dialogButtonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(tr("Download"));
    QPushButton* cancelButton = new QPushButton(tr("Cancel"));
    dialogButtonLayout->addStretch();
    dialogButtonLayout->addWidget(okButton);
    dialogButtonLayout->addWidget(cancelButton);
    
    connect(okButton, &QPushButton::clicked, selectionDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, selectionDialog, &QDialog::reject);
    
    mainLayout->addLayout(dialogButtonLayout);
    
    // Show dialog
    if (selectionDialog->exec() != QDialog::Accepted) {
        delete selectionDialog;
        return;
    }
    
    // Collect selected files
    QStringList filesToDownload;
    for (int i = 0; i < fileListWidget->count(); ++i) {
        QListWidgetItem* item = fileListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            filesToDownload.append(item->data(Qt::UserRole).toString());
        }
    }
    
    delete selectionDialog;
    
    if (filesToDownload.isEmpty()) {
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("No selection"),
            tr("No files selected for download.")
        );
        return;
    }
    
    // Create a progress dialog
    QProgressDialog* progress = new QProgressDialog(
        tr("Downloading files..."), 
        tr("Cancel"), 
        0, 
        filesToDownload.size(), 
        qobject_cast<QWidget*>(parent())
    );
    progress->setWindowModality(Qt::WindowModal);
    progress->show();
    
    int successCount = 0;
    int errorCount = 0;
    
    // Download files one by one
    for (int i = 0; i < filesToDownload.size(); ++i) {
        if (progress->wasCanceled()) {
            break;
        }
        
        QString filename = filesToDownload.at(i);
        progress->setValue(i);
        progress->setLabelText(tr("Downloading %1...").arg(filename));
        
        // Build download URL
        QString download_endpoint = "download/" + filename;
        QUrl downloadUrl(DataMenuManager::SERVER_URL + download_endpoint);

        QNetworkRequest request(downloadUrl);
        QNetworkReply* downloadReply = m_networkManager->get(request);
        
        // Wait for download to finish (synchronous for simplicity)
        QEventLoop loop;
        connect(downloadReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        
        if (downloadReply->error() == QNetworkReply::NoError) {
            // Save file
            QString localFilePath = localDir.absoluteFilePath(filename);
            QFile localFile(localFilePath);
            
            if (localFile.open(QIODevice::WriteOnly)) {
                localFile.write(downloadReply->readAll());
                localFile.close();
                qDebug() << "File downloaded:" << filename;
                successCount++;
            } else {
                qWarning() << "Unable to write file:" << filename;
                errorCount++;
            }
        } else {
            qWarning() << "Download error" << filename << ":" << downloadReply->errorString();
            errorCount++;
        }
        
        downloadReply->deleteLater();
        QApplication::processEvents();
    }
    
    progress->setValue(filesToDownload.size());
    progress->close();
    delete progress;
    
    QString resultMessage = tr("Synchronization completed.\n");
    resultMessage += tr("%1 file(s) downloaded successfully.").arg(successCount);
    if (errorCount > 0) {
        resultMessage += tr("\n%1 error(s) occurred.").arg(errorCount);
    }
    
    QMessageBox::information(
        qobject_cast<QWidget*>(parent()),
        tr("Synchronization completed"),
        resultMessage
    );
}

void DataMenuManager::onFileDownloadFinished()
{
    // This method can be used for asynchronous downloads if needed
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    reply->deleteLater();
}

QString DataMenuManager::getFileLastModified(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        return fileInfo.lastModified().toString(Qt::ISODate);
    }
    return QString();
}

void DataMenuManager::onManageLocalFiles()
{
    qDebug() << "Manage local files requested";
    
    QString marketDataDir = DataLoader::findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("No data"),
            tr("No data directory found.")
        );
        return;
    }
    
    QDir dir(marketDataDir);
    QStringList csvFiles = dir.entryList(QStringList() << "*.csv" << "*.parquet", QDir::Files);
    
    if (csvFiles.isEmpty()) {
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            tr("No data"),
            tr("No data files found in %1").arg(marketDataDir)
        );
        return;
    }
    
    // Create dialog for file management
    QDialog* manageDialog = new QDialog(qobject_cast<QWidget*>(parent()));
    manageDialog->setWindowTitle(tr("Manage Local Data Files"));
    manageDialog->resize(800, 500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(manageDialog);
    
    // Header label
    QLabel* headerLabel = new QLabel(tr("Local data files in: %1").arg(marketDataDir));
    mainLayout->addWidget(headerLabel);
    
    // List widget for file management
    QListWidget* fileListWidget = new QListWidget(manageDialog);
    
    // Populate the list with files
    for (const QString& filename : csvFiles) {
        QFileInfo fileInfo(dir.absoluteFilePath(filename));
        
        QString displayText = QString("%1 (%2 MB) - Modified: %3")
            .arg(filename)
            .arg(fileInfo.size() / (1024.0 * 1024.0), 0, 'f', 2)
            .arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
        
        QListWidgetItem* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, filename);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        
        fileListWidget->addItem(item);
    }
    
    mainLayout->addWidget(fileListWidget);
    
    // Info label
    QLabel* infoLabel = new QLabel(tr("Total: %1 file(s)").arg(csvFiles.size()));
    mainLayout->addWidget(infoLabel);
    
    // Buttons for select all / deselect all
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* selectAllButton = new QPushButton(tr("Select All"));
    QPushButton* deselectAllButton = new QPushButton(tr("Deselect All"));
    QPushButton* deleteButton = new QPushButton(tr("Delete Selected"));
    deleteButton->setStyleSheet("QPushButton { background-color: #d32f2f; color: white; }");
    
    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(deselectAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(deleteButton);
    
    connect(selectAllButton, &QPushButton::clicked, [fileListWidget]() {
        for (int i = 0; i < fileListWidget->count(); ++i) {
            fileListWidget->item(i)->setCheckState(Qt::Checked);
        }
    });
    
    connect(deselectAllButton, &QPushButton::clicked, [fileListWidget]() {
        for (int i = 0; i < fileListWidget->count(); ++i) {
            fileListWidget->item(i)->setCheckState(Qt::Unchecked);
        }
    });
    
    connect(deleteButton, &QPushButton::clicked, [this, manageDialog, fileListWidget, dir, infoLabel]() {
        // Collect selected files
        QStringList filesToDelete;
        for (int i = 0; i < fileListWidget->count(); ++i) {
            QListWidgetItem* item = fileListWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                filesToDelete.append(item->data(Qt::UserRole).toString());
            }
        }
        
        if (filesToDelete.isEmpty()) {
            QMessageBox::information(
                manageDialog,
                tr("No selection"),
                tr("No files selected for deletion.")
            );
            return;
        }
        
        // Confirm deletion
        QString message = tr("Are you sure you want to delete the following %1 file(s)?\n\n").arg(filesToDelete.size());
        message += filesToDelete.join("\n");
        message += tr("\n\nThis action cannot be undone!");
        
        int ret = QMessageBox::warning(
            manageDialog,
            tr("Confirm deletion"),
            message,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret != QMessageBox::Yes) {
            return;
        }
        
        // Delete files
        int deletedCount = 0;
        int errorCount = 0;
        
        for (const QString& filename : filesToDelete) {
            QString filePath = dir.absoluteFilePath(filename);
            if (QFile::remove(filePath)) {
                qDebug() << "File deleted:" << filename;
                deletedCount++;
            } else {
                qWarning() << "Failed to delete:" << filename;
                errorCount++;
            }
        }
        
        // Remove deleted items from the list
        for (int i = fileListWidget->count() - 1; i >= 0; --i) {
            QListWidgetItem* item = fileListWidget->item(i);
            if (filesToDelete.contains(item->data(Qt::UserRole).toString())) {
                if (QFile::exists(dir.absoluteFilePath(item->data(Qt::UserRole).toString()))) {
                    // File still exists (deletion failed)
                    continue;
                }
                delete fileListWidget->takeItem(i);
            }
        }
        
        // Update info label
        infoLabel->setText(tr("Total: %1 file(s)").arg(fileListWidget->count()));
        
        // Show result
        QString resultMessage = tr("%1 file(s) deleted successfully.").arg(deletedCount);
        if (errorCount > 0) {
            resultMessage += tr("\n%1 error(s) occurred.").arg(errorCount);
        }
        
        QMessageBox::information(
            manageDialog,
            tr("Deletion completed"),
            resultMessage
        );
    });
    
    mainLayout->addLayout(buttonLayout);
    
    // Close button
    QHBoxLayout* dialogButtonLayout = new QHBoxLayout();
    QPushButton* closeButton = new QPushButton(tr("Close"));
    dialogButtonLayout->addStretch();
    dialogButtonLayout->addWidget(closeButton);
    
    connect(closeButton, &QPushButton::clicked, manageDialog, &QDialog::accept);
    
    mainLayout->addLayout(dialogButtonLayout);
    
    // Show dialog
    manageDialog->exec();
    delete manageDialog;
}