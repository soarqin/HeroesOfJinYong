#include "content/static_bundle.hh"
#include "content/event.hh"
#include "content/factors.hh"
#include "content/warfielddata.hh"
#include "test_support.hh"

#include <iostream>

int main() {
    try {
        hojy::content::Factors factors{};
        hojy::content::Event events;
        hojy::content::WarfieldData warfields;
        const hojy::content::StaticBundle bundle(factors, events, warfields);
        HOJY_CHECK_EQ(&bundle.factors(), &factors);
        HOJY_CHECK_EQ(&bundle.events(), &events);
        HOJY_CHECK_EQ(&bundle.warfields(), &warfields);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
