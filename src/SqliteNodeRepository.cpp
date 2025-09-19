// SqliteNodeRepository — реализация INodeRepository поверх QtSql (все write — транзакции)
//
// Ключевые моменты реализации:
//  - Используется QSqlDatabase/QSqlQuery. Все операции изменения данных (insert/update/delete)
//    обёрнуты в транзакции (transaction/commit, при ошибках — rollback).
//  - Ошибки БД маппятся на исключения Errors::DbError, нарушения уникальности — на Errors::DuplicateName.
//  - Временные метки updated_at/created_at пишутся в формате ISO UTC (см. nowIso()).
//  - Семантика optional соответствует контракту интерфейса: NULL в БД => пустой optional.
#include "INodeRepository.h"
#include "Errors.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDateTime>
#include <qobject.h>
#include <qtmetamacros.h>

static QString nowIso() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

class SqliteNodeRepository final : public INodeRepository  {

public:
    explicit SqliteNodeRepository(const QSqlDatabase& db)
        : m_db(std::move(db)) {}

    qint64 insert(qint64 parentId, const QString &name, const std::optional<QString> &payload) override {
        // Вставка дочернего узла. Уникальность имени среди детей одного родителя
        // обеспечивается уникальным индексом на (parent_id, name) на стороне БД.
        if (!m_db.transaction()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }
        const QString ts = nowIso();
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO nodes(parent_id, name, payload, created_at, updated_at) VALUES(?, ?, ?, ?, ?)");
        q.addBindValue(parentId);
        q.addBindValue(name);
        if (payload.has_value()) q.addBindValue(payload.value()); else q.addBindValue(QVariant(QVariant::String));
        q.addBindValue(ts);
        q.addBindValue(ts);
        if (!q.exec()) {
            m_db.rollback();
            const auto err = q.lastError().text();
            if (err.contains("UNIQUE")) {
                throw Errors::DuplicateName(err.toStdString());
            }
            throw Errors::DbError(err.toStdString());
        }
        qint64 id = q.lastInsertId().toLongLong();
        if (!m_db.commit()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }

        return id;
    }

    void updateName(qint64 id, const QString &newName) override {
        if (!m_db.transaction()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }

        // Перед обновлением проверяем существование узла (через получение parent_id).
        // Уникальность среди сиблингов обеспечивает индекс (parent_id, name) в БД.
        auto parentOpt = getParentId(id);
        if (!parentOpt.has_value()) {
            m_db.rollback();
            throw Errors::NotFound("Node not found");
        }

        QSqlQuery q(m_db);
        q.prepare("UPDATE nodes SET name = ?, updated_at = ? WHERE id = ?");
        q.addBindValue(newName);
        q.addBindValue(nowIso());
        q.addBindValue(id);
        if (!q.exec()) {
            m_db.rollback();
            const auto err = q.lastError().text();
            if (err.contains("UNIQUE")) throw Errors::DuplicateName(err.toStdString());
            throw Errors::DbError(err.toStdString());
        }
        if (!m_db.commit()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }
    }

    void updateParent(qint64 id, qint64 newParentId) override {
        if (!m_db.transaction()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }
        QSqlQuery q(m_db);
        q.prepare("UPDATE nodes SET parent_id = ?, updated_at = ? WHERE id = ?");
        q.addBindValue(newParentId);
        q.addBindValue(nowIso());
        q.addBindValue(id);
        if (!q.exec()) {
            m_db.rollback();
            const auto err = q.lastError().text();
            if (err.contains("UNIQUE")) throw Errors::DuplicateName(err.toStdString());
            throw Errors::DbError(err.toStdString());
        }
        if (!m_db.commit()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }
    }

    void remove(qint64 id) override {
        if (!m_db.transaction()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }
        QSqlQuery q(m_db);
        q.prepare("DELETE FROM nodes WHERE id = ?");
        q.addBindValue(id);
        if (!q.exec()) {
            m_db.rollback();
            throw Errors::DbError(q.lastError().text().toStdString());
        }
        if (!m_db.commit()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }
    }

