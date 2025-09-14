#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QLocale>
#include <QTranslator>
#include <QMessageBox>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application properties
    app.setApplicationName("LuaAutoCompleteQt6");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("LuaAutoComplete");
    app.setApplicationDisplayName("Lua AutoComplete Editor");

    // Set application icon if available
    const QString iconPath = QDir(app.applicationDirPath()).filePath("../share/LuaAutoCompleteQt6/icons/lua-icon.png");
    if (QFile::exists(iconPath)) {
        app.setWindowIcon(QIcon(iconPath));
    }

    // Setup translations
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "luaautocomplete_" + QLocale(locale).name();
        const QString translationPath = QDir(app.applicationDirPath()).filePath("../share/LuaAutoCompleteQt6/translations/" + baseName);
        if (translator.load(translationPath)) {
            app.installTranslator(&translator);
            break;
        }
    }

    // Create and show main window
    try {
        MainWindow window;
        window.show();

        // Open file if provided as command line argument
        if (argc > 1) {
            const QString filePath = QString::fromLocal8Bit(argv[1]);
            if (QFile::exists(filePath)) {
                window.openFile(filePath);
            }
        }

        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Critical Error",
            QString("Failed to start application: %1").arg(e.what()));
        return -1;
    }
}
