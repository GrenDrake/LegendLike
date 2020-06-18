#include <SDL2/SDL.h>
#include <sstream>

#include "actor.h"
#include "board.h"
#include "game.h"
#include "gamestate.h"
#include "command.h"

static SDL_Rect doneButton = { -1 };

void gfx_DrawMap(System &system) {
    int dispWidth, dispHeight;
    SDL_GetRendererOutputSize(system.renderer, &dispWidth, &dispHeight);
    Board *board = system.getBoard();
    const int lineHeight = system.smallFont->getLineHeight();

    int mapTileWidth = dispWidth / board->width();
    int mapTileHeight = dispHeight / board->height();
    if (mapTileWidth > mapTileHeight)   mapTileWidth = mapTileHeight;
    else                                mapTileHeight = mapTileWidth;

    const int offsetX = (dispWidth  - mapTileWidth  * board->width()) / 2;
    const int offsetY = (dispHeight - mapTileHeight * board->height()) / 2;

    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(system.renderer);

    for (int y = 0; y < board->height(); ++y) {
        for (int x = 0; x < board->width(); ++x) {
            const Point here(x, y);

            if (!board->isKnown(here)) continue;
            const bool visible = board->isVisible(here);

            SDL_Rect texturePosition = {
                offsetX + x * mapTileWidth,
                offsetY + y * mapTileHeight,
                mapTileWidth,      mapTileHeight
            };

            const TileInfo &tileInfo = TileInfo::get(board->getTile(here));
            if (visible) {
                SDL_SetRenderDrawColor(system.renderer, tileInfo.red, tileInfo.green, tileInfo.blue, SDL_ALPHA_OPAQUE);
            } else {
                SDL_SetRenderDrawColor(system.renderer, tileInfo.red / 3, tileInfo.green / 3, tileInfo.blue / 3, SDL_ALPHA_OPAQUE);
            }
            SDL_RenderFillRect(system.renderer, &texturePosition);

            if (visible) {
                Actor *actorHere = board->actorAt(here);
                if (actorHere) {
                    SDL_Rect objectPosition = {
                        texturePosition.x + 1, texturePosition.y + 1,
                        texturePosition.w - 2, texturePosition.h - 2
                    };
                    if (objectPosition.w < 1) {
                        objectPosition = texturePosition;
                    }

                    if (actorHere->typeInfo->aiType == aiPlayer) {
                        SDL_SetRenderDrawColor(system.renderer, 32, 192, 32, SDL_ALPHA_OPAQUE);
                    } else {
                        SDL_SetRenderDrawColor(system.renderer, 192, 32, 192, SDL_ALPHA_OPAQUE);
                    }
                    SDL_RenderFillRect(system.renderer, &objectPosition);
                }
            }
        }
    }

    if (doneButton.x < 0) {
        doneButton.w = 200;
        doneButton.h = lineHeight + 8;
        doneButton.x = 0;
        doneButton.y = dispHeight - doneButton.h;
    }
    gfx_DrawButton(system, doneButton, false, "Done");

    if (system.showFPS) {
        std::stringstream ss;
        ss << "  FPS: " << system.getFPS();
        system.smallFont->out(0, 0, ss.str().c_str());
    }

    system.advanceFrame();
    SDL_RenderPresent(system.renderer);
}

void doShowMap(System &system) {
    while (1) {
        gfx_DrawMap(system);

        // ////////////////////////////////////////////////////////////////////
        // event loop
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (pointInBox(event.button.x, event.button.y, doneButton)) {
                    return;
                }
            } else {
                const CommandDef &cmd = getCommand(system, event, mapCommands);
                switch(cmd.command) {
                    case Command::Quit:
                        system.wantsToQuit = true;
                    case Command::Close:
                        return;
                    default:
                        // do nothing
                        break;
                }
            }
        }
    }

}
