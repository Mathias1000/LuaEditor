#include "LuaEditor.h"
#include "AutoCompleter.h"

#include <QPainter>
#include <QTextBlock>
#include <QFont>
#include <QFontMetrics>
#include <QApplication>
#include <QAbstractItemView>
#include <QStringListModel>
#include <QShortcut>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextDocument>
#include <QCompleter>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace {
    // einfache Identifier-RE
    const QRegularExpression kIdentRe(uR"([A-Za-z_][A-Za-z0-9_]*)"_qs);
    const QRegularExpression kFuncDefRe(uR"(\bfunction\s+([A-Za-z_][A-Za-z0-9_]*)\b)"_qs);
    const QRegularExpression kLocalVarRe(uR"(\blocal\s+([A-Za-z_][A-Za-z0-9_]*)\b)"_qs);
}

LuaEditor::LuaEditor(std::shared_ptr<LuaParser> parser, QWidget* parent)
    : QPlainTextEdit(parent),
      m_parser(std::move(parser)),
      m_lineNumberArea(std::make_unique<LineNumberArea>(this))
{
    setupEditor();
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));

    // Connect signals for line number area
    connect(this, &LuaEditor::blockCountChanged,
            this, &LuaEditor::updateLineNumberAreaWidth);
    connect(this, &LuaEditor::updateRequest,
            this, &LuaEditor::updateLineNumberArea);
    connect(this, &LuaEditor::cursorPositionChanged, this, [this] {
        highlightCurrentLine();
    });

    // Navigation Shortcuts
    auto* f12 = new QShortcut(QKeySequence(Qt::Key_F12), this);
    connect(f12, &QShortcut::activated, this, &LuaEditor::findNextReference);

    auto* ctrlF12 = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F12), this);
    connect(ctrlF12, &QShortcut::activated, this, &LuaEditor::goToDefinition);

    // Textänderungen triggern Index & Symbole
    connect(this, &QPlainTextEdit::textChanged, this, [this] {
        updateFunctionIndex();
        updateSymbols();
        parseImports(); // Parse imports when text changes
    });

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    updateFunctionIndex();
    updateSymbols();

    // Initialize search paths for modules
    m_searchPaths << "." << "./modules" << "./lib" << "./scripts";
    parseImports();
}

void LuaEditor::setupEditor()
{
    QFont font(u"Courier New"_qs, 11);
    font.setFixedPitch(true);
    setFont(font);

    const QFontMetrics metrics(font);
    setTabStopDistance(TAB_STOP_WIDTH * metrics.horizontalAdvance(' '));

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setAcceptDrops(true);
    setTextInteractionFlags(Qt::TextEditorInteraction);
}

void LuaEditor::setCompleter(AutoCompleter* completer)
{
    m_autoCompleter = completer;
    if (!m_autoCompleter) return;

    // Set this editor as the widget for the completer
    if (m_autoCompleter->completer()) {
        m_autoCompleter->completer()->setWidget(this);
        m_autoCompleter->completer()->setCompletionPrefix("");
    }

    // when the completer has selected something, insert it into the editor
    connect(m_autoCompleter, &AutoCompleter::activated,
            this, &LuaEditor::insertCompletion);
}

int LuaEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

QString LuaEditor::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

QString LuaEditor::currentLineText() const
{
    return textCursor().block().text();
}

