# Windows
CC=gcc
SDL2=../lib/SDL2/i686-w64-mingw32
LIBFOV=../lib/libfov-1.0.4/fov/
PHYSFS=../lib/physfs-3.0.2/
CFLAGS=-Wall -g -std=c99 -pedantic -Dmain=SDL2main -I$(SDL2)/include -I$(PHYSFS)src
CXXFLAGS=-Wall -g -std=c++11 -pedantic -I$(SDL2)/include -I$(LIBFOV) -I$(PHYSFS)src
GAME_LIBS=-L$(LIBFOV).libs/ -L$(PHYSFS)build/ -L$(SDL2)/lib \
          -lmingw32 -lfov -lphysfs -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_image

# OSX
# CFLAGS=-Wall -g -std=c99 -pedantic -I$(PHYSFS)src
# LIBFOV=../libfov-1.0.4/fov/
# PHYSFS=../physfs-3.0.2/
# CXXFLAGS=-Wall -g -std=c++11 -pedantic `sdl-config --cflags` -I$(LIBFOV) -I$(PHYSFS)src
# GAME_LIBS=-L$(LIBFOV).libs/ -L$(PHYSFS) -lfov -lphysfs -lSDL2_mixer -lSDL2_image `sdl2-config --libs`

GAME_OBJS=src/game.o src/gameloop.o src/mode_fullmap.o src/board.o src/board_fov.o \
	 src/gen_dungeon.o src/system.o src/gfx.o src/command.o src/dataload.o \
	 src/vm.o src/gfx_font.o src/physfsrwops.o src/point.o src/gfx_menu.o \
	 src/mode_mainmenu.o src/creature.o src/gfx_resource.o src/gfx_ui.o src/config.o src/textutil.o \
	 src/logger.o src/gen_enemies.o src/mode_charinfo.o
GAME=game
DATA_FILES=data_src/gamedata.src  data_src/map0000.inc data_src/map0001.inc

ASSEMBLE=build/build

BEASTGEN_OBJS=tools/beastgen.o
BEASTGEN=beastgen
MOVED_OBJS=tools/moved.o tools/common.o
MOVED=moved
TYPED_OBJS=tools/typed.o tools/common.o
TYPED=typed

GAME_DAT=data/game.dat

all: $(GAME) datafiles tools

$(GAME): $(GAME_OBJS)
	$(CXX) $(GAME_OBJS) $(GAME_LIBS) -o $(GAME)
assembler:
	cd build && make

datafiles: $(GAME_DAT) $(MOVES_DAT)
$(GAME_DAT): $(DATA_FILES)
	$(ASSEMBLE) $(DATA_FILES) -o $(GAME_DAT)

tools: $(BEASTGEN) $(MOVED) $(TYPED)
$(BEASTGEN): $(BEASTGEN_OBJS)
	$(CC) $(BEASTGEN_OBJS) -o $(BEASTGEN)
$(MOVED): $(MOVED_OBJS)
	$(CC) $(MOVED_OBJS) -lncurses -o $(MOVED)
$(TYPED): $(TYPED_OBJS)
	$(CC) $(TYPED_OBJS) -lncurses -o $(TYPED)

clean:
	$(RM) src/*.o $(GAME) $(GAME_DAT)

.PHONY: all clean assembler datafiles tools