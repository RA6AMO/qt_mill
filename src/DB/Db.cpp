// Db.cpp — реализация открытия/миграций SQLite и обеспечения корня (id=1)
#include "Db.h"
#include "Errors.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDateTime>

namespace {
void execOrThrow(QSqlDatabase &db, const QString &sql) {
    QSqlQuery q(db);
    if (!q.exec(sql)) {
        throw Errors::DbError(q.lastError().text().toStdString());
    }
}

void enableForeignKeys(QSqlDatabase &db) {
    execOrThrow(db, "PRAGMA foreign_keys = ON");
}

void applyMigrations(QSqlDatabase &db) {
    const QString createNodes = R"SQL(
CREATE TABLE IF NOT EXISTS nodes (
    id INTEGER PRIMARY KEY,
    parent_id INTEGER NULL REFERENCES nodes(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    payload TEXT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);)SQL";
    execOrThrow(db, createNodes);
    execOrThrow(db, "CREATE UNIQUE INDEX IF NOT EXISTS idx_nodes_parent_name ON nodes(parent_id, name)");
    execOrThrow(db, "CREATE INDEX IF NOT EXISTS idx_nodes_parent ON nodes(parent_id)");
}

void ensureRoot(QSqlDatabase &db) {
    QSqlQuery check(db);
    check.prepare("SELECT id FROM nodes WHERE id = 1");
    if (!check.exec()) {
        throw Errors::DbError(check.lastError().text().toStdString());
    }
    if (!check.next()) {
        const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        QSqlQuery ins(db);
        ins.prepare("INSERT INTO nodes (id, parent_id, name, payload, created_at, updated_at) VALUES (1, NULL, '', NULL, ?, ?)");
        ins.addBindValue(now);
        ins.addBindValue(now);
        if (!ins.exec()) {
            throw Errors::DbError(ins.lastError().text().toStdString());
        }
    }
}
}

QString Db::openAndInit(const QString &connectionName, const QString &filePath) {
    QSqlDatabase db;
    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
        if (!db.isOpen()) {
            db.open();
        }
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(filePath);
        if (!db.open()) {
            throw Errors::DbError("Cannot open SQLite DB: " + db.lastError().text().toStdString());
        }
    }
    enableForeignKeys(db);
    applyMigrations(db);
    ensureRoot(db);
    return connectionName;
}
