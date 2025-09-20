#include "AutoCompleter.h"

#include <QAbstractItemView>
#include <QScrollBar>

AutoCompleter::AutoCompleter(QObject* parent)
    : QObject(parent)
    , m_model(std::make_unique<QStringListModel>(this))
    , m_completer(new QCompleter(this))
{
    m_completer->setModel(m_model.get());
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchStartsWith);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);

    // Weiterreichen des aktivierten Textes
    connect(m_completer,
           QOverload<const QString&>::of(&QCompleter::activated),
           this,
           [this](const QString& s) {
               emit activated(s);
           });
}

void AutoCompleter::updateCompleter(const QStringList& items)
{
    // Einfache Deduplizierung / Sortierung
    QStringList sorted = items;
    sorted.removeDuplicates();
    sorted.sort(Qt::CaseInsensitive);

    m_model->setStringList(sorted);
}

void AutoCompleter::showPopup(const QRect& rect)
{
    if (!m_completer || !m_completer->widget()) {
        // kein Ziel-Widget → nichts zu tun
        return;
    }
    // Popup anzeigen – Größe anhand Inhalt
    auto* view = m_completer->popup();
    if (!view) return;

    const int w = view->sizeHintForColumn(0)
                + view->verticalScrollBar()->sizeHint().width()
                + 24;
    QRect r = rect;
    r.setWidth(w);
    m_completer->complete(r);
}

void AutoCompleter::hidePopup()
{
    if (auto* view = m_completer ? m_completer->popup() : nullptr) {
        view->hide();
    }
}

void AutoCompleter::setWidget(QWidget* widget)
{
    m_completer->setWidget(widget);
}