#pragma once

#include <sstream>
#include <stdexcept>
#include <utility>

namespace hojy::test {

template<typename A, typename B>
void checkEqual(const A &actual, const B &expected, const char *actualExpr,
                const char *expectedExpr, const char *file, int line) {
    if (actual == expected) { return; }
    std::ostringstream stream;
    stream << file << ':' << line << ": expected " << expectedExpr
           << ", got " << actualExpr;
    throw std::runtime_error(stream.str());
}

template<typename Exception, typename Func>
void checkThrows(Func &&func, const char *expression, const char *exceptionName,
                 const char *file, int line) {
    try {
        std::forward<Func>(func)();
    } catch (const Exception &) {
        return;
    } catch (...) {
        std::ostringstream stream;
        stream << file << ':' << line << ": " << expression
               << " threw an unexpected exception instead of " << exceptionName;
        throw std::runtime_error(stream.str());
    }
    std::ostringstream stream;
    stream << file << ':' << line << ": " << expression
           << " did not throw " << exceptionName;
    throw std::runtime_error(stream.str());
}

}

#define HOJY_CHECK_EQ(actual, expected) \
    ::hojy::test::checkEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define HOJY_CHECK_THROWS(exceptionType, expression) \
    ::hojy::test::checkThrows<exceptionType>([&]() { expression; }, #expression, \
                                              #exceptionType, __FILE__, __LINE__)
