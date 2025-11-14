#ifndef DOWNLOADPROGRESSDIALOG_H
#define DOWNLOADPROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QElapsedTimer>

class DownloadProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadProgressDialog(int totalFiles, QWidget* parent = nullptr);
    ~DownloadProgressDialog();

    void updateProgress(qint64 bytesReceived, qint64 bytesTotal);
    void setCurrentFile(const QString& filename, int fileIndex);
    void setOverallProgress(int completed, int total);
    bool wasCanceled() const { return m_canceled; }

private slots:
    void onCancelClicked();

private:
    // Overall progress
    QProgressBar* m_overallProgressBar;
    QLabel* m_overallLabel;
    
    // Current file progress
    QProgressBar* m_fileProgressBar;
    QLabel* m_fileLabel;
    QLabel* m_speedLabel;
    QLabel* m_sizeLabel;
    QLabel* m_etaLabel;
    
    QPushButton* m_cancelButton;
    
    // Tracking
    QString m_currentFilename;
    int m_currentFileIndex;
    int m_totalFiles;
    int m_completedFiles;
    bool m_canceled;
    
    // Speed calculation
    QElapsedTimer m_timer;
    qint64 m_lastBytesReceived;
    qint64 m_lastTimestamp;
    QList<double> m_speedSamples; // For smoothing
    
    void setupUI();
    QString formatSize(qint64 bytes);
    QString formatSpeed(double bytesPerSecond);
    QString formatTime(int seconds);
};

#endif // DOWNLOADPROGRESSDIALOG_H
