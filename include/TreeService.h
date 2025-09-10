// TreeService — бизнес-логика CRUD/move/buildPath/resolvePath/listChildren/payload
#pragma once

#include <QtGlobal>
#include <QString>
#include <optional>
#include <vector>
#include <unordered_map>

#include "Node.h"
class INodeRepository;
class INodeFactory;

class TreeService {
public:
    static constexpr qint64 ROOT_ID = 1;

    TreeService(std::unique_ptr<INodeRepository> repo,
                std::unique_ptr<INodeFactory> factory);

    qint64 createNode(qint64 parentId, const QString &name, std::optional<QString> payload = {});
    void renameNode(qint64 id, const QString &newName);
    void moveNode(qint64 id, qint64 newParentId);
    void deleteNode(qint64 id);

    QString buildPath(qint64 id);
    qint64 resolvePath(const QString &path);

    std::vector<NodeDTO> listChildren(qint64 parentId, size_t limit = SIZE_MAX, size_t offset = 0);

    void setPayload(qint64 id, const QString &payloadJson);
    QString getPayload(qint64 id);

private:
    std::unique_ptr<INodeRepository> m_repo;
    std::unique_ptr<INodeFactory> m_factory;

    // Простой кеш id -> (parentId, name) для ускорения buildPath/resolvePath
    struct CacheEntry { qint64 parentId; QString name; };
    std::unordered_map<qint64, CacheEntry> m_metaCache;

    void ensureValidName(const QString &name) const;
    bool isDescendant(qint64 nodeId, qint64 potentialAncestorId);
    void warmCache(qint64 id);
    void invalidateCache(qint64 id);
};
