#pragma once

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QKeyEvent>
#include <QMap>
#include <QSet>
#include <QHash>
#include <QStringList>
#include <memory>
#include "LuaParser.h"

class AutoCompleter;
class QFocusEvent;
class QResizeEvent;
class QPaintEvent;

/**
 * Qt6 / C++23 Lua Editor mit:
 *  - Zeilennummern
 *  - einfacher Einrückungslogik
 *  - F12 / Strg+F12 Navigation (lokal)
 *  - Autocomplete-Anbindung über AutoCompleter (Popup)
 *
 * Parser-Integration ist optional; die Completion-Liste wird
 * aus dem Dokumentinhalt gebaut (Identifier-Scan + Kontext '.' / ':').
 */
class LuaEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit LuaEditor(std::shared_ptr<LuaParser> parser, QWidget* parent = nullptr);
    ~LuaEditor() override = default;

    void setCompleter(AutoCompleter* completer);
    void performCompletion(); // Manual completion trigger

    [[nodiscard]] int lineNumberAreaWidth() const;
    [[nodiscard]] QString wordUnderCursor() const;
    [[nodiscard]] QString currentLineText() const;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    // Autocomplete
    void insertCompletion(const QString &completion);

    // Line numbers / visuals
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

    // Navigation
    void findNextReference();  // F12: nächstes Vorkommen des Wortes unter dem Cursor
    void goToDefinition();     // Ctrl+F12: zur Definition (function <Name>) springen
    void updateFunctionIndex();// bei Textänderung: Funktions-Index aktualisieren
    void updateSymbols();      // bei Textänderung: Symbole (Variablen & Funktionen) aktualisieren


    // import System

    void parseImports();


private:
    void setupEditor();
    [[nodiscard]] QString textUnderCursor() const;
    void showCompletion();                            // Kontextbezogenes Popup auslösen
    [[nodiscard]] QStringList buildCompletionItems() const; // Identifier-Liste (global & kontextbezogen)
    [[nodiscard]] QString detectChainUnderCursor(QString *trigger = nullptr) const;

    // Import system methods
    QStringList loadModuleFunctions(const QString& moduleName);
    QStringList parseModuleFunctions(const QString& content, const QString& moduleName);

    // Parser (must be declared before m_lineNumberArea due to constructor order)
    std::shared_ptr<LuaParser> m_parser;

    // Line number area forward declaration + member
    class LineNumberArea;
    std::unique_ptr<LineNumberArea> m_lineNumberArea;

    // Auto completion
    AutoCompleter* m_autoCompleter{nullptr};

    // Symbolindex (lokal im Dokument)
    QMap<QString, int> m_functionIndex;                 // Funktionsname -> Zeilennummer (blockNumber)
    QSet<QString> m_userFunctions;                      // im Dokument gefundene Funktionsnamen
    QHash<QString, QList<QTextCursor>> m_symbolReferences; // Name -> alle Fundstellen im Dokument

    // Import system
    QHash<QString, QStringList> m_importedModules;      // Modulname -> verfügbare Funktionen
    QStringList m_searchPaths;                          // Suchpfade für Module
    QSet<QString> m_loadedFiles;                        // Bereits geladene Dateien (verhindert Zyklen)

    QString m_lastSearchSymbol;     // Das Symbol, das aktuell "aktiv" ist
    int m_lastSearchIndex = -1;     // Index innerhalb der Referenzliste

    // Editor settings
    static constexpr int TAB_STOP_WIDTH = 4;

    void lineNumberAreaPaintEvent(QPaintEvent *event);

    // Inner class for line number area
    class LineNumberArea : public QWidget
    {
    public:
        explicit LineNumberArea(LuaEditor *editor) : QWidget(editor), m_editor(editor) {}

        [[nodiscard]] QSize sizeHint() const override {
            return QSize(m_editor->lineNumberAreaWidth(), 0);
        }

    protected:
        void paintEvent(QPaintEvent *event) override {
            m_editor->lineNumberAreaPaintEvent(event);
        }

    private:
        LuaEditor* m_editor = nullptr;
    };
};