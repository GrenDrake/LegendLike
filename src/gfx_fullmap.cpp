

#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#include "creature.h"
#include "board.h"
#include "game.h"
#include "gfx.h"
#include "command.h"

const int maxScale = 20;
const int minScale = 4;
static int scale = minScale;
static std::vector<SDL_Rect> tabButtons;
static Point scroll;

void gfx_DrawMap(System &system) {
    int dispWidth, dispHeight;
    SDL_GetRendererOutputSize(system.renderer, &dispWidth, &dispHeight);
    Board *board = system.getBoard();
    const int lineHeight = system.smallFont->getLineHeight();

    const int fullmapTileWidth = dispWidth / scale;
    const int fullmapTileHeight = dispHeight / scale;

    const int playerX = system.getPlayer()->position.x() + scroll.x();
    const int playerY = system.getPlayer()->position.y() + scroll.y();
    const int offsetX = playerX - fullmapTileWidth / 2;
    const int offsetY = playerY - fullmapTileHeight / 2;

    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(system.renderer);

    for (int y = 0; y < fullmapTileHeight; ++y) {
        for (int x = 0; x < fullmapTileWidth; ++x) {
            const Point here(x + offsetX, y + offsetY);
            if (here.x() < 0 || here.y() < 0 || here.x() >= board->width() || here.y() >= board->height()) {
                continue;
            }
            if (!board->isKnown(here)) continue;
            const bool visible = board->isVisible(here);

            SDL_Rect texturePosition = {
                x * scale,  y * scale,
                scale,      scale
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

    const int tabWidth = 200;
    const int tabHeight = lineHeight + 8;
    int xPos = 0;
    int yPos = dispHeight - tabHeight;
    tabButtons.clear();
    for (int i = 0; i < 3; ++i) {
        std::string text;
        switch(i) {
            case 0: text = "Zoom In"; break;
            case 1: text = "Zoom Out"; break;
            case 2: text = "Done"; break;
        }

        gfx_DrawButton(system, xPos, yPos, tabWidth, tabHeight, false, text);

        SDL_Rect box = { xPos, yPos, tabWidth, tabHeight };
        tabButtons.push_back(box);
        xPos += tabWidth;
    }

    SDL_RenderPresent(system.renderer);
}

void doShowMap(System &system) {
    scroll = Point(0, 0);

    while (1) {
        gfx_DrawMap(system);

        // ////////////////////////////////////////////////////////////////////
        // event loop
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x;
                int my = event.button.y;
                for (unsigned i = 0; i < tabButtons.size(); ++i) {
                    const SDL_Rect &box = tabButtons[i];
                    if (mx < box.x || my < box.y) continue;
                    if (mx >= box.x + box.w || my >= box.y + box.h) continue;
                    switch (i) {
                        case 0:
                            if (scale < maxScale) ++scale;
                            break;
                        case 1:
                            if (scale > minScale) --scale;
                            break;
                        case 2:
                            return;
                    }
                    break;
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) event.wheel.y = -event.wheel.y;
                if (event.wheel.y < 0) {
                    if (scale > minScale) --scale;
                } else if (event.wheel.y > 0) {
                    if (scale < maxScale) ++scale;
                }
            } else {
                const CommandDef &cmd = getCommand(event, mapCommands);
                switch(cmd.command) {
                    case Command::Quit:
                        system.wantsToQuit = true;
                    case Command::Cancel:
                        return;

                    case Command::Move:
                        scroll = scroll.shift(cmd.direction, 10);
                        break;

                    case Command::PrevMode:
                        if (scale > minScale) --scale;
                        break;
                    case Command::NextMode:
                        if (scale < maxScale) ++scale;
                        break;

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
        }

        SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));
    }

}