void LuaEditor::keyPressEvent(QKeyEvent *event)
{
    // Handle completer popup
    if (m_autoCompleter && m_autoCompleter->completer()->popup()->isVisible()) {
        // These keys are forwarded by the completer to the widget
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            event->ignore();
            return; // let the completer do default behavior
        default:
            break;
        }
    }

    // Tab → Spaces
    if (event->key() == Qt::Key_Tab) {
        // If completer is visible, let it handle tab
        if (m_autoCompleter && m_autoCompleter->completer()->popup()->isVisible()) {
            event->ignore();
            return;
        }
        insertPlainText(QString(TAB_STOP_WIDTH, ' '));
        return;
    }

    // Enter/Return: einfache Auto-Einrückung
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QPlainTextEdit::keyPressEvent(event);

        const QString prevLine = textCursor().block().previous().text();
        const QString trimmed  = prevLine.trimmed();
        const QString indent   = prevLine.left(prevLine.length() - trimmed.length());

        QString extraIndent;
        if (trimmed.endsWith(u"then"_qs) ||
            trimmed.endsWith(u"do"_qs) ||
            trimmed.startsWith(u"function"_qs) ||
            trimmed.startsWith(u"if"_qs) ||
            trimmed.startsWith(u"for"_qs) ||
            trimmed.startsWith(u"while"_qs) ||
            trimmed.startsWith(u"repeat"_qs)) {
            extraIndent = QString(TAB_STOP_WIDTH, ' ');
        }

        const QString currentLine = textCursor().block().text().trimmed();
        if (currentLine == u"end"_qs) {
            insertPlainText(indent.left(qMax(0, indent.length() - TAB_STOP_WIDTH)));
            return;
        }

        insertPlainText(indent + extraIndent);
        return;
    }

    // Check if we're typing . or : to trigger immediate completion
    bool triggerCompletion = false;
    if (event->text() == "." || event->text() == ":") {
        triggerCompletion = true;
    }

    // Process the key press
    QPlainTextEdit::keyPressEvent(event);

    // Trigger completion after processing the key
    if (triggerCompletion) {
        // Small delay to allow the character to be inserted first
        QTimer::singleShot(10, this, &LuaEditor::performCompletion);
    } else {
        performCompletion();
    }
}

// ---------- Import System ----------

void LuaEditor::parseImports()
{
    // Clear previous imports to reparse
    m_importedModules.clear();
    m_loadedFiles.clear();

    // Find require() statements in current document
    QRegularExpression requireRe(R"(\b(\w+)\s*=\s*require\s*\(\s*['"](.*?)['"]\s*\))");
    QRegularExpression directRequireRe(R"(\brequire\s*\(\s*['"](.*?)['"]\s*\))");

    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        const QString line = block.text();

        // Pattern: local myModule = require("modulename")
        auto it = requireRe.globalMatch(line);
        while (it.hasNext()) {
            const auto match = it.next();
            const QString moduleName = match.captured(1);      // myModule
            const QString fileName = match.captured(2);        // modulename

            qDebug() << "Found require:" << moduleName << "=" << fileName;

            // Load the module file and parse its functions
            QStringList functions = loadModuleFunctions(fileName);
            if (!functions.isEmpty()) {
                m_importedModules[moduleName] = functions;
                qDebug() << "Loaded" << functions.size() << "functions from" << fileName;
            }
        }

        // Pattern: require("modulename") -- without assignment
        auto directIt = directRequireRe.globalMatch(line);
        while (directIt.hasNext()) {
            const auto match = directIt.next();
            const QString fileName = match.captured(1);

            qDebug() << "Found direct require:" << fileName;

            // Load functions into global scope
            QStringList functions = loadModuleFunctions(fileName);
            if (!functions.isEmpty()) {
                m_importedModules["_global"] = m_importedModules.value("_global") + functions;
                qDebug() << "Added" << functions.size() << "global functions from" << fileName;
            }
        }
    }
}

QStringList LuaEditor::loadModuleFunctions(const QString& moduleName)
{
    QStringList functions;

    // Try different file extensions and paths
    QStringList extensions = {".lua", ""};

    for (const QString& searchPath : m_searchPaths) {
        for (const QString& ext : extensions) {
            QString fullPath = QDir(searchPath).filePath(moduleName + ext);
            QFileInfo fileInfo(fullPath);

            if (fileInfo.exists() && fileInfo.isReadable()) {
                QString absolutePath = fileInfo.absoluteFilePath();

                // Avoid loading the same file multiple times
                if (m_loadedFiles.contains(absolutePath)) {
                    qDebug() << "File already loaded:" << absolutePath;
                    return m_importedModules.values().first(); // Return cached
                }

                qDebug() << "Loading module from:" << absolutePath;
                m_loadedFiles.insert(absolutePath);

                QFile file(absolutePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream stream(&file);
                    QString content = stream.readAll();

                    functions = parseModuleFunctions(content, moduleName);
                    if (!functions.isEmpty()) {
                        return functions;
                    }
                }
            }
        }
    }

    qDebug() << "Module not found:" << moduleName;
    return functions;
}

QStringList LuaEditor::parseModuleFunctions(const QString& content, const QString& moduleName)
{
    QStringList functions;
    QSet<QString> uniqueFunctions;

    // Parse different function definition patterns
    QStringList lines = content.split('\n');

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();

        // Pattern 1: function functionName()
        QRegularExpression funcRe(R"(\bfunction\s+([A-Za-z_][A-Za-z0-9_]*)\s*\()");
        auto funcMatch = funcRe.match(trimmedLine);
        if (funcMatch.hasMatch()) {
            const QString funcName = funcMatch.captured(1);
            uniqueFunctions.insert(funcName);
            qDebug() << "Found function:" << funcName;
            continue;
        }

        // Pattern 2: local function functionName()
        QRegularExpression localFuncRe(R"(\blocal\s+function\s+([A-Za-z_][A-Za-z0-9_]*)\s*\()");
        auto localMatch = localFuncRe.match(trimmedLine);
        if (localMatch.hasMatch()) {
            const QString funcName = localMatch.captured(1);
            uniqueFunctions.insert(funcName);
            qDebug() << "Found local function:" << funcName;
            continue;
        }

        // Pattern 3: M.functionName = function() (module pattern)
        QRegularExpression moduleMethodRe(R"(\b([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*)\s*=\s*function)");
        auto moduleMatch = moduleMethodRe.match(trimmedLine);
        if (moduleMatch.hasMatch()) {
            const QString methodName = moduleMatch.captured(2);
            uniqueFunctions.insert(methodName);
            qDebug() << "Found module method:" << methodName;
            continue;
        }

        // Pattern 4: return { func1 = func1, func2 = func2 } (common in modules)
        if (trimmedLine.startsWith("return {")) {
            QRegularExpression returnRe(R"(([A-Za-z_][A-Za-z0-9_]*)\s*=)");
            auto returnIt = returnRe.globalMatch(trimmedLine);
            while (returnIt.hasNext()) {
                const auto match = returnIt.next();
                const QString exportedName = match.captured(1);
                uniqueFunctions.insert(exportedName);
                qDebug() << "Found exported function:" << exportedName;
            }
        }
    }

    functions = uniqueFunctions.values();
    functions.sort(Qt::CaseInsensitive);

    return functions;
}

