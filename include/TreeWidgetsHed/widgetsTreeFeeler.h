// В этот файл перенесём интерфейс/вспомогательный класс для связи UI и сервиса
// WidgetsTreeFeeler — связующее звено QTreeWidget ↔ TreeService (ленивая загрузка, контекст, rename)
#pragma once

#include <QObject>
#include <QPointer>
#include <QMenu>
#include <QHash>
#include <QTreeWidgetItem>

class TreeService;
class TreeWidgetEx;

// Связывает QTreeWidget и TreeService: ленивая подгрузка, контекстное меню, rename, add, delete
class WidgetsTreeFeeler : public QObject {
    Q_OBJECT
public:
    explicit WidgetsTreeFeeler(TreeWidgetEx *tree, TreeService *service, QObject *parent = nullptr);

    void initialize(); // Подписка на сигналы и начальная загрузка root-детей

    signals:
    void itemClicked(qint64 id);

private slots:
    void onItemExpanded(QTreeWidgetItem *item);
    void onItemChanged(QTreeWidgetItem *item, int column);
    void onCustomContextMenuRequested(const QPoint &pos);
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onRequestMove(qint64 nodeId, qint64 newParentId, bool &accepted);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    QPointer<TreeWidgetEx> m_tree;
    TreeService *m_service; // не владеем

    // id->item и item->id загружаем через Qt::UserRole, а map держим для быстрых обращений
    QHash<qint64, QTreeWidgetItem*> m_idToItem;

    void loadChildrenInto(QTreeWidgetItem *parentItem, qint64 parentId);
    void addPlaceholderIfNeeded(QTreeWidgetItem *item, bool hasChildren);
    bool isPlaceholder(QTreeWidgetItem *item) const;
    void createChild(QTreeWidgetItem *parentItem);
    void renameItem(QTreeWidgetItem *item);
    void deleteItem(QTreeWidgetItem *item);
};
