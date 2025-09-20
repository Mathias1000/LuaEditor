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
    void setWidget(QWidget* widget);

    // Popup-Steuerung
    void showPopup(const QRect& rect);
    void hidePopup();

    // Zugriff auf den internen QCompleter
    QCompleter* completer() const { return m_completer; }

    signals:
        void activated(const QString& text);

private:
    std::unique_ptr<QStringListModel> m_model;
    QCompleter* m_completer = nullptr;
};