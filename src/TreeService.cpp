// TreeService.cpp — реализация бизнес-операций над деревом и путей
#include "TreeService.h"
#include "Errors.h"
#include "INodeRepository.h"
#include "INodeFactory.h"

#include <QStringList>

TreeService::TreeService(std::unique_ptr<INodeRepository> repo,
                         std::unique_ptr<INodeFactory> factory)
    : m_repo(std::move(repo)), m_factory(std::move(factory)) {}

void TreeService::ensureValidName(const QString &name) const {
    const QString normalized = m_factory->normalizeName(name);
    m_factory->validateName(normalized);
}

qint64 TreeService::createNode(qint64 parentId, const QString &name, std::optional<QString> payload) {
    ensureValidName(name);
    // Уникальность среди сиблингов обеспечит уникальный индекс
    const QString normalized = m_factory->normalizeName(name);
    return m_repo->insert(parentId, normalized, payload);
}

void TreeService::renameNode(qint64 id, const QString &newName) {
    ensureValidName(newName);
    const QString normalized = m_factory->normalizeName(newName);
    m_repo->updateName(id, normalized);
    invalidateCache(id);
}

static bool safeEq(qint64 a, qint64 b) { return a == b; }

bool TreeService::isDescendant(qint64 nodeId, qint64 potentialAncestorId) {
    if (safeEq(nodeId, potentialAncestorId)) return true;
    auto parentOpt = m_repo->getParentId(nodeId);
    while (parentOpt.has_value()) {
        const qint64 p = parentOpt.value();
        if (safeEq(p, potentialAncestorId)) return true;
        parentOpt = m_repo->getParentId(p);
    }
    return false;
}

void TreeService::moveNode(qint64 id, qint64 newParentId) {
    if (isDescendant(newParentId, id)) {
        throw Errors::MoveIntoDescendant("Cannot move into own descendant");
    }
    m_repo->updateParent(id, newParentId);
    invalidateCache(id);
}

void TreeService::deleteNode(qint64 id) {
    if (safeEq(id, ROOT_ID)) {
        throw Errors::InvalidName("Cannot delete root");
    }
    m_repo->remove(id);
    invalidateCache(id);
}

void TreeService::warmCache(qint64 id) {
    if (m_metaCache.find(id) != m_metaCache.end()) return;
    auto r = m_repo->get(id);
    if (!r.has_value()) throw Errors::NotFound("Node not found");
    CacheEntry ce { r->parentId.value_or(0), r->name };
    m_metaCache[id] = std::move(ce);
}

void TreeService::invalidateCache(qint64 id) {
    m_metaCache.erase(id);
}

QString TreeService::buildPath(qint64 id) {
    if (safeEq(id, ROOT_ID)) return QString();
    QStringList segments;
    auto current = id;
    while (true) {
        warmCache(current);
        const auto it = m_metaCache.find(current);
        if (it == m_metaCache.end()) break;
        const auto &meta = it->second;
        if (safeEq(current, ROOT_ID)) break;
        if (!safeEq(current, ROOT_ID)) segments.push_front(meta.name);
        if (safeEq(meta.parentId, 0) || safeEq(meta.parentId, ROOT_ID)) {
            break;
        }
        current = meta.parentId;
    }
    return segments.join('/');
}

qint64 TreeService::resolvePath(const QString &path) {
    if (path.isEmpty()) return ROOT_ID;
    const auto segments = path.split('/', Qt::SkipEmptyParts);
    qint64 current = ROOT_ID;
    for (const auto &segRaw : segments) {
        const QString seg = m_factory->normalizeName(segRaw);
        m_factory->validateName(seg);
        auto child = m_repo->findChildByName(current, seg);
        if (!child.has_value()) {
            throw Errors::NotFound("Path segment not found");
        }
        current = child->id;
    }
    return current;
}

std::vector<NodeDTO> TreeService::listChildren(qint64 parentId, size_t limit, size_t offset) {
    auto rows = m_repo->getChildren(parentId);
    std::vector<NodeDTO> out;
    out.reserve(rows.size());
    size_t i = 0;
    for (const auto &r : rows) {
        if (i++ < offset) continue;
        if (out.size() >= limit) break;
        NodeDTO dto;
        dto.id = r.id;
        dto.parentId = r.parentId.value_or(0);
        dto.name = r.name;
        dto.hasChildren = m_repo->hasChildren(r.id);
        out.push_back(std::move(dto));
    }
    return out;
}

void TreeService::setPayload(qint64 id, const QString &payloadJson) {
    m_repo->setPayload(id, payloadJson);
}

QString TreeService::getPayload(qint64 id) {
    auto p = m_repo->getPayload(id);
    if (!p.has_value()) return QString();
    return p.value();
}
