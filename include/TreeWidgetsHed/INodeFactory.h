// INodeFactory — нормализация/валидация имён и создание доменных узлов.
#pragma once

#include <QString>
#include <QtGlobal>
#include <memory>
#include "Node.h"

class INodeFactory {
public:
    virtual ~INodeFactory() = default;

    // trim + единые правила (без пустых и без '/')
    virtual QString normalizeName(const QString &raw) const = 0;

    // пробрасывает InvalidName при нарушении правил
    virtual void validateName(const QString &name) const = 0;

    // Создаёт доменную сущность (без UI-ссылок)
    virtual std::unique_ptr<Node> create(qint64 id, qint64 parentId, const QString &name) const = 0;
};

// Фабричная функция для получения стандартной реализации
std::unique_ptr<INodeFactory> makeNodeFactory();
