LIBFOV=../libfov-1.0.4/fov/
PHYSFS=../physfs-3.0.2/

CFLAGS=-Wall -g -std=c99 -pedantic -I$(PHYSFS)src
CXXFLAGS=-Wall -g -std=c++11 -pedantic `sdl-config --cflags` -I$(LIBFOV) -I$(PHYSFS)src
GAME_LIBS=-L$(LIBFOV).libs/ -L$(PHYSFS) -lfov -lphysfs -lSDL2_gfx -lSDL2_mixer -lSDL2_image `sdl2-config --libs`

GAME_OBJS=src/game.o src/gameloop.o src/gfx_fullmap.o src/board.o src/mapactor.o  \
	 src/gen_dungeon.o src/gamestate.o src/gfx.o src/command.o src/dataload.o \
	 src/vm.o src/gfx_font.o src/physfsrwops.o src/point.o src/gfx_menu.o \
	 src/menu.o src/beast.o src/gfx_resource.o src/gfx_ui.o src/config.o src/textutil.o \
	 src/logger.o src/combat.o src/menu_credits.o src/gen_enemies.o src/gfx_party.o
GAME=game

ASSEMBLE_OBJS=assembler/assemble.o assembler/assem_tokens.o \
      assembler/assem_token.o assembler/assem_build.o assembler/assem_labels.o \
	  assembler/utility.o
ASSEMBLE=./assemble

BEASTGEN_OBJS=tools/beastgen.o
BEASTGEN=beastgen
TILED_OBJS=tools/tiled.o tools/common.o
TILED=tiled
TYPED_OBJS=tools/typed.o tools/common.o
TYPED=typed

GAME_DAT=data/game.dat
BEAST_DAT=data/beasts.dat
MOVES_DAT=data/moves.dat

all: $(ASSEMBLE) $(GAME) datafiles tools

$(ASSEMBLE): $(ASSEMBLE_OBJS)
	$(CC) $(ASSEMBLE_OBJS) -o $(ASSEMBLE)

$(GAME): $(GAME_OBJS)
	$(CXX) $(GAME_OBJS) $(GAME_LIBS) -o $(GAME)

datafiles: $(GAME_DAT) $(BEAST_DAT) $(MOVES_DAT)
$(GAME_DAT): $(ASSEMBLE) data_src/gamedata.src data_src/stddefs.inc data_src/map0000.inc data_src/map0001.inc
	cd data_src && ../assemble gamedata.src ../$(GAME_DAT)
$(BEAST_DAT): $(ASSEMBLE) data_src/beasts.src
	cd data_src && ../assemble beasts.src ../$(BEAST_DAT)
$(MOVES_DAT): $(ASSEMBLE) data_src/moves.src
	cd data_src && ../assemble moves.src ../$(MOVES_DAT)

tools: $(BEASTGEN) $(TILED) $(TYPED)
$(BEASTGEN): $(BEASTGEN_OBJS)
	$(CC) $(BEASTGEN_OBJS) -o $(BEASTGEN)
$(TILED): $(TILED_OBJS)
	$(CC) $(TILED_OBJS) -lncurses -o $(TILED)
$(TYPED): $(TYPED_OBJS)
	$(CC) $(TYPED_OBJS) -lncurses -o $(TYPED)

clean:
	$(RM) src/*.o assembler/*.o $(ASSEMBLE) $(GAME) $(GAME_DAT)

.PHONY: all clean datafiles tools