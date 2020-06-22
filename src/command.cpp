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

CommandDef gameCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::SystemMenu,  Dir::None,  SDLK_ESCAPE },
    { Command::Cancel,      Dir::None,  SDLK_z },

    { Command::Move,        Dir::North, SDLK_UP },
    { Command::Move,        Dir::North, SDLK_KP_8 },
    { Command::Move,        Dir::North, SDLK_k },
    { Command::Move,        Dir::Northeast, SDLK_KP_9 },
    { Command::Move,        Dir::Northeast, SDLK_u },
    { Command::Move,        Dir::East,  SDLK_RIGHT },
    { Command::Move,        Dir::East,  SDLK_KP_6 },
    { Command::Move,        Dir::East,  SDLK_l },
    { Command::Move,        Dir::Southeast, SDLK_KP_3 },
    { Command::Move,        Dir::Southeast, SDLK_n },
    { Command::Move,        Dir::South, SDLK_DOWN },
    { Command::Move,        Dir::South, SDLK_KP_2 },
    { Command::Move,        Dir::South, SDLK_j },
    { Command::Move,        Dir::Southwest, SDLK_KP_1 },
    { Command::Move,        Dir::Southwest, SDLK_b },
    { Command::Move,        Dir::West,  SDLK_LEFT },
    { Command::Move,        Dir::West,  SDLK_KP_4 },
    { Command::Move,        Dir::West,  SDLK_h },
    { Command::Move,        Dir::Northwest, SDLK_KP_7 },
    { Command::Move,        Dir::Northwest, SDLK_y },

    { Command::Interact,    Dir::Here,  SDLK_COMMA,     KMOD_LSHIFT },
    { Command::Interact,    Dir::Here,  SDLK_PERIOD,    KMOD_LSHIFT },
    { Command::Interact,    Dir::Here,  SDLK_KP_PERIOD, KMOD_LSHIFT },
    { Command::Wait,        Dir::None,  SDLK_PERIOD },
    { Command::Wait,        Dir::None,  SDLK_SPACE },
    { Command::Wait,        Dir::None,  SDLK_KP_PERIOD },

    { Command::Examine,     Dir::None,  SDLK_x },
    { Command::SelectItem,  Dir::None,  SDLK_RETURN },
    { Command::SelectItem,  Dir::None,  SDLK_KP_ENTER },
    { Command::ShowMap,     Dir::None,  SDLK_m },
    { Command::Inventory,   Dir::None,  SDLK_i },
    { Command::ShowTooltip, Dir::None,  SDLK_t },
    { Command::NextSubweapon,   Dir::None, SDLK_RIGHTBRACKET },
    { Command::PrevSubweapon,   Dir::None, SDLK_LEFTBRACKET },
    { Command::Subweapon,       Dir::None, SDLK_s },

    { Command::Debug_Restore, Dir::None, SDLK_F1 },
    { Command::Debug_Reveal,Dir::None,  SDLK_F3 },
    { Command::Debug_NoFOV, Dir::None,  SDLK_F4 },
    { Command::Debug_ShowInfo, Dir::None, SDLK_F5 },
    { Command::Debug_ShowFPS,  Dir::None, SDLK_F7 },
    { Command::Debug_TestPathfinder, Dir::None, SDLK_F9 },
    { Command::Debug_WriteMapBinary, Dir::None, SDLK_F10 },
    { Command::Debug_MapEditMode, Dir::None, SDLK_F12 },
    { Command::Debug_WarpMap,       Dir::None, SDLK_F12, KMOD_LSHIFT },
    { Command::Debug_SelectTile, Dir::None, SDLK_F11 },

    { Command::None }
};

CommandDef characterCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::Move,        Dir::North, SDLK_UP },
    { Command::Move,        Dir::South, SDLK_DOWN },
    { Command::Move,        Dir::West,  SDLK_LEFT },
    { Command::Move,        Dir::East,  SDLK_RIGHT },
    { Command::Interact,    Dir::None,  SDLK_SPACE },
    { Command::Interact,    Dir::None,  SDLK_RETURN },
    { Command::Close,       Dir::None,  SDLK_ESCAPE },
    { Command::Close,       Dir::None,  SDLK_z },
    { Command::Close,       Dir::None,  SDLK_z,         KMOD_LSHIFT },
    { Command::Rename,      Dir::None,  SDLK_r },
    { Command::PrevMode,    Dir::None,  SDLK_TAB,       KMOD_LSHIFT },
    { Command::NextMode,    Dir::None,  SDLK_TAB },

    { Command::None }
};

CommandDef mapCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::Close,       Dir::None,  SDLK_ESCAPE },
    { Command::Close,       Dir::None,  SDLK_SPACE },
    { Command::Close,       Dir::None,  SDLK_RETURN },
    { Command::Close,       Dir::None,  SDLK_m },
    { Command::Close,       Dir::None,  SDLK_z },
    { Command::Close,       Dir::None,  SDLK_z,         KMOD_LSHIFT },

    { Command::None }
};

CommandDef menuCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::Interact,    Dir::None,  SDLK_KP_ENTER },
    { Command::Interact,    Dir::None,  SDLK_RETURN },
    { Command::Interact,    Dir::None,  SDLK_SPACE },
    { Command::Close,       Dir::None,  SDLK_ESCAPE },
    { Command::Close,       Dir::None,  SDLK_z },
    { Command::Close,       Dir::None,  SDLK_z,         KMOD_LSHIFT },
    { Command::Move,        Dir::North, SDLK_UP },
    { Command::Move,        Dir::South, SDLK_DOWN },
    { Command::Move,        Dir::West,  SDLK_LEFT },
    { Command::Move,        Dir::East,  SDLK_RIGHT },
    { Command::Debug_ShowInfo, Dir::None, SDLK_F5 },

    { Command::None }
};

CommandDef infoCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::Interact,    Dir::None,  SDLK_SPACE },
    { Command::Close,       Dir::None,  SDLK_KP_ENTER },
    { Command::Close,       Dir::None,  SDLK_RETURN },
    { Command::Close,       Dir::None,  SDLK_ESCAPE },
    { Command::Close,       Dir::None,  SDLK_z },
    { Command::Close,       Dir::None,  SDLK_z,         KMOD_LSHIFT },
    { Command::Move,        Dir::North, SDLK_UP },
    { Command::Move,        Dir::South, SDLK_DOWN },

    { Command::None }
};

CommandDef confirmCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::Interact,    Dir::None,  SDLK_SPACE },
    { Command::Interact,    Dir::None,  SDLK_KP_ENTER },
    { Command::Interact,    Dir::None,  SDLK_RETURN },
    { Command::Close,       Dir::None,  SDLK_ESCAPE },
    { Command::Close,       Dir::None,  SDLK_z },
    { Command::Close,       Dir::None,  SDLK_z,         KMOD_LSHIFT },
    { Command::Cancel,      Dir::None,  SDLK_n },
    { Command::Cancel,      Dir::None,  SDLK_n,         KMOD_LSHIFT },
    { Command::Move,        Dir::None,  SDLK_y },
    { Command::Move,        Dir::None,  SDLK_y,         KMOD_LSHIFT },

    { Command::None }
};

const CommandDef& getCommand(System &system, SDL_Event &event, const CommandDef *commandList) {
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

bool passCommand(System &state) {
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
    }
    return out;
}
