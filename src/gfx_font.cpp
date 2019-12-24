#include <SDL2/SDL.h>

#include "gfx.h"

Font::Font(SDL_Renderer *renderer, SDL_Texture *texture)
: mRenderer(renderer), mTexture(texture), mCharWidth(8), mCharHeight(8),
  mLineSpace(10)
{ }

Font::~Font() {
}

void Font::setMetrics(int width, int height, int linespace) {
    mCharWidth = width;
    mCharHeight = height;
    mLineSpace = linespace;
}

int Font::getCharWidth() const {
    return mCharWidth;
}

int Font::getCharHeight() const {
    return mCharHeight;
}

int Font::getLineHeight() const {
    return mCharHeight + mLineSpace;
}

void Font::out(int x, int y, const std::string &text, const Color &defaultColor) {
    if (!mTexture || !mRenderer) return;

    SDL_SetTextureColorMod(mTexture, defaultColor.r, defaultColor.g, defaultColor.b);

    SDL_Rect src = { 0, 0, mCharWidth, mCharHeight };
    SDL_Rect dest = { x, y, mCharWidth, mCharHeight };
    for (std::string::size_type i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\n') {
            dest.y += getLineHeight();
            dest.x = x;
            continue;
        }
        if (c == 27) {
            // formatting escape
            ++i;
            if (i >= text.size()) break;
            int cmd = text[i];
            switch(cmd) {
                case 1: // reset formatting
                    SDL_SetTextureColorMod(mTexture, defaultColor.r, defaultColor.g, defaultColor.b);
                    break;
                case 2: { // set text colour
                    ++i;
                    if (i >= text.size()) break;
                    int r = text[i];
                    ++i;
                    if (i >= text.size()) break;
                    int g = text[i];
                    ++i;
                    if (i >= text.size()) break;
                    int b = text[i];
                    SDL_SetTextureColorMod(mTexture, r, g, b);
                    break; }
            }
            continue;
        }
        src.x = c * mCharWidth;
        SDL_RenderCopy(mRenderer, mTexture, &src, &dest);
        dest.x += mCharWidth;
    }
}
