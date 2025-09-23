#include "ai/ModelInterface.h"
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

// llama.cpp includes
#include "llama.h"
#include <vector>
#include <string>
#include <thread>
#include <algorithm>
#include <cmath>

namespace ai {

// =============================================================================
// LlamaCpp Model Implementation (Full llama.cpp support)
// =============================================================================
class LlamaCppModel : public ModelInterface {
private:
    QString m_lastError;
    QString m_modelPath;
    bool m_isLoaded = false;
    
    // llama.cpp context and model
    llama_model* m_model = nullptr;
    llama_context* m_context = nullptr;
    
    // Model parameters
    struct llama_model_params m_modelParams;
    struct llama_context_params m_contextParams;
    
    // Tokenization helpers
    std::vector<llama_token> tokenize(const std::string& text, bool addBos = true) {
        std::vector<llama_token> tokens;
        if (!m_model) return tokens;
        
        int n_tokens = text.length() + (addBos ? 1 : 0) + 1;
        tokens.resize(n_tokens);
        
        // Use new API for tokenization
        const auto vocab = llama_model_get_vocab(m_model);
        n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), 
                                tokens.data(), tokens.size(), addBos, false);
        
        if (n_tokens < 0) {
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), 
                                    tokens.data(), tokens.size(), addBos, false);
        }
        
        tokens.resize(n_tokens);
        return tokens;
    }

