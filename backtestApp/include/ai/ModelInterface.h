#pragma once

#include <QString>
#include <QObject>
#include <memory>
#include <functional>

namespace ai {

/**
 * @brief Generic interface for AI model inference
 * 
 * This interface provides a unified way to interact with different AI models
 * regardless of the underlying implementation (llama.cpp, OpenVINO, etc.)
 */
class ModelInterface {
public:
    virtual ~ModelInterface() = default;

    /**
     * @brief Load a model from file
     * @param modelPath Path to the model file
     * @return True if loading was successful
     */
    virtual bool loadModel(const QString& modelPath) = 0;

    /**
     * @brief Run inference on the given prompt
     * @param prompt Input text prompt
     * @param maxTokens Maximum number of tokens to generate (for text generation)
     * @return Generated response
     */
    virtual QString inference(const QString& prompt, int maxTokens = 512) = 0;

    /**
     * @brief Check if model is loaded and ready for inference
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Get the last error message
     */
    virtual QString getLastError() const = 0;

    /**
     * @brief Get model information (name, version, etc.)
     */
    virtual QString getModelInfo() const = 0;
};

/**
 * @brief Factory for creating AI model instances
 */
class ModelFactory {
public:
    enum class ModelType {
        Auto,        // Auto-detect from file extension
        LlamaCpp,    // llama.cpp GGUF models
    };

    /**
     * @brief Create a model instance based on type
     * @param type Type of model to create
     * @return Unique pointer to model interface
     */
    static std::unique_ptr<ModelInterface> createModel(ModelType type = ModelType::Auto);

    /**
     * @brief Create a model instance based on file extension
     * @param modelPath Path to model file (used to detect type)
     * @return Unique pointer to model interface
     */
    static std::unique_ptr<ModelInterface> createModel(const QString& modelPath);

    /**
     * @brief Check if a model type is supported
     * @param type Model type to check
     * @return True if supported
     */
    static bool isSupported(ModelType type);

    /**
     * @brief Get list of supported file extensions
     */
    static QStringList getSupportedExtensions();
};

/**
 * @brief Asynchronous AI inference manager
 * 
 * Handles AI inference in a separate thread to prevent UI blocking
 */
class InferenceManager : public QObject {
    Q_OBJECT

public:
    explicit InferenceManager(QObject* parent = nullptr);
    ~InferenceManager();

    /**
     * @brief Set the AI model to use
     * @param model Unique pointer to model interface
     */
    void setModel(std::unique_ptr<ModelInterface> model);

    /**
     * @brief Run inference asynchronously
     * @param prompt Input prompt
     * @param maxTokens Maximum tokens to generate
     */
    void runInferenceAsync(const QString& prompt, int maxTokens = 512);

    /**
     * @brief Check if inference is currently running
     */
    bool isRunning() const;

    /**
     * @brief Cancel current inference if running
     */
    void cancelInference();

signals:
    /**
     * @brief Emitted when inference is completed
     * @param result Generated text
     */
    void inferenceCompleted(const QString& result);

    /**
     * @brief Emitted when inference fails
     * @param error Error message
     */
    void inferenceError(const QString& error);

    /**
     * @brief Emitted to report inference progress (0-100)
     * @param progress Progress percentage
     */
    void inferenceProgress(int progress);

private:
    class InferenceWorker;
    std::unique_ptr<InferenceWorker> m_worker;
    QThread* m_workerThread;
    std::unique_ptr<ModelInterface> m_model;
    bool m_isRunning;
};

} // namespace ai