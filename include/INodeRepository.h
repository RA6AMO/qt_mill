// INodeRepository — интерфейс доступа к SQLite (CRUD, выборки, payload)
#pragma once

#include <QtGlobal>
#include <QString>
#include <optional>
#include <vector>

struct RepoRow {
    qint64 id {0};
    std::optional<qint64> parentId;
    QString name;
    std::optional<QString> payload;
};

class INodeRepository {
public:
    virtual ~INodeRepository() = default;

    virtual qint64 insert(qint64 parentId, const QString &name, const std::optional<QString> &payload) = 0;
    virtual void updateName(qint64 id, const QString &newName) = 0;
    virtual void updateParent(qint64 id, qint64 newParentId) = 0;
    virtual void remove(qint64 id) = 0;

    virtual std::optional<RepoRow> get(qint64 id) = 0;
    virtual std::optional<RepoRow> findChildByName(qint64 parentId, const QString &name) = 0;
    virtual std::vector<RepoRow> getChildren(qint64 parentId) = 0;
    virtual std::optional<qint64> getParentId(qint64 id) = 0;

    virtual bool hasChildren(qint64 id) = 0;

    // Для payload
    virtual void setPayload(qint64 id, const QString &payloadJson) = 0;
    virtual std::optional<QString> getPayload(qint64 id) = 0;
};

// Фабрика репозитория (скрываем QSqlDatabase в реализационном cpp)
class QSqlDatabase;
std::unique_ptr<INodeRepository> makeSqliteNodeRepository(const QSqlDatabase &db);