void LuaEditor::focusInEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusInEvent(event);
    performCompletion();
}

void LuaEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void LuaEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
}

void LuaEditor::performCompletion()
{
    if (!m_autoCompleter || !m_autoCompleter->completer()) return;

    // Check if we're right after a . or :
    QString trigger;
    QString chain = detectChainUnderCursor(&trigger);

    QString completionPrefix;
    bool showImmediately = false;

    // Debug: let's see what we detect
    // qDebug() << "Chain:" << chain << "Trigger:" << trigger;

    // If we have a chain (like "monster."), show all members immediately
    if (!chain.isEmpty() && !trigger.isEmpty()) {
        // We're right after . or :, show all available members
        completionPrefix = "";
        showImmediately = true;
    } else {
        // Otherwise, use the current word as prefix
        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        completionPrefix = tc.selectedText();

        // Don't show completion for very short prefixes
        if (completionPrefix.length() < 1) {
            m_autoCompleter->hidePopup();
            return;
        }
    }

    // Build completion list based on context
    const auto items = buildCompletionItems();
    if (items.isEmpty()) {
        m_autoCompleter->hidePopup();
        return;
    }

    // Filter out the current word from completion list to avoid self-completion
    QStringList filteredItems = items;
    if (!completionPrefix.isEmpty() && !showImmediately) {
        filteredItems.removeAll(completionPrefix);
    }

    // Update the completer model with filtered items
    m_autoCompleter->updateCompleter(filteredItems);

    // Set the completion prefix
    m_autoCompleter->completer()->setCompletionPrefix(completionPrefix);

    // Show popup if we have matches or if we should show immediately
    if (m_autoCompleter->completer()->completionCount() > 0 || showImmediately) {
        QRect cr = cursorRect();
        cr.setWidth(300); // Fixed width for now
        m_autoCompleter->completer()->complete(cr);
    } else {
        m_autoCompleter->hidePopup();
    }
}

void LuaEditor::insertCompletion(const QString &completion)
{
    if (!m_autoCompleter || !m_autoCompleter->completer()) return;

    QTextCursor tc = textCursor();

    // Select and replace the current word
    tc.select(QTextCursor::WordUnderCursor);
    tc.insertText(completion);

    setTextCursor(tc);
}

void LuaEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LuaEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(Qt::yellow).lighter(160));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void LuaEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

