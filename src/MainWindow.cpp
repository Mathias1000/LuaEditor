#include "MainWindow.h"
#include "LuaHighlighter.h"

#include <QCloseEvent>
#include <QFileInfo>
#include <QTextStream>
#include <QVBoxLayout>

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

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_editor.get());

    m_symbolsList = new QListWidget(this);
    m_symbolsList->setMaximumWidth(300);
    m_symbolsList->setMinimumWidth(200);
    m_mainSplitter->addWidget(m_symbolsList);

    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);

    auto* layout = new QVBoxLayout(m_centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_mainSplitter);
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
    m_symbolsList->clear();

    // Get globals from parser
    const QStringList globals = m_parser->getGlobals();
    for (const QString& symbol : globals) {
        m_symbolsList->addItem("🌐 " + symbol); // Global symbol
    }

    // You could also add members if you want to show all symbols
    // For now, just showing globals for simplicity
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