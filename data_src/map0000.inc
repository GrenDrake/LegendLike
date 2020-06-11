; data for map of the starting town

map0_str_professor:     .string "professor"
map0_str_gateguard:     .string "gate guard"
map0_str_combatdemoguy: .string "combat demo guy"
map0_str_partygifter:   .string "party gifter guy"
map0_str_archdean:      .string "archdean"
map0_str_sign:          .string "sign"
map0_strActor:          .string "actor"
map0_strSage:           .string "sage"

.define map0_str_academysign_text       5
.define map0_str_gateguard_nopass       4
.define map0_str_gateguard_allowpass    3
.define map0_str_archdean_givebeast     2
.define map0_str_professor_talk         1
.define map0_str_fledTestFight          6
.define map0_str_giftparty              7

.define map0_strRumblingNoise           8
.define map0_strSageSaysHi              9
.define map0_strTalkWell                17
.define map0_strTalkFire                18

map0_gotfirst_flag:
    .byte 0

map0_talk_professor:
    @saystr         map0_str_professor_talk
    textbox
    p_reset
    ret

map0_talk_academysign:
    @saystr         map0_str_academysign_text
    textbox
    ret
map0_talk_archdean:
    @p_damage       1 10
    @saystr         map0_str_archdean_givebeast
    textbox
    ret

map0_talk_gateguard:
    @saystr         map0_str_gateguard_nopass
    textbox
    ret
map0_talk_gateguard_havebeast:
    @saystr         map0_str_gateguard_allowpass
    textbox
    ret
map0_talkwell:
    @saystr         map0_strTalkWell
    textbox
    ret
map0_talkfire:
    @saystr         map0_strTalkFire
    textbox
    ret

map0_test_giveparty:
    @p_giveitem     12
    @p_giveitem     13
    @p_learn        3
    @p_learn        4
    @p_learn        5
    @p_learn        6
    @p_learn        7
    @saystr         map0_str_giftparty
    textbox
    ret

map0_test_fight:
    @p_giveitem     0
    @p_giveitem     1
    @p_giveitem     2
    @p_giveitem     3
    @p_giveitem     4
    @p_giveitem     5
    @p_giveitem     7
    @p_giveitem     8
    @p_giveitem     10
    @p_giveitem     11
    @p_forget       5
    @saystr         map0_str_fledTestFight
    textbox
    ret

testfight:
    .word 9147  5  -1 -1 -1 -1
    .word 0     0  -1 -1 -1 -1
    .word 0     0  -1 -1 -1 -1
    .word 0     0  -1 -1 -1 -1
    .word 0     ; fight type
    .word 2     ; fight bg

map0_enter:
    ret

map0_downToAcademyCellar:
    @warpto         21 49 1
    ret
map0_downToDungeon:
    @warpto         29 28 1
    ret

map0_name:
    .string "0000.map"
