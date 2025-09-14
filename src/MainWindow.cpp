#include "MainWindow.h"
#include "LuaHighlighter.h"

#include <QCloseEvent>
#include <QFileInfo>
#include <QTextStream>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_editor(std::make_unique<LuaEditor>(m_parser,this))
    , m_parser(std::make_unique<LuaParser>())
    , m_completer(std::make_unique<AutoCompleter>(this))
{
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    createConnections();

    // Connect parser & completer
    m_completer->setParser(m_parser.get());
    m_editor->setCompleter(m_completer.get());

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

    connect(m_parser.get(), &LuaParser::parsingCompleted, this, &MainWindow::onParsingCompleted);
    connect(m_parser.get(), &LuaParser::parsingError, this, &MainWindow::onParsingError);
}

// === File Handling ===

void MainWindow::newFile()
{
    if (maybeSave()) {
        m_editor->clear();
        setCurrentFile(QString());
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
}

void MainWindow::saveFile()
{
    if (m_currentFile.isEmpty()) {
        saveFileAs();
    } else {
        if (!saveDocument(m_currentFile))
            QMessageBox::warning(this, tr("Save Error"), tr("Failed to save the file."));
    }
}

void MainWindow::saveFileAs()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save Lua File"), QString(),
        tr("Lua Files (*.lua);;All Files (*)"));
    if (!fileName.isEmpty())
        if (!saveDocument(fileName))
            QMessageBox::warning(this, tr("Save Error"), tr("Failed to save the file."));
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

// === Editor / Parser callbacks ===

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

    m_parser->parseAsync(m_editor->toPlainText());
    m_parsingProgress->setVisible(true);
    m_parsingProgress->setRange(0, 0);
}

void MainWindow::onCursorPositionChanged()
{
    updateStatusBar();
}

void MainWindow::onParsingCompleted()
{
    m_parsingProgress->setVisible(false);
    m_statusLabel->setText(tr("Parsing completed"));

    m_symbolsList->clear();
    for (const auto& sym : m_parser->getSymbols())
        m_symbolsList->addItem(QString::fromStdString(sym));
}

void MainWindow::onParsingError(const QString& error)
{
    m_parsingProgress->setVisible(false);
    m_statusLabel->setText(tr("Parsing error: %1").arg(error));
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
    case QMessageBox::Save: return saveFile(), true;
    case QMessageBox::Cancel: return false;
    default: return true;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) event->accept();
    else event->ignore();
}
