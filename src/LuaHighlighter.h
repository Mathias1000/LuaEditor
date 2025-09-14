#pragma once

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QColor>
#include <QFont>
#include <array>
#include <string_view>

class LuaHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit LuaHighlighter(QTextDocument *parent = nullptr);
    ~LuaHighlighter() override = default;

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    
    void setupHighlightingRules();
    void setupFormats();
    
    // Highlighting rules
    std::vector<HighlightingRule> m_highlightingRules;
    
    // Text formats
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_quotationFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_builtinFormat;

    // Multi-line comment handling
    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;

    // Color scheme
    static constexpr std::string_view KEYWORD_COLOR   = "#0000FF"; // Blue
    static constexpr std::string_view COMMENT_COLOR   = "#008000"; // Green
    static constexpr std::string_view STRING_COLOR    = "#800080"; // Purple
    static constexpr std::string_view FUNCTION_COLOR  = "#FF8000"; // Orange
    static constexpr std::string_view NUMBER_COLOR    = "#FF0000"; // Red
    static constexpr std::string_view OPERATOR_COLOR  = "#808080"; // Gray
    static constexpr std::string_view BUILTIN_COLOR   = "#008080"; // Teal

    // Lua keywords (Lua 5.4.8)

    static constexpr auto LUA_KEYWORDS = std::to_array<const char*> ({
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "if", "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while", "goto"
    });

    // Lua built-in globals + häufige Bibliotheksfunktionen (Lua 5.4.8)

    static constexpr auto LUA_BUILTINS = std::to_array<const char*> ({
        // Basisfunktionen
        "assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs",
        "load", "loadfile", "next", "pairs", "pcall", "print", "rawequal", "rawget",
        "rawlen", "rawset", "require", "select", "setmetatable", "tonumber",
        "tostring", "type", "xpcall", "_G", "_VERSION",

        // Table
        "table.concat", "table.insert", "table.move", "table.pack",
        "table.remove", "table.sort", "table.unpack",

        // String
        "string.byte", "string.char", "string.dump", "string.find",
        "string.format", "string.gmatch", "string.gsub", "string.len",
        "string.lower", "string.match", "string.rep", "string.reverse",
        "string.sub", "string.upper", "string.pack", "string.unpack",

        // Math
        "math.abs", "math.acos", "math.asin", "math.atan", "math.ceil",
        "math.cos", "math.deg", "math.exp", "math.floor", "math.fmod",
        "math.log", "math.max", "math.min", "math.modf", "math.rad",
        "math.random", "math.sin", "math.sqrt", "math.tan", "math.tointeger",
        "math.type", "math.ult",

        // OS
        "os.clock", "os.date", "os.difftime", "os.execute", "os.exit",
        "os.getenv", "os.remove", "os.rename", "os.setlocale", "os.time"
    });
};
