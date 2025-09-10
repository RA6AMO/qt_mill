// Errors.h — типизированные исключения домена/БД/операций
#pragma once

#include <stdexcept>
#include <QString>

namespace Errors {

struct DuplicateName : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct InvalidName : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct NotFound : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct MoveIntoDescendant : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct DbError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}
