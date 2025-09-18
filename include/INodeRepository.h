// INodeRepository — интерфейс доступа к SQLite (CRUD, выборки, payload)
//
// Назначение:
//  - Абстракция над таблицей nodes (иерархическое дерево: parent_id, name, payload).
//  - Чётко определяет контракт: как интерпретируются null/значения, что возвращается при отсутствии записи.
//  - Реализация должна заботиться о транзакциях для модифицирующих операций.
#pragma once

#include <QtGlobal>
#include <QString>
#include <optional>
#include <qtmetamacros.h>
#include <vector>
#include <QObject>



struct RepoRow {
    // Уникальный идентификатор строки (PRIMARY KEY)
    qint64 id {0};
    // Родительский узел. Semantics:
    //  - std::nullopt => поле отсутствует (обычно запись не найдена, см. методы, которые его заполняют)
    //  - пустой optional (has_value == false) в RepoRow означает, что parent_id = NULL (корень)
    std::optional<qint64> parentId;
    // Имя узла (используется в ограничении уникальности вместе с parent_id)
    QString name;
    // Дополнительные данные. Semantics:
    //  - std::nullopt => поле payload в БД равно NULL
    //  - std::optional("") => пустая строка сохранена в БД как пустая строка, не NULL
    std::optional<QString> payload;
};

class INodeRepository : public QObject {
public:
    virtual ~INodeRepository() = default;

    Q_OBJECT
    // Создаёт новую запись узла.
    //  - parentId: идентификатор родителя (ожидается существование)
    //  - name: имя узла; в БД предполагается уникальный индекс (parent_id, name)
    //  - payload: если не задано => сохранится NULL, иначе строка
    // Возвращает id вставленной записи. Должна быть транзакция.
    virtual qint64 insert(qint64 parentId, const QString &name, const std::optional<QString> &payload) = 0;

    // Обновляет имя узла по id. Должна учитывать уникальность среди сиблингов.
    virtual void updateName(qint64 id, const QString &newName) = 0;

    // Меняет родителя узла. Должна учитывать уникальность (parent_id, name).
    virtual void updateParent(qint64 id, qint64 newParentId) = 0;

    // Удаляет узел по id. Если есть внешние ключи/потомки — ответственность на уровне БД/вызывающего кода.
    virtual void remove(qint64 id) = 0;

    // Возвращает полную запись по id.
    //  - std::nullopt => запись не найдена
    virtual std::optional<RepoRow> get(qint64 id) = 0;

    // Ищет прямого потомка по имени у заданного родителя.
    //  - std::nullopt => не найден
    virtual std::optional<RepoRow> findChildByName(qint64 parentId, const QString &name) = 0;

    // Возвращает всех детей родителя, отсортированных по имени.
    virtual std::vector<RepoRow> getChildren(qint64 parentId) = 0;

    // Возвращает parent_id для узла.
    //  - std::nullopt => запись не найдена
    //  - пустой optional (has_value == false) => parent_id IS NULL (корневой узел)
    virtual std::optional<qint64> getParentId(qint64 id) = 0;

    // Быстрая проверка наличия хотя бы одного ребёнка.
    virtual bool hasChildren(qint64 id) = 0;

    // Для payload:
    //  - setPayload: сохраняет строку (не NULL). Для обнуления поля используйте отдельный метод (если появится)
    //  - getPayload: std::nullopt если запись не найдена; пустой optional если payload IS NULL
    virtual void setPayload(qint64 id, const QString &payloadJson) = 0;
    virtual std::optional<QString> getPayload(qint64 id) = 0;

    signals:
        void treeMapChanged();
};

// Фабрика репозитория (скрываем QSqlDatabase в реализационном cpp)
// Ожидается, что реализация инкапсулирует подключение и обеспечит корректные транзакции для write-операций.
class QSqlDatabase;
std::unique_ptr<INodeRepository> makeSqliteNodeRepository(const QSqlDatabase &db);
