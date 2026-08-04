#include "app/application.hh"

#include "test_support.hh"

#include <iostream>

namespace {

void testApplicationUsesTheDocumentedFixedTickPeriod() {
    HOJY_CHECK_EQ(hojy::app::Application::FixedTickMicros, 16666ULL);
    HOJY_CHECK_EQ(hojy::app::Application::CompatibilityDivisor, 4U);
}

}

int main() {
    try {
        testApplicationUsesTheDocumentedFixedTickPeriod();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
