

#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#include "creature.h"
#include "board.h"
#include "game.h"
#include "gfx.h"
#include "command.h"

void showFullMap(System &system) {
    int dispWidth, dispHeight;
    SDL_GetRendererOutputSize(system.renderer, &dispWidth, &dispHeight);
    Board *board = system.getBoard();

    int mapTileHeight = dispHeight / board->height();
    int mapTileWidth = dispWidth / board->width();
    if (mapTileHeight < mapTileWidth) {
        mapTileWidth = mapTileHeight;
    } else {
        mapTileHeight = mapTileWidth;
    }

    int offsetX = (dispWidth - mapTileWidth * board->width()) / 2;
    int offsetY = (dispHeight - mapTileHeight * board->height()) / 2;

    while (1) {
        SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(system.renderer);

        for (int y = 0; y < dispHeight && y < board->height(); ++y) {
            for (int x = 0; x < dispWidth && x < board->width(); ++x) {
                const Point here(x, y);
                if (here.x() < 0 || here.y() < 0 || here.x() >= board->width() || here.y() >= board->height()) {
                    continue;
                }
                if (!board->isKnown(here)) continue;
                bool visible = board->isVisible(here);

                SDL_Rect texturePosition = {
                    offsetX + x * mapTileWidth, offsetY + y * mapTileHeight,
                    mapTileWidth - 1, mapTileHeight - 1
                };

                const TileInfo &tileInfo = TileInfo::get(board->getTile(here));
                if (visible) {
                    SDL_SetRenderDrawColor(system.renderer, tileInfo.red, tileInfo.green, tileInfo.blue, SDL_ALPHA_OPAQUE);
                } else {
                    SDL_SetRenderDrawColor(system.renderer, tileInfo.red / 3, tileInfo.green / 3, tileInfo.blue / 3, SDL_ALPHA_OPAQUE);
                }
                SDL_RenderFillRect(system.renderer, &texturePosition);

                if (visible) {
                    Creature *creatureHere = board->actorAt(here);
                    if (creatureHere) {
                        SDL_Rect objectPosition = {
                            texturePosition.x + 1, texturePosition.y + 1,
                            texturePosition.w - 2, texturePosition.h - 2
                        };
                        if (objectPosition.w < 1) {
                            objectPosition = texturePosition;
                        }

                        if (creatureHere->aiType == aiPlayer) {
                            SDL_SetRenderDrawColor(system.renderer, 32, 192, 32, SDL_ALPHA_OPAQUE);
                        } else {
                            SDL_SetRenderDrawColor(system.renderer, 192, 32, 192, SDL_ALPHA_OPAQUE);
                        }
                        SDL_RenderFillRect(system.renderer, &objectPosition);
                    }
                }
            }
        }
        SDL_RenderPresent(system.renderer);

        // ////////////////////////////////////////////////////////////////////
        // event loop
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(event, gameCommands);
            switch(cmd.command) {
                case Command::Quit:
                    system.wantsToQuit = true;
                case Command::Cancel:
                case Command::ReturnToMenu:
                case Command::ShowMap:
                    return;
                case Command::Debug_Reveal:
                    system.getBoard()->dbgRevealAll();
                    break;
                case Command::Debug_NoFOV:
                    system.getBoard()->dbgToggleFOV();
                    break;
                default:
                    // do nothing
                    break;
            }
        }

        SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));
    }
}
