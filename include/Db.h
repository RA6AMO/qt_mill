// Db.h — открытие SQLite, включение PRAGMA foreign_keys и миграции (без утечки QtSql в заголовок)
#pragma once

#include <QString>

// Обёртка для инициализации SQLite + миграции (без утечки QtSql в заголовок)
class Db {
public:
    // Открывает/реинициализирует соединение по connectionName, применяет миграции
    // Возвращает имя соединения (то же самое, что передали)
    static QString openAndInit(const QString &connectionName, const QString &filePath);

    // Имя таблицы
    static constexpr const char* TABLE_NODES = "nodes";
};
