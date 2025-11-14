#include "ui/menu/downloadProgressDialog.h"
#include <QGroupBox>
#include <QDateTime>

DownloadProgressDialog::DownloadProgressDialog(int totalFiles, QWidget* parent)
    : QDialog(parent)
    , m_totalFiles(totalFiles)
    , m_completedFiles(0)
    , m_canceled(false)
    , m_lastBytesReceived(0)
    , m_lastTimestamp(0)
{
    setWindowTitle(tr("Downloading Market Data"));
    setMinimumWidth(500);
    setMinimumHeight(250);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    setupUI();
    
    m_timer.start();
}

DownloadProgressDialog::~DownloadProgressDialog()
{
}

void DownloadProgressDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Overall progress group
    QGroupBox* overallGroup = new QGroupBox(tr("Overall Progress"));
    QVBoxLayout* overallLayout = new QVBoxLayout(overallGroup);
    
    m_overallLabel = new QLabel(tr("Starting download..."));
    m_overallLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    overallLayout->addWidget(m_overallLabel);
    
    m_overallProgressBar = new QProgressBar();
    m_overallProgressBar->setMaximum(m_totalFiles);
    m_overallProgressBar->setValue(0);
    m_overallProgressBar->setTextVisible(true);
    m_overallProgressBar->setFormat("%v / %m files (%p%)");
    m_overallProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #bdc3c7;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   background-color: #ecf0f1;"
        "   height: 25px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "       stop:0 #3498db, stop:1 #2980b9);"
        "   border-radius: 3px;"
        "}"
    );
    overallLayout->addWidget(m_overallProgressBar);
    
    mainLayout->addWidget(overallGroup);
    
    // Current file progress group
    QGroupBox* fileGroup = new QGroupBox(tr("Current File"));
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    
    m_fileLabel = new QLabel(tr("No file"));
    m_fileLabel->setStyleSheet("font-weight: bold; color: #34495e;");
    m_fileLabel->setWordWrap(true);
    fileLayout->addWidget(m_fileLabel);
    
    m_fileProgressBar = new QProgressBar();
    m_fileProgressBar->setMaximum(100);
    m_fileProgressBar->setValue(0);
    m_fileProgressBar->setTextVisible(true);
    m_fileProgressBar->setFormat("%p%");
    m_fileProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #bdc3c7;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   background-color: #ecf0f1;"
        "   height: 25px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "       stop:0 #2ecc71, stop:1 #27ae60);"
        "   border-radius: 3px;"
        "}"
    );
    fileLayout->addWidget(m_fileProgressBar);
    
    // Info labels layout (2 columns)
    QHBoxLayout* infoLayout = new QHBoxLayout();
    
    // Left column
    QVBoxLayout* leftColumn = new QVBoxLayout();
    m_sizeLabel = new QLabel(tr("Size: --"));
    m_sizeLabel->setStyleSheet("color: #7f8c8d;");
    m_speedLabel = new QLabel(tr("Speed: --"));
    m_speedLabel->setStyleSheet("color: #7f8c8d;");
    leftColumn->addWidget(m_sizeLabel);
    leftColumn->addWidget(m_speedLabel);
    
    // Right column
    QVBoxLayout* rightColumn = new QVBoxLayout();
    m_etaLabel = new QLabel(tr("Time remaining: --"));
    m_etaLabel->setStyleSheet("color: #7f8c8d;");
    m_etaLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rightColumn->addWidget(m_etaLabel);
    rightColumn->addStretch();
    
    infoLayout->addLayout(leftColumn);
    infoLayout->addStretch();
    infoLayout->addLayout(rightColumn);
    
    fileLayout->addLayout(infoLayout);
    
    mainLayout->addWidget(fileGroup);
    
    // Cancel button
    mainLayout->addStretch();
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #e74c3c;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 20px;"
        "   border-radius: 4px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #c0392b;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #a93226;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &DownloadProgressDialog::onCancelClicked);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
}

