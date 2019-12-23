#include <iomanip>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

#include "creature.h"
#include "gfx.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "gamestate.h"
#include "command.h"


void repaint(System &state, bool callPresent) {
    const int sidebarWidth = 30 * state.smallFont->getCharWidth();
    const int sidebarX = screenWidth - sidebarWidth;
    const int mapOffsetX = tileWidth / 3 * 2;
    const int mapOffsetY = tileHeight / 3 * 2;
    const int mapWidthPixels = screenWidth - sidebarWidth;
    const int mapHeightPixels = screenHeight;
    const int mapWidthTiles = mapWidthPixels / tileWidth + 1;
    const int mapHeightTiles = mapHeightPixels / tileHeight + 1;

    const std::uint32_t uiColor = 0xF0F0F0FF;

    state.fps = SDL_getFramerate(static_cast<FPSmanager*>(state.fpsManager));
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(state.renderer);

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    // DRAW MAP
    int viewX = state.game->getPlayer()->position.x() - (mapWidthTiles / 2);
    int viewY = state.game->getPlayer()->position.y() - (mapHeightTiles  / 2);

    SDL_Rect clipRect = { 0, 0, mapWidthPixels, mapHeightPixels };
    SDL_RenderSetClipRect(state.renderer, &clipRect);
    for (int y = 0; y < mapHeightTiles; ++y) {
        for (int x = 0; x < mapWidthTiles; ++x) {
            const Point here(viewX + x, viewY + y);
            if (here.x() < 0 || here.y() < 0 || here.x() >= state.game->getBoard()->width() || here.y() >= state.game->getBoard()->height()) {
                continue;
            }
            if (!state.game->getBoard()->isKnown(here)) continue;
            bool visible = state.game->getBoard()->isVisible(here);

            int tileHere = state.game->getBoard()->getTile(here);
            SDL_Rect texturePosition = {
                x * tileWidth - mapOffsetX,
                y * tileHeight - mapOffsetY,
                tileWidth, tileHeight
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
                Creature *creature = state.game->getBoard()->actorAt(here);
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
    Creature *player = state.game->getPlayer();
    const double healthPercent = static_cast<double>(player->curHealth) / static_cast<double>(player->getStat(Stat::Health));
    const double energyPercent = static_cast<double>(player->curEnergy) / static_cast<double>(player->getStat(Stat::Energy));

    state.smallFont->out(xPos, yPos, player->name);
    yPos += lineHeight;
    gfx_DrawBar(state, xPos, yPos, sidebarWidth - 16, barHeight, healthPercent, Color{127,255,127});
    yPos += barHeight;
    gfx_DrawBar(state, xPos, yPos, sidebarWidth - 16, barHeight, energyPercent, Color{127,127,255});
    yPos += barHeight * 2;

    state.tinyFont->out(xPos, yPos, "HP: " + std::to_string(player->curHealth) + "/" + std::to_string(player->getStat(Stat::Health)));
    state.tinyFont->out(xPos + sidebarWidth / 2, yPos, "EN: " + std::to_string(player->curEnergy) + "/" + std::to_string(player->getStat(Stat::Energy)));
    yPos += tinyLineHeight;
    const int statTop = yPos;
    for (int i = 2; i < statCount; ++i) {
        Stat stat = static_cast<Stat>(i);
        std::stringstream line;
        line << getAbbrev(stat) << ": " << player->getStat(stat);
        state.tinyFont->out(xPos, yPos, line.str());
        yPos += tinyLineHeight;
    }
    yPos = statTop;
    xPos += sidebarWidth / 2;
    for (int i = 0; i < damageTypeCount; ++i) {
        DamageType damageType = static_cast<DamageType>(i);
        std::stringstream line;
        line << std::fixed << std::setprecision(3);
        line << getAbbrev(damageType) << ": " << player->getResist(damageType);
        state.tinyFont->out(xPos, yPos, line.str());
        yPos += tinyLineHeight;
    }

    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  FPS INFO
    if (state.showInfo) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        mouseX += mapOffsetX;
        mouseY += mapOffsetY;
        if (mouseX < mapWidthPixels) {
            std::stringstream ss;
            int tileX = viewX + mouseX / (tileWidth + 1);
            int tileY = viewY + mouseY / (tileHeight + 1);
            int tileHere = state.game->getBoard()->getTile(Point(tileX,tileY));
            if (tileHere != tileOutOfBounds) {
                Point here(tileX, tileY);
                Creature *creature = state.game->getBoard()->actorAt(here);
                if (creature) {
                    ss << creature->name << "   ";
                }
                const TileInfo &tileInfo = TileInfo::get(tileHere);
                ss << tileInfo.name << "   " << tileX << ", " << tileY;
                ss << " V:" << state.game->getBoard()->isVisible(here);
                ss << " K:" << state.game->getBoard()->isKnown(here);
                ss << " MAPID: " << state.game->getDepth();
                ss << " Turn: " << state.game->getTurn();
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
        ss << "  FPS: " << state.fps;
        state.smallFont->out(0, 0, ss.str().c_str());
    }

    hlineColor(state.renderer, 0, screenWidth, mapHeightPixels, uiColor);
    vlineColor(state.renderer, mapWidthPixels, 0, mapHeightPixels, uiColor);


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
    SDL_framerateDelay(static_cast<FPSmanager*>(state.fpsManager));
}