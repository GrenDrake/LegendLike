; data for map of the starting town basement level


map1_upToAcademy:
    @warpto         21 49 0
    ret
map1_upToDungeonHouse:
    @warpto         29 28 0
    ret

.npc octopus1 name="octopus" x=37 y=25 typeId=npcTypeOctopus
.npc octopus2 name="octopus" x=41 y=27 typeId=npcTypeOctopus

map1_build:
    ; create town frame
    .push_count     tileGrass tileGrass2 tileGrass3 tileGrass tileGrass2 tileGrass3 tileRocks tileTree1 tileTree2 tileTree3
    @mf_clear       tileWall

    ; the lab
    @mf_drawbox     17 49 12 39 tileWall
    @mf_fillbox     16 48 13 40 tileInteriorFloor


    ; the academy
    @mf_drawbox     28 54 20 48 tileWall
    @mf_fillbox     27 53 21 49 tileInteriorFloor
    @mf_settile     21 49 tileUp
    @mf_addevent    21 49 map1_upToAcademy eventManual


    ; your home
    @mf_drawbox     25 19 20 13 tileWall
    @mf_fillbox     24 18 21 14 tileInteriorFloor

    ; dungeon guard house
    @mf_drawbox     34 33 24 23 tileWall
    @mf_fillbox     33 32 25 24 tileInteriorFloor
    @mf_settile     29 28 tileUp
    @mf_settile     30 28 tileDown
    @mf_addevent    29 28 map1_upToDungeonHouse eventManual

    ; village green
    @mf_vertline    30 25 33 tileWindow
    .push_count     tileWater
    @mf_fillrand    30 43 23 34
    @mf_addactor    octopus1
    @mf_addactor    octopus2

    ret
