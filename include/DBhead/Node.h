// Node.h — доменная модель узла и DTO (UI-минимум). Компаратор — case-sensitive.
#pragma once

#include <QString>
#include <QtGlobal>
#include <map>
#include <memory>

// Компаратор: чувствительный к регистру (Qt::CaseSensitive)
struct CaseSensitiveComparator {
    bool operator()(const QString &lhs, const QString &rhs) const {
        return lhs.compare(rhs, Qt::CaseSensitive) < 0;
    }
};

// DTO для передачи в UI/слои без рекурсивных владений
struct NodeDTO {
    qint64 id {0};
    qint64 parentId {0};
    QString name;
    bool hasChildren {false};
};

// Доменная модель (в памяти)
class Node {
public:
    qint64 id {0};
    qint64 parentId {0};
    QString name;
    std::map<QString, std::unique_ptr<Node>, CaseSensitiveComparator> children;

public:
    Node() = default;
    Node(qint64 nodeId, qint64 parentNodeId, const QString &nodeName)
        : id(nodeId), parentId(parentNodeId), name(nodeName) {}
};
