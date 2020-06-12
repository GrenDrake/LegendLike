LIBFOV=../libfov-1.0.4/fov/
PHYSFS=../physfs-3.0.2/

# Windows
CC=gcc
SDL2=../SDL2/i686-w64-mingw32
CFLAGS=-Wall -g -std=c99 -pedantic -Dmain=SDL2main -I$(SDL2)/include -I$(PHYSFS)src
CXXFLAGS=-Wall -g -std=c++11 -pedantic -I$(SDL2)/include -I$(LIBFOV) -I$(PHYSFS)src
GAME_LIBS=-L$(LIBFOV).libs/ -L$(PHYSFS)build/ -L$(SDL2)/lib \
          -lmingw32 -lfov -lphysfs -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_image
# OSX
# CFLAGS=-Wall -g -std=c99 -pedantic -I$(PHYSFS)src
# CXXFLAGS=-Wall -g -std=c++11 -pedantic `sdl-config --cflags` -I$(LIBFOV) -I$(PHYSFS)src
# GAME_LIBS=-L$(LIBFOV).libs/ -L$(PHYSFS) -lfov -lphysfs -lSDL2_mixer -lSDL2_image `sdl2-config --libs`

GAME_OBJS=src/game.o src/gameloop.o src/mode_fullmap.o src/board.o src/board_fov.o \
	 src/gen_dungeon.o src/system.o src/gfx.o src/command.o src/dataload.o \
	 src/vm.o src/gfx_font.o src/physfsrwops.o src/point.o src/gfx_menu.o \
	 src/mode_mainmenu.o src/creature.o src/gfx_resource.o src/gfx_ui.o src/config.o src/textutil.o \
	 src/logger.o src/gen_enemies.o src/mode_charinfo.o
GAME=game

ASSEMBLE=build/build

BEASTGEN_OBJS=tools/beastgen.o
BEASTGEN=beastgen
MOVED_OBJS=tools/moved.o tools/common.o
MOVED=moved
TILED_OBJS=tools/tiled.o tools/common.o
TILED=tiled
TYPED_OBJS=tools/typed.o tools/common.o
TYPED=typed

GAME_DAT=data/game.dat
BEAST_DAT=data/beasts.dat

all: $(GAME) datafiles tools

$(GAME): $(GAME_OBJS)
	$(CXX) $(GAME_OBJS) $(GAME_LIBS) -o $(GAME)
assembler:
	cd build && make

datafiles: $(GAME_DAT) $(BEAST_DAT) $(MOVES_DAT)
$(GAME_DAT): build/build data_src/gamedata.src data_src/stddefs.inc data_src/map0000.inc data_src/map0001.inc
	$(ASSEMBLE) data_src/gamedata.src data_src/stddefs.inc data_src/map0000.inc data_src/map0001.inc -o $(GAME_DAT)
#	cd data_src && ../assemble gamedata.src ../$(GAME_DAT)
$(BEAST_DAT): data_src/beasts.src
	$(ASSEMBLE) data_src/beasts.src -o $(BEAST_DAT)

tools: $(BEASTGEN) $(MOVED) $(TILED) $(TYPED)
$(BEASTGEN): $(BEASTGEN_OBJS)
	$(CC) $(BEASTGEN_OBJS) -o $(BEASTGEN)
$(MOVED): $(MOVED_OBJS)
	$(CC) $(MOVED_OBJS) -lncurses -o $(MOVED)
$(TILED): $(TILED_OBJS)
	$(CC) $(TILED_OBJS) -lncurses -o $(TILED)
$(TYPED): $(TYPED_OBJS)
	$(CC) $(TYPED_OBJS) -lncurses -o $(TYPED)

clean:
	$(RM) src/*.o assembler/*.o $(ASSEMBLE) $(GAME) $(GAME_DAT)

.PHONY: all clean assembler datafiles tools