map0_build:
    @mf_fromfile    map0_name
    ; the lab
    @mf_addactor    aiStill         14 46 2 map0_talk_professor map0_str_professor 0
    ; the academy
    @mf_addevent    24 50 map0_downToAcademyCellar eventManual
    @mf_addactor    aiPaceBox       21 51 2 map0_talk_archdean map0_str_archdean 0
    @mf_addactor    aiAvoidPlayer   23 51 4 0 map0_strActor 0
    @mf_addevent    25 47 map0_talk_academysign eventManual
    ; beast pens
    @mf_addactor    aiPaceHorz     11 19 5 0 map0_strActor 0
    @mf_addactor    aiRandom       14 24 5 0 map0_strActor 0
    @mf_addactor    aiPaceVert     15 30 5 0 map0_strActor 0
    @mf_addactor    aiFollowPlayer 13 33 5 0 map0_strActor 0
    ; party gifter tester
    @mf_addactor    aiStill 18 27 2 map0_test_giveparty map0_str_partygifter 0
    @mf_addactor    aiRandom 16 25 5 0 0 0
    ; combat tester
    @mf_addactor    aiStill 21 28 4 map0_test_fight map0_str_combatdemoguy 0
    ; dungeon guard house
    @mf_addevent    29 28 map0_downToDungeon eventManual
    @mf_addactor    aiStill 25 29 4 map0_talk_gateguard map0_str_gateguard 0
    ; village green
    @mf_addevent    26 40 map0_talkwell eventManual
    @mf_addevent    28 36 map0_talkfire eventManual
    @mf_addactor    aiStill 25 37 2 map0_talkSage map0_strSage 0

    ret

    ; create town frame
    .push_count     tileGrass tileGrass2 tileGrass3 tileGrass tileGrass2 tileGrass3 tileRocks tileTree1 tileTree2 tileTree3
    @mf_fillrand    66 66 0 0
    @mf_fillbox     56 56 10 10 tileFence
    @mf_fillbox     55 55 11 11 tileGround

    ; the lab
    @mf_drawbox     17 49 12 39 tileWall
    @mf_fillbox     16 48 13 40 tileInteriorFloor
    @mf_fillbox     16 54 12 50 tileWall
    @mf_settile     17 44 tileDoorClosed
    @mf_settile     13 41 tileBookcase
    @mf_settile     13 46 tileBookcase
    @mf_settile     13 47 tileShelf
    @mf_settile     13 43 tileTable
    @mf_settile     13 44 tileTable
    @mf_settile     14 43 tileChair1
    @mf_settile     16 48 tilePot
    @mf_settile     16 40 tilePot
    @mf_settile     16 43 tileCandelabra
    @mf_settile     15 49 tileLockedDoor
    @mf_settile     17 47 tileWindow
    @mf_settile     17 46 tileWindow
    @mf_settile     17 41 tileWindow
    @mf_settile     17 42 tileWindow
    @mf_settile     12 42 tileWindow
    @mf_settile     12 45 tileWindow
    @mf_settile     15 39 tileWindow
    @mf_settile     14 39 tileWindow
    @mf_settile     17 41 tileLabSignLeft
    @mf_settile     17 42 tileLabSignRight
    @mf_addactor    aiStill         14 46 2 map0_talk_professor map0_str_professor 0

    ; the academy
    @mf_drawbox     28 54 20 48 tileWall
    @mf_fillbox     27 53 21 49 tileInteriorFloor
    @mf_settile     21 49 tileDown
    @mf_settile     24 48 tileDoorClosed
    @mf_settile     22 48 tileWindow
    @mf_settile     26 48 tileWindow
    @mf_settile     25 47 tileSign
    @mf_addevent    24 50 map0_downToAcademyCellar eventManual
    @mf_addactor    aiPaceBox       21 51 2 map0_talk_archdean map0_str_archdean 0
    @mf_addactor    aiAvoidPlayer   23 51 4 0 map0_strActor 0
    @mf_addevent    25 47 map0_talk_academysign eventManual

    ; your home
    @mf_drawbox     25 19 20 13 tileWall
    @mf_fillbox     24 18 21 14 tileInteriorFloor
    @mf_settile     22 19 tileDoorClosed
    @mf_settile     23 19 tileWindow
    @mf_settile     25 17 tileWindow
    @mf_settile     20 17 tileWindow
    @mf_settile     22 13 tileWindow
    @mf_settile     21 16 tileCandelabra
    @mf_settile     23 14 tileBed
    @mf_settile     21 15 tileMirror
    @mf_settile     21 17 tileSink
    @mf_settile     21 18 tileStove
    @mf_settile     24 17 tileTable
    @mf_settile     24 16 tileChair1
    @mf_settile     24 18 tileChair2

    ; beast pens
    @mf_drawbox     17 36 10 27 tileFence
    @mf_settile     14 22 tileSkull
    @mf_addactor    aiPaceHorz     11 19 5 0 map0_strActor 0
    @mf_addactor    aiRandom       14 24 5 0 map0_strActor 0

    @mf_drawbox     17 27 10 17 tileFence
    @mf_settile     14 31 tileLargeSkull
    @mf_addactor    aiPaceVert     15 30 5 0 map0_strActor 0
    @mf_addactor    aiFollowPlayer 13 33 5 0 map0_strActor 0

    ; party gifter tester
    @mf_addactor    aiStill 18 27 2 map0_test_giveparty map0_str_partygifter 0
    @mf_addactor    aiRandom 16 25 5 0 0 0
    ; combat tester
    @mf_addactor    aiStill 21 28 4 map0_test_fight map0_str_combatdemoguy 0

    ; dungeon guard house
    @mf_drawbox     32 31 26 25 tileWall
    @mf_fillbox     31 30 27 26 tileInteriorFloor
    @mf_settile     29 28 tileDown
    @mf_addevent    29 28 map0_downToDungeon eventManual
    @mf_settile     26 27 tileFence
    @mf_settile     26 28 tileClosedGate
    @mf_settile     26 29 tileFence
    @mf_addactor    aiStill 25 29 4 map0_talk_gateguard map0_str_gateguard 0

    ; village green
    .push_count     tileGrass tileGrass tileGrass2 tileGrass3
    @mf_fillrand    30 43 23 34
    @mf_settile     26 40 tileWell
    @mf_addevent    26 40 map0_talkwell eventManual
    @mf_settile     28 36 tileFire
    @mf_addevent    28 36 map0_talkfire eventManual
    @mf_addactor    aiStill 25 37 2 map0_talkSage map0_strSage 0

    ; draw roads
    @mf_vertline    40 18 44 tileNSRoad
    @mf_vertline    25 23 28  tileNSRoad
    @mf_horzline    43 22 20 tileESRoad
    @mf_horzline    47 24 45 tileESRoad
    @mf_horzline    55 32 45 tileESRoad
    @mf_settile     22 44 tileNSWRoad
    @mf_settile     24 44 tileNESRoad
    @mf_settile     32 44 tileNESRoad
    @mf_settile     22 28 tileESWRoad
    @mf_settile     32 56 tileClosedGate
    @mf_addactor    aiStill 31 55 4 map0_talk_gateguard map0_str_gateguard 0

    ; event demo
    @mf_fillbox     43 51 37 45 tileWall

    push       50
    push       40
    push       45
    push       tileGround
    mf_horzline

    push       40
    push       45
    push       tileRightArrow
    mf_settile
    push       40
    push       48
    push       tileDoorOpen
    mf_settile
    push       40
    push       51
    push       tileRightArrow
    mf_settile
    push       43
    push       48
    push       tileUpArrow
    mf_settile
    push       37
    push       48
    push       tileUpArrow
    mf_settile

    push       40
    push       46
    push       map0_townPassageRight
    push       0
    mf_addevent
    push       40
    push       50
    push       map0_townPassageLeft
    push       0
    mf_addevent
    push       43
    push       48
    push       map0_townTeleportBottom
    push       0
    mf_addevent

    ; extra tile demo (col 2)
    push       43
    push       42
    push       tileStool
    mf_settile
    push       51
    push       42
    push       tileBathtub
    mf_settile
    ; extra tile demo (col 3)
    push       43
    push       41
    push       tileToilet1
    mf_settile
    push       44
    push       41
    push       tileToilet2
    mf_settile
    push       45
    push       41
    push       tileCabinet
    mf_settile
    push       46
    push       41
    push       tileInnSign
    mf_settile

    ret

map0_townPassageLeft:
    push       40
    push       48
    push       tileWindow
    mf_settile
    @saystr     map0_strRumblingNoise
    textbox
    ret
map0_townPassageRight:
    push       40
    push       48
    push       tileDoorOpen
    mf_settile
    @saystr     map0_strRumblingNoise
    textbox
    ret

map0_townTeleportBottom:
    push       37
    push       48
    push       -1
    warpto
    ret


map0_talkSage:
    @saystr     map0_strSageSaysHi
    textbox
    ret

