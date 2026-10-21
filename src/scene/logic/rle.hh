#pragma once

#include <string>

namespace hojy::scene::logic {

/** Validate the legacy row-RLE container without resolving render resources. */
[[nodiscard]] bool validateRleData(const std::string &data) noexcept;

}
