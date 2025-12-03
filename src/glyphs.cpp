#include "glyphs.h"

namespace glyphs {

bool use_unicode = true;
bool use_color = true;

void init(bool unicode, bool color) {
    use_unicode = unicode;
    use_color = color;
}

std::string color(const char* code) {
    if (!use_color) return "";
    return std::string(code);
}

} // namespace glyphs


