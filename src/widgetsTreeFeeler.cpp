// WidgetsTreeFeeler.cpp — реализация ленивой подгрузки/контекстного меню/rename/удаления
#include "widgetsTreeFeeler.h"
#include "TreeWidgetEx.h"
#include "TreeService.h"
#include <QInputDialog>
#include <QMessageBox>

static constexpr int COLUMN_NAME = 0;

static QTreeWidgetItem* makeItem(const NodeDTO &dto) {
    auto *item = new QTreeWidgetItem();
    item->setText(COLUMN_NAME, dto.name);
    item->setData(COLUMN_NAME, Qt::UserRole, QVariant::fromValue<qlonglong>(dto.id));
    return item;
}

static bool hasOnlyPlaceholder(QTreeWidgetItem *item) {
    return item->childCount() == 1 && item->child(0)->data(COLUMN_NAME, Qt::UserRole).toLongLong() == 0;
}

WidgetsTreeFeeler::WidgetsTreeFeeler(TreeWidgetEx *tree, TreeService *service, QObject *parent)
    : QObject(parent), m_tree(tree), m_service(service) {}

void WidgetsTreeFeeler::initialize() {
    if (!m_tree) return;
    m_tree->setColumnCount(1);
    m_tree->setHeaderHidden(true);
    m_tree->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_tree, &QTreeWidget::itemExpanded, this, &WidgetsTreeFeeler::onItemExpanded);
    connect(m_tree, &QTreeWidget::itemChanged, this, &WidgetsTreeFeeler::onItemChanged);
    connect(m_tree, &QWidget::customContextMenuRequested, this, &WidgetsTreeFeeler::onCustomContextMenuRequested);
    connect(m_tree, &TreeWidgetEx::requestMove, this, &WidgetsTreeFeeler::onRequestMove);

    // Начальная загрузка: дети корня (id=1) — это топ-уровень
    m_tree->clear();
    loadChildrenInto(nullptr, TreeService::ROOT_ID);
}

void WidgetsTreeFeeler::loadChildrenInto(QTreeWidgetItem *parentItem, qint64 parentId) {
    const auto children = m_service->listChildren(parentId);
    for (const auto &dto : children) {
        QTreeWidgetItem *item = makeItem(dto);
        if (parentItem) parentItem->addChild(item); else m_tree->addTopLevelItem(item);
        m_idToItem.insert(dto.id, item);
        addPlaceholderIfNeeded(item, dto.hasChildren);
    }
}

void WidgetsTreeFeeler::addPlaceholderIfNeeded(QTreeWidgetItem *item, bool hasChildren) {
    if (hasChildren) {
        auto *ph = new QTreeWidgetItem();
        ph->setText(COLUMN_NAME, QString("..."));
        ph->setData(COLUMN_NAME, Qt::UserRole, QVariant::fromValue<qlonglong>(0)); // 0 — плейсхолдер
        item->addChild(ph);
    }
}

bool WidgetsTreeFeeler::isPlaceholder(QTreeWidgetItem *item) const {
    return item && item->data(COLUMN_NAME, Qt::UserRole).toLongLong() == 0;
}

void WidgetsTreeFeeler::onItemExpanded(QTreeWidgetItem *item) {
    if (!item) return;
    if (hasOnlyPlaceholder(item)) {
        // удалить плейсхолдер и подгрузить настоящих детей
        auto *ph = item->child(0);
        item->removeChild(ph);
        delete ph;
        const qint64 id = item->data(COLUMN_NAME, Qt::UserRole).toLongLong();
        loadChildrenInto(item, id);
    }
}

void WidgetsTreeFeeler::onItemChanged(QTreeWidgetItem *item, int column) {
    if (!item || column != COLUMN_NAME) return;
    const qint64 id = item->data(COLUMN_NAME, Qt::UserRole).toLongLong();
    if (id == 0) return; // плейсхолдер
    const QString newText = item->text(COLUMN_NAME);
    try {
        m_service->renameNode(id, newText);
    } catch (const std::exception &ex) {
        const QString path = m_service->buildPath(id);
        const int slash = path.lastIndexOf('/');
        const QString base = (slash >= 0) ? path.mid(slash + 1) : path;
        QSignalBlocker blocker(m_tree);
        item->setText(COLUMN_NAME, base);
        QMessageBox::warning(m_tree, "Ошибка", QString::fromUtf8(ex.what()));
    }
}

void WidgetsTreeFeeler::onCustomContextMenuRequested(const QPoint &pos) {
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    QMenu menu(m_tree);
    QAction *addAct = menu.addAction("Добавить ребёнка");
    QAction *renAct = menu.addAction("Переименовать");
    QAction *delAct = menu.addAction("Удалить");

    QObject::connect(addAct, &QAction::triggered, this, [this, item]() { createChild(item); });
    QObject::connect(renAct, &QAction::triggered, this, [this, item]() { renameItem(item); });
    QObject::connect(delAct, &QAction::triggered, this, [this, item]() { deleteItem(item); });

    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}

void WidgetsTreeFeeler::createChild(QTreeWidgetItem *parentItem) {
    qint64 parentId = TreeService::ROOT_ID;
    if (parentItem) parentId = parentItem->data(COLUMN_NAME, Qt::UserRole).toLongLong();
    bool ok = false;
    QString name = QInputDialog::getText(m_tree, "Новое имя", "Имя узла:", QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    try {
        const qint64 newId = m_service->createNode(parentId, name, {});
        NodeDTO dto{ newId, parentId, name, false };
        QTreeWidgetItem *newItem = makeItem(dto);
        if (parentItem) parentItem->addChild(newItem); else m_tree->addTopLevelItem(newItem);
        m_idToItem.insert(newId, newItem);
    } catch (const std::exception &ex) {
        QMessageBox::warning(m_tree, "Ошибка", QString::fromUtf8(ex.what()));
    }
}

void WidgetsTreeFeeler::renameItem(QTreeWidgetItem *item) {
    if (!item) return;
    m_tree->editItem(item, COLUMN_NAME);
}

void WidgetsTreeFeeler::deleteItem(QTreeWidgetItem *item) {
    if (!item) return;
    const qint64 id = item->data(COLUMN_NAME, Qt::UserRole).toLongLong();
    if (id == 0) return;
    if (QMessageBox::question(m_tree, "Подтверждение", "Удалить узел и все дочерние?") != QMessageBox::Yes) return;
    try {
        m_service->deleteNode(id);
        delete item;
    } catch (const std::exception &ex) {
        QMessageBox::warning(m_tree, "Ошибка", QString::fromUtf8(ex.what()));
    }
}

void WidgetsTreeFeeler::onRequestMove(qint64 nodeId, qint64 newParentId, bool &accepted) {
    try {
        m_service->moveNode(nodeId, newParentId);
        accepted = true;
    } catch (const std::exception &ex) {
        accepted = false;
        QMessageBox::warning(m_tree, "Перемещение", QString::fromUtf8(ex.what()));
    }
}
