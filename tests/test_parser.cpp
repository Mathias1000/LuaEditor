#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include "LuaParser.h"

class TestLuaParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testBasicParsing();
    void testFunctionExtraction();
    void testVariableExtraction();
    void testTableExtraction();
    void testErrorDetection();
    void testCompletions();
    void testBuiltinSymbols();

private:
    std::unique_ptr<LuaParser> m_parser;
};

void TestLuaParser::initTestCase()
{
    m_parser = std::make_unique<LuaParser>();
}

void TestLuaParser::cleanupTestCase()
{
    m_parser.reset();
}

void TestLuaParser::testBasicParsing()
{
    const QString code = R"(
        local x = 10
        function test()
            return x
        end
    )";
    
    const bool result = m_parser->parse(code);
    QVERIFY(result);
    QVERIFY(!m_parser->hasErrors());
}

void TestLuaParser::testFunctionExtraction()
{
    const QString code = R"(
        function hello(name)
            print("Hello, " .. name)
        end
        
        local function localFunc(a, b)
            return a + b
        end
    )";
    
    const bool result = m_parser->parse(code);
    QVERIFY(result);
    
    const auto symbols = m_parser->getSymbols();
    
    // Should find at least the functions
    bool foundHello = false;
    bool foundLocalFunc = false;
    
    for (const auto& symbol : symbols) {
        if (symbol == "hello") {
            foundHello = true;
        }
        if (symbol == "localFunc") {
            foundLocalFunc = true;
        }
    }
    
    QVERIFY(foundHello);
    QVERIFY(foundLocalFunc);
}

void TestLuaParser::testVariableExtraction()
{
    const QString code = R"(
        local myVar = 42
        local anotherVar = "hello"
        globalVar = true
    )";
    
    const bool result = m_parser->parse(code);
    QVERIFY(result);
    
    const auto symbols = m_parser->getSymbols();
    
    // Should find local variables
    bool foundMyVar = false;
    bool foundAnotherVar = false;
    
    for (const auto& symbol : symbols) {
        if (symbol == "myVar") {
            foundMyVar = true;
        }
        if (symbol == "anotherVar") {
            foundAnotherVar = true;
        }
    }
    
    QVERIFY(foundMyVar);
    QVERIFY(foundAnotherVar);
}

void TestLuaParser::testTableExtraction()
{
    const QString code = R"(
        local myTable = {
            key1 = "value1",
            key2 = 42
        }
        
        anotherTable = {}
    )";
    
    const bool result = m_parser->parse(code);
    QVERIFY(result);
    
    const auto symbols = m_parser->getSymbols();
    
    // Should find tables
    bool foundMyTable = false;
    bool foundAnotherTable = false;
    
    for (const auto& symbol : symbols) {
        if (symbol == "myTable") {
            foundMyTable = true;
        }
        if (symbol == "anotherTable") {
            foundAnotherTable = true;
        }
    }
    
    QVERIFY(foundMyTable);
    QVERIFY(foundAnotherTable);
}

void TestLuaParser::testErrorDetection()
{
    const QString invalidCode = R"(
        function incomplete(
            -- Missing closing parenthesis and end
    )";
    
    const bool result = m_parser->parse(invalidCode);
    QVERIFY(!result);
    QVERIFY(m_parser->hasErrors());
    
    const auto errors = m_parser->getErrors();
    QVERIFY(!errors.empty());
}

void TestLuaParser::testCompletions()
{
    const QString code = R"(
        function myFunction(param1, param2)
            local localVar = 10
            return localVar
        end
    )";
    
    m_parser->parse(code);
    
    // Test prefix completion
    const QStringList completions = m_parser->getCompletions("my");
    QVERIFY(completions.contains("myFunction"));
    
    // Test empty prefix (should return all symbols)
    const QStringList allCompletions = m_parser->getCompletions("");
    QVERIFY(allCompletions.size() > 0);
}

void TestLuaParser::testBuiltinSymbols()
{
    // Parse empty code to get built-in symbols
    m_parser->parse("");
    
    const auto symbols = m_parser->getSymbols();
    
    // Should contain built-in functions
    bool foundPrint = false;
    bool foundType = false;
    
    for (const auto& symbol : symbols) {
        if (symbol == "print") {
            foundPrint = true;
        }
        if (symbol == "type") {
            foundType = true;
        }
    }
    
    QVERIFY(foundPrint);
    QVERIFY(foundType);
}

QTEST_MAIN(TestLuaParser)
#include "test_parser.moc"
