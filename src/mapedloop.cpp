#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>

#include <SDL2/SDL.h>

#include "command.h"
#include "config.h"
#include "actor.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "vm.h"
#include "random.h"
#include "gamestate.h"
#include "textutil.h"


int dbg_tilePicker(GameState &state);
Dir gfx_GetDirection(GameState &system, const std::string &prompt, bool allowHere = false);
void gfx_handleMapedInput(GameState &state);

void mapedloop(GameState &state) {
    state.returnToMenu = false;
    state.getBoard()->calcFOV(state.getPlayer()->position);

    state.runDirection = Dir::None;
    state.lastticks = SDL_GetTicks();

    while (!state.wantsToQuit && !state.returnToMenu) {
        repaint(state);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(state, event, mapedCommands);
            switch(cmd.command) {
                case Command::None:
                case Command::Cancel:
                    // do nothing
                    break;
                case Command::SystemMenu:
                case Command::Debug_MapEditMode:
                    return;
                case Command::Quit:
                    state.wantsToQuit = true;
                    break;

                case Command::Move: {
                    Actor *player = state.getPlayer();
                    player->position = player->position.shift(cmd.direction);
                    break; }
                case Command::Run: {
                    Point dest = state.getPlayer()->position.shift(cmd.direction);
                    state.getBoard()->setTile(dest, state.mapEditTile);
                    break; }

                case Command::Interact: {
                    Point dest = state.getPlayer()->position.shift(cmd.direction);
                    state.mapEditTile = state.getBoard()->getTile(dest);
                    break; }
                case Command::ShowMap:
                    doShowMap(state);
                    break;
                case Command::ShowTooltip:
                    state.showTooltip = !state.showTooltip;
                    break;

                case Command::Maped_NextTile: {
                    ++state.mapEditTile;
                    break; }
                case Command::Maped_PrevTile: {
                    --state.mapEditTile;
                    break; }
                case Command::Maped_ShiftMap: {
                    Dir d = gfx_GetDirection(state, "Shift map");
                    if (d == Dir::None) break;
                    state.getBoard()->dbgShiftMap(d);
                    break; }

                case Command::Debug_WarpMap: {
                    std::string mapIdStr;
                    if (gfx_EditText(state, "Map Number", mapIdStr, 10)) {
                        int mapId = strToInt(mapIdStr);
                        if (mapId >= 0) {
                            Point position = state.getPlayer()->position;
                            state.warpTo(mapId, position.x(), position.y());
                        }
                    }
                    break; }
                case Command::Debug_Teleport: {
                    std::string xPos = std::to_string(state.getPlayer()->position.x());
                    std::string yPos = std::to_string(state.getPlayer()->position.y());
                    if (!gfx_EditText(state, "X Coord", xPos, 10)) break;
                    if (!gfx_EditText(state, "Y Coord", yPos, 10)) break;
                    int x = strToInt(xPos);
                    int y = strToInt(yPos);
                    if (x < 0) x = 0;
                    if (y < 0) y = 0;
                    if (x > state.getBoard()->width())  x = state.getBoard()->width() - 1;
                    if (y > state.getBoard()->height()) y = state.getBoard()->height() - 1;
                    state.warpTo(-1, x, y);
                    break; }
                case Command::Debug_ShowInfo:
                    state.showInfo = !state.showInfo;
                    break;
                case Command::Debug_ShowFPS:
                    state.showFPS = !state.showFPS;
                    break;
                case Command::Maped_SelectTile: {
                    int tile = dbg_tilePicker(state);
                    if (tile >= 0) state.mapEditTile = tile;
                    break; }
                case Command::Debug_WriteMapBinary: {
                    if (state.getBoard()->writeToFile("binary.map")) {
                        state.addError("Wrote map to disk as binary.map.");
                    } else {
                        state.addError("Failed to write map to disk.");
                    }
                    break; }
                case Command::Maped_Mark: {
                    state.cursor = state.getPlayer()->position;
                    break; }
                case Command::Maped_FillRect: {
                    Point topLeft = state.cursor;
                    Point playerPos = state.getPlayer()->position;
                    if (playerPos.y() < topLeft.y()) {
                        int tmp = playerPos.y();
                        playerPos = Point(playerPos.x(), topLeft.y());
                        topLeft = Point(topLeft.x(), tmp);
                    }
                    if (playerPos.x() < topLeft.x()) {
                        int tmp = playerPos.x();
                        playerPos = Point(topLeft.x(), playerPos.y());
                        topLeft = Point(tmp, topLeft.y());
                    }
                    std::vector<int> grassList{ 8, 55, 56 };
                    std::vector<int> treeList{ 59, 58, 60, 57, 8, 55, 56, 8, 55, 56 };
                    for (int y = topLeft.y(); y <= playerPos.y(); ++y) {
                        for (int x = topLeft.x(); x <= playerPos.x(); ++x) {
                            int tile = state.mapEditTile;
                            if (tile == -1) tile = grassList[state.coreRNG.next32() % grassList.size()];
                            else if (tile == -2) tile = treeList[state.coreRNG.next32() % treeList.size()];
                            state.getBoard()->setTile(Point(x, y), tile);
                        }
                    }
                    break; }

                default:
                    /* we don't need to worry about the other kinds of command */
                    break;
            }
        }
    }
}

int dbg_tilePicker(GameState &state) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);
    int tilesHigh = screenHeight / (tileWidth * 2);
    int tilesWide = screenHeight / (tileHeight * 2);
    Point cursor;

    while (1) {
        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(state.renderer);
        for (int y = 0; y < tilesHigh; ++y) {
            for (int x = 0; x < tilesWide; ++x) {
                unsigned tileHere = x + y * tilesWide;
                if (tileHere >= TileInfo::types.size()) continue;
                const TileInfo &info = TileInfo::get(tileHere);

                SDL_Rect texturePosition = {
                    x * tileWidth * 2,
                    y * tileHeight * 2,
                    tileWidth * 2, tileHeight * 2
                };

                SDL_Texture *tile = info.art;
                if (info.animLength > 1) {
                    tile = info.frames[0];
                }
                if (tile) {
                    SDL_RenderCopy(state.renderer, tile, nullptr, &texturePosition);
                }

                if (cursor.x() == x && cursor.y() == y) {
                    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                    SDL_RenderDrawRect(state.renderer, &texturePosition);
                }

            }
        }
        std::stringstream msg;
        msg << "Cursor:" << cursor.x() << ',' << cursor.y() << "   tile:";
        unsigned tileId = cursor.x() + cursor.y() * tilesWide;
        msg << tileId << ": ";
        if (tileId < TileInfo::types.size()) {
            const TileInfo &info = TileInfo::get(tileId);
            msg << info.name;
        }
        state.smallFont->out(0, screenHeight - state.smallFont->getCharHeight(), msg.str());

        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            const CommandDef &command = getCommand(state, event, gameCommands);
            switch(command.command) {
                case Command::Move: {
                    cursor = cursor.shift(command.direction);
                    if (cursor.x() < 0) cursor = Point(0, cursor.y());
                    if (cursor.y() < 0) cursor = Point(cursor.x(), 0);
                    break; }
                case Command::Wait:
                case Command::Interact: {
                    unsigned tileId = cursor.x() + cursor.y() * tilesWide;
                    if (tileId < TileInfo::types.size()) {
                        return tileId;
                    }
                    return -1; }
                case Command::Cancel:
                    return -1;
                default:
                    // we don't need to handle most of the commands
                    break;
            }
        }

    }


}