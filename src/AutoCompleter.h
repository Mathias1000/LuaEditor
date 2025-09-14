#pragma once

#include <QObject>
#include <QCompleter>
#include <QStringListModel>
#include <memory>

/**
 * Dünne Hülle um QCompleter für Qt6:
 *  - updateCompleter(QStringList)
 *  - showPopup(QRect) / hidePopup()
 *  - Signal activated(QString) zum Einfügen im Editor
 */
class AutoCompleter : public QObject
{
    Q_OBJECT
public:
    explicit AutoCompleter(QObject* parent = nullptr);

    void updateCompleter(const QStringList& items);

    // Popup-Steuerung
    void showPopup(const QRect& rect);
    void hidePopup();

    signals:
        void activated(const QString& text);

private:
    std::unique_ptr<QStringListModel> m_model;
    QCompleter* m_completer = nullptr;
};
