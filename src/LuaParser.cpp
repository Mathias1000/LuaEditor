#include "LuaParser.h"
#include <QRegularExpression>
#include <algorithm>

// ================= SymbolTable =================

void SymbolTable::clear() {
    m_symbolsByQName.clear();
    m_children.clear();
    m_globals.clear();
    m_usages.clear();
    m_tables.clear();
}

QString SymbolTable::qualifiedName(const QString& parent, const QString& name) {
    if (parent.isEmpty()) return name;
    if (name.isEmpty())   return parent;
    return parent + '.' + name;
}

void SymbolTable::addSymbol(const Symbol& s) {
    const QString q = qualifiedName(s.parent, s.name);
    m_symbolsByQName.insert(q, s);
    m_children[s.parent].insert(s.name);
    if (s.parent.isEmpty())
        m_globals.insert(s.name);
    if (s.kind == SymbolKind::Table)
        m_tables.insert(q);
}

void SymbolTable::addReference(const Reference& r) {
    m_usages[r.qualifiedName].push_back(r);
}

QStringList SymbolTable::getGlobals() const {
    auto vals = m_globals.values();
    std::sort(vals.begin(), vals.end(), [](const QString& a, const QString& b){ return a.localeAwareCompare(b) < 0; });
    return vals;
}

QStringList SymbolTable::getMembers(const QString& parent) const {
    if (!m_children.contains(parent)) return {};
    auto vals = m_children.value(parent).values();
    std::sort(vals.begin(), vals.end(), [](const QString& a, const QString& b){ return a.localeAwareCompare(b) < 0; });
    return vals;
}

std::optional<Symbol> SymbolTable::findDefinition(const QString& name, const QString& parent) const {
    const QString q = qualifiedName(parent, name);
    if (m_symbolsByQName.contains(q)) return m_symbolsByQName.value(q);
    if (parent.isEmpty() && m_symbolsByQName.contains(name)) return m_symbolsByQName.value(name);
    return std::nullopt;
}

QVector<Reference> SymbolTable::findUsages(const QString& name, const QString& parent) const {
    const QString q = qualifiedName(parent, name);
    auto it = m_usages.find(q);
    if (it == m_usages.end()) return {};
    return it.value();
}

bool SymbolTable::isKnownTable(const QString& qname) const {
    return m_tables.contains(qname);
}

void SymbolTable::mergeFrom(const SymbolTable& other) {
    for (auto it = other.m_symbolsByQName.constBegin(); it != other.m_symbolsByQName.constEnd(); ++it) {
        m_symbolsByQName[it.key()] = it.value();
    }
    for (auto it = other.m_children.constBegin(); it != other.m_children.constEnd(); ++it) {
        m_children[it.key()].unite(it.value());
    }
    m_globals.unite(other.m_globals);
    for (auto it = other.m_usages.constBegin(); it != other.m_usages.constEnd(); ++it) {
        auto& vec = m_usages[it.key()];
        vec += it.value();
    }
    m_tables.unite(other.m_tables);
}

// ================= LuaParser =================

void LuaParser::parseFile(const QString& code, const QString& filePath) {
    SymbolTable single = parseOne(code, filePath);
    m_projectTable.mergeFrom(single);
}

void LuaParser::resetProject() {
    m_projectTable.clear();
}

SymbolTable LuaParser::parseOne(const QString& code, const QString& filePath) const {
    SymbolTable st;
    parseFunctionDefs(code, filePath, st);
    parseTablesAndFields(code, filePath, st);
    parseUsages(code, filePath, st);
    return st;
}

// ----- Helpers -----

SourcePos LuaParser::posFromOffset(const QString& text, int offset) {
    // Defensive clamps gegen Out-of-Range
    const int sz = static_cast<int>(text.size());
    const int lim = std::clamp(offset, 0, sz);

    SourcePos p{1,1};
    int lastNl = -1;
    for (int i = 0; i < lim; ++i) {
        if (text.at(i) == QChar('\n')) {
            ++p.line;
            lastNl = i;
        }
    }
    p.column = lim - lastNl;
    return p;
}

QString LuaParser::parentOfChain(const QString& chain) {
    const int idx = static_cast<int>(chain.lastIndexOf('.'));
    return (idx < 0) ? QString() : chain.left(idx);
}
QString LuaParser::lastOfChain(const QString& chain) {
    const int idx = static_cast<int>(chain.lastIndexOf('.'));
    return (idx < 0) ? chain : chain.mid(idx+1);
}

// ----- Regex-Bausteine -----

static const QRegularExpression RX_FUNC_DEF_1(
    R"(\bfunction\s+([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|:)[A-Za-z_][A-Za-z0-9_]*)*)\s*\(([^)]*)\))",
    QRegularExpression::MultilineOption);

static const QRegularExpression RX_FUNC_DEF_2(
    R"(\b([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)\s*=\s*function\s*\(([^)]*)\))",
    QRegularExpression::MultilineOption);

static const QRegularExpression RX_TABLE_ASSIGN(
    R"(\b([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)\s*=\s*\{)",
    QRegularExpression::MultilineOption);

static const QRegularExpression RX_FIELD_ASSIGN(
    R"(\b([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)\s*=\s*(?!function\b|\{))",
    QRegularExpression::MultilineOption);

static const QRegularExpression RX_CALL_CHAIN(
    R"(\b([A-Za-z_][A-Za-z0-9_]*)(?:(?::|\.)[A-Za-z_][A-Za-z0-9_]*)*\s*\()",
    QRegularExpression::MultilineOption);

static const QRegularExpression RX_ANY_CHAIN(
    R"(\b([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)\b)",
    QRegularExpression::MultilineOption);

// ----- Parsen: Funktionsdefinitionen -----

void LuaParser::parseFunctionDefs(const QString& code, const QString& file, SymbolTable& st) const {
    auto scan = [&](const QRegularExpression& rx) {
        auto it = rx.globalMatch(code);
        while (it.hasNext()) {
            const auto m = it.next();
            const QString full = m.captured(1);      // "GameObject:New" | "GameObject.Update" | "foo" | "A.B.C"
            const QString params = m.captured(2);
            const int start = static_cast<int>(m.capturedStart(1));

            const bool isMethod = full.contains(':');
            const int lastSep = static_cast<int>(full.lastIndexOf(isMethod ? QChar(':') : QChar('.')));

            QString parent, name;
            if (lastSep >= 0) {
                parent = full.left(lastSep);
                name = full.mid(lastSep + 1);
            } else {
                name = full;
                parent.clear();
            }

            Symbol s;
            s.kind = isMethod ? SymbolKind::Method : SymbolKind::Function;
            s.name = name;
            s.parent = parent;
            s.isMethod = isMethod;
            s.signature = "(" + QString(params).trimmed() + ")";
            s.pos = posFromOffset(code, start);
            s.filePath = file;

            if (s.name.startsWith("__"))
                s.kind = SymbolKind::Metamethod;

            st.addSymbol(s);

            Reference r{ SymbolTable::qualifiedName(parent, name), s.pos, true, file };
            st.addReference(r);
        }
    };

    scan(RX_FUNC_DEF_1);
    scan(RX_FUNC_DEF_2);
}

// ----- Parsen: Tabellen & Felder -----

void LuaParser::parseTablesAndFields(const QString& code, const QString& file, SymbolTable& st) const {
    // Tables: <Chain> = {
    {
        auto it = RX_TABLE_ASSIGN.globalMatch(code);
        while (it.hasNext()) {
            const auto m = it.next();
            const QString chain = m.captured(1);
            const int start = static_cast<int>(m.capturedStart(1));

            Symbol s;
            s.kind = SymbolKind::Table;
            s.name = lastOfChain(chain);
            s.parent = parentOfChain(chain);
            s.pos = posFromOffset(code, start);
            s.filePath = file;
            st.addSymbol(s);

            st.addReference({ SymbolTable::qualifiedName(s.parent, s.name), s.pos, true, file });
        }
    }

    // Fields: <Chain> = <expr> (ohne function/ohne { )
    {
        auto it = RX_FIELD_ASSIGN.globalMatch(code);
        while (it.hasNext()) {
            const auto m = it.next();
            const QString chain = m.captured(1);
            const int start = static_cast<int>(m.capturedStart(1));

            Symbol s;
            s.kind = SymbolKind::Field;
            s.name = lastOfChain(chain);
            s.parent = parentOfChain(chain);
            s.pos = posFromOffset(code, start);
            s.filePath = file;

            if (s.name.startsWith("__"))
                s.kind = SymbolKind::Metamethod;

            st.addSymbol(s);
            st.addReference({ SymbolTable::qualifiedName(s.parent, s.name), s.pos, true, file });
        }
    }

    // (Optional) Keys innerhalb von { ... } – heuristisch auszulösen wäre möglich,
    // aber für den ersten stabilen Wurf nicht nötig.
}

// ----- Parsen: generische Verwendungen -----

void LuaParser::parseUsages(const QString& code, const QString& file, SymbolTable& st) const {
    // Aufrufe: A:B(...), A.B(...), foo(...)
    {
        auto it = RX_CALL_CHAIN.globalMatch(code);
        while (it.hasNext()) {
            const auto m = it.next();
            QString whole = m.captured(0);
            const int start = static_cast<int>(m.capturedStart(0));

            if (whole.endsWith('(')) whole.chop(1);
            whole.replace(':', '.');
            whole = whole.trimmed();

            st.addReference({ whole, posFromOffset(code, start), false, file });
        }
    }

    // Standalone-Ketten (Identifier, Memberketten)
    {
        auto it = RX_ANY_CHAIN.globalMatch(code);
        while (it.hasNext()) {
            const auto m = it.next();
            const QString chain = m.captured(1);
            const int start = static_cast<int>(m.capturedStart(1));
            st.addReference({ chain, posFromOffset(code, start), false, file });
        }
    }
}
