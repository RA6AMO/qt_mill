// TreeWidgetEx.cpp — подтверждение drag&drop перемещений через сервис
#include "TreeWidgetEx.h"
#include <QDropEvent>
#include <QMimeData>

// В Qt InternalMove по умолчанию перемещает элементы. Мы перехватим и подтвердим у сервиса.

TreeWidgetEx::TreeWidgetEx(QWidget *parent)
    : QTreeWidget(parent) {
    setDragDropMode(QAbstractItemView::InternalMove);
}

static QTreeWidgetItem* itemAtPos(QTreeWidget *tw, const QPoint &pos) {
    return tw->itemAt(pos);
}

void TreeWidgetEx::dropEvent(QDropEvent *event) {
    // Определим целевого родителя
    QTreeWidgetItem *targetItem = itemAtPos(this, event->position().toPoint());
    if (!targetItem) {
        // Перемещение в корень (дети системного root id=1)
    }

    // Источник — выделенный элемент
    auto selected = currentItem();
    if (!selected) { event->ignore(); return; }

    const qint64 sourceId = selected->data(0, Qt::UserRole).toLongLong();
    qint64 newParentId = 1; // root
    if (targetItem) {
        newParentId = targetItem->data(0, Qt::UserRole).toLongLong();
    }

    bool accepted = false;
    emit requestMove(sourceId, newParentId, accepted);
    if (!accepted) {
        event->ignore();
        return;
    }
    // Если сервис одобрил — позволяем базовой реализации выполнить перемещение
    QTreeWidget::dropEvent(event);
}