void DownloadProgressDialog::setCurrentFile(const QString& filename, int fileIndex)
{
    m_currentFilename = filename;
    m_currentFileIndex = fileIndex;
    m_lastBytesReceived = 0;
    m_lastTimestamp = m_timer.elapsed();
    m_speedSamples.clear();
    
    m_fileLabel->setText(QString("📄 %1").arg(filename));
    m_fileProgressBar->setValue(0);
    m_sizeLabel->setText(tr("Size: --"));
    m_speedLabel->setText(tr("Speed: --"));
    m_etaLabel->setText(tr("Time remaining: --"));
    
    m_overallLabel->setText(tr("Downloading file %1 of %2...").arg(fileIndex + 1).arg(m_totalFiles));
}

void DownloadProgressDialog::updateProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0) return;
    
    // Update file progress bar
    int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
    m_fileProgressBar->setValue(percentage);
    
    // Update size info
    m_sizeLabel->setText(tr("Size: %1 / %2")
        .arg(formatSize(bytesReceived))
        .arg(formatSize(bytesTotal)));
    
    // Calculate speed (with smoothing)
    qint64 currentTime = m_timer.elapsed();
    qint64 timeDiff = currentTime - m_lastTimestamp;
    
    if (timeDiff > 100 && m_lastBytesReceived > 0) { // Update every 100ms
        qint64 bytesDiff = bytesReceived - m_lastBytesReceived;
        double speed = (bytesDiff * 1000.0) / timeDiff; // bytes per second
        
        // Add to samples for smoothing (keep last 10 samples)
        m_speedSamples.append(speed);
        if (m_speedSamples.size() > 10) {
            m_speedSamples.removeFirst();
        }
        
        // Calculate average speed
        double avgSpeed = 0;
        for (double s : m_speedSamples) {
            avgSpeed += s;
        }
        avgSpeed /= m_speedSamples.size();
        
        m_speedLabel->setText(tr("Speed: %1").arg(formatSpeed(avgSpeed)));
        
        // Calculate ETA
        qint64 remainingBytes = bytesTotal - bytesReceived;
        if (avgSpeed > 0) {
            int etaSeconds = static_cast<int>(remainingBytes / avgSpeed);
            m_etaLabel->setText(tr("Time remaining: %1").arg(formatTime(etaSeconds)));
        }
        
        m_lastBytesReceived = bytesReceived;
        m_lastTimestamp = currentTime;
    }
    
    // Initial update
    if (m_lastBytesReceived == 0) {
        m_lastBytesReceived = bytesReceived;
        m_lastTimestamp = currentTime;
    }
}

void DownloadProgressDialog::setOverallProgress(int completed, int total)
{
    m_completedFiles = completed;
    m_overallProgressBar->setValue(completed);
    
    if (completed == total) {
        m_overallLabel->setText(tr("✓ Download completed!"));
        m_overallLabel->setStyleSheet("font-weight: bold; color: #27ae60;");
        m_cancelButton->setText(tr("Close"));
        m_cancelButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #3498db;"
            "   color: white;"
            "   border: none;"
            "   padding: 8px 20px;"
            "   border-radius: 4px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: #2980b9;"
            "}"
        );
    }
}

void DownloadProgressDialog::onCancelClicked()
{
    if (m_completedFiles == m_totalFiles) {
        accept();
    } else {
        m_canceled = true;
        reject();
    }
}

QString DownloadProgressDialog::formatSize(qint64 bytes)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    
    if (bytes >= GB) {
        return QString("%1 GB").arg(bytes / GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(bytes / MB, 0, 'f', 1);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(bytes / KB, 0, 'f', 1);
    } else {
        return QString("%1 B").arg(bytes);
    }
}

QString DownloadProgressDialog::formatSpeed(double bytesPerSecond)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    
    // Convert to Mbps (megabits per second)
    double mbps = (bytesPerSecond * 8.0) / (1000.0 * 1000.0);
    
    if (mbps >= 1.0) {
        return QString("%1 Mbps").arg(mbps, 0, 'f', 2);
    } else {
        // Show in KB/s for slower speeds
        double kbps = bytesPerSecond / KB;
        return QString("%1 KB/s").arg(kbps, 0, 'f', 1);
    }
}

QString DownloadProgressDialog::formatTime(int seconds)
{
    if (seconds < 0) return "--";
    
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(minutes).arg(secs);
    } else if (minutes > 0) {
        return QString("%1m %2s").arg(minutes).arg(secs);
    } else {
        return QString("%1s").arg(secs);
    }
}
