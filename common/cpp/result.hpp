#ifndef QAPP_RESULT_HPP
#define QAPP_RESULT_HPP

#include <QString>
#include <utility>

namespace qapp {

/// Result type matching JS pattern: { success: true, data } | { success: false, error }
template<typename T>
struct Result {
    bool success;
    T data;
    QString error;

    static Result ok(T value) {
        return {true, std::move(value), {}};
    }
    static Result fail(const QString &msg) {
        return {false, T{}, msg};
    }
};

} // namespace qapp

#endif // QAPP_RESULT_HPP
