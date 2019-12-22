#include <sstream>
#include <string>
#include <vector>
#include <physfs.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include "game.h"
#include "gfx.h"
#include "menu.h"
#include "textutil.h"

std::vector<std::string> creditsText;

static void loadCredits() {
    SDL_version version;
    const SDL_version *pVersion;
    PHYSFS_Version physfsVersion;
    std::stringstream line;

    line << GAME_NAME << ' ' << GAME_MAJOR << '.' << GAME_MINOR << '.' << GAME_PATCH;
    creditsText.push_back(line.str());
    line.str(""); line.clear();
    creditsText.push_back("");

    SDL_GetVersion(&version);
    line << "SDL " << ' ' << static_cast<int>(version.major) << '.' << static_cast<int>(version.minor) << '.' << static_cast<int>(version.patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    pVersion = IMG_Linked_Version();
    line << "SDL_image " << ' ' << static_cast<int>(pVersion->major) << '.' << static_cast<int>(pVersion->minor) << '.' << static_cast<int>(pVersion->patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    pVersion = Mix_Linked_Version();
    line << "SDL_mixer " << ' ' << static_cast<int>(pVersion->major) << '.' << static_cast<int>(pVersion->minor) << '.' << static_cast<int>(pVersion->patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    PHYSFS_getLinkedVersion(&physfsVersion);
    line << "PHYSFS " << ' ' << static_cast<int>(physfsVersion.major) << '.' << static_cast<int>(physfsVersion.minor) << '.' << static_cast<int>(physfsVersion.patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    creditsText.push_back("libfov 1.0.4");
    creditsText.push_back("");
    creditsText.push_back("");

    char *rawFile = slurpFile("credits.txt");
    std::vector<std::string> lines = explode(rawFile, "\n");
    delete[] rawFile;
    for (const std::string &line : lines) {
        if (line == "#")    creditsText.push_back("");
        else                creditsText.push_back(line);
    }
}

void doCredits(System &state) {
    if (creditsText.empty()) loadCredits();
    gfx_RunInfo(state, creditsText, true);
}
