#ifndef COMMAND_H_345493784
#define COMMAND_H_345493784

#include <iosfwd>
#include "point.h"

union SDL_Event;
class GameState;

enum class Command {
    None,
    Cancel,
    Close,
    Quit,
    Move,
    Run,
    Interact,
    Wait,
    ShowMap,
    SystemMenu,
    Inventory,
    Rename,
    ShowTooltip,
    PrevMode,
    NextMode,
    Examine,
    SelectItem,
    NextSubweapon,
    PrevSubweapon,
    Subweapon,

    Debug_Reveal,
    Debug_NoFOV,
    Debug_ShowInfo,
    Debug_ShowFPS,
    Debug_TestPathfinder,
    Debug_WriteMapBinary,
    Debug_Restore,
    Debug_MapEditMode,
    Debug_WarpMap,
    Debug_Teleport,

    Maped_SetTile,
    Maped_PickTile,
    Maped_Mark,
    Maped_FillRect,
    Maped_SelectTile,
    Maped_PrevTile,
    Maped_NextTile,
    Maped_ShiftMap,
};

struct CommandDef {
    Command command;
    Dir direction;
    int key, mod;
};

const CommandDef& getCommand(GameState &system, SDL_Event &event, const CommandDef *commandList);
bool passCommand(GameState &state);
std::ostream& operator<<(std::ostream &out, const Command &cmd);

extern CommandDef gameCommands[];
extern CommandDef mapedCommands[];
extern CommandDef mapCommands[];
extern CommandDef characterCommands[];
extern CommandDef menuCommands[];
extern CommandDef infoCommands[];
extern CommandDef confirmCommands[];

#endif
