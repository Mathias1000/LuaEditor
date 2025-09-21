#include "AutoCompleter.h"

#include <QAbstractItemView>
#include <QScrollBar>
#include <QListView>

AutoCompleter::AutoCompleter(QObject* parent)
    : QObject(parent)
    , m_model(std::make_unique<QStringListModel>(this))
    , m_completer(new QCompleter(this))
{
    m_completer->setModel(m_model.get());
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchStartsWith);  // Prefix matching
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(12);  // Reasonable number of items

    // Enable performance optimizations if the popup is a QListView
    auto* popup = m_completer->popup();
    if (auto* listView = qobject_cast<QListView*>(popup)) {
        listView->setUniformItemSizes(true); // Performance improvement
    }

    // Connect to completion changes for reactive updates
    connect(m_model.get(), &QAbstractItemModel::modelReset,
            this, [this]() {
        // Force update of completion when model changes
        if (m_completer->popup()->isVisible()) {
            m_completer->popup()->setCurrentIndex(m_completer->currentIndex());
        }
    });

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

void AutoCompleter::setWidget(QWidget* widget)
{
    m_completer->setWidget(widget);
}

void AutoCompleter::showPopup(const QRect& rect)
{
    if (!m_completer || !m_completer->widget()) {
        // kein Ziel-Widget → nichts zu tun
        return;
    }
    // Popup anzeigen — Größe anhand Inhalt
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