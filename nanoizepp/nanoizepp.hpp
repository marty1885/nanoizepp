#pragma once

#include <string>
#include <string_view>

namespace nanoizepp {
/**
 * @brief Miniaturize HTML
 * @param html HTML to miniaturize
 * @return Miniaturized HTML
*/
std::string nanoize(std::string_view html, size_t indent = 0, bool newline = false);
}