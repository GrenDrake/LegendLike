#include <SDL2/SDL.h>

#include "command.h"

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
    { Command::Debug_SetCursor,     Dir::None, SDLK_0 },
    { Command::Debug_Fill,     Dir::None, SDLK_9 },
    { Command::Debug_Teleport, Dir::None, SDLK_6 },

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
