#include <algorithm>
#include <iomanip>
#include <sstream>
#include <deque>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "actor.h"
#include "gamestate.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "command.h"
#include "textutil.h"
#include "config.h"
#include "logger.h"

void gfx_drawMap(GameState &state, const AnimFrame *frame) {
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

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    // DRAW MAP
    int viewX = state.getPlayer()->position.x() - (mapWidthTiles / 2);
    int viewY = state.getPlayer()->position.y() - (mapHeightTiles  / 2);

    SDL_Rect clipRect = { 0, 0, mapWidthPixels, mapHeightPixels };
    SDL_RenderSetClipRect(state.renderer, &clipRect);
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
                    SDL_Texture *tile = tileInfo.art;
                    if (tileInfo.animLength > 1) {
                        int frameNumber = (state.framecount / 12) % tileInfo.animLength;
                        tile = tileInfo.frames[frameNumber];
                    }
                    if (tile) {
                        if (visible) SDL_SetTextureColorMod(tile, 255, 255, 255);
                        else         SDL_SetTextureColorMod(tile,  96,  96,  96);
                        SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                    }
                }

                if (visible) {
                    Actor *actor = state.getBoard()->actorAt(here);
                    Item *item = state.getBoard()->itemAt(here);
                    if (actor) {
                        SDL_Texture *tile = actor->typeInfo->art;
                        if (tile) {
                            SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                        }
                        double hpPercent = static_cast<double>(actor->curHealth) / actor->typeInfo->maxHealth;
                        if (hpPercent < 1.0) {
                            SDL_Rect box = texturePosition;
                            box.h = tileScale;
                            box.w *= hpPercent;
                            SDL_SetRenderDrawColor(state.renderer, 127, 255, 127, SDL_ALPHA_OPAQUE);
                            SDL_RenderFillRect(state.renderer, &box);
                        }
                    } else if (item) {
                        SDL_Texture *tile = item->typeInfo->art;
                        if (tile) {
                            SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                        }
                    }

                    if (frame) {
                        auto iter = frame->data.find(here);
                        if (iter != frame->data.end()) {
                            SDL_RenderCopy(state.renderer, iter->second, nullptr, &texturePosition);
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
        }
    }
    SDL_RenderSetClipRect(state.renderer, nullptr);
}

void gfx_drawSidebar(GameState &state) {
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
    Actor *player = state.getPlayer();
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

    if (state.mapEditTile >= 0) {
        SDL_Rect artPos{ xPos, yPos, 32, 32 };
        const TileInfo &info = TileInfo::get(state.mapEditTile);
        SDL_RenderCopy(state.renderer, info.art, nullptr, &artPos);
        state.tinyFont->out(xPos + 36, yPos, std::to_string(state.mapEditTile));
        yPos += tinyLineHeight;
        state.tinyFont->out(xPos + 36, yPos, info.name);
        yPos += tinyLineHeight;
        state.tinyFont->out(xPos + 36, yPos, std::to_string(player->position.x()) + ", " + std::to_string(player->position.y()));
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
    std::stringstream mapName;
    mapName << state.getBoard()->getInfo().name;
    const World &world = state.getWorld();
    if (world.index >= 0) {
        mapName << " (" << world.name << ')';
    }
    state.tinyFont->out(xPos, yPos, mapName.str());
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
        Actor *actor = state.getBoard()->actorAt(here);
        if (tileHere != tileOutOfBounds && state.showInfo) {
            std::stringstream ss;
            if (actor) {
                ss << actor->getName() << "   ";
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

    gfx_VLine(state, mapWidthPixels, 0, mapHeightPixels, uiColor);
    if (state.showFPS) {
        std::stringstream ss;
        ss << "  FPS: " << state.getFPS();
        state.smallFont->out(0, 0, ss.str().c_str());
    }
}

void gfx_doDrawToolTip(GameState &state) {
    if (!state.showTooltip) return;

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
    int viewX = state.getPlayer()->position.x() - (mapWidthTiles / 2);
    int viewY = state.getPlayer()->position.y() - (mapHeightTiles  / 2);

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (mouseX < mapWidthPixels) {
        int mapMouseX = mouseX + mapOffsetX;
        int mapMouseY = mouseY + mapOffsetY;
        int tileX = viewX + mapMouseX / (scaledTileWidth);
        int tileY = viewY + mapMouseY / (scaledTileHeight);
        Point here(tileX, tileY);
        int tileHere = state.getBoard()->getTile(here);
        Actor *actor = state.getBoard()->actorAt(here);
        Item *item = state.getBoard()->itemAt(here);

        if (!state.getBoard()->isVisible(here)) return;
        if (tileHere != tileOutOfBounds) {
            if (state.getBoard()->isKnown(here)) { // show tooltip
                std::stringstream ss;
                if (actor) {
                    ss << actor->getName() << "\n";
                }
                if (item) {
                    ss << item->typeInfo->name << "\n";
                }
                const TileInfo &tileInfo = TileInfo::get(tileHere);
                ss << tileInfo.name;
                gfx_DrawTooltip(state, mouseX, mouseY + 20, ss.str());
            }
        }
    }
}

bool repaint(GameState &state, const AnimFrame *frame, bool callPresent) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);
    const int animDelay = state.config->getInt("anim_delay", 100);

    bool doNextFrame = false;
    do {
        bool didFrame = false;
        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(state.renderer);
        do {
            doNextFrame = false;
            if (state.animationQueue.empty()) {
                gfx_drawMap(state, nullptr);
            } else {
                AnimFrame *frame = &state.animationQueue.front();
                switch(frame->special) {
                    case animFrame:
                        gfx_drawMap(state, frame);
                        didFrame = true;
                        break;
                    case animText:
                        doNextFrame = true;
                        state.addMessage(frame->text);
                        break;
                    case animRollText:
                        doNextFrame = true;
                        state.addInfo(frame->text);
                        break;
                    case animDamage:
                        state.getBoard()->doDamage(state, frame->actor, frame->damageAmount, frame->damageType, frame->text);
                        break;
                    default:
                        doNextFrame = true;
                        Logger &logger = Logger::getInstance();
                        logger.error("Unknown animation frame type " + std::to_string(frame->special));

                }
                state.animationQueue.pop_front();
            }
        } while (doNextFrame);
        gfx_drawSidebar(state);
        if (callPresent) gfx_doDrawToolTip(state);

        state.advanceFrame();
        if (callPresent) SDL_RenderPresent(state.renderer);
        if (didFrame) {
            passCommand(state);
            SDL_Delay(animDelay);
        }
    } while (!state.animationQueue.empty());

    return false;
}
