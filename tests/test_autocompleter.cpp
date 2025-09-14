#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include "AutoCompleter.h"
#include "LuaParser.h"

class TestAutoCompleter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testBasicCompletion();
    void testContextualCompletion();
    void testTriggerCharacters();
    void testStringCompletion();
    void testCommentCompletion();
    void testPrefixExtraction();

private:
    std::unique_ptr<AutoCompleter> m_completer;
    std::unique_ptr<LuaParser> m_parser;
};

void TestAutoCompleter::initTestCase()
{
    m_parser = std::make_unique<LuaParser>();
    m_completer = std::make_unique<AutoCompleter>();
    m_completer->setParser(m_parser.get());
}

void TestAutoCompleter::cleanupTestCase()
{
    m_completer.reset();
    m_parser.reset();
}

void TestAutoCompleter::testBasicCompletion()
{
    // Test that basic keywords are available
    const QStringList completions = m_completer->getCompletions("fu");
    QVERIFY(completions.contains("function"));
    
    // Test that built-ins are available
    const QStringList printCompletions = m_completer->getCompletions("pr");
    QVERIFY(printCompletions.contains("print"));
}

void TestAutoCompleter::testContextualCompletion()
{
    const QString code = R"(
        function myFunction(param)
            local localVar = 10
            string.
        end
    )";
    
    // Parse code to get user-defined symbols
    m_parser->parse(code);
    
    // Test contextual completion after "string."
    const QStringList contextual = m_completer->getContextualCompletions(code, code.indexOf("string.") + 7);
    
    // Should include string methods
    bool hasStringMethods = false;
    for (const auto& completion : contextual) {
        if (completion.startsWith("string.")) {
            hasStringMethods = true;
            break;
        }
    }
    QVERIFY(hasStringMethods);
}

void TestAutoCompleter::testTriggerCharacters()
{
    // Test that appropriate characters trigger completion
    QVERIFY(AutoCompleter::shouldTriggerCompletion('a'));
    QVERIFY(AutoCompleter::shouldTriggerCompletion('_'));
    QVERIFY(AutoCompleter::shouldTriggerCompletion('.'));
    
    // Test that inappropriate characters don't trigger completion
    QVERIFY(!AutoCompleter::shouldTriggerCompletion(' '));
    QVERIFY(!AutoCompleter::shouldTriggerCompletion('\n'));
    QVERIFY(!AutoCompleter::shouldTriggerCompletion('('));
}

void TestAutoCompleter::testStringCompletion()
{
    const QString code = R"(local str = "hello )";
    const int cursorPos = code.length();
    
    // Should not complete inside strings
    m_completer->updateCompletions(code, cursorPos);
    QVERIFY(!m_completer->popup()->isVisible());
}

void TestAutoCompleter::testCommentCompletion()
{
    const QString code = R"(-- This is a comment pr)";
    const int cursorPos = code.length();
    
    // Should not complete inside comments
    m_completer->updateCompletions(code, cursorPos);
    QVERIFY(!m_completer->popup()->isVisible());
}

void TestAutoCompleter::testPrefixExtraction()
{
    // Test various prefix extraction scenarios
    const QString code1 = "function myFunc";
    const QString prefix1 = m_completer->getCompletions("myF");
    // This is a basic test - more sophisticated prefix extraction
    // would be tested through the private extractPrefix method
    
    QVERIFY(!prefix1.isEmpty());
}

QTEST_MAIN(TestAutoCompleter)
#include "test_autocompleter.moc"
