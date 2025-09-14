#include "LuaHighlighter.h"

LuaHighlighter::LuaHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
    setupHighlightingRules();
}

void LuaHighlighter::setupFormats()
{
    // Keywords
    m_keywordFormat.setForeground(QColor(QString::fromUtf8(KEYWORD_COLOR.data())));
    m_keywordFormat.setFontWeight(QFont::Bold);

    // Comments
    m_commentFormat.setForeground(QBrush(QColor(QString::fromUtf8(COMMENT_COLOR.data()))));
    m_commentFormat.setFontItalic(true);

    // Strings
    m_quotationFormat.setForeground(QBrush(QColor(QString::fromUtf8(STRING_COLOR.data()))));

    // Functions
    m_functionFormat.setForeground(QBrush(QColor(QString::fromUtf8(FUNCTION_COLOR.data()))));
    m_functionFormat.setFontWeight(QFont::Bold);

    // Numbers
    m_numberFormat.setForeground(QBrush(QColor(QString::fromUtf8(NUMBER_COLOR.data()))));

    // Operators
    m_operatorFormat.setForeground(QBrush(QColor(QString::fromUtf8(OPERATOR_COLOR.data()))));
    m_operatorFormat.setFontWeight(QFont::Bold);

    // Built-ins
    m_builtinFormat.setForeground(QBrush(QColor(QString::fromUtf8(BUILTIN_COLOR.data()))));
    m_builtinFormat.setFontWeight(QFont::Bold);
}


void LuaHighlighter::setupHighlightingRules()
{
    HighlightingRule rule;

    // Keywords
    QString keywordPattern = R"(\b(?:)";
    for (size_t i = 0; i < LUA_KEYWORDS.size(); ++i) {
        if (i > 0) keywordPattern += "|";
        keywordPattern += QString::fromUtf8(LUA_KEYWORDS[i]);
    }
    keywordPattern += R"()\b)";

    rule.pattern = QRegularExpression(keywordPattern);
    rule.pattern.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    rule.format = m_keywordFormat;
    m_highlightingRules.push_back(rule);

    // Built-in functions
    QString builtinPattern = R"(\b(?:)";
    for (size_t i = 0; i < LUA_BUILTINS.size(); ++i) {
        if (i > 0) builtinPattern += "|";
        builtinPattern += QString::fromUtf8(LUA_BUILTINS[i]);
    }
    builtinPattern += R"()\b)";

    rule.pattern = QRegularExpression(builtinPattern);
    rule.format = m_builtinFormat;
    m_highlightingRules.push_back(rule);

    // Function definitions → nur Name markieren (Gruppe 1)
    rule.pattern = QRegularExpression(R"(\bfunction\s+([a-zA-Z_][a-zA-Z0-9_]*)\b)");
    rule.format = m_functionFormat;
    m_highlightingRules.push_back(rule);

    // Function calls → nur Name markieren (Gruppe 1)
    rule.pattern = QRegularExpression(R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\()");
    rule.format = m_functionFormat;
    m_highlightingRules.push_back(rule);

    // Numbers
    rule.pattern = QRegularExpression(R"(\b\d+\.?\d*([eE][+-]?\d+)?\b)");
    rule.format = m_numberFormat;
    m_highlightingRules.push_back(rule);

    // Single line comments
    rule.pattern = QRegularExpression(R"(--.*)");
    rule.format = m_commentFormat;
    m_highlightingRules.push_back(rule);

    // Strings
    rule.pattern = QRegularExpression(R"('(?:[^'\\]|\\.)*')");
    rule.format = m_quotationFormat;
    m_highlightingRules.push_back(rule);

    rule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
    rule.format = m_quotationFormat;
    m_highlightingRules.push_back(rule);

    // Long strings [[...]]
    rule.pattern = QRegularExpression(R"(\[\[.*?\]\])");
    rule.format = m_quotationFormat;
    m_highlightingRules.push_back(rule);

    // Operators
    rule.pattern = QRegularExpression(R"([+\-*/=<>~#%^&|])");
    rule.format = m_operatorFormat;
    m_highlightingRules.push_back(rule);

    // Multi-line comments
    m_commentStartExpression = QRegularExpression(R"(--\[\[)");
    m_commentEndExpression = QRegularExpression(R"(\]\])");
}



void LuaHighlighter::highlightBlock(const QString &text)
{
    // Apply single-line rules
    for (const auto& rule : m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();

            // Falls Capture-Gruppe existiert → nur diese einfärben
            if (match.lastCapturedIndex() >= 1) {
                for (int i = 1; i <= match.lastCapturedIndex(); ++i) {
                    if (match.capturedStart(i) >= 0)
                        setFormat(match.capturedStart(i), match.capturedLength(i), rule.format);
                }
            } else {
                // sonst gesamtes Match einfärben
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

    // Handle multi-line comments
    setCurrentBlockState(0);

    QRegularExpressionMatch startMatch;
    qsizetype startIndex = 0;
    if (previousBlockState() != 1) {
        startMatch = m_commentStartExpression.match(text);
        startIndex = startMatch.capturedStart();
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = m_commentEndExpression.match(text, startIndex);
        qsizetype endIndex = endMatch.capturedStart();
        qsizetype commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, m_commentFormat);
        startMatch = m_commentStartExpression.match(text, startIndex + commentLength);
        startIndex = startMatch.capturedStart();
    }
}