QString LuaEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void LuaEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea.get());
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();
    int blockNumber  = block.blockNumber();
    int top          = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom       = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, m_lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ---------- Navigation & Index ----------

void LuaEditor::updateFunctionIndex()
{
    m_functionIndex.clear();
    m_userFunctions.clear();

    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        const QString line = block.text();
        auto it = kFuncDefRe.globalMatch(line);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const QString name = m.captured(1);
            if (!name.isEmpty()) {
                m_functionIndex.insert(name, block.blockNumber());
                m_userFunctions.insert(name);
            }
        }
    }
}

void LuaEditor::goToDefinition()
{
    const QString ident = wordUnderCursor();
    if (ident.isEmpty()) return;

    const auto it = m_functionIndex.constFind(ident);
    if (it == m_functionIndex.constEnd()) return;

    QTextBlock block = document()->findBlockByNumber(it.value());
    if (!block.isValid()) return;

    QTextCursor target(block);
    setTextCursor(target);
    centerCursor();
}

void LuaEditor::findNextReference()
{
    QString ident = wordUnderCursor();

    if (m_lastSearchSymbol.isEmpty() || ident != m_lastSearchSymbol) {
        m_lastSearchSymbol = ident;
        m_lastSearchIndex = -1;
    }
    if (m_lastSearchSymbol.isEmpty()) return;

    QList<QTextEdit::ExtraSelection> selections;

    if (m_symbolReferences.contains(m_lastSearchSymbol)) {
        const auto &cursors = m_symbolReferences[m_lastSearchSymbol];
        for (const QTextCursor &c : cursors) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = c;
            sel.format.setBackground(QColor(Qt::cyan).lighter(160));
            selections.append(sel);
        }

        if (!cursors.isEmpty()) {
            m_lastSearchIndex = (m_lastSearchIndex + 1) % static_cast<int>(cursors.size());
            QTextCursor target = cursors[m_lastSearchIndex];
            setTextCursor(target);
            centerCursor();
        }
    }

    QTextEdit::ExtraSelection lineSel;
    lineSel.format.setBackground(QColor(Qt::yellow).lighter(160));
    lineSel.format.setProperty(QTextFormat::FullWidthSelection, true);
    lineSel.cursor = textCursor();
    lineSel.cursor.clearSelection();
    selections.append(lineSel);

    setExtraSelections(selections);
}

void LuaEditor::updateSymbols()
{
    m_symbolReferences.clear();

    QTextDocument *doc = document();

    // self.member
    static const QRegularExpression memberRE(
        uR"(\bself\.([A-Za-z_][A-Za-z0-9_]*))"_qs
    );
    // Objekt:funktion()
    static const QRegularExpression callRE(
        uR"(([A-Za-z_][A-Za-z0-9_]*)(?::([A-Za-z_][A-Za-z0-9_]*))\s*\()"_qs
    );

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        const QString text = block.text();

        // Funktionen (Definitionen)
        auto fit = kFuncDefRe.globalMatch(text);
        while (fit.hasNext()) {
            const auto m = fit.next();
            const QString name = m.captured(1);
            if (name.isEmpty()) continue;
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + static_cast<int>(m.capturedStart(1)));
            m_symbolReferences[name].append(cursor);
        }

        // Lokale Variablen
        auto vit = kLocalVarRe.globalMatch(text);
        while (vit.hasNext()) {
            const auto m = vit.next();
            const QString name = m.captured(1);
            if (name.isEmpty()) continue;
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + static_cast<int>(m.capturedStart(1)));
            m_symbolReferences[name].append(cursor);
        }

        // self.member
        auto mit = memberRE.globalMatch(text);
        while (mit.hasNext()) {
            const auto m = mit.next();
            const QString name = m.captured(1);
            if (name.isEmpty()) continue;
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + static_cast<int>(m.capturedStart(1)));
            m_symbolReferences[name].append(cursor);
        }

        // Objekt:funktion()
        auto cit = callRE.globalMatch(text);
        while (cit.hasNext()) {
            const auto m = cit.next();
            const QString funcName = m.captured(2);
            if (funcName.isEmpty()) continue;
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + static_cast<int>(m.capturedStart(2)));
            m_symbolReferences[funcName].append(cursor);
        }
    }
}

// ---------- Completion ----------

