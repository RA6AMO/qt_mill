// Реализация фабрики имён/узлов: trim/валидация и создание Node
#include "INodeFactory.h"
#include <stdexcept>

namespace Errors {
struct InvalidName : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
}

class NodeFactory final : public INodeFactory {
public:
    QString normalizeName(const QString &raw) const override {
        QString trimmed = raw;
        return trimmed.trimmed();
    }

    void validateName(const QString &name) const override {
        if (name.isEmpty()) {
            throw Errors::InvalidName("Empty name");
        }
        if (name.size() > 255) {
            throw Errors::InvalidName("Name too long");
        }
        if (name.contains('/')) {
            throw Errors::InvalidName("Name contains '/'");
        }
    }

    std::unique_ptr<Node> create(qint64 id, qint64 parentId, const QString &name) const override {
        return std::make_unique<Node>(id, parentId, name);
    }
};

// Фабричный метод для удобного получения реализации (можно расширить DI позже)
std::unique_ptr<INodeFactory> makeNodeFactory() {
    return std::make_unique<NodeFactory>();
}