public:
    LlamaCppModel() {
        // Initialize llama backend
        llama_backend_init();
        
        // Set default parameters
        m_modelParams = llama_model_default_params();
        m_contextParams = llama_context_default_params();
        
        // Configure context parameters for trading analysis
        m_contextParams.n_ctx = 8192;  // Large context for comprehensive analysis
        m_contextParams.n_batch = 512;
        m_contextParams.n_threads = std::min(4, (int)std::thread::hardware_concurrency());
    }
    
    ~LlamaCppModel() {
        if (m_context) {
            llama_free(m_context);
            m_context = nullptr;
        }
        if (m_model) {
            llama_model_free(m_model);
            m_model = nullptr;
        }
        llama_backend_free();
    }

    bool loadModel(const QString& modelPath) override {
        m_modelPath = modelPath;
        QFileInfo fileInfo(modelPath);
        
        if (!fileInfo.exists()) {
            m_lastError = "GGUF model file not found: " + modelPath;
            return false;
        }

        // Clean up previous model if exists
        if (m_context) {
            llama_free(m_context);
            m_context = nullptr;
        }
        if (m_model) {
            llama_model_free(m_model);
            m_model = nullptr;
        }

        try {
            // Load the model using new API
            m_model = llama_model_load_from_file(modelPath.toStdString().c_str(), m_modelParams);
            if (!m_model) {
                m_lastError = "Failed to load llama model from: " + modelPath;
                return false;
            }

            // Create context using new API
            m_context = llama_init_from_model(m_model, m_contextParams);
            if (!m_context) {
                m_lastError = "Failed to create llama context";
                llama_model_free(m_model);
                m_model = nullptr;
                return false;
            }

            m_isLoaded = true;
            m_lastError.clear();
            
            qDebug() << "Successfully loaded llama model:" << modelPath;
            const auto vocab = llama_model_get_vocab(m_model);
            qDebug() << "Model vocab size:" << llama_vocab_n_tokens(vocab);
            qDebug() << "Model context size:" << llama_n_ctx(m_context);
            
            return true;
            
        } catch (const std::exception& e) {
            m_lastError = QString("Exception loading model: %1").arg(e.what());
            return false;
        }
    }

    // Helper function to convert markdown-style text to HTML
    QString convertMarkdownToHtml(const QString& markdown) const {
        QString html = markdown;
        
        // Convert headers
        html.replace(QRegularExpression("### (.+)"), "<h3>\\1</h3>");
        html.replace(QRegularExpression("## (.+)"), "<h2>\\1</h2>");
        html.replace(QRegularExpression("# (.+)"), "<h1>\\1</h1>");
        
        // Convert bullet points
        html.replace(QRegularExpression("\\* (.+)"), "<li>\\1</li>");
        
        // Wrap consecutive <li> items in <ul>
        html.replace(QRegularExpression("(<li>.*?</li>(?:\\s*<li>.*?</li>)*)"), "<ul>\\1</ul>");
        
        // Convert bold text
        html.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
        
        // Convert line breaks to paragraphs
        QStringList paragraphs = html.split("\n\n");
        QStringList htmlParagraphs;
        
        for (const QString& para : paragraphs) {
            QString trimmed = para.trimmed();
            if (!trimmed.isEmpty() && !trimmed.startsWith("<h") && !trimmed.startsWith("<ul>")) {
                if (!trimmed.startsWith("<li>")) {
                    htmlParagraphs.append("<p>" + trimmed.replace("\n", "<br/>") + "</p>");
                } else {
                    htmlParagraphs.append(trimmed);
                }
            } else {
                htmlParagraphs.append(trimmed);
            }
        }
        
        return htmlParagraphs.join("\n");
    }

    QString inference(const QString& prompt, int maxTokens) override {
        if (!m_isLoaded || !m_context || !m_model) {
            m_lastError = "Model not loaded or ready";
            return "";
        }

        try {
            // Create a comprehensive prompt for trading analysis with HTML output
            std::string systemPrompt = R"(You are an expert quantitative trading analyst. Analyze the provided backtest results and strategy configuration to provide actionable insights.

IMPORTANT: Your response MUST be formatted as clean HTML suitable for display in a trading application. Use proper HTML tags like <h2>, <h3>, <p>, <ul>, <li>, <strong>, <em> etc.

Structure your analysis as follows:
1. Key Strengths (bulleted list)
2. Areas for Improvement (bulleted list)  
3. Risk Assessment
4. Concrete Recommendations
5. Market Conditions Suitability

Focus on:
- Performance strengths and weaknesses
- Risk assessment and drawdown analysis  
- Strategy parameter optimization suggestions
- Market condition suitability
- Concrete recommendations for improvement

Start your response with <div> and end with </div>. Use proper HTML formatting throughout.)";

            std::string fullPrompt = systemPrompt + "\n\nBacktest Data and Strategy Configuration:\n" + prompt.toStdString() + "\n\nHTML Analysis:";
            
            // Tokenize the prompt
            auto tokens = tokenize(fullPrompt, true);
            
            if (tokens.empty()) {
                m_lastError = "Failed to tokenize prompt";
                return "";
            }

            // Prepare for generation
            std::string result;
            const int n_ctx = llama_n_ctx(m_context);
            const int maxGeneratedTokens = std::min(maxTokens, n_ctx - (int)tokens.size() - 10);
            
            if (maxGeneratedTokens <= 0) {
                m_lastError = "Prompt too long for context window";
                return "";
            }

            // Clear the KV cache - API function name may have changed
            // TODO: Find correct cache clearing function for current llama.cpp version
            // llama_kv_cache_clear(m_context);
            
            // Evaluate the prompt tokens using new batch API
            for (size_t i = 0; i < tokens.size(); i += m_contextParams.n_batch) {
                size_t batch_size = std::min((size_t)m_contextParams.n_batch, tokens.size() - i);
                
                // Create batch for this chunk
                auto batch = llama_batch_get_one(&tokens[i], (int32_t)batch_size);
                
                if (llama_decode(m_context, batch) != 0) {
                    m_lastError = "Failed to evaluate prompt tokens";
                    return "";
                }
            }

            // Generate response tokens
            std::vector<llama_token> generated_tokens;
            
            for (int i = 0; i < maxGeneratedTokens; ++i) {
                // Get logits for the last token
                float* logits = llama_get_logits_ith(m_context, -1);
                
                // Sample next token (simple greedy sampling for deterministic results)
                llama_token next_token = 0;
                float max_logit = -INFINITY;
                
                const auto vocab = llama_model_get_vocab(m_model);
                int32_t n_vocab = llama_vocab_n_tokens(vocab);
                
                for (llama_token token_id = 0; token_id < n_vocab; ++token_id) {
                    if (logits[token_id] > max_logit) {
                        max_logit = logits[token_id];
                        next_token = token_id;
                    }
                }
                
                // Check for end of sequence
                if (next_token == llama_vocab_eos(vocab)) 
                    break;
                 
                generated_tokens.push_back(next_token);
                
                // Decode the token to text
                char buffer[256];
                int n = llama_token_to_piece(vocab, next_token, buffer, sizeof(buffer), 0, false);
                if (n > 0) 
                    result.append(buffer, n);
                 
                // Evaluate the new token
                auto batch = llama_batch_get_one(&next_token, 1);
                if (llama_decode(m_context, batch) != 0) {
                    m_lastError = "Failed to evaluate generated token";
                    break;
                }
                
                // Stop on reasonable completion indicators
                if (result.find("</html>") != std::string::npos || 
                    result.find("## Summary") != std::string::npos ||
                    result.length() > maxTokens * 4) { // Rough character estimate
                    break;
                }
            }

            // Clean up result and ensure it's valid HTML
            QString qResult = QString::fromStdString(result);
            
            // Convert markdown-style formatting to HTML if needed
            if (!qResult.contains("<div>") && !qResult.contains("<h") && !qResult.contains("<p>")) {
                // This looks like markdown/plain text, convert to HTML
                qResult = convertMarkdownToHtml(qResult);
            }
            
            // Ensure proper HTML structure
            if (!qResult.startsWith("<div>") && !qResult.startsWith("<html>")) {
                qResult = "<div style='font-family: Arial, sans-serif; line-height: 1.6;'>" + qResult + "</div>";
            }
            
            qDebug() << "Generated" << generated_tokens.size() << "tokens";
            return qResult;
            
        } catch (const std::exception& e) {
            m_lastError = QString("Inference error: %1").arg(e.what());
            return "";
        }
    }

    bool isReady() const override {
        return m_isLoaded && m_model && m_context;
    }

    QString getLastError() const override {
        return m_lastError;
    }

    QString getModelInfo() const override {
        if (!m_model) {
            return "Llama.cpp Model (Not Loaded)";
        }
        
        QString info = QString("Llama.cpp Model\n");
        info += QString("File: %1\n").arg(QFileInfo(m_modelPath).fileName());
        const auto vocab = llama_model_get_vocab(m_model);
        info += QString("Vocabulary: %1 tokens\n").arg(llama_vocab_n_tokens(vocab));
        info += QString("Context: %1 tokens\n").arg(llama_n_ctx(m_context));
        info += QString("Embedding size: %1").arg(llama_model_n_embd(m_model));
        
        return info;
    }
};

// =============================================================================
// Model Factory Implementation
// =============================================================================
std::unique_ptr<ModelInterface> ModelFactory::createModel(ModelType type) {
    switch (type) {                        
        case ModelType::LlamaCpp:
            return std::make_unique<LlamaCppModel>();
            
        case ModelType::Auto:
        default:
            return std::make_unique<LlamaCppModel>();
    }
}

std::unique_ptr<ModelInterface> ModelFactory::createModel(const QString& modelPath) {
    QFileInfo fileInfo(modelPath);
    QString extension = fileInfo.suffix().toLower();
    
    if (extension == "gguf" || extension == "bin") 
        return createModel(ModelType::LlamaCpp);
    else 
        // error
        return nullptr;
}

bool ModelFactory::isSupported(ModelType type) {
    switch (type) {
        case ModelType::LlamaCpp:
            return true;  // Now supported with llama.cpp integration
        case ModelType::Auto:
            return true;
    }
    return false;
}

QStringList ModelFactory::getSupportedExtensions() {
    QStringList extensions;
    extensions << "";  // Always supported
    
    // Add others based on what's compiled in
    if (isSupported(ModelType::LlamaCpp)) 
        extensions << "gguf" << "bin";
    
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