void LuaEditor::showCompletion()
{
    performCompletion();
}

QString LuaEditor::detectChainUnderCursor(QString *trigger) const
{
    QTextCursor tc = textCursor();
    const int pos = tc.position();
    const QString text = toPlainText();

    if (pos <= 0 || pos > text.size()) return {};

    // Prüfe ob wir direkt nach einem . oder : stehen
    int checkPos = pos - 1;
    if (checkPos >= 0 && checkPos < text.size()) {
        const QChar prevChar = text.at(checkPos);
        if (prevChar == u'.' || prevChar == u':') {
            if (trigger) *trigger = QString(prevChar);

            // Gehe zurück und sammle die Kette vor dem . oder :
            int end = checkPos - 1;
            int start = end;

            auto isIdentChar = [](QChar c) -> bool {
                return c.isLetterOrNumber() || c == u'_';
            };

            QStringList parts;
            while (start >= 0) {
                // Überspringe Whitespace
                while (start >= 0 && text.at(start).isSpace()) --start;
                if (start < 0) break;

                // Sammle Identifier
                int identEnd = start;
                while (start >= 0 && isIdentChar(text.at(start))) --start;
                const int identStart = start + 1;

                if (identStart <= identEnd) {
                    const QString ident = text.mid(identStart, identEnd - identStart + 1);
                    if (!ident.isEmpty()) {
                        parts.prepend(ident);
                    }
                }

                // Prüfe auf weiteren . oder :
                while (start >= 0 && text.at(start).isSpace()) --start;
                if (start >= 0 && (text.at(start) == u'.' || text.at(start) == u':')) {
                    --start;
                    continue;
                }
                break;
            }

            return parts.join(u'.');
        }
    }

    return {};
}

