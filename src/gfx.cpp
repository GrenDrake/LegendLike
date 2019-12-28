#include <iomanip>
#include <sstream>

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

void repaint(System &state, bool callPresent) {
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

    const Color uiColor{255, 255, 255};

    gfx_Clear(state);

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    // DRAW MAP
    int viewX = state.getPlayer()->position.x() - (mapWidthTiles / 2);
    int viewY = state.getPlayer()->position.y() - (mapHeightTiles  / 2);

    SDL_Rect clipRect = { 0, 0, mapWidthPixels, mapHeightPixels };
    SDL_RenderSetClipRect(state.renderer, &clipRect);
    for (int y = 0; y < mapHeightTiles; ++y) {
        for (int x = 0; x < mapWidthTiles; ++x) {
            const Point here(viewX + x, viewY + y);
            if (here.x() < 0 || here.y() < 0 || here.x() >= state.getBoard()->width() || here.y() >= state.getBoard()->height()) {
                continue;
            }
            if (!state.getBoard()->isKnown(here)) continue;
            bool visible = state.getBoard()->isVisible(here);

            int tileHere = state.getBoard()->getTile(here);
            SDL_Rect texturePosition = {
                x * scaledTileWidth - mapOffsetX,
                y * scaledTileHeight - mapOffsetY,
                scaledTileWidth, scaledTileHeight
            };

            if (tileHere != tileOutOfBounds) {
                const TileInfo &tileInfo = TileInfo::get(tileHere);
                SDL_Texture *tile = state.getTile(tileInfo.artIndex);
                if (tile) {
                    if (visible) SDL_SetTextureColorMod(tile, 255, 255, 255);
                    else         SDL_SetTextureColorMod(tile,  96,  96,  96);
                    SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                }
            }

            if (visible) {
                Creature *creature = state.getBoard()->actorAt(here);
                if (creature) {
                    SDL_Texture *tile = state.getTile(creature->typeInfo->artIndex);
                    if (tile) {
                        SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                    }
                }
            }
        }
    }
    SDL_RenderSetClipRect(state.renderer, nullptr);

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  PLAYER INFO
    int yPos = 0;
    int xPos = sidebarX + 8;
    const int lineHeight = state.smallFont->getLineHeight();
    const int tinyLineHeight = state.tinyFont->getLineHeight();
    const int barHeight = lineHeight / 2;
    const int column2 = xPos + sidebarWidth / 2;
    Creature *player = state.getPlayer();
    const double healthPercent = static_cast<double>(player->curHealth) / static_cast<double>(player->getStat(Stat::Health));
    const double energyPercent = static_cast<double>(player->curEnergy) / static_cast<double>(player->getStat(Stat::Energy));

    state.smallFont->out(xPos, yPos, player->name);
    yPos += lineHeight;
    gfx_DrawBar(state, xPos, yPos, sidebarWidth - 16, barHeight, healthPercent, Color{127,255,127});
    yPos += barHeight;
    gfx_DrawBar(state, xPos, yPos, sidebarWidth - 16, barHeight, energyPercent, Color{127,127,255});
    yPos += barHeight * 2;

    state.tinyFont->out(xPos, yPos, "HP: " + std::to_string(player->curHealth) + "/" + std::to_string(player->getStat(Stat::Health)));
    state.tinyFont->out(column2, yPos, "EN: " + std::to_string(player->curEnergy) + "/" + std::to_string(player->getStat(Stat::Energy)));
    yPos += tinyLineHeight;
    const int statTop = yPos;
    for (int i = 2; i < statCount; ++i) {
        Stat stat = static_cast<Stat>(i);
        std::stringstream line;
        line << getAbbrev(stat) << ": " << player->getStat(stat);
        state.tinyFont->out(xPos, yPos, line.str());
        yPos += tinyLineHeight;
    }
    for (unsigned i = 0; i < quickSlotCount; ++i) {
        std::string content = "";
        switch(state.quickSlots[i].type) {
            case quickSlotAbility:
                content = MoveType::get(state.quickSlots[i].action).name;
                break;
            case quickSlotItem:
                content = "item";
                break;
        }
        state.tinyFont->out(xPos, yPos, std::to_string(i+1) + ": " + content);
        yPos += tinyLineHeight;
    }
    yPos = statTop;
    for (int i = 0; i < damageTypeCount; ++i) {
        DamageType damageType = static_cast<DamageType>(i);
        std::stringstream line;
        line << std::fixed << std::setprecision(3);
        line << getAbbrev(damageType) << ": " << player->getResist(damageType);
        state.tinyFont->out(column2, yPos, line.str());
        yPos += tinyLineHeight;
    }

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
                    ss << creature->name << "   ";
                }
                const TileInfo &tileInfo = TileInfo::get(tileHere);
                ss << tileInfo.name << "   " << tileX << ", " << tileY;
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
        ss << "  FPS: " << state.getActualFps();
        state.smallFont->out(0, 0, ss.str().c_str());
    }

    gfx_HLine(state, 0, screenWidth, mapHeightPixels, uiColor);
    gfx_VLine(state, mapWidthPixels, 0, mapHeightPixels, uiColor);


    if (callPresent) SDL_RenderPresent(state.renderer);
}

void gfx_frameDelay(System &state) {
    repaint(state);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const CommandDef &cmd = getCommand(event, gameCommands);
        if (cmd.command == Command::Quit) {
            state.wantsToQuit = true;
            return;
        }
    }
    state.waitFrame();
}
