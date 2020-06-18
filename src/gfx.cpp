#include <algorithm>
#include <iomanip>
#include <sstream>
#include <deque>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "creature.h"
#include "gfx.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "command.h"
#include "textutil.h"
#include "config.h"

void gfx_drawMap(System &state) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    int tileScale = state.config->getInt("tile_scale", 1);
    if (tileScale < 1) tileScale = 1;
    const int scaledTileWidth = tileWidth * tileScale;
    const int scaledTileHeight = tileHeight * tileScale;

    const int sidebarWidth = 30 * state.smallFont->getCharWidth();
    const int mapOffsetX = scaledTileWidth / 3 * 2;
    const int mapOffsetY = scaledTileHeight / 3 * 2;
    const int mapWidthPixels = screenWidth - sidebarWidth;
    const int mapHeightPixels = screenHeight;
    const int mapWidthTiles = mapWidthPixels / scaledTileWidth + 2;
    const int mapHeightTiles = mapHeightPixels / scaledTileHeight + 2;

    Animation noAnimation{AnimType::None};
    Animation &curAnim = state.animationQueue.empty() ? noAnimation : state.animationQueue.front();

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    // DRAW MAP
    int viewX = state.getPlayer()->position.x() - (mapWidthTiles / 2);
    int viewY = state.getPlayer()->position.y() - (mapHeightTiles  / 2);

    SDL_Rect clipRect = { 0, 0, mapWidthPixels, mapHeightPixels };
    SDL_RenderSetClipRect(state.renderer, &clipRect);
    bool didAnimation = false;
    for (int y = 0; y < mapHeightTiles; ++y) {
        for (int x = 0; x < mapWidthTiles; ++x) {
            const Point here(viewX + x, viewY + y);
            if (!state.getBoard()->valid(here)) {
                continue;
            }
            bool visible = state.getBoard()->isVisible(here);
            SDL_Rect texturePosition = {
                x * scaledTileWidth - mapOffsetX,
                y * scaledTileHeight - mapOffsetY,
                scaledTileWidth, scaledTileHeight
            };

            if (state.getBoard()->isKnown(here)) {
                int tileHere = state.getBoard()->getTile(here);

                if (tileHere != tileOutOfBounds) {
                    const TileInfo &tileInfo = TileInfo::get(tileHere);
                    // SDL_Texture *tile = tileInfo.art;
                    // if (tileInfo.animLength > 0) {
                    //     tile = state.getTile(tileInfo.artIndex + ((state.framecount / 12) % (tileInfo.animLength + 1)));
                    // } else {
                    //     tile = state.getTile(tileInfo.artIndex);
                    // }
                    if (tileInfo.art) {
                        if (visible) SDL_SetTextureColorMod(tileInfo.art, 255, 255, 255);
                        else         SDL_SetTextureColorMod(tileInfo.art,  96,  96,  96);
                        SDL_RenderCopy(state.renderer, tileInfo.art, nullptr, &texturePosition);
                    }
                }

                if (visible) {
                    Creature *creature = state.getBoard()->actorAt(here);
                    if (creature) {
                        SDL_Texture *tile = creature->typeInfo->art;
                        if (tile) {
                            SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                        }
                        double hpPercent = static_cast<double>(creature->curHealth) / creature->typeInfo->maxHealth;
                        if (hpPercent < 1.0) {
                            SDL_Rect box = texturePosition;
                            box.h = tileScale;
                            box.w *= hpPercent;
                            SDL_SetRenderDrawColor(state.renderer, 127, 255, 127, SDL_ALPHA_OPAQUE);
                            SDL_RenderFillRect(state.renderer, &box);
                        }
                    }
                }
            }

            if (state.getBoard()->at(here).mark) {
                SDL_Rect markBox = {
                    texturePosition.x + 2,
                    texturePosition.y + 2,
                    8, 8
                };
                SDL_SetRenderDrawColor(state.renderer, 63, 63, 196, 63);
                SDL_RenderFillRect(state.renderer, &markBox);
            }

            if (state.cursor == here) {
                SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(state.renderer, &texturePosition);
            }

            if (!didAnimation) {
                switch(curAnim.type) {
                    case AnimType::None:
                        // do nothing
                        if (!state.animationQueue.empty()) {
                            state.animationQueue.pop_front();
                        }
                        break;
                    case AnimType::Travel:
                        // draw first cell
                        if (here == curAnim.points.front()) {
                            didAnimation = true;
                            curAnim.points.pop_front();
                            if (visible) {
                                // SDL_Texture *tile = state.getTile(curAnim.tileNum);
                                // SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                            }
                            if (curAnim.points.empty()) {
                                state.animationQueue.pop_front();
                            }
                        }
                        break;
                    case AnimType::All:
                        // draw all cells
                        if (std::find(curAnim.points.begin(), curAnim.points.end(), here) != curAnim.points.end()) {
                            didAnimation = true;
                            // SDL_Texture *tile = state.getTile(curAnim.tileNum);
                            // SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                        }
                        break;
                }
            }
        }
    }
    if (curAnim.type == AnimType::All) {
        state.animationQueue.pop_front();
    }
    if (curAnim.type == AnimType::Travel && !didAnimation) {
        curAnim.points.pop_front();
        if (curAnim.points.empty()) {
            state.animationQueue.pop_front();
        }
    }
    SDL_RenderSetClipRect(state.renderer, nullptr);
}

void gfx_drawSidebar(System &state) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    int tileScale = state.config->getInt("tile_scale", 1);
    if (tileScale < 1) tileScale = 1;
    const int scaledTileWidth = tileWidth * tileScale;
    const int scaledTileHeight = tileHeight * tileScale;

    const int sidebarWidth = 30 * state.smallFont->getCharWidth();
    const int sidebarX = screenWidth - sidebarWidth;
    const int mapOffsetX = scaledTileWidth / 3 * 2;
    const int mapOffsetY = scaledTileHeight / 3 * 2;
    const int mapWidthPixels = screenWidth - sidebarWidth;
    const int mapHeightPixels = screenHeight;
    const int mapWidthTiles = mapWidthPixels / scaledTileWidth + 2;
    const int mapHeightTiles = mapHeightPixels / scaledTileHeight + 2;
    int viewX = state.getPlayer()->position.x() - (mapWidthTiles / 2);
    int viewY = state.getPlayer()->position.y() - (mapHeightTiles  / 2);

    const Color uiColor{255, 255, 255};

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  PLAYER INFO
    int yPos = 0;
    int xPos = sidebarX + 8;
    const int lineHeight = state.smallFont->getLineHeight();
    const int tinyLineHeight = state.tinyFont->getLineHeight();
    const int barHeight = lineHeight / 2;
    const int column2 = xPos + sidebarWidth / 2;
    Creature *player = state.getPlayer();
    const double healthPercent = static_cast<double>(player->curHealth) / static_cast<double>(player->typeInfo->maxHealth);
    const double energyPercent = static_cast<double>(player->curEnergy) / static_cast<double>(player->typeInfo->maxEnergy);

    state.smallFont->out(xPos, yPos, player->name);
    yPos += lineHeight;
    gfx_DrawBar(state, xPos, yPos, sidebarWidth - 16, barHeight, healthPercent, Color{127,255,127});
    yPos += barHeight;
    gfx_DrawBar(state, xPos, yPos, sidebarWidth - 16, barHeight, energyPercent, Color{127,127,255});
    yPos += barHeight * 2;

    state.tinyFont->out(xPos, yPos, "HP: " + std::to_string(player->curHealth) + "/" + std::to_string(player->typeInfo->maxHealth));
    state.tinyFont->out(column2, yPos, "EN: " + std::to_string(player->curEnergy) + "/" + std::to_string(player->typeInfo->maxEnergy));
    yPos += tinyLineHeight;
    const int statTop = yPos;

    if (state.mapEditMode) {
        SDL_Rect artPos{ xPos, yPos, 32, 32 };
        const TileInfo &info = TileInfo::get(state.mapEditTile);
        SDL_RenderCopy(state.renderer, info.art, nullptr, &artPos);
        state.tinyFont->out(xPos + 36, yPos, std::to_string(state.mapEditTile));
        yPos += tinyLineHeight;
        state.tinyFont->out(xPos + 36, yPos, info.name);
        yPos += tinyLineHeight;

    } else {
        SDL_Rect artPos{ xPos, yPos, 16, 16 };
        SDL_Texture *swordArt = state.getImage("ui/sword.png");
        SDL_RenderCopy(state.renderer, swordArt, nullptr, &artPos);
        state.tinyFont->out(xPos + 20, yPos, std::to_string(state.swordLevel));
        yPos += 20; artPos.y += 20;
        swordArt = state.getImage("ui/armour.png");
        SDL_RenderCopy(state.renderer, swordArt, nullptr, &artPos);
        state.tinyFont->out(xPos + 20, yPos, std::to_string(state.armourLevel));
        yPos += 40; artPos.y += 40;
        swordArt = state.getImage("ui/arrow.png");
        SDL_RenderCopy(state.renderer, swordArt, nullptr, &artPos);
        state.tinyFont->out(xPos + 20, yPos, std::to_string(state.arrowCount) + "/" + std::to_string(state.arrowCapacity));
        yPos += 20; artPos.y += 20;
        swordArt = state.getImage("ui/bomb.png");
        SDL_RenderCopy(state.renderer, swordArt, nullptr, &artPos);
        state.tinyFont->out(xPos + 20, yPos, std::to_string(state.bombCount) + "/" + std::to_string(state.bombCapacity));
        yPos += 20; artPos.y += 20;
        swordArt = state.getImage("ui/coin.png");
        SDL_RenderCopy(state.renderer, swordArt, nullptr, &artPos);
        state.tinyFont->out(xPos + 20, yPos, std::to_string(state.coinCount));
        int firstYPos = yPos + 10;

        yPos = statTop;
        int xPos2 = xPos + sidebarWidth / 2;
        artPos.x = xPos2;
        for (int i = 0; i < SW_COUNT; ++i) {
            if (state.subweaponLevel[i] <= 0) continue;
            SDL_Texture *swordArt = state.getImage(state.subweapons[i].artfile);
            artPos.y = yPos;
            SDL_RenderCopy(state.renderer, swordArt, nullptr, &artPos);
            if (i == SW_BOW) {
                state.tinyFont->out(xPos2 + 20, yPos, std::to_string(state.subweaponLevel[i]));
            }
            if (i == state.currentSubweapon) {
                swordArt = state.getImage("ui/subweapon_cursor.png");
                SDL_Rect cursorRect { xPos2 - 8, yPos, 8, 16 };
                SDL_RenderCopy(state.renderer, swordArt, nullptr, &cursorRect);
            }
            yPos += 20;
        }
        yPos -= 10;
        if (firstYPos > yPos) yPos = firstYPos;
    }

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  MAP AND TIMER INFO
    const int mapInfoTop = yPos + tinyLineHeight;
    gfx_HLine(state, sidebarX, screenWidth, mapInfoTop - 1, uiColor);
    yPos += tinyLineHeight * 1.5;
    state.tinyFont->out(xPos, yPos, state.getBoard()->getInfo().name);
    yPos += tinyLineHeight / 2;

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  MESSAGE LOG
    const int logTop = yPos + tinyLineHeight;
    const int wrapWidth = (sidebarWidth - 8) / state.tinyFont->getCharWidth() - 2;
    const SDL_Rect logArea = {
        sidebarX, logTop,
        screenWidth - sidebarX,
        screenHeight - logTop
    };
    SDL_RenderSetClipRect(state.renderer, &logArea);
    yPos = screenHeight - tinyLineHeight;
    for (int i = state.messages.size() - 1; i >= 0; --i) {
        Color lineColour = Color{160,160,160};
        const Message &m = state.messages[i];
        if (m.newTurns >= state.turnNumber - 1) {
            lineColour = {255, 255, 255};
        }
        bool wasRule = false;
        if (m.text == "---") {
            gfx_HLine(state, sidebarX + 8, screenWidth - 8, yPos + tinyLineHeight / 2, Color{160, 160, 160});
            yPos -= tinyLineHeight;
            wasRule = true;
        } else {
            std::vector<std::string> lines;
            std::string text = m.text;
            wordwrap(text, wrapWidth, lines);
            for (int j = lines.size() - 1; j >= 0; --j) {
                const std::string &line = lines[j];
                if (j == 0) {
                    state.tinyFont->out(xPos, yPos, line, lineColour);
                } else {
                    state.tinyFont->out(xPos, yPos, "  " + line, lineColour);
                }
                yPos -= tinyLineHeight;
                if (yPos < logTop - tinyLineHeight) break;
            }
        }
        if (!wasRule) yPos -= tinyLineHeight / 4;
        if (yPos < logTop - tinyLineHeight) break;
    }
    SDL_RenderSetClipRect(state.renderer, nullptr);
    gfx_HLine(state, sidebarX, screenWidth, logTop - 1, uiColor);

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  FPS INFO
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (mouseX < mapWidthPixels) {
        int mapMouseX = mouseX + mapOffsetX;
        int mapMouseY = mouseY + mapOffsetY;
        int tileX = viewX + mapMouseX / (scaledTileWidth);
        int tileY = viewY + mapMouseY / (scaledTileHeight);
            Point here(tileX, tileY);
        int tileHere = state.getBoard()->getTile(here);
        Creature *creature = state.getBoard()->actorAt(here);
        if (tileHere != tileOutOfBounds) {
            if (state.showTooltip && state.getBoard()->isKnown(here)) { // show tooltip
                std::stringstream ss;
                if (creature && state.getBoard()->isVisible(here)) {
                    ss << creature->name << "\n";
                }
                const TileInfo &tileInfo = TileInfo::get(tileHere);
                ss << tileInfo.name;
                gfx_DrawTooltip(state, mouseX, mouseY + 20, ss.str());
            }
            if (state.showInfo) {
                std::stringstream ss;
                if (creature) {
                    ss << creature->getName() << "   ";
                }
                const TileInfo &tileInfo = TileInfo::get(tileHere);
                ss << tileInfo.name << " [" << tileHere << "]   " << tileX << ", " << tileY;
                ss << " V:" << state.getBoard()->isVisible(here);
                ss << " K:" << state.getBoard()->isKnown(here);
                ss << " MAPID: " << state.depth;
                ss << " Turn: " << state.turnNumber;
                ss << " track: " << state.getTrackNumber();
                state.smallFont->out(screenWidth - (ss.str().size() + 1) * state.smallFont->getCharWidth(), screenHeight - state.smallFont->getLineHeight() * 2, ss.str());
                const TrackInfo &info = state.getTrackInfo();
                if (info.number >= 0) {
                    std::string line = info.name + " by " + info.artist;
                    state.smallFont->out(screenWidth - (line.size() + 1) * state.smallFont->getCharWidth(), screenHeight - state.smallFont->getLineHeight(), line);
                }
            }
        }
    }
    if (state.showFPS) {
        std::stringstream ss;
        ss << "  FPS: " << state.getFPS();
        state.smallFont->out(0, 0, ss.str().c_str());
    }
}

bool repaint(System &state, bool callPresent) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    const int sidebarWidth = 30 * state.smallFont->getCharWidth();
    const int mapWidthPixels = screenWidth - sidebarWidth;
    const int mapHeightPixels = screenHeight;

    const Color uiColor{255, 255, 255};

    gfx_Clear(state);

    Animation noAnimation{AnimType::None};

    gfx_drawMap(state);
    gfx_drawSidebar(state);

    gfx_HLine(state, 0, screenWidth, mapHeightPixels, uiColor);
    gfx_VLine(state, mapWidthPixels, 0, mapHeightPixels, uiColor);

    state.advanceFrame();
    if (callPresent) SDL_RenderPresent(state.renderer);
    return false;
}

void gfx_frameDelay(System &state) {
    repaint(state);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const CommandDef &cmd = getCommand(state, event, gameCommands);
        if (cmd.command == Command::Quit) {
            state.wantsToQuit = true;
            return;
        }
    }
}
