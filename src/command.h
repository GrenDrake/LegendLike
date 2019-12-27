#ifndef COMMAND_H_345493784
#define COMMAND_H_345493784

#include <iosfwd>
#include "point.h"

union SDL_Event;

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
    ReturnToMenu,
    CharacterInfo,
    Inventory,
    AbilityList,
    Rename,
    ShowTooltip,
    PrevMode,
    NextMode,
    QuickKey_1,
    QuickKey_2,
    QuickKey_3,
    QuickKey_4,

    Debug_Reveal,
    Debug_NoFOV,
    Debug_ShowInfo,
    Debug_ShowFPS,
    Debug_Heal,
};

struct CommandDef {
    Command command;
    Dir direction;
    int key, mod;
};

const CommandDef& getCommand(SDL_Event &event, const CommandDef *commandList);
std::ostream& operator<<(std::ostream &out, const Command &cmd);

extern CommandDef gameCommands[];
extern CommandDef mapCommands[];
extern CommandDef characterCommands[];

#endif
