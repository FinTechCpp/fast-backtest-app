#include "ai/ModelInterface.h"
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSettings>

// CPR includes for HTTP requests
#include <cpr/cpr.h>
#include <string>
#include <thread>
#include <algorithm>
#include <cmath>

namespace ai {

// =============================================================================
// Mistral AI Model Implementation
// =============================================================================
class MistralAIModel : public ModelInterface {
private:
    QString m_lastError;
    QString m_apiKey;
    QString m_modelName;
    bool m_isReady = false;
    
    // API configuration
    const QString API_BASE_URL = "https://api.mistral.ai/v1/chat/completions";
    
public:
    MistralAIModel() {
        m_modelName = "mistral-small-latest"; // Default model
        loadApiKey();
    }
    
    ~MistralAIModel() = default;

    bool loadModel(const QString& apiKeyOrPath) override {
        if (apiKeyOrPath.startsWith("sk-") || apiKeyOrPath.startsWith("api_")) {
            // This looks like an API key
            m_apiKey = apiKeyOrPath;
        } else {
            // Try to load from file
            QFileInfo fileInfo(apiKeyOrPath);
            if (fileInfo.exists()) {
                QSettings settings(apiKeyOrPath, QSettings::IniFormat);
                m_apiKey = settings.value("mistral/api_key", "").toString();
                m_modelName = settings.value("mistral/model", "mistral-small-latest").toString();
            } else {
                m_lastError = "API key file not found: " + apiKeyOrPath;
                return false;
            }
        }
        
        if (m_apiKey.isEmpty()) {
            m_lastError = "No API key provided";
            return false;
        }
        
        // Test the API key with a simple request
        if (testApiConnection()) {
            m_isReady = true;
            m_lastError.clear();
            qDebug() << "Successfully configured Mistral AI API";
            return true;
        } else {
            m_isReady = false;
            return false;
        }
    }
    
private:
    void loadApiKey() {
        // Try to load API key from settings or environment
        QSettings settings;
        m_apiKey = settings.value("mistral/api_key", "").toString();
        
        if (m_apiKey.isEmpty()) {
            // Try environment variable
            m_apiKey = qgetenv("MISTRAL_API_KEY");
        }
        
        if (!m_apiKey.isEmpty()) {
            m_isReady = testApiConnection();
        }
    }
    
