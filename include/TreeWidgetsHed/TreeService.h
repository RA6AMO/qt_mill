// TreeService — бизнес-логика CRUD/move/buildPath/resolvePath/listChildren/payload
#pragma once

#include <QtGlobal>
#include <QString>
#include <optional>
#include <qtypes.h>
#include <vector>
#include <unordered_map>

#include "INodeRepository.h"
#include "Node.h"
class INodeRepository;
class INodeFactory;

// Сервис работы с деревом узлов: CRUD-операции, перемещение, построение и
// разрешение путей, чтение/запись payload. Хранит небольшой кеш метаданных
// (parentId, name) для ускорения операций с путями.
class TreeService {
public:
    static constexpr qint64 ROOT_ID = 1;

    // В корне дерева находится специальный узел с фиксированным id

    // Внедрение репозитория хранения и фабрики нормализации/валидации имен
    TreeService(std::unique_ptr<INodeRepository> repo,
                std::unique_ptr<INodeFactory> factory);

    // Создает дочерний узел: нормализует и валидирует имя, затем сохраняет.
    // Уникальность среди сиблингов обеспечивает репозиторий/БД.
    qint64 createNode(qint64 parentId, const QString &name, std::optional<QString> payload = {});

    // Переименовывает узел: нормализует/валидирует имя и инвалидирует кеш.
    void renameNode(qint64 id, const QString &newName);

    // Перемещает узел к новому родителю; запрещено перемещение в собственного потомка.
    void moveNode(qint64 id, qint64 newParentId);

    // Удаляет узел (кроме корня). Кеш очищается для затронутых узлов.
    void deleteNode(qint64 id);

    // Строит путь вида "a/b/c" от корня до указанного узла, используя кеш.
    QString buildPath(qint64 id);

    // Разрешает строковый путь в id узла. Каждый сегмент нормализуется и валидируется.
    qint64 resolvePath(const QString &path);

    // Возвращает список детей родителя с пагинацией и признаком наличия потомков.
    std::vector<NodeDTO> listChildren(qint64 parentId, size_t limit = SIZE_MAX, size_t offset = 0);

    // Записывает/читает произвольный JSON payload, связанный с узлом.
    void setPayload(qint64 id, const QString &payloadJson);
    QString getPayload(qint64 id);

    void resetTreeMap();

    const std::map<qint64, RepoRow> getTree();

private:
    // Доступ к хранилищу узлов (БД) и бизнес-правилам имен.
    std::unique_ptr<INodeRepository> m_repo;
    std::unique_ptr<INodeFactory> m_factory;

    // Простой кеш id -> (parentId, name) для ускорения buildPath/resolvePath
    struct CacheEntry { qint64 parentId; QString name; };
    std::unordered_map<qint64, CacheEntry> m_metaCache;

    // Унифицирует и валидирует имя через фабрику.
    void ensureValidName(const QString &name) const;

    // Проверяет, является ли nodeId потомком potentialAncestorId (или совпадает с ним).
    bool isDescendant(qint64 nodeId, qint64 potentialAncestorId);

    // Подгружает метаданные узла в кеш при необходимости.
    void warmCache(qint64 id);

    // Удаляет запись о метаданных узла из кеша.
    void invalidateCache(qint64 id);

    std::map<qint64, RepoRow> tree;
    void fillTreeMapRecursive(qint64 nodeId, std::map<qint64, RepoRow>& tree);

};
