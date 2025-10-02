#include "MainWindow.h"
#include "LuaHighlighter.h"

#include <QCloseEvent>
#include <QFileInfo>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>

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

    m_completer->setWidget(m_editor.get());
    m_editor->setCompleter(m_completer.get());

    auto* highlighter = new LuaHighlighter(m_editor->document());
    Q_UNUSED(highlighter)

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

    m_symbolPanel = new QWidget(this);
    m_symbolPanel->setMaximumHeight(50);
    auto* symbolLayout = new QHBoxLayout(m_symbolPanel);
    symbolLayout->setContentsMargins(5, 5, 5, 5);
    symbolLayout->setSpacing(5);

    m_loadSymbolButton = new QPushButton("LoadSymbol", this);
    m_loadSymbolButton->setMinimumHeight(35);
    m_loadSymbolButton->setMaximumWidth(100);
    m_loadSymbolButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2ecc71;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 8px 16px;"
        "   font-weight: bold;"
        "   font-size: 10pt;"
        "}"
        "QPushButton:hover {"
        "   background-color: #27ae60;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #229954;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #95a5a6;"
        "   color: #ecf0f1;"
        "}"
    );
    symbolLayout->addWidget(m_loadSymbolButton);

    m_globalsButton = new QPushButton("📋 Globals", this);
    m_globalsButton->setMinimumHeight(35);
    m_globalsButton->setEnabled(false);
    m_globalsButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(240, 240, 240, 220);"
        "   border: 1px solid rgba(100, 100, 100, 180);"
        "   border-radius: 5px;"
        "   padding: 8px 16px;"
        "   font-size: 10pt;"
        "   text-align: left;"
        "}"
        "QPushButton:hover:enabled {"
        "   background-color: rgba(200, 200, 200, 230);"
        "}"
        "QPushButton:pressed:enabled {"
        "   background-color: rgba(150, 150, 150, 240);"
        "}"
        "QPushButton:disabled {"
        "   background-color: rgba(220, 220, 220, 150);"
        "   color: rgba(100, 100, 100, 150);"
        "}"
    );
    symbolLayout->addWidget(m_globalsButton, 1);

    m_functionsButton = new QPushButton("⚙️ Functions", this);
    m_functionsButton->setMinimumHeight(35);
    m_functionsButton->setEnabled(false);
    m_functionsButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(255, 250, 240, 220);"
        "   border: 1px solid rgba(100, 100, 100, 180);"
        "   border-radius: 5px;"
        "   padding: 8px 16px;"
        "   font-size: 10pt;"
        "   text-align: left;"
        "}"
        "QPushButton:hover:enabled {"
        "   background-color: rgba(255, 230, 200, 230);"
        "}"
        "QPushButton:pressed:enabled {"
        "   background-color: rgba(255, 210, 160, 240);"
        "}"
        "QPushButton:disabled {"
        "   background-color: rgba(240, 235, 230, 150);"
        "   color: rgba(100, 100, 100, 150);"
        "}"
    );
    symbolLayout->addWidget(m_functionsButton, 1);

    m_tablesButton = new QPushButton("📦 Tables", this);
    m_tablesButton->setMinimumHeight(35);
    m_tablesButton->setEnabled(false);
    m_tablesButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(240, 255, 240, 220);"
        "   border: 1px solid rgba(100, 100, 100, 180);"
        "   border-radius: 5px;"
        "   padding: 8px 16px;"
        "   font-size: 10pt;"
        "   text-align: left;"
        "}"
        "QPushButton:hover:enabled {"
        "   background-color: rgba(200, 255, 200, 230);"
        "}"
        "QPushButton:pressed:enabled {"
        "   background-color: rgba(160, 255, 160, 240);"
        "}"
        "QPushButton:disabled {"
        "   background-color: rgba(230, 240, 230, 150);"
        "   color: rgba(100, 100, 100, 150);"
        "}"
    );
    symbolLayout->addWidget(m_tablesButton, 1);

    mainLayout->addWidget(m_symbolPanel);
    mainLayout->addWidget(m_editor.get());

    m_globalsList = new QListWidget(this);
    m_globalsList->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_globalsList->setAttribute(Qt::WA_ShowWithoutActivating, false);
    m_globalsList->setMaximumHeight(300);
    m_globalsList->setMinimumWidth(300);
    m_globalsList->setStyleSheet(
        "QListWidget {"
        "   background-color: rgba(240, 240, 240, 250);"
        "   border: 2px solid rgba(100, 100, 100, 220);"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "   font-size: 10pt;"
        "}"
        "QListWidget::item {"
        "   padding: 5px;"
        "   color: #000000;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: rgba(100, 150, 200, 150);"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(50, 100, 200, 200);"
        "   color: white;"
        "}"
    );
    m_globalsList->hide();

    m_functionsList = new QListWidget(this);
    m_functionsList->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_functionsList->setAttribute(Qt::WA_ShowWithoutActivating, false);
    m_functionsList->setMaximumHeight(300);
    m_functionsList->setMinimumWidth(300);
    m_functionsList->setStyleSheet(
        "QListWidget {"
        "   background-color: rgba(255, 250, 240, 250);"
        "   border: 2px solid rgba(100, 100, 100, 220);"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "   font-size: 10pt;"
        "}"
        "QListWidget::item {"
        "   padding: 5px;"
        "   color: #000000;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: rgba(200, 150, 100, 150);"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(200, 100, 50, 200);"
        "   color: white;"
        "}"
    );
    m_functionsList->hide();

    m_tablesList = new QListWidget(this);
    m_tablesList->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_tablesList->setAttribute(Qt::WA_ShowWithoutActivating, false);
    m_tablesList->setMaximumHeight(300);
    m_tablesList->setMinimumWidth(300);
    m_tablesList->setStyleSheet(
        "QListWidget {"
        "   background-color: rgba(240, 255, 240, 250);"
        "   border: 2px solid rgba(100, 100, 100, 220);"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "   font-size: 10pt;"
        "}"
        "QListWidget::item {"
        "   padding: 5px;"
        "   color: #000000;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: rgba(100, 200, 100, 150);"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(50, 150, 50, 200);"
        "   color: white;"
        "}"
    );
    m_tablesList->hide();

    m_globalsList->installEventFilter(this);
    m_functionsList->installEventFilter(this);
    m_tablesList->installEventFilter(this);
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
    connect(m_globalsList, &QListWidget::itemClicked, this, &MainWindow::onGlobalsItemClicked);
    connect(m_functionsList, &QListWidget::itemClicked, this, &MainWindow::onFunctionsItemClicked);
    connect(m_tablesList, &QListWidget::itemClicked, this, &MainWindow::onTablesItemClicked);
    connect(m_loadSymbolButton, &QPushButton::clicked, this, &MainWindow::onLoadSymbolClicked);
    connect(m_globalsButton, &QPushButton::clicked, this, &MainWindow::toggleGlobalsList);
    connect(m_functionsButton, &QPushButton::clicked, this, &MainWindow::toggleFunctionsList);
    connect(m_tablesButton, &QPushButton::clicked, this, &MainWindow::toggleTablesList);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (obj == m_globalsList || obj == m_functionsList || obj == m_tablesList) {
            QWidget* widget = qobject_cast<QWidget*>(obj);
            if (widget) widget->hide();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::hideAllPopups()
{
    m_globalsList->hide();
    m_functionsList->hide();
    m_tablesList->hide();
}

void MainWindow::toggleGlobalsList()
{
    m_functionsList->hide();
    m_tablesList->hide();
    if (m_globalsList->isVisible()) {
        m_globalsList->hide();
    } else {
        QPoint pos = m_globalsButton->mapToGlobal(QPoint(0, m_globalsButton->height()));
        m_globalsList->move(pos);
        m_globalsList->setFixedWidth(m_globalsButton->width());
        m_globalsList->show();
        m_globalsList->setFocus();
    }
}

void MainWindow::toggleFunctionsList()
{
    m_globalsList->hide();
    m_tablesList->hide();
    if (m_functionsList->isVisible()) {
        m_functionsList->hide();
    } else {
        QPoint pos = m_functionsButton->mapToGlobal(QPoint(0, m_functionsButton->height()));
        m_functionsList->move(pos);
        m_functionsList->setFixedWidth(m_functionsButton->width());
        m_functionsList->show();
        m_functionsList->setFocus();
    }
}

void MainWindow::toggleTablesList()
{
    m_globalsList->hide();
    m_functionsList->hide();
    if (m_tablesList->isVisible()) {
        m_tablesList->hide();
    } else {
        QPoint pos = m_tablesButton->mapToGlobal(QPoint(0, m_tablesButton->height()));
        m_tablesList->move(pos);
        m_tablesList->setFixedWidth(m_tablesButton->width());
        m_tablesList->show();
        m_tablesList->setFocus();
    }
}

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
    m_globalsList->clear();
    m_functionsList->clear();
    m_tablesList->clear();

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

    const QStringList globals = m_parser->getGlobals();

    int globalsCount = 0;
    int functionsCount = 0;
    int tablesCount = 0;

    for (const QString& symbol : globals) {
        auto def = m_parser->findDefinition(symbol);
        if (def.has_value()) {
            QString displayText = symbol;
            switch (def->kind) {
                case SymbolKind::Function:
                    displayText += " " + def->signature;
                    m_functionsList->addItem(displayText);
                    functionsCount++;
                    break;
                case SymbolKind::Method:
                    displayText += " " + def->signature;
                    m_functionsList->addItem(displayText);
                    functionsCount++;
                    break;
                case SymbolKind::Table:
                    m_tablesList->addItem(displayText);
                    tablesCount++;
                    break;
                case SymbolKind::Variable:
                case SymbolKind::Field:
                    m_globalsList->addItem(displayText);
                    globalsCount++;
                    break;
                default:
                    m_globalsList->addItem(displayText);
                    globalsCount++;
                    break;
            }
        } else {
            m_globalsList->addItem(symbol);
            globalsCount++;
        }
    }

    m_globalsButton->setEnabled(globalsCount > 0);
    m_functionsButton->setEnabled(functionsCount > 0);
    m_tablesButton->setEnabled(tablesCount > 0);

    m_globalsButton->setText(QString("📋 Globals (%1)").arg(globalsCount));
    m_functionsButton->setText(QString("⚙️ Functions (%1)").arg(functionsCount));
    m_tablesButton->setText(QString("📦 Tables (%1)").arg(tablesCount));
}

void MainWindow::onGlobalsItemClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags) return;
    QString symbolText = item->text().split(' ').first();
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
    m_globalsList->hide();
}

void MainWindow::onFunctionsItemClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags) return;
    QString symbolText = item->text().split(' ').first();
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
    m_functionsList->hide();
}

void MainWindow::onTablesItemClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags) return;
    QString symbolText = item->text().split(' ').first();
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
    m_tablesList->hide();
}

void MainWindow::onLoadSymbolClicked()
{
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