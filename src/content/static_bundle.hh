#pragma once

namespace hojy::content {
struct Factors;
class Event;
class WarfieldData;
}

namespace hojy::content {

// Read-only logical view over the legacy resource owners.  It avoids copying
// large tables while consumers migrate away from direct global access.
class StaticBundle final {
public:
    StaticBundle(const ::hojy::content::Factors &factors,
                 const ::hojy::content::Event &events,
                 const ::hojy::content::WarfieldData &warfields) noexcept:
        factors_(&factors), events_(&events), warfields_(&warfields) {
    }

    [[nodiscard]] const ::hojy::content::Factors &factors() const noexcept { return *factors_; }
    [[nodiscard]] const ::hojy::content::Event &events() const noexcept { return *events_; }
    [[nodiscard]] const ::hojy::content::WarfieldData &warfields() const noexcept { return *warfields_; }

private:
    const ::hojy::content::Factors *factors_ = nullptr;
    const ::hojy::content::Event *events_ = nullptr;
    const ::hojy::content::WarfieldData *warfields_ = nullptr;
};

}
