
.short 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 -1

.string prologText "After years of training in the ways of battling with demon beasts, you have completed your apprenticeship and are at long last ready to take on your mastery quest.\n\nBut first, you must attend the academy one final time to claim your pre-graduation present: your own demon-beast companion.\n"

.define tileWall   55
.define tileGrass  56
.define tileGrass2 57
.define tileGrass3 58
.define tileTree1  59
.define tileTree2  60
.define tileTree3  61
.define tileRocks  62
.define tileFence  63
.define tileGround 64

.define tileWindow 100
.define tileDoorOpen 100
.define tileInteriorFloor 100
.define tileUp 100
.define eventManual 100
.define tileDown 100
.define tileWater 100
.define aiRandom 100
.define aiStill 100

.export start
start:
    ; teleport the player to the starting location
    @warpto     23 17 0
    @saystr     prologText
    @textbox    0 0 0
    ret


.export map_info
map_info:
    ; level 0 (town)
    .short      0  ; depth
    .short      66 ; width
    .short      66 ; height
    .word       map0_build ; build func
    .word       map0_enter ; onEnter
    .word       0 ; map flags
    .word       1 ; music track
    .string_pad 20 "Town"

    ; level 1 (basements)
    .short      1  ; depth
    .short      66 ; width
    .short      66 ; height
    .word       map1_build ; build func
    .word       0 ; onEnter
    .word       0 ; map flags
    .word       1 ; music track
    .string_pad 20 "Town"

    ; level 1 (dungeon)
    .short      2
    .short      95
    .short      95
    .word       map_build_dungeon
    .word       0 ; no onEnter
    .word       3
    .word       2 ; music track
    .string_pad 20 "Dungeon Level 1"

    ; level 2 (dungeon)
    .short      3
    .short      95
    .short      95
    .word       map_build_dungeon
    .word       0 ; no onEnter
    .word       3
    .word       2 ; music track
    .string_pad 20 "Dungeon Level 2"

    ; level 3 (dungeon)
    .short      4
    .short      95
    .short      95
    .word       map_build_dungeon_bottom
    .word       0 ; no onEnter
    .word       1
    .word       2 ; music track
    .string_pad 20 "Dungeon Level 3"

    ; end of list marker
    .short      -1

.string random_foe_name "random foe"

dungeon_fight_1:
    .word 9147  5  -1 -1 -1 -1
    .word 9148  7  -1 -1 -1 -1
    .word 0     0  -1 -1 -1 -1
    .word 0     0  -1 -1 -1 -1
    .word 0     ; fight type
    .word 2     ; fight bg

dungeon_fight_2:
    .word 8631  3  -1 -1 -1 -1
    .word 8631  4  -1 -1 -1 -1
    .word 8632  4  -1 -1 -1 -1
    .word 0     0  -1 -1 -1 -1
    .word 0     ; fight type
    .word 2     ; fight bg

dungeon_foe_info:
    .word   100     ; foes to add to map

    .word   11      ; map actor art ID
    .word   aiRandom ; map actor AI
    .word   random_foe_name ; map actor name
    .word   dungeon_fight_1

    .word   12      ; map actor art ID
    .word   aiRandom ; map actor AI
    .word   random_foe_name ; map actor name
    .word   dungeon_fight_2

    .word   -1      ; random foe end marker

    @mf_addactor    aiStill 14 46 11 map0_talk_professor map0_str_professor 0

map_build_dungeon:
    @mf_makemaze    3
    @mf_makefoes    dungeon_foe_info
    ret

map_build_dungeon_bottom:
    @mf_makemaze    1
    @mf_makefoes    dungeon_foe_info
    ret

