#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QVector>

class LuaSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit LuaSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightRule> m_rules;
};

class LuaScriptDialog : public QDialog {
    Q_OBJECT

public:
    explicit LuaScriptDialog(QWidget* parent = nullptr);
    ~LuaScriptDialog() override = default;

    void setScriptEnabled(bool enabled);
    bool isScriptEnabled() const;

    void setScriptCode(const std::string& code);
    std::string scriptCode() const;

private:
    void setupUI();
    void applyTemplateIfEmpty();

private:
    QCheckBox* m_enableScriptCheck = nullptr;
    QPlainTextEdit* m_editor = nullptr;
    LuaSyntaxHighlighter* m_highlighter = nullptr;
};
