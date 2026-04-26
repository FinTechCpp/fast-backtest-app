#include "ui/dialogs/LuaScriptDialog.h"

#include "Managers/LuaScriptEngine.hpp"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextDocument>
#include <QVBoxLayout>

LuaSyntaxHighlighter::LuaSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#0b5394"));
    keywordFormat.setFontWeight(QFont::Bold);

    const QStringList keywords = {
        "\\band\\b", "\\bbreak\\b", "\\bdo\\b", "\\belse\\b", "\\belseif\\b",
        "\\bend\\b", "\\bfalse\\b", "\\bfor\\b", "\\bfunction\\b", "\\bif\\b",
        "\\bin\\b", "\\blocal\\b", "\\bnil\\b", "\\bnot\\b", "\\bor\\b",
        "\\brepeat\\b", "\\breturn\\b", "\\bthen\\b", "\\btrue\\b", "\\buntil\\b", "\\bwhile\\b"
    };

    for (const QString& pattern : keywords) {
        m_rules.push_back({QRegularExpression(pattern), keywordFormat});
    }

    QTextCharFormat functionFormat;
    functionFormat.setForeground(QColor("#674ea7"));
    functionFormat.setFontItalic(true);
    m_rules.push_back({QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\()"), functionFormat});

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor("#b45f06"));
    m_rules.push_back({QRegularExpression("\\b\\d+(\\.\\d+)?\\b"), numberFormat});

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor("#38761d"));
    m_rules.push_back({QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\""), stringFormat});
    m_rules.push_back({QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'"), stringFormat});

    QTextCharFormat commentFormat;
    commentFormat.setForeground(QColor("#6a737d"));
    m_rules.push_back({QRegularExpression("--[^\\n]*"), commentFormat});
}

void LuaSyntaxHighlighter::highlightBlock(const QString& text)
{
    for (const HighlightRule& rule : m_rules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            const QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

LuaScriptDialog::LuaScriptDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Optional Lua Script");
    setMinimumSize(900, 650);
    setupUI();
}

void LuaScriptDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* title = new QLabel(
        "Optional advanced scripting."
        " If enabled, function on_candle(candle, position) can emit trading signals.",
        this);
    title->setWordWrap(true);
    title->setStyleSheet(
        "QLabel {"
        "  color: #333333;"
        "  font-size: 13px;"
        "  padding: 4px 2px;"
        "}");
    mainLayout->addWidget(title);

    m_enableScriptCheck = new QCheckBox("Enable custom Lua script", this);
    m_enableScriptCheck->setChecked(false);
    mainLayout->addWidget(m_enableScriptCheck);

    m_editor = new QPlainTextEdit(this);
    QFont editorFont("Monospace");
    editorFont.setStyleHint(QFont::TypeWriter);
    editorFont.setPointSize(11);
    m_editor->setFont(editorFont);
    m_editor->setPlaceholderText(
        "function on_candle(candle, position)\n"
        "    -- return { type = SignalType.NONE }\n"
        "end\n");
    m_editor->setTabStopDistance(m_editor->fontMetrics().horizontalAdvance(' ') * 4);
    m_highlighter = new LuaSyntaxHighlighter(m_editor->document());
    mainLayout->addWidget(m_editor, 1);

    QHBoxLayout* helpersLayout = new QHBoxLayout();
    QPushButton* templateButton = new QPushButton("Insert template", this);
    templateButton->setMinimumHeight(34);
    helpersLayout->addWidget(templateButton);

    QPushButton* checkScriptButton = new QPushButton("Check script", this);
    checkScriptButton->setMinimumHeight(34);
    helpersLayout->addWidget(checkScriptButton);

    helpersLayout->addStretch();
    mainLayout->addLayout(helpersLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    mainLayout->addWidget(buttonBox);

    connect(m_enableScriptCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        m_editor->setEnabled(enabled);
    });

    connect(templateButton, &QPushButton::clicked, this, [this]() {
        applyTemplateIfEmpty();
    });

    connect(checkScriptButton, &QPushButton::clicked, this, [this]() {
        if (!m_enableScriptCheck->isChecked() || m_editor->toPlainText().trimmed().isEmpty()) {
            QMessageBox::information(this, "Check script", "Nothing to check (script empty or disabled).");
            return;
        }
        std::string err_msg;
        if (LuaScriptEngine::validate_script(m_editor->toPlainText().toStdString(), err_msg)) {
            QMessageBox::information(this, "Check script", "No syntax errors found. Excellent!");
        } else {
            QMessageBox::critical(this, "Script error", QString("Error found in script:\n\n%1").arg(QString::fromStdString(err_msg)));
        }
    });

    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_enableScriptCheck->isChecked()) {
            if (m_editor->toPlainText().trimmed().isEmpty()) {
                const int answer = QMessageBox::question(
                    this,
                    "Empty script",
                    "The Lua script is enabled but empty. Disable it and continue?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes);

                if (answer == QMessageBox::Yes) {
                    m_enableScriptCheck->setChecked(false);
                } else {
                    return;
                }
            }
        }
        accept();
    });

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_editor->setEnabled(m_enableScriptCheck->isChecked());
}

