// TreeService.cpp — реализация бизнес-операций над деревом и путей
#include "TreeService.h"
#include "Errors.h"
#include "INodeRepository.h"
#include "INodeFactory.h"

#include <QStringList>

// Внедряем зависимости: репозиторий (доступ к БД) и фабрика (нормализация/валидация имен)
TreeService::TreeService(std::unique_ptr<INodeRepository> repo,
                         std::unique_ptr<INodeFactory> factory)
    : m_repo(std::move(repo)), m_factory(std::move(factory))
    {
        resetTreeMap();
    }

// Унифицирует и валидирует имя через фабрику имен
void TreeService::ensureValidName(const QString &name) const {
    const QString normalized = m_factory->normalizeName(name);
    m_factory->validateName(normalized);
}

// Создание дочернего узла: имя нормализуется/валидируется, затем сохраняется в БД
qint64 TreeService::createNode(qint64 parentId, const QString &name, std::optional<QString> payload) {
    ensureValidName(name);
    // Уникальность среди сиблингов обеспечит уникальный индекс
    const QString normalized = m_factory->normalizeName(name);
    return m_repo->insert(parentId, normalized, payload);
}

// Переименование узла и инвалидация кеша метаданных этого узла
void TreeService::renameNode(qint64 id, const QString &newName) {
    ensureValidName(newName);
    const QString normalized = m_factory->normalizeName(newName);
    m_repo->updateName(id, normalized);
    invalidateCache(id);
}

// Небольшой враппер для явного сравнения qint64
static bool safeEq(qint64 a, qint64 b) { return a == b; }

// Проверяет, находится ли nodeId в поддереве potentialAncestorId (или совпадает)
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

// Перемещает узел к новому родителю; запрещено переносить в собственного потомка
void TreeService::moveNode(qint64 id, qint64 newParentId) {
    if (isDescendant(newParentId, id)) {
        throw Errors::MoveIntoDescendant("Cannot move into own descendant");
    }
    m_repo->updateParent(id, newParentId);
    invalidateCache(id);
}

// Удаляет узел (кроме корня). Кеш очищается для затронутых узлов.
void TreeService::deleteNode(qint64 id) {
    if (safeEq(id, ROOT_ID)) {
        throw Errors::InvalidName("Cannot delete root");
    }
    m_repo->remove(id);
    invalidateCache(id);
}

// Подкачивает метаданные узла в кеш, если их еще нет
void TreeService::warmCache(qint64 id) {
    if (m_metaCache.find(id) != m_metaCache.end()) return;
    auto r = m_repo->get(id);
    if (!r.has_value()) throw Errors::NotFound("Node not found");
    CacheEntry ce { r->parentId.value_or(0), r->name };
    m_metaCache[id] = std::move(ce);
}

// Инвалидация записи кеша по конкретному узлу
void TreeService::invalidateCache(qint64 id) {
    m_metaCache.erase(id);
}

// Собирает путь от корня до узла вида "a/b/c". Корню соответствует пустая строка
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

// Ищет узел по строковому пути. Пустой путь => корень. Каждый сегмент нормализуется/валидируется
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

// Возвращает детей с пагинацией и признаком наличия потомков (для ленивой подгрузки UI)
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

// Сохраняет произвольный JSON payload узла
void TreeService::setPayload(qint64 id, const QString &payloadJson) {
    m_repo->setPayload(id, payloadJson);
}

// Возвращает JSON payload узла; при отсутствии — пустую строку
QString TreeService::getPayload(qint64 id) {
    auto p = m_repo->getPayload(id);
    if (!p.has_value()) return QString();
    return p.value();

}

void TreeService::fillTreeMapRecursive(qint64 nodeId, std::map<qint64, RepoRow>& tree) {
    if (!m_repo) return;

    // Получаем данные узла по id
    auto nodeOpt = m_repo->get(nodeId);
    if (!nodeOpt.has_value()) {
        return; // Узел не найден
    }

    RepoRow node = nodeOpt.value();

    // Добавляем узел в map
    tree[nodeId] = node;

    // Получаем всех детей этого узла
    std::vector<RepoRow> children = m_repo->getChildren(nodeId);

    // Рекурсивно обрабатываем каждого ребёнка
    for (const auto& child : children) {
        fillTreeMapRecursive(child.id, tree);
    }
}

void TreeService::resetTreeMap() {
    tree.clear();
    fillTreeMapRecursive(1, tree);
}

const std::map<qint64, RepoRow> TreeService::getTree(){
    return tree;
}