    bool testApiConnection() override {
        try {
            // Simple test request to verify API key
            cpr::Response response = cpr::Post(
                cpr::Url{API_BASE_URL.toStdString()},
                cpr::Header{
                    {"Authorization", ("Bearer " + m_apiKey).toStdString()},
                    {"Content-Type", "application/json"}
                },
                cpr::Body{R"({
                    "model": ")" + m_modelName.toStdString() + R"(",
                    "messages": [{"role": "user", "content": "Test"}],
                    "max_tokens": 5
                })"},
                cpr::Timeout{10000}
            );
            
            if (response.status_code == 200) {
                return true;
            } else if (response.status_code == 401) {
                m_lastError = "Invalid API key";
                return false;
            } else {
                m_lastError = QString("API test failed: %1 - %2")
                    .arg(response.status_code)
                    .arg(QString::fromStdString(response.text));
                return false;
            }
        } catch (const std::exception& e) {
            m_lastError = QString("Connection error: %1").arg(e.what());
            return false;
        }
    }

public:
    QString inference(const QString& prompt, int maxTokens) override {
        if (!m_isReady) {
            m_lastError = "Mistral AI model not ready";
            return "";
        }

        try {
            // Create the JSON request for Mistral API
            QJsonObject request;
            request["model"] = m_modelName;
            request["max_tokens"] = maxTokens;
            request["temperature"] = 0.7;
            request["top_p"] = 0.9;
            
            // Create messages array
            QJsonArray messages;
            
            // System message for analysis context
            QJsonObject systemMessage;
            systemMessage["role"] = "system";
            systemMessage["content"] = "Tu es un expert en analyse quantitative de stratégies de trading. "
                "Analyse les données fournies et fournis une analyse approfondie et perspicace en format HTML. "
                "Utilise les balises HTML appropriées comme <h2>, <h3>, <p>, <ul>, <li>, <strong> etc. "
                "Commence par <div> et termine par </div>.";
            messages.append(systemMessage);
            
            // User message with the actual prompt
            QJsonObject userMessage;
            userMessage["role"] = "user";
            userMessage["content"] = prompt;
            messages.append(userMessage);
            
            request["messages"] = messages;
            
            // Convert to JSON string
            QJsonDocument doc(request);
            QString jsonString = doc.toJson(QJsonDocument::Compact);
            
            // Make the API request
            cpr::Response response = cpr::Post(
                cpr::Url{API_BASE_URL.toStdString()},
                cpr::Header{
                    {"Authorization", ("Bearer " + m_apiKey).toStdString()},
                    {"Content-Type", "application/json"}
                },
                cpr::Body{jsonString.toStdString()},
                cpr::Timeout{30000} // 30 seconds timeout
            );
            
            if (response.status_code != 200) {
                m_lastError = QString("API request failed: %1 - %2")
                    .arg(response.status_code)
                    .arg(QString::fromStdString(response.text));
                return "";
            }
            
            // Parse the response
            QJsonParseError parseError;
            QJsonDocument responseDoc = QJsonDocument::fromJson(
                QByteArray::fromStdString(response.text), &parseError);
            
            if (parseError.error != QJsonParseError::NoError) {
                m_lastError = "Failed to parse API response: " + parseError.errorString();
                return "";
            }
            
            QJsonObject responseObj = responseDoc.object();
            QJsonArray choices = responseObj["choices"].toArray();
            
            if (choices.isEmpty()) {
                m_lastError = "No response choices in API result";
                return "";
            }
            
            QJsonObject firstChoice = choices[0].toObject();
            QJsonObject message = firstChoice["message"].toObject();
            QString content = message["content"].toString();
            
            // Ensure proper HTML structure
            if (!content.startsWith("<div>") && !content.contains("<h")) {
                content = "<div style='font-family: Arial, sans-serif; line-height: 1.6;'>" + content + "</div>";
            }
            
            qDebug() << "Mistral AI response received successfully";
            return content;
            
        } catch (const std::exception& e) {
            m_lastError = QString("Inference error: %1").arg(e.what());
            return "";
        }
    }

    bool isReady() const override {
        return m_isReady && !m_apiKey.isEmpty();
    }

    QString getLastError() const override {
        return m_lastError;
    }

    QString getModelInfo() const override {
        if (!m_isReady) {
            return "Mistral AI Model (Not Ready)";
        }
        
        QString info = QString("Mistral AI Model\n");
        info += QString("Model: %1\n").arg(m_modelName);
        info += QString("API Status: Connected");
        
        return info;
    }
    
    // New method to set model
    void setModel(const QString& modelName) {
        m_modelName = modelName;
    }
    
    // New method to set API key
    bool setApiKey(const QString& apiKey) override {
        m_apiKey = apiKey;
        if (!m_apiKey.isEmpty()) {
            m_isReady = testApiConnection();
            return m_isReady;
        }
        return false;
    }
};

// =============================================================================
// Model Factory Implementation
// =============================================================================
std::unique_ptr<ModelInterface> ModelFactory::createModel(ModelType type) {
    switch (type) {                        
        case ModelType::MistralAI:
        case ModelType::Auto:
        default:
            return std::make_unique<MistralAIModel>();
    }
}

std::unique_ptr<ModelInterface> ModelFactory::createModel(const QString& modelPath) {
    // Toujours retourner MistralAI maintenant
    return std::make_unique<MistralAIModel>();
}

