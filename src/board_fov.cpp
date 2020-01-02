// This gets to be in its own file because my IDE struggles to find the "fov.h"
// header file which then messes up the autocomplete/error detection for the
// rest of the file

#include "board.h"
#include "fov.h"

void apply(void *mapVoid, int x, int y, int dx, int dy, void *src) {
    Board *map = static_cast<Board*>(mapVoid);
    map->setSeen(Point(x, y));
	// if (((MAP *)map)->onMap(x, y))
	// 	((MAP *)map)->setSeen(x, y);
}
bool opaque(void *mapVoid, int x, int y) {
    Board *map = static_cast<Board*>(mapVoid);
    int t = map->getTile(Point(x, y));
    const TileInfo &info = TileInfo::get(t);
    return info.block_los;
	// return ((MAP *)map)->blockLOS(x, y);
}

void Board::calcFOV(const Point &origin) {
    resetFOV();
    fov_settings_type fov_settings;
    fov_settings_init(&fov_settings);
    fov_settings_set_opacity_test_function(&fov_settings, opaque);
    fov_settings_set_apply_lighting_function(&fov_settings, apply);
    fov_circle(&fov_settings, this, NULL, origin.x(), origin.y(), 100);
    setSeen(origin);
    fov_settings_free(&fov_settings);
}