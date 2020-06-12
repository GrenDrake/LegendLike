; data for map of the starting town

.string map0_str_professor "professor"
.string map0_str_gateguard "gate guard"
.string map0_str_combatdemoguy "combat demo guy"
.string map0_str_partygifter "party gifter guy"
.string map0_str_archdean "archdean"
.string map0_str_sign "sign"
.string map0_str_academysign_text "Demon Beast Academy (Under Renovation)"
.string map0_str_gateguard_nopass "It is not safe beyond the gate; you must claim a beast from the academy before you may depart."
.string map0_str_gateguard_allowpass "I see you have a beast with you. You may pass."
.string map0_str_archdean_givebeast "Unfortunately, with the work being done on the academy, are usual arrangements are not available. Thus, please, take this beast to get you started."
.string map0_str_archdean_alreadygavebeast "New graduates are only entitled to one beast from the academy to help them get started. If you want more, you'll need to catch them yourself."
.string map0_str_professor_talk "Are your beasts hurt? If they are, allow me to heal them."
.string map0_str_professor_talk_nobeasts "If your beasts become hurt, I can heal them."

map0_gotfirst_flag:
    .byte 0

map0_talk_professor:
    p_size
    push            map0_talk_professor_nobeasts
    jz
    @saystr         map0_str_professor_talk
    @textbox        0 0 0
    @p_reset        -1
    ret
map0_talk_professor_nobeasts:
    @saystr         map0_str_professor_talk_nobeasts
    @textbox        0 0 0
    ret
map0_talk_academysign:
    @saystr         map0_str_academysign_text
    @textbox        0 0 0
    ret
map0_talk_archdean:
    @readb          map0_gotfirst_flag
    push            map0_talk_archdean_alreadyhave
    jnz
    @saystr         map0_str_archdean_givebeast
    @textbox        0 0 0
    @p_add          8631 5
    @storeb         1 map0_gotfirst_flag
    ret
map0_talk_archdean_alreadyhave:
    @saystr         map0_str_archdean_alreadygavebeast
    @textbox        0 0 0
    ret

map0_talk_gateguard:
    p_size
    push            map0_talk_gateguard_havebeast
    jnz
    @saystr         map0_str_gateguard_nopass
    @textbox        0 0 0
    ret
map0_talk_gateguard_havebeast:
    @saystr         map0_str_gateguard_allowpass
    @textbox        0 0 0
    ret

.string map0_str_giftparty "I am the stablemaster. Here, have some beasts."

map0_test_giveparty:
    @p_add          8631 5
    @p_add          8632 5
    @p_add          9147 5
    @p_add          9148 5
    @saystr         map0_str_giftparty
    @textbox        0 0 0
    ret

map0_test_fight:
    p_size
    push            map0_test_fight_nobeasts
    jz
map0_test_fight_start:
    @combat         testfight
    stkdup
    push            map0_test_fight_flee
    jlz
    push            map0_test_fight_lost
    jz
    @saystr         map0_str_wonTestFight
    @textbox        0 0 0
    ret
map0_test_fight_lost:
    @saystr         map0_str_lostTestFight
    @textbox        0 0 0
    ret
map0_test_fight_flee:
    @saystr         map0_str_fledTestFight
    @textbox        0 0 0
    @textbox        0 0 "Hello there!"
    ret

map0_test_fight_nobeasts:
    @p_add          8631 5
    @saystr         map0_str_test_fight_givebeast
    @textbox        0 0 map0_test_fight_start
    ret

.string map0_str_test_fight_givebeast "Here. You don't appear to have a beast, so have this one."
.string map0_str_wonTestFight "Congradulations! If you're ever in the mood for a rematch, come speak to me again."
.string map0_str_lostTestFight "I'm sorry that you lost. Get your beasties healed at the lab and come back to try again."
.string map0_str_fledTestFight "Too much for you, eh?"

testfight:
    .word 9147  5  -1 -1 -1 -1
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
    push            map0_downToDungeon
    ret

map0_build:
    ; create town frame
    .push_count     tileGrass tileGrass2 tileGrass3 tileGrass tileGrass2 tileGrass3 tileRocks tileTree1 tileTree2 tileTree3
    @mf_fillrand    66 66 0 0
    @mf_fillbox     56 56 10 10 tileFence
    @mf_fillbox     55 55 11 11 tileGround

    ret

map0_townPassageLeft:
    push       40
    push       48
    push       tileWindow
    mf_settile
    @saystr     map0_strRumblingNoise
    @textbox    0 0 0
    ret
map0_townPassageRight:
    push       40
    push       48
    push       tileDoorOpen
    mf_settile
    @saystr     map0_strRumblingNoise
    @textbox    0 0 0
    ret

map0_townTeleportBottom:
    push       37
    push       48
    push       -1
    warpto
    ret

.string map0_strRumblingNoise "There is a rumbling sound as the walls shift."

map0_talkScorpian:
    @saystr     map0_strScorpianSaysHi
    @textbox    1   map0_strPortrait1 0
    ret

.string map0_strPortrait1 "por0001.png"
.string map0_strScorpianSaysHi "The scorpian greets you cheerfully.\n\nYou respond politely."
.string map0_strActor "actor"
.string map0_strScorpian "scorpian"