QStringList LuaEditor::buildCompletionItems() const
{
    QString trigger;
    const QString chain = detectChainUnderCursor(&trigger);

    qDebug() << "buildCompletionItems - Chain:" << chain << "Trigger:" << trigger;

    // Wenn kein '.' oder ':' → nur globale Vorschläge (keine Member)
    if (chain.isEmpty() || trigger.isEmpty()) {
        // Alle Identifier im Dokument einsammeln (global), aber KEINE Member
        QSet<QString> all;

        // Nur Variablennamen und Funktionen, keine Member-Zuweisungen
        for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
            const QString line = block.text().trimmed();

            // Überspringe Zeilen die Member-Zuweisungen sind (enthalten '.')
            if (line.contains('.') && line.contains('=')) {
                continue;  // Keine Member vorschlagen
            }

            // Nur lokale Variablen und Funktionsnamen
            QRegularExpression localVarRe(R"(\blocal\s+([A-Za-z_][A-Za-z0-9_]*))");
            auto localIt = localVarRe.globalMatch(line);
            while (localIt.hasNext()) {
                const auto m = localIt.next();
                const QString varName = m.captured(1);
                all.insert(varName);
            }

            // Funktionsdefinitionen
            QRegularExpression funcRe(R"(\bfunction\s+([A-Za-z_][A-Za-z0-9_]*))");
            auto funcIt = funcRe.globalMatch(line);
            while (funcIt.hasNext()) {
                const auto m = funcIt.next();
                const QString funcName = m.captured(1);
                if (!funcName.contains('.') && !funcName.contains(':')) {
                    all.insert(funcName);
                }
            }

            // Einfache Variablenzuweisungen (ohne Punkt)
            QRegularExpression simpleVarRe(R"(^([A-Za-z_][A-Za-z0-9_]*)\s*=)");
            auto varIt = simpleVarRe.globalMatch(line);
            while (varIt.hasNext()) {
                const auto m = varIt.next();
                const QString varName = m.captured(1);
                all.insert(varName);
            }
        }

        // Lua-Standardfunktionen hinzufügen
        QStringList luaBuiltins = {
            // Basisfunktionen
            "assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs",
            "load", "loadfile", "next", "pairs", "pcall", "print", "rawequal", "rawget",
            "rawlen", "rawset", "require", "select", "setmetatable", "tonumber",
            "tostring", "type", "xpcall", "_G", "_VERSION",
            // Lua-Keywords
            "and", "break", "do", "else", "elseif", "end", "false", "for",
            "function", "if", "in", "local", "nil", "not", "or", "repeat",
            "return", "then", "true", "until", "while", "goto",
            // Standard-Libraries
            "table", "string", "math", "os", "io", "debug", "coroutine"
        };

        for (const QString& builtin : luaBuiltins) {
            all.insert(builtin);
        }

        // Add imported global functions
        if (m_importedModules.contains("_global")) {
            const QStringList globalImports = m_importedModules.value("_global");
            for (const QString& func : globalImports) {
                all.insert(func);
            }
        }

        // Add all imported module names (so they can be used for completion)
        for (auto it = m_importedModules.constBegin(); it != m_importedModules.constEnd(); ++it) {
            if (it.key() != "_global") {
                all.insert(it.key());
            }
        }

        QStringList list = all.values();
        list.sort(Qt::CaseInsensitive);
        qDebug() << "Returning global suggestions:" << list.size() << "items";
        return list;
    }

    qDebug() << "Looking for members of:" << chain;

    // Nur bei '.' oder ':' → Member/Methoden vorschlagen
    const QString parent = chain;
    const bool isMethodCall = (trigger == ":");

    qDebug() << "IsMethodCall:" << isMethodCall;

    QSet<QString> members;

    // Check if parent is an imported module
    if (m_importedModules.contains(parent) && !isMethodCall) {
        const QStringList moduleFunctions = m_importedModules.value(parent);
        for (const QString& func : moduleFunctions) {
            members.insert(func);
            qDebug() << "Added imported function:" << func;
        }
    }

    // Scan durch das Dokument für Member/Methoden
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        const QString line = block.text();
        qDebug() << "Scanning line:" << line;

        // Einfache Patterns - suche nach parent.member oder parent:method
        if (!isMethodCall && line.contains(parent + ".")) {
            // Suche parent.something
            QRegularExpression simpleFieldRe(QRegularExpression::escape(parent) + R"(\.([A-Za-z_][A-Za-z0-9_]*))");
            auto it = simpleFieldRe.globalMatch(line);
            while (it.hasNext()) {
                const auto m = it.next();
                const QString memberName = m.captured(1);
                qDebug() << "Found member:" << memberName;
                members.insert(memberName);
            }
        }

        if (isMethodCall && line.contains(parent + ":")) {
            // Suche parent:method
            QRegularExpression simpleMethodRe(QRegularExpression::escape(parent) + R"(:([A-Za-z_][A-Za-z0-9_]*))");
            auto it = simpleMethodRe.globalMatch(line);
            while (it.hasNext()) {
                const auto m = it.next();
                const QString methodName = m.captured(1);
                qDebug() << "Found method:" << methodName;
                members.insert(methodName);
            }
        }
    }

    // Standard-Library-Methoden hinzufügen basierend auf dem Parent
    if (parent == "string" && !isMethodCall) {
        QStringList stringMethods = {"find", "gsub", "len", "sub", "upper", "lower",
                                   "format", "match", "gmatch", "rep", "reverse", "byte", "char"};
        for (const QString& method : stringMethods) {
            members.insert(method);
        }
    }
    else if (parent == "table" && !isMethodCall) {
        QStringList tableMethods = {"insert", "remove", "concat", "sort", "unpack", "pack", "move"};
        for (const QString& method : tableMethods) {
            members.insert(method);
        }
    }
    else if (parent == "math" && !isMethodCall) {
        QStringList mathMethods = {"abs", "ceil", "floor", "max", "min", "random", "sqrt",
                                 "sin", "cos", "tan", "log", "exp", "deg", "rad", "modf", "fmod"};
        for (const QString& method : mathMethods) {
            members.insert(method);
        }
    }
    else if (parent == "os" && !isMethodCall) {
        QStringList osMethods = {"time", "date", "clock", "execute", "exit", "getenv", "remove", "rename"};
        for (const QString& method : osMethods) {
            members.insert(method);
        }
    }

    // Bei Methoden-Aufruf (:) auch häufige Lua-Methoden hinzufügen
    if (isMethodCall) {
        members.insert("new");
        members.insert("init");
        members.insert("update");
        members.insert("destroy");
        members.insert("toString");
        members.insert("getName");
        members.insert("setName");
        members.insert("getType");
        members.insert("clone");
    }

    QStringList list = members.values();
    list.sort(Qt::CaseInsensitive);
    qDebug() << "Returning" << list.size() << "members:" << list;
    return list;
}