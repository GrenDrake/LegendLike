#include <ostream>
#include <SDL2/SDL.h>
#include <sstream>

#include "command.h"
#include "config.h"
#include "gamestate.h"
#include "point.h"

CommandDef commandQuit = { Command::Quit };
CommandDef commandNone = { Command::None };
CommandDef commandSpecial = { Command::None };

const CommandDef& getCommand(GameState &system, SDL_Event &event, const CommandDef *commandList) {
    if (event.type == SDL_USEREVENT) {
        system.fps = system.timerFrames;
        system.timerFrames = 0;
    }
    if (event.type == SDL_QUIT)     return commandQuit;
    if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            system.config->set("xres", std::to_string(event.window.data1));
            system.config->set("yres", std::to_string(event.window.data2));
        }
        if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
            system.config->set("maximized", "1");
        }
        if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
            system.config->set("maximized", "0");
        }
    }

    if (commandList == nullptr) return commandNone;
    if (event.type != SDL_KEYDOWN)  return commandNone;
    event.key.keysym.mod &= KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI;
    // convert right modifier keys into left equivelents
    if (event.key.keysym.mod & KMOD_RSHIFT) {
        event.key.keysym.mod &= ~KMOD_RSHIFT;
        event.key.keysym.mod |= KMOD_LSHIFT;
    }
    if (event.key.keysym.mod & KMOD_RALT) {
        event.key.keysym.mod &= ~KMOD_RALT;
        event.key.keysym.mod |= KMOD_LALT;
    }
    if (event.key.keysym.mod & KMOD_RCTRL) {
        event.key.keysym.mod &= ~KMOD_RCTRL;
        event.key.keysym.mod |= KMOD_LCTRL;
    }

    for (unsigned i = 0; commandList[i].command != Command::None; ++i) {
        if (commandList[i].key != event.key.keysym.sym) continue;
        if (commandList[i].command == Command::Move) {
            if (event.key.keysym.mod == KMOD_LSHIFT) {
                commandSpecial = commandList[i];
                commandSpecial.command = Command::Run;
                return commandSpecial;
            } else if (event.key.keysym.mod == KMOD_LALT) {
                commandSpecial = commandList[i];
                commandSpecial.command = Command::Interact;
                return commandSpecial;
            } else if (event.key.keysym.mod == KMOD_NONE) {
                return commandList[i];
            }
        } else {
            if (commandList[i].mod == 0 && event.key.keysym.mod) continue;
            if ((commandList[i].mod & event.key.keysym.mod) != commandList[i].mod) continue;
            return commandList[i];
        }
    }

    return commandNone;
}

bool passCommand(GameState &state) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        getCommand(state, event, nullptr);
        if (event.type == SDL_KEYDOWN) return true;
    }
    return false;
}

std::ostream& operator<<(std::ostream &out, const Command &cmd) {
    switch(cmd) {
        case Command::None:             out << "nothing"; break;
        case Command::Cancel:           out << "cancel"; break;
        case Command::Close:            out << "close"; break;
        case Command::Quit:             out << "quit"; break;
        case Command::Move:             out << "move"; break;
        case Command::Run:              out << "run"; break;
        case Command::Interact:         out << "interact"; break;
        case Command::Wait:             out << "wait"; break;
        case Command::ShowMap:          out << "full map"; break;
        case Command::SystemMenu:       out << "system menu"; break;
        case Command::Inventory:        out << "inventory"; break;
        case Command::Rename:           out << "rename"; break;
        case Command::ShowTooltip:      out << "show tooltip"; break;

        case Command::NextMode:         out << "next mode"; break;
        case Command::PrevMode:         out << "prev mode"; break;
        case Command::Examine:          out << "examine"; break;
        case Command::SelectItem:       out << "select item"; break;

        case Command::NextSubweapon:    out << "next subweapon"; break;
        case Command::PrevSubweapon:    out << "prev subweapon"; break;
        case Command::Subweapon:        out << "subweapon"; break;

        case Command::Debug_Restore:    out << "full restore (debug)"; break;
        case Command::Debug_Reveal:     out << "reveal full map (debug)"; break;
        case Command::Debug_NoFOV:      out << "disable FOV (debug)"; break;
        case Command::Debug_ShowInfo:   out << "show info (debug)"; break;
        case Command::Debug_ShowFPS:    out << "show fps (debug)"; break;
        case Command::Debug_WriteMapBinary: out << "write map to file (debug)"; break;
        case Command::Debug_WarpMap:        out << "warp to map (debug)"; break;
        case Command::Debug_TestPathfinder: out << "test pathfinder (debug)"; break;
        case Command::Debug_MapEditMode:    out << "map editor mode (debug)"; break;
        case Command::Debug_SelectTile:     out << "map editor mode (debug)"; break;
        case Command::Debug_Teleport:       out << "teleport (debug)"; break;
        case Command::Debug_SetCursor:      out << "set cursor (debug)"; break;
        case Command::Debug_Fill:           out << "fill region (debug)"; break;
    }
    return out;
}
