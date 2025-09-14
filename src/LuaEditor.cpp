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
        showCompletion(); // Cursor bewegt → ggf. Completion aktualisieren
    });

    // Navigation Shortcuts
    auto* f12 = new QShortcut(QKeySequence(Qt::Key_F12), this);
    connect(f12, &QShortcut::activated, this, &LuaEditor::findNextReference);

    auto* ctrlF12 = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F12), this);
    connect(ctrlF12, &QShortcut::activated, this, &LuaEditor::goToDefinition);

    // Textänderungen triggern Index & Symbole & evtl. Completion
    connect(this, &QPlainTextEdit::textChanged, this, [this] {
        updateFunctionIndex();
        updateSymbols();
        showCompletion();
    });

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    updateFunctionIndex();
    updateSymbols();
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

    // wenn der Completer ausgewählt hat, in den Editor einfügen
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
    // Tab → Spaces
    if (event->key() == Qt::Key_Tab) {
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

    QPlainTextEdit::keyPressEvent(event);
    showCompletion(); // nach jedem Tastendruck ggf. aktualisieren
}

void LuaEditor::focusInEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusInEvent(event);
    showCompletion();
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

void LuaEditor::insertCompletion(const QString &completion)
{
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor); // falls mitten im Wort
    tc.select(QTextCursor::WordUnderCursor);
    tc.removeSelectedText();
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
            m_lastSearchIndex = (m_lastSearchIndex + 1) % cursors.size();
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
            cursor.setPosition(block.position() + m.capturedStart(1));
            m_symbolReferences[name].append(cursor);
        }

        // Lokale Variablen
        auto vit = kLocalVarRe.globalMatch(text);
        while (vit.hasNext()) {
            const auto m = vit.next();
            const QString name = m.captured(1);
            if (name.isEmpty()) continue;
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + m.capturedStart(1));
            m_symbolReferences[name].append(cursor);
        }

        // self.member
        auto mit = memberRE.globalMatch(text);
        while (mit.hasNext()) {
            const auto m = mit.next();
            const QString name = m.captured(1);
            if (name.isEmpty()) continue;
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + m.capturedStart(1));
            m_symbolReferences[name].append(cursor);
        }

        // Objekt:funktion()
        auto cit = callRE.globalMatch(text);
        while (cit.hasNext()) {
            const auto m = cit.next();
            const QString funcName = m.captured(2);
            if (funcName.isEmpty()) continue;
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + m.capturedStart(2));
            m_symbolReferences[funcName].append(cursor);
        }
    }
}

// ---------- Completion ----------

void LuaEditor::showCompletion()
{
    if (!m_autoCompleter) return;

    const auto items = buildCompletionItems();
    if (items.isEmpty()) {
        m_autoCompleter->hidePopup();
        return;
    }

    m_autoCompleter->updateCompleter(items);
    m_autoCompleter->showPopup(cursorRect());
}

QString LuaEditor::detectChainUnderCursor(QString *trigger) const
{
    // Liefert den Ketten-Präfix (z.B. "GameObject" oder "GameObject.Position")
    // direkt vor '.' oder ':' am Cursor. Optional: trigger = "." oder ":".
    QTextCursor tc = textCursor();
    const int pos = tc.position();

    const QString text = toPlainText();
    if (pos <= 0 || pos > text.size()) return {};

    int i = pos - 1;
    // überspringe Whitespaces nach links
    while (i >= 0 && text.at(i).isSpace()) --i;
    if (i < 0) return {};

    // aktuelles Zeichen muss '.' oder ':' sein
    const QChar ch = text.at(i);
    if (ch != u'.' && ch != u':') return {};

    if (trigger) *trigger = QString(ch);

    // gehe nach links und sammle Kettenstücke
    int end = i - 1;
    int start = end;

    auto isIdentChar = [](QChar c)->bool {
        return c.isLetterOrNumber() || c == u'_';
    };

    QStringList parts;
    while (start >= 0) {
        // skip spaces
        while (start >= 0 && text.at(start).isSpace()) --start;
        if (start < 0) break;

        int identEnd = start;
        while (start >= 0 && isIdentChar(text.at(start))) --start;
        const int identStart = start + 1;
        if (identStart > identEnd) break;

        const QString ident = text.mid(identStart, identEnd - identStart + 1);
        if (ident.isEmpty()) break;
        parts.prepend(ident);

        // nun erwarte entweder Anfang oder ein '.' zwischen Identifikatoren
        int dotIdx = identStart - 1;
        while (dotIdx >= 0 && text.at(dotIdx).isSpace()) --dotIdx;
        if (dotIdx >= 0 && text.at(dotIdx) == u'.') {
            start = dotIdx - 1;
            continue;
        }
        break;
    }

    return parts.join(u'.');
}

QStringList LuaEditor::buildCompletionItems() const
{
    QString trigger;
    const QString chain = detectChainUnderCursor(&trigger);

    // Alle Identifier im Dokument einsammeln (global)
    QSet<QString> all;
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        const QString line = block.text();
        auto it = kIdentRe.globalMatch(line);
        while (it.hasNext()) {
            const auto m = it.next();
            const QString id = m.capturedView(0).toString();
            if (!id.isEmpty())
                all.insert(id);
        }
    }

    // Wenn kein '.' oder ':' → globale Vorschläge
    if (chain.isEmpty() || trigger.isEmpty()) {
        QStringList list = all.values();
        list.sort(Qt::CaseInsensitive);
        return list;
    }

    // Kontext: Kette vor '.' / ':'
    // Wir suchen nach Zuweisungen/Definitionen, die zu diesem Kettenpräfix passen
    // Beispiele:
    //   GameObject = {}
    //   GameObject.Name = ...
    //   GameObject:New = function(...) end
    //   function GameObject.Update(...) end
    //   function GameObject:New(...) end
    //
    // Wir extrahieren für "GameObject" dann "Name", "Update", "New", ...
    const QString parent = chain;

    // Patterns
    QRegularExpression fieldRe(
        QRegularExpression::anchoredPattern(
            QRegularExpression::escape(parent) + R"(\.([A-Za-z_][A-Za-z0-9_]*))"
        )
    );
    QRegularExpression methodRe(
        QRegularExpression::anchoredPattern(
            QRegularExpression::escape(parent) + R"(:([A-Za-z_][A-Za-z0-9_]*))"
        )
    );
    QRegularExpression staticFuncDefRe(
        QRegularExpression::anchoredPattern(
            uR"(function\s+)"_qs +
            QRegularExpression::escape(parent) + R"(\.([A-Za-z_][A-Za-z0-9_]*))"
        )
    );
    QRegularExpression methodFuncDefRe(
        QRegularExpression::anchoredPattern(
            uR"(function\s+)"_qs +
            QRegularExpression::escape(parent) + R"(:([A-Za-z_][A-Za-z0-9_]*))"
        )
    );

    QSet<QString> members;

    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        const QString line = block.text();

        for (auto it = fieldRe.globalMatch(line); it.hasNext();) {
            const auto m = it.next();
            members.insert(m.captured(1));
        }
        for (auto it = methodRe.globalMatch(line); it.hasNext();) {
            const auto m = it.next();
            members.insert(m.captured(1));
        }
        for (auto it = staticFuncDefRe.globalMatch(line); it.hasNext();) {
            const auto m = it.next();
            members.insert(m.captured(1));
        }
        for (auto it = methodFuncDefRe.globalMatch(line); it.hasNext();) {
            const auto m = it.next();
            members.insert(m.captured(1));
        }
    }

    QStringList list = members.values();
    list.sort(Qt::CaseInsensitive);
    return list;
}
