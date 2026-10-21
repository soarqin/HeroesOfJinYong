#include "text_input.hh"

#include <SDL.h>

namespace hojy::app {

void TextInputService::begin() const {
    SDL_StartTextInput();
}

void TextInputService::setRect(int x, int y, int w, int h) const {
    SDL_Rect rect{x, y, w, h};
    SDL_SetTextInputRect(&rect);
}

void TextInputService::end() const {
    SDL_StopTextInput();
}

TextInputService &textInput() {
    static TextInputService service;
    return service;
}

}