bool ModelFactory::isSupported(ModelType type) {
    switch (type) {
        case ModelType::MistralAI:
        case ModelType::Auto:
            return true;
    }
    return false;
}

QStringList ModelFactory::getSupportedExtensions() {
    QStringList extensions;
    extensions << "ini" << "txt" << "key"; // Fichiers de configuration API
    return extensions;
}

// =============================================================================
// Inference Worker (for async processing)
// =============================================================================
class InferenceManager::InferenceWorker : public QObject {
    Q_OBJECT

public:
    InferenceWorker(ModelInterface* model) : m_model(model), m_cancelled(false) {}

    void setPrompt(const QString& prompt, int maxTokens) {
        m_prompt = prompt;
        m_maxTokens = maxTokens;
    }

    void cancel() {
        m_cancelled = true;
    }

public slots:
    void performInference() {
        if (!m_model || !m_model->isReady()) {
            emit inferenceError("Model not ready for inference");
            return;
        }

        m_cancelled = false;
        
        // Start with initial progress
        emit inferenceProgress(10);
        
        if (m_cancelled) {
            emit inferenceError("Inference cancelled by user");
            return;
        }
        
        // Show we're processing
        emit inferenceProgress(25);

        try {
            // Perform actual inference (this will take the real time)
            QString result = m_model->inference(m_prompt, m_maxTokens);
            
            if (!m_cancelled) {
                // Show we're finishing up
                emit inferenceProgress(95);
                
                // Small delay to show completion progress
                QThread::msleep(200);
                
                // Complete progress
                emit inferenceProgress(100);
                emit inferenceCompleted(result);
            }
        } catch (const std::exception& e) {
            if (!m_cancelled) {
                emit inferenceError(QString("Inference error: %1").arg(e.what()));
            }
        }
    }

signals:
    void inferenceCompleted(const QString& result);
    void inferenceError(const QString& error);
    void inferenceProgress(int progress);

private:
    ModelInterface* m_model;
    QString m_prompt;
    int m_maxTokens;
    bool m_cancelled;
};

// =============================================================================
// Inference Manager Implementation
// =============================================================================
InferenceManager::InferenceManager(QObject* parent)
    : QObject(parent), m_workerThread(nullptr), m_isRunning(false)
{
}

InferenceManager::~InferenceManager()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void InferenceManager::setModel(std::unique_ptr<ModelInterface> model)
{
    m_model = std::move(model);
}

void InferenceManager::runInferenceAsync(const QString& prompt, int maxTokens)
{
    if (m_isRunning) {
        emit inferenceError("Inference already running");
        return;
    }

    if (!m_model || !m_model->isReady()) {
        emit inferenceError("No model loaded or model not ready");
        return;
    }

    m_isRunning = true;

    // Clean up previous thread if exists
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        m_workerThread->deleteLater();
    }

    // Create new worker thread
    m_workerThread = new QThread(this);
    m_worker = std::make_unique<InferenceWorker>(m_model.get());
    m_worker->moveToThread(m_workerThread);
    m_worker->setPrompt(prompt, maxTokens);

    // Connect signals
    connect(m_workerThread, &QThread::started, m_worker.get(), &InferenceWorker::performInference);
    connect(m_worker.get(), &InferenceWorker::inferenceCompleted, this, [this](const QString& result) {
        m_isRunning = false;
        emit inferenceCompleted(result);
    });
    connect(m_worker.get(), &InferenceWorker::inferenceError, this, [this](const QString& error) {
        m_isRunning = false;
        emit inferenceError(error);
    });
    connect(m_worker.get(), &InferenceWorker::inferenceProgress, this, &InferenceManager::inferenceProgress);

    // Start the thread
    m_workerThread->start();
}

bool InferenceManager::isRunning() const
{
    return m_isRunning;
}

void InferenceManager::cancelInference()
{
    if (m_worker) {
        m_worker->cancel();
    }
    m_isRunning = false;
}

} // namespace ai

#include "ModelInterface.moc"