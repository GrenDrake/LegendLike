; data for map of the starting town

.location locSwordUpgrade1  item=itmSwordUpgrade
.location locSwordUpgrade2  item=itmSwordUpgrade
.location locArmourUpgrade1 item=itmArmourUpgrade
.location locHealthUpgrade1 item=itmHealthUpgrade
.location locEnergyUpgrade1 item=itmEnergyUpgrade
.location locBow            item=itmBow
.location locHookshot       item=itmHookshot
.location locIceRod         item=itmIceRod
.location locFireRod        item=itmFireRod
.location locMattock        item=itmMattock
.location locAmmoArrow1     item=itmAmmoArrow
.location locAmmoBomb1      item=itmAmmoBomb
.location locCoin1          item=itmCoin
.location locCapArrow1      item=itmCapArrow
.location locCapBomb1       item=itmCapBomb


map0_talk_professor:
    @saystr         "Professor: \"Are you injured? Allow me to heal you.\""
    textbox
    p_reset
    ret

map0_talk_academysign:
    @saystr         "Sign: \"Academy\""
    textbox
    ret
map0_talk_archdean:
    @p_damage       5
    @saystr         "Archdean: \"Welcome! I wish I could give you a better greeting, but work on the academy is still in progress.\""
    textbox
    @p_giveitem     locCoin1
    ret

map0_talk_gateguard:
    @saystr         "Guard: \"It is not safe beyond this point. Speak to the archdean at the academy before you leave.\""
    textbox
    ret
map0_talkwell:
    @saystr         "The well is full of fresh, clean water."
    textbox
    ret
map0_talkfire:
    @saystr         "Ouch! The fire is hot!"
    textbox
    ret

map0_test_giveparty:
    @p_giveitem     locCapArrow1
    @p_giveitem     locCapBomb1
    @p_giveitem     locSwordUpgrade2
    @saystr         "Woman: \"I am the stablemaster.\""
    textbox
    ret

map0_test_fight:
    @p_giveitem     locSwordUpgrade1
    @p_giveitem     locMattock
    @p_giveitem     locHealthUpgrade1
    @p_giveitem     locHookshot
    @p_giveitem     locAmmoArrow1
    @p_giveitem     locFireRod
    @p_giveitem     locAmmoBomb1
    @p_giveitem     locIceRod
    @p_giveitem     locEnergyUpgrade1
    @p_giveitem     locArmourUpgrade1
    @p_giveitem     locBow
    @saystr         "Man: \"The depths too much for you, eh?\""
    textbox
    ret


map0_downToAcademyCellar:
    @warpto         21 49 1
    ret

map0_downToDungeon:
    @warpto         29 28 1
    ret

map0_talkSage:
    @saystr     "Sage: Hello! It is a pleasant morning, is it not?"
    textbox
    ret

.npc npcProfessor      name="professor"    x=46 y=14 aiType=aiStill         typeId=npcTypeSage  talkFunc=map0_talk_professor
.npc npcArchdean       name="archdean"     x=51 y=21 aiType=aiPaceBox       typeId=npcTypeSage  talkFunc=map0_talk_archdean
.npc npcStudent        name="student"      x=51 y=23 aiType=aiAvoidPlayer   typeId=npcTypeGuard
.npc npcCow1           name="cow"          x=19 y=11 aiType=aiRandom        typeId=npcTypeCow
.npc npcCow2           name="cow"          x=24 y=14 aiType=aiPaceHorz      typeId=npcTypeCow
.npc npcCow3           name="cow"          x=30 y=15 aiType=aiPaceVert      typeId=npcTypeCow
.npc npcCow4           name="cow"          x=33 y=13 aiType=aiFollowPlayer  typeId=npcTypeCow
.npc npcStablemaster   name="stablemaster" x=27 y=18 aiType=aiStill         typeId=npcTypeSage  talkFunc=map0_test_giveparty
.npc npcDungeonGuard   name="guard"        x=29 y=25 aiType=aiStill         typeId=npcTypeGuard talkFunc=map0_talk_gateguard
.npc npcTownCivilian1  name="civilian"     x=28 y=21 aiType=aiRandom        typeId=npcTypeGuard talkFunc=map0_test_fight
.npc npcTownCivilian2  name="sage"         x=37 y=25 aiType=aiStill         typeId=npcTypeSage  talkFunc=map0_talkSage


map0_enter:
    ret


.string map0_name "0000.map"
map0_build:
    @mf_fromfile    map0_name
    ; the lab
    @mf_addactor    npcProfessor
    ; the academy
    @mf_addevent    24 50 map0_downToAcademyCellar eventManual
    @mf_addactor    npcArchdean
    @mf_addactor    npcStudent
    @mf_addevent    25 47 map0_talk_academysign eventManual
    ; beast pens
    @mf_addactor    npcCow1
    @mf_addactor    npcCow2
    @mf_addactor    npcCow3
    @mf_addactor    npcCow4
    ; stablemaster
    @mf_addactor    npcStablemaster
    ; combat tester
    @mf_addactor    npcTownCivilian1
    ; dungeon guard house
    @mf_addevent    29 28 map0_downToDungeon eventManual
    @mf_addactor    npcDungeonGuard
    ; village green
    @mf_addevent    26 40 map0_talkwell eventManual
    @mf_addevent    28 36 map0_talkfire eventManual
    @mf_addactor    npcTownCivilian2

    ret