void LuaScriptDialog::applyTemplateIfEmpty()
{
    if (!m_editor->toPlainText().trimmed().isEmpty()) {
        return;
    }

    const QString templateCode =
        "-- Optional Lua strategy script\n"
        "-- Called for each new candle\n"
        "-- candle fields: open, high, low, close, year, month, day, hour, minute, second\n"
        "-- position fields: is_open, entry_price, take_profit_price, closed_trade_pnl\n"
        "-- helpers: get_candle(offset), candles_count(), get_position(), log(message), set_required_history(count), drawpoint(price, marker_type), drawvline(marker_type_or_color, color)\n"
        "-- Return nil or a signal table:\n"
        "-- { type = SignalType.BUY, quantity = 1.0, stop_loss = 20.0, take_profit = 40.0 }\n"
        "-- { type = SignalType.SELL, quantity = 1.0, stop_loss = 20.0, take_profit = 40.0 }\n"
        "-- { type = SignalType.LIQUIDATE, quantity = 1.0 }  -- quantity in ]0,1[ for partial close\n"
        "-- { type = SignalType.MOVE_SL, new_sl = 1.2345, price = 1.2360 }\n"
        "\n"
        "set_required_history(200)  -- Request 200 candles of history for indicator calculations\n"
        "function on_candle(candle, position)\n"
        "    local prev = get_candle(1)\n"
        "\n"
        "    local is_green = candle.close > candle.open\n"
        "    local is_red = candle.close < candle.open\n"
        "\n"
        "    -- Entry: buy only on a green candle when no position is open\n"
        "    if not position.is_open and is_green then\n"
        "        return { type = SignalType.BUY }\n"
        "    end\n"
        "\n"
        "    -- Exit: sell (liquidate) on the first red candle\n"
        "    if position.is_open and prev ~= nil then\n"
        "        if is_red then\n"
        "            return { type = SignalType.LIQUIDATE }\n"
        "        end\n"
        "    end\n"
        "\n"
        "    return nil\n"
        "end\n";

    m_editor->setPlainText(templateCode);
    m_enableScriptCheck->setChecked(true);
}

void LuaScriptDialog::setScriptEnabled(bool enabled)
{
    m_enableScriptCheck->setChecked(enabled);
}

bool LuaScriptDialog::isScriptEnabled() const
{
    return m_enableScriptCheck->isChecked();
}

void LuaScriptDialog::setScriptCode(const std::string& code)
{
    m_editor->setPlainText(QString::fromStdString(code));
}

std::string LuaScriptDialog::scriptCode() const
{
    return m_editor->toPlainText().toStdString();
}
