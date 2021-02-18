#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <utility>
#include <cstdint>

namespace hojy::util {

class Conv {
public:
    std::wstring toUnicode(std::string_view str);
    std::string fromUnicode(std::wstring_view wstr);

protected:
    void postInit();

protected:
    std::vector<std::pair<std::uint16_t, std::uint16_t>> table_, tableRev_;
};

class Big5Conv: public Conv {
public:
    Big5Conv();
};

extern Big5Conv big5Conv;

}
