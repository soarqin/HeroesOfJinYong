#pragma once

namespace hojy::app {

/** Platform text-input bridge. Scene logic only requests operations through this interface. */
class TextInputService final {
public:
    void begin() const;
    void setRect(int x, int y, int w, int h) const;
    void end() const;
};

TextInputService &textInput();

}
