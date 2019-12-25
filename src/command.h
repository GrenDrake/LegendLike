#ifndef COMMAND_H_345493784
#define COMMAND_H_345493784

#include <iosfwd>
#include "point.h"

union SDL_Event;

enum class Command {
    None,
    Cancel,
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
extern CommandDef combatCommands[];
extern CommandDef mapCommands[];
extern CommandDef partyCommands[];

#endif
