
# LegendLike

LegendLike is a game designed as a crossover between the roguelike genre and
[the Link to the Past Randomizer](https://alttpr.com/en).

It is still in an early stage of development and lacks many core features.
Attempting to play it is not recommended at this time.


## Playing

Due to its early development state, no binaries are currently provided; if you
want to try out LegendLike despite its early status, you'll need to build it
yourself.


## Building

LegendLike requires the following libraries:

  * libfov
  * [PhysicsFS](https://www.icculus.org/physfs/)
  * [SDL2](https://www.libsdl.org/index.php) including
      [SDL2_image](https://www.libsdl.org/projects/SDL_image/) and
      [SDL2_mixer](https://www.libsdl.org/projects/SDL_mixer/).

LegendLike can be built using the provided makefiles. You will need to edit the
paths to the libraries used by the game in the makefile to point to their actual
locations.

Once build, LegendLike can be run by running the produced executable.


## The "Build" Program

Included in this repository is the "build" program used to build LegendLike's
datafiles. This program requires no libraries and should be able to be built
using any standard compiler. Using the included makefile is recommended for
systems that include make, but it should also be possible to build it by
dropping all the source files into a Visual Studio project or the like; there
are no special requirements.

Running the build program is as simple as entering

```
./build <source file 1> <source file 2>
```

on the command line. Be aware that all sources must be named on the command
line; build provides no "include" functionality. Currently there are two
command line arguments:

**-o \<filename\>**: specify the name of the data file to create. If not
specified, build will create `output.bin` in the current directory.

**-dump**: creates a number of text files with debugging information. This is
intended for debugging the assembler and is unlikely to be otherwise useful.

The build program uses a specialized form of assembly language to produce the
main game file. Documentation for this language will be produced at a latter
time. In the meantime, it is recommended the curious study the existing data
file sources in this repository.
