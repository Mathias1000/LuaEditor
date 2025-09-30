#include "MainWindow.h"
#include "LuaHighlighter.h"

#include <QCloseEvent>
#include <QFileInfo>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_parser(std::make_shared<LuaParser>())
    , m_editor(std::make_unique<LuaEditor>(m_parser, this))
    , m_completer(std::make_unique<AutoCompleter>(this))
{
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    createConnections();

    // Connect completer to editor - this must happen after construction
    m_completer->setWidget(m_editor.get());
    m_editor->setCompleter(m_completer.get());

    // Add syntax highlighting
    auto* highlighter = new LuaHighlighter(m_editor->document());
    Q_UNUSED(highlighter) // Prevent unused variable warning

    updateWindowTitle();
    updateStatusBar();

    m_editor->setFocus();
    resize(1200, 800);
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);


    // Create symbol panel (horizontal layout below toolbar)
    m_symbolPanel = new QWidget(this);
    m_symbolPanel->setMaximumHeight(50);
    auto* symbolLayout = new QHBoxLayout(m_symbolPanel);
    symbolLayout->setContentsMargins(5, 5, 5, 5);
    symbolLayout->setSpacing(5);

    // LoadSymbol button (green, on the left)
    m_loadSymbolButton = new QPushButton("LoadSymbol", this);
    m_loadSymbolButton->setMinimumHeight(35);
    m_loadSymbolButton->setMaximumWidth(80);
    m_loadSymbolButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2ecc71;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 8px 16px;"
        "   font-weight: bold;"
        "   font-size: 11pt;"
        "}"
        "QPushButton:hover {"
        "   background-color: #27ae60;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #229954;"
        "}"
    );
    symbolLayout->addWidget(m_loadSymbolButton);

    // Globals List
    m_globalsList = new QListWidget(this);
    m_globalsList->setMaximumHeight(80);
    m_globalsList->setStyleSheet(
        "QListWidget {"
        "   background-color: rgba(240, 240, 240, 220);"
        "   border: 1px solid rgba(100, 100, 100, 180);"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "   font-size: 10pt;"
        "}"
        "QListWidget::item {"
        "   padding: 3px;"
        "   color: #000000;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: rgba(100, 150, 200, 120);"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(50, 100, 200, 180);"
        "   color: white;"
        "}"
    );

    // Functions List
    m_functionsList = new QListWidget(this);
    m_functionsList->setMaximumHeight(150);
    m_functionsList->setStyleSheet(
        "QListWidget {"
        "   background-color: rgba(255, 250, 240, 220);"
        "   border: 1px solid rgba(100, 100, 100, 180);"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "   font-size: 10pt;"
        "}"
        "QListWidget::item {"
        "   padding: 3px;"
        "   color: #000000;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: rgba(200, 150, 100, 120);"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(200, 100, 50, 180);"
        "   color: white;"
        "}"
    );

    // Tables List
    m_tablesList = new QListWidget(this);
    m_tablesList->setMaximumHeight(80);
    m_tablesList->setStyleSheet(
        "QListWidget {"
        "   background-color: rgba(240, 255, 240, 220);"
        "   border: 1px solid rgba(100, 100, 100, 180);"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "   font-size: 10pt;"
        "}"
        "QListWidget::item {"
        "   padding: 3px;"
        "   color: #000000;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: rgba(100, 200, 100, 120);"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(50, 150, 50, 180);"
        "   color: white;"
        "}"
    );

    // Add titles/headers as first items
    auto* globalsHeader = new QListWidgetItem("📋 Globals");
    globalsHeader->setFlags(Qt::NoItemFlags);
    globalsHeader->setForeground(QBrush(QColor(50, 50, 50)));
    QFont headerFont = globalsHeader->font();
    headerFont.setBold(true);
    globalsHeader->setFont(headerFont);
    m_globalsList->addItem(globalsHeader);

    auto* functionsHeader = new QListWidgetItem("⚙️ Functions");
    functionsHeader->setFlags(Qt::NoItemFlags);
    functionsHeader->setForeground(QBrush(QColor(50, 50, 50)));
    functionsHeader->setFont(headerFont);
    m_functionsList->addItem(functionsHeader);

    auto* tablesHeader = new QListWidgetItem("📦 Tables");
    tablesHeader->setFlags(Qt::NoItemFlags);
    tablesHeader->setForeground(QBrush(QColor(50, 50, 50)));
    tablesHeader->setFont(headerFont);
    m_tablesList->addItem(tablesHeader);

    // Add lists to layout (equal width distribution)
    symbolLayout->addWidget(m_globalsList, 1);
    symbolLayout->addWidget(m_functionsList, 1);
    symbolLayout->addWidget(m_tablesList, 1);

    // Add symbol panel to main layout
    mainLayout->addWidget(m_symbolPanel);

    // Add editor below
    mainLayout->addWidget(m_editor.get());
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(tr("&File"));

    m_newAction = new QAction(tr("&New"), this);
    m_newAction->setShortcuts(QKeySequence::New);
    fileMenu->addAction(m_newAction);




    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setShortcuts(QKeySequence::Open);
    fileMenu->addAction(m_openAction);

    fileMenu->addSeparator();

    m_saveAction = new QAction(tr("&Save"), this);
    m_saveAction->setShortcuts(QKeySequence::Save);
    fileMenu->addAction(m_saveAction);


    m_saveAsAction = new QAction(tr("Save &As..."), this);
    m_saveAsAction->setShortcuts(QKeySequence::SaveAs);
    fileMenu->addAction(m_saveAsAction);

    fileMenu->addSeparator();

    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcuts(QKeySequence::Quit);
    fileMenu->addAction(m_exitAction);

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    m_aboutAction = new QAction(tr("&About"), this);
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::setupToolBar()
{
    auto* fileToolBar = addToolBar(tr("File"));



    fileToolBar->addAction(m_newAction);
    fileToolBar->addAction(m_openAction);
    fileToolBar->addAction(m_saveAction);



}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel);

    statusBar()->addPermanentWidget(new QLabel(" | ", this));

    m_cursorPosLabel = new QLabel("Line: 1, Col: 1", this);
    statusBar()->addPermanentWidget(m_cursorPosLabel);

    m_parsingProgress = new QProgressBar(this);
    m_parsingProgress->setVisible(false);
    statusBar()->addPermanentWidget(m_parsingProgress);
}

void MainWindow::createConnections()
{
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newFile);
    connect(m_openAction, &QAction::triggered, this, QOverload<>::of(&MainWindow::openFile));
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::about);

    connect(m_editor.get(), &LuaEditor::textChanged, this, &MainWindow::onTextChanged);
    connect(m_editor.get(), &LuaEditor::cursorPositionChanged, this, &MainWindow::onCursorPositionChanged);

    // Connect list item clicks
    connect(m_globalsList, &QListWidget::itemClicked, this, &MainWindow::onGlobalsItemClicked);
    connect(m_functionsList, &QListWidget::itemClicked, this, &MainWindow::onFunctionsItemClicked);
    connect(m_tablesList, &QListWidget::itemClicked, this, &MainWindow::onTablesItemClicked);

    // Connect LoadSymbol button
    connect(m_loadSymbolButton, &QPushButton::clicked, this, &MainWindow::onLoadSymbolClicked);
}

// === File Handling ===

void MainWindow::newFile()
{
    if (maybeSave()) {
        m_editor->clear();
        setCurrentFile(QString());
        updateSymbolsList();
    }
}

void MainWindow::openFile()
{
    if (maybeSave()) {
        const QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Lua File"), QString(),
            tr("Lua Files (*.lua);;All Files (*)"));
        if (!fileName.isEmpty())
            openFile(fileName);
    }
}

void MainWindow::openFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot read file %1:\n%2").arg(filePath, file.errorString()));
        return;
    }

    QTextStream in(&file);
    m_editor->setPlainText(in.readAll());

    setCurrentFile(filePath);
    m_statusLabel->setText(tr("File loaded"));

    // Parse the loaded file
    m_parser->parseFile(m_editor->toPlainText(), filePath);
    updateSymbolsList();
}

bool MainWindow::saveFile()
{
    if (m_currentFile.isEmpty()) {
        return saveFileAs();
    } else {
        return saveDocument(m_currentFile);
    }
}

bool MainWindow::saveFileAs()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save Lua File"), QString(),
        tr("Lua Files (*.lua);;All Files (*)"));
    if (!fileName.isEmpty()) {
        return saveDocument(fileName);
    }
    return false;
}

bool MainWindow::saveDocument(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot write file %1:\n%2").arg(fileName, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();

    setCurrentFile(fileName);
    m_statusLabel->setText(tr("File saved"));
    return true;
}

// === Editor callbacks ===

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Lua AutoComplete"),
        tr("<h2>Lua AutoComplete Editor 1.0</h2>"
           "<p>A modern Qt6-based Lua editor with intelligent autocompletion.</p>"));
}

void MainWindow::onTextChanged()
{
    m_isModified = true;
    updateWindowTitle();

    // Parse the current content
    m_parser->parseFile(m_editor->toPlainText(), m_currentFile.isEmpty() ? "untitled.lua" : m_currentFile);
    updateSymbolsList();

    m_statusLabel->setText(tr("Document modified"));
}

void MainWindow::onCursorPositionChanged()
{
    updateStatusBar();
}

void MainWindow::updateSymbolsList()
{
    // Clear lists (except headers)
    while (m_globalsList->count() > 1) {
        delete m_globalsList->takeItem(1);
    }
    while (m_functionsList->count() > 1) {
        delete m_functionsList->takeItem(1);
    }
    while (m_tablesList->count() > 1) {
        delete m_tablesList->takeItem(1);
    }

    // Get globals from parser
    const QStringList globals = m_parser->getGlobals();

    // Separate into different categories
    for (const QString& symbol : globals) {
        auto def = m_parser->findDefinition(symbol);
        if (def.has_value()) {
            QString displayText = symbol;

            switch (def->kind) {
                case SymbolKind::Function:
                    displayText += " " + def->signature;
                    m_functionsList->addItem(displayText);
                    break;
                case SymbolKind::Method:
                    displayText += " " + def->signature;
                    m_functionsList->addItem(displayText);
                    break;
                case SymbolKind::Table:
                    m_tablesList->addItem(displayText);
                    break;
                case SymbolKind::Variable:
                case SymbolKind::Field:
                    m_globalsList->addItem(displayText);
                    break;
                default:
                    m_globalsList->addItem(displayText);
                    break;
            }
        } else {
            m_globalsList->addItem(symbol);
        }
    }
}

void MainWindow::onGlobalsItemClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags) return; // Skip header

    QString symbolText = item->text();
    symbolText = symbolText.split(' ').first(); // Get just the symbol name

    auto def = m_parser->findDefinition(symbolText);
    if (def.has_value()) {
        QTextBlock block = m_editor->document()->findBlockByNumber(def->pos.line - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            m_editor->setTextCursor(cursor);
            m_editor->centerCursor();
            m_editor->setFocus();
        }
    }
}

void MainWindow::onFunctionsItemClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags) return; // Skip header

    QString symbolText = item->text();
    symbolText = symbolText.split(' ').first(); // Get just the function name

    auto def = m_parser->findDefinition(symbolText);
    if (def.has_value()) {
        QTextBlock block = m_editor->document()->findBlockByNumber(def->pos.line - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            m_editor->setTextCursor(cursor);
            m_editor->centerCursor();
            m_editor->setFocus();
        }
    }
}

void MainWindow::onTablesItemClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags) return; // Skip header

    QString symbolText = item->text();
    symbolText = symbolText.split(' ').first(); // Get just the table name

    auto def = m_parser->findDefinition(symbolText);
    if (def.has_value()) {
        QTextBlock block = m_editor->document()->findBlockByNumber(def->pos.line - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            m_editor->setTextCursor(cursor);
            m_editor->centerCursor();
            m_editor->setFocus();
        }
    }
}

void MainWindow::onLoadSymbolClicked()
{
    // Trigger a full re-parse and update of all symbols
    if (!m_currentFile.isEmpty()) {
        m_parser->resetProject();
        m_parser->parseFile(m_editor->toPlainText(), m_currentFile);
    } else {
        m_parser->resetProject();
        m_parser->parseFile(m_editor->toPlainText(), "untitled.lua");
    }

    updateSymbolsList();
    m_statusLabel->setText(tr("Symbols reloaded"));
}

// === Window helpers ===

void MainWindow::updateWindowTitle()
{
    QString title = QString::fromUtf8(WINDOW_TITLE.data());

    if (!m_currentFile.isEmpty())
        title += " - " + strippedName(m_currentFile);
    else
        title += " - Untitled";

    if (m_isModified)
        title += "*";

    setWindowTitle(title);
}

void MainWindow::updateStatusBar()
{
    const QTextCursor cursor = m_editor->textCursor();
    m_cursorPosLabel->setText(
        tr("Line: %1, Col: %2").arg(cursor.blockNumber() + 1).arg(cursor.columnNumber() + 1));
}

void MainWindow::setCurrentFile(const QString& fileName)
{
    m_currentFile = fileName;
    m_isModified = false;
    updateWindowTitle();

    const QString shown = m_currentFile.isEmpty() ? "untitled.lua" : strippedName(m_currentFile);
    setWindowFilePath(shown);
}

QString MainWindow::strippedName(const QString& fullFileName) const
{
    return QFileInfo(fullFileName).fileName();
}

bool MainWindow::maybeSave()
{
    if (!m_isModified)
        return true;

    const auto ret = QMessageBox::warning(
        this, tr("Lua AutoComplete"),
        tr("The document has been modified.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
    case QMessageBox::Save:
        return saveFile();
    case QMessageBox::Cancel:
        return false;
    default:
        return true;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}