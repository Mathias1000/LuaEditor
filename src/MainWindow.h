#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <memory>

#include "LuaEditor.h"
#include "LuaParser.h"
#include "AutoCompleter.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    void openFile(const QString& filePath);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void about();
    void onTextChanged();
    void onCursorPositionChanged();
    void onParsingCompleted();
    void onParsingError(const QString& error);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void createConnections();
    void updateWindowTitle();
    void updateStatusBar();
    [[nodiscard]] bool maybeSave();
    [[nodiscard]] bool saveDocument(const QString& fileName);
    void setCurrentFile(const QString& fileName);
    [[nodiscard]] QString strippedName(const QString& fullFileName) const;

    // Core components
    std::unique_ptr<LuaEditor> m_editor;
    std::unique_ptr<LuaParser> m_parser;
    std::unique_ptr<AutoCompleter> m_completer;

    QWidget* m_centralWidget{nullptr};
    QSplitter* m_mainSplitter{nullptr};
    QListWidget* m_symbolsList{nullptr};

    // Actions
    QAction* m_newAction{nullptr};
    QAction* m_openAction{nullptr};
    QAction* m_saveAction{nullptr};
    QAction* m_saveAsAction{nullptr};
    QAction* m_exitAction{nullptr};
    QAction* m_aboutAction{nullptr};

    // Status bar widgets
    QLabel* m_statusLabel{nullptr};
    QLabel* m_cursorPosLabel{nullptr};
    QProgressBar* m_parsingProgress{nullptr};

    // File management
    QString m_currentFile;
    bool m_isModified{false};

    static constexpr std::string_view WINDOW_TITLE = "Lua AutoComplete Editor";
};
