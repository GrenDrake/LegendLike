#include <ostream>
#include <SDL2/SDL.h>

#include "command.h"
#include "point.h"

CommandDef commandQuit = { Command::Quit };
CommandDef commandNone = { Command::None };

CommandDef gameCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::ReturnToMenu,Dir::None,  SDLK_ESCAPE },
    { Command::Cancel,      Dir::None,  SDLK_z },
    { Command::Move,        Dir::North, SDLK_UP },
    { Command::Move,        Dir::East,  SDLK_RIGHT },
    { Command::Move,        Dir::South, SDLK_DOWN },
    { Command::Move,        Dir::West,  SDLK_LEFT },
    { Command::Run,         Dir::North, SDLK_UP,        KMOD_LSHIFT },
    { Command::Run,         Dir::East,  SDLK_RIGHT,     KMOD_LSHIFT },
    { Command::Run,         Dir::South, SDLK_DOWN,      KMOD_LSHIFT },
    { Command::Run,         Dir::West,  SDLK_LEFT,      KMOD_LSHIFT },
    { Command::Interact,    Dir::Here,  SDLK_COMMA,     KMOD_LSHIFT },
    { Command::Interact,    Dir::Here,  SDLK_PERIOD,    KMOD_LSHIFT },
    { Command::Interact,    Dir::North, SDLK_UP,        KMOD_LALT },
    { Command::Interact,    Dir::East,  SDLK_RIGHT,     KMOD_LALT },
    { Command::Interact,    Dir::South, SDLK_DOWN,      KMOD_LALT },
    { Command::Interact,    Dir::West,  SDLK_LEFT,      KMOD_LALT },
    { Command::Wait,        Dir::None,  SDLK_PERIOD },
    { Command::Wait,        Dir::None,  SDLK_SPACE },
    { Command::ShowMap,     Dir::None,  SDLK_m },
    { Command::PartyInfo,   Dir::None,  SDLK_p },
    { Command::ShowTooltip, Dir::None,  SDLK_t },
    { Command::Debug_Reveal,Dir::None,  SDLK_F3 },
    { Command::Debug_NoFOV, Dir::None,  SDLK_F4 },
    { Command::Debug_ShowInfo, Dir::None, SDLK_F5 },
    { Command::Debug_ShowFPS,  Dir::None, SDLK_F7 },

    { Command::None }
};

CommandDef partyCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::Move,        Dir::North, SDLK_UP },
    { Command::Move,        Dir::South, SDLK_DOWN },
    { Command::Move,        Dir::West,  SDLK_LEFT },
    { Command::Move,        Dir::East,  SDLK_RIGHT },
    { Command::Interact,    Dir::None,  SDLK_SPACE },
    { Command::Interact,    Dir::None,  SDLK_RETURN },
    { Command::Cancel,      Dir::None,  SDLK_ESCAPE },
    { Command::Cancel,      Dir::None,  SDLK_z },
    { Command::Rename,      Dir::None,  SDLK_r },

    { Command::None }
};

CommandDef combatCommands[] = {
    { Command::Quit,        Dir::None,  SDLK_q,         KMOD_LSHIFT },
    { Command::Move,        Dir::North, SDLK_UP },
    { Command::Move,        Dir::South, SDLK_DOWN },
    { Command::Interact,    Dir::None,  SDLK_RIGHT },
    { Command::Interact,    Dir::None,  SDLK_SPACE },
    { Command::Interact,    Dir::None,  SDLK_RETURN },
    { Command::Cancel,      Dir::None,  SDLK_LEFT },
    { Command::Cancel,      Dir::None,  SDLK_ESCAPE },
    { Command::Debug_ShowFPS,  Dir::None, SDLK_F7 },
    { Command::Debug_Heal,  Dir::None,  SDLK_F9 },

    { Command::None }
};

const CommandDef& getCommand(SDL_Event &event, const CommandDef *commandList) {
    if (event.type == SDL_QUIT)     return commandQuit;
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
        if (commandList[i].mod == 0 && event.key.keysym.mod) continue;
        if ((commandList[i].mod & event.key.keysym.mod) != commandList[i].mod) continue;
        return commandList[i];
    }

    return commandNone;
}

std::ostream& operator<<(std::ostream &out, const Command &cmd) {
    switch(cmd) {
        case Command::None:             out << "nothing"; break;
        case Command::Cancel:           out << "cancel"; break;
        case Command::Quit:             out << "quit"; break;
        case Command::Move:             out << "move"; break;
        case Command::Run:              out << "run"; break;
        case Command::Interact:         out << "interact"; break;
        case Command::Wait:             out << "wait"; break;
        case Command::ShowMap:          out << "full map"; break;
        case Command::ReturnToMenu:     out << "return to menu"; break;
        case Command::PartyInfo:        out << "party info"; break;
        case Command::Rename:           out << "rename"; break;
        case Command::ShowTooltip:      out << "show tooltip"; break;

        case Command::Debug_Reveal:     out << "reveal full map (debug)"; break;
        case Command::Debug_NoFOV:      out << "disable FOV (debug)"; break;
        case Command::Debug_ShowInfo:   out << "show info (debug)"; break;
        case Command::Debug_ShowFPS:    out << "show fps (debug)"; break;
        case Command::Debug_Heal:       out << "full heal party (debug)"; break;
    }
    return out;
}
