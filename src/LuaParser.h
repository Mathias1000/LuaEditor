#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <QSet>
#include <optional>

// ======================= Symbol-Datenstrukturen =======================

enum class SymbolKind {
    Table,
    Function,   // statisch: "Parent.Child" oder global: "foo"
    Method,     // Doppelpunkt-Methoden: "Parent:New"
    Field,      // Parent.Child = <expr>
    Variable,   // local foo / foo = ...
    Metamethod  // __index, __call, ...
};

struct SourcePos {
    int line = 0;   // 1-basiert
    int column = 0; // 1-basiert (Qt UTF-16 Code units)
};

struct Reference {
    QString qualifiedName; // "GameObject.Position.Reset"
    SourcePos pos;
    bool isDefinition = false;
    QString filePath;
};

struct Symbol {
    SymbolKind kind = SymbolKind::Variable;
    QString name;           // "Reset"
    QString parent;         // "GameObject.Position" oder ""
    bool isMethod = false;  // true bei ":"-Methoden
    QString signature;      // "(a, b)"
    SourcePos pos;          // Def-Position
    QString filePath;       // Quelle
};

// ======================= Symboltabelle =======================

class SymbolTable {
public:
    void clear();

    // Insert
    void addSymbol(const Symbol& s);
    void addReference(const Reference& r);

    // Queries (API)
    QStringList getGlobals() const;
    QStringList getMembers(const QString& parent) const;

    std::optional<Symbol> findDefinition(const QString& name, const QString& parent = {}) const;
    QVector<Reference>    findUsages(const QString& name, const QString& parent = {}) const;

    // Helpers
    static QString qualifiedName(const QString& parent, const QString& name);
    bool isKnownTable(const QString& qname) const;

    // Multi-File merge
    void mergeFrom(const SymbolTable& other);

private:
    QHash<QString, Symbol> m_symbolsByQName;      // QName -> Symbol
    QHash<QString, QSet<QString>> m_children;     // ParentQName -> { member names }
    QSet<QString> m_globals;                      // globale Namen
    QHash<QString, QVector<Reference>> m_usages;  // QName -> Usages
    QSet<QString> m_tables;                       // Menge bekannter Tabellen
};

// ======================= LuaParser (nicht QObject) =======================

class LuaParser {
public:
    LuaParser() = default;

    // Parse eine Datei (Code + Pfad) und mergen in Projektweite Tabelle
    void parseFile(const QString& code, const QString& filePath);
    SymbolTable parseOne(const QString& code, const QString& filePath) const;

    // Projektweite Reparse (resetten)
    void resetProject();



    // Editor-API
    QStringList getGlobals() const { return m_projectTable.getGlobals(); }
    QStringList getMembers(const QString& parent) const { return m_projectTable.getMembers(parent); }

    std::optional<Symbol> findDefinition(const QString& name, const QString& parent = {}) const {
        return m_projectTable.findDefinition(name, parent);
    }
    QVector<Reference> findUsages(const QString& name, const QString& parent = {}) const {
        return m_projectTable.findUsages(name, parent);
    }

    const SymbolTable& symbolTable() const { return m_projectTable; }

private:
    // Einzeldurchlauf für eine Datei → liefert eine SymbolTable; wird in m_projectTable gemerged.

    // Parsing-Schritte
    void parseFunctionDefs(const QString& code, const QString& file, SymbolTable& st) const;
    void parseTablesAndFields(const QString& code, const QString& file, SymbolTable& st) const;
    void parseUsages(const QString& code, const QString& file, SymbolTable& st) const;

    // Helpers
    static SourcePos posFromOffset(const QString& text, int offset);
    static QString parentOfChain(const QString& chain);
    static QString lastOfChain(const QString& chain);

private:
    SymbolTable m_projectTable;
};