    std::optional<RepoRow> get(qint64 id) override {
        QSqlQuery q(m_db);
        q.prepare("SELECT id, parent_id, name, payload FROM nodes WHERE id = ?");
        q.addBindValue(id);
        if (!q.exec()) {
            throw Errors::DbError(q.lastError().text().toStdString());
        }
        if (!q.next()) return std::nullopt;
        RepoRow r;
        r.id = q.value(0).toLongLong();
        if (q.value(1).isNull()) r.parentId.reset(); else r.parentId = q.value(1).toLongLong();
        r.name = q.value(2).toString();
        if (q.value(3).isNull()) r.payload.reset(); else r.payload = q.value(3).toString();
        return r;
    }

    std::optional<RepoRow> findChildByName(qint64 parentId, const QString &name) override {
        QSqlQuery q(m_db);
        q.prepare("SELECT id, parent_id, name, payload FROM nodes WHERE parent_id = ? AND name = ?");
        q.addBindValue(parentId);
        q.addBindValue(name);
        if (!q.exec()) throw Errors::DbError(q.lastError().text().toStdString());
        if (!q.next()) return std::nullopt;
        RepoRow r;
        r.id = q.value(0).toLongLong();
        if (q.value(1).isNull()) r.parentId.reset(); else r.parentId = q.value(1).toLongLong();
        r.name = q.value(2).toString();
        if (q.value(3).isNull()) r.payload.reset(); else r.payload = q.value(3).toString();
        return r;
    }

    std::vector<RepoRow> getChildren(qint64 parentId) override {
        QSqlQuery q(m_db);
        // Сортировка по имени в бинарной коллации для детерминированного порядка
        q.prepare("SELECT id, parent_id, name, payload FROM nodes WHERE parent_id = ? ORDER BY name COLLATE BINARY ASC");
        q.addBindValue(parentId);
        if (!q.exec()) throw Errors::DbError(q.lastError().text().toStdString());
        std::vector<RepoRow> rows;
        while (q.next()) {
            RepoRow r;
            r.id = q.value(0).toLongLong();
            if (q.value(1).isNull()) r.parentId.reset(); else r.parentId = q.value(1).toLongLong();
            r.name = q.value(2).toString();
            if (q.value(3).isNull()) r.payload.reset(); else r.payload = q.value(3).toString();
            rows.push_back(std::move(r));
        }
        return rows;
    }

    std::optional<qint64> getParentId(qint64 id) override {
        QSqlQuery q(m_db);
        q.prepare("SELECT parent_id FROM nodes WHERE id = ?");
        q.addBindValue(id);
        if (!q.exec()) throw Errors::DbError(q.lastError().text().toStdString());
        if (!q.next()) return std::nullopt;
        if (q.value(0).isNull()) return std::optional<qint64>{}; // корневой узел (parent_id IS NULL)
        return q.value(0).toLongLong();
    }

    bool hasChildren(qint64 id) override {
        QSqlQuery q(m_db);
        q.prepare("SELECT 1 FROM nodes WHERE parent_id = ? LIMIT 1");
        q.addBindValue(id);
        if (!q.exec()) throw Errors::DbError(q.lastError().text().toStdString());
        return q.next();
    }

    void setPayload(qint64 id, const QString &payloadJson) override {
        if (!m_db.transaction()) {
            throw Errors::DbError(m_db.lastError().text().toStdString());
        }
        QSqlQuery q(m_db);
        q.prepare("UPDATE nodes SET payload = ?, updated_at = ? WHERE id = ?");
        q.addBindValue(payloadJson);
        q.addBindValue(nowIso());
        q.addBindValue(id);
        if (!q.exec()) { m_db.rollback(); throw Errors::DbError(q.lastError().text().toStdString()); }
        if (!m_db.commit()) throw Errors::DbError(m_db.lastError().text().toStdString());
    }

    std::optional<QString> getPayload(qint64 id) override {
        QSqlQuery q(m_db);
        q.prepare("SELECT payload FROM nodes WHERE id = ?");
        q.addBindValue(id);
        if (!q.exec()) throw Errors::DbError(q.lastError().text().toStdString());
        if (!q.next()) return std::nullopt;
        if (q.value(0).isNull()) return std::optional<QString>{}; // payload IS NULL
        return q.value(0).toString();
    }

private:
    QSqlDatabase m_db;
};

std::unique_ptr<INodeRepository> makeSqliteNodeRepository(const QSqlDatabase &db) {
    return std::make_unique<SqliteNodeRepository>(db);
}
