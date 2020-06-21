; data for map of the starting town

.location locSwordUpgrade1  item=itemClassSwordUpgrade
.location locSwordUpgrade2  item=itemClassSwordUpgrade
.location locArmourUpgrade1 item=itemClassArmourUpgrade
.location locHealthUpgrade1 item=itemClassHealthUpgrade
.location locEnergyUpgrade1 item=itemClassEnergyUpgrade
.location locBow            item=itemClassBow
.location locHookshot       item=itemClassHookshot
.location locIceRod         item=itemClassIceRod
.location locFireRod        item=itemClassFireRod
.location locPickaxe        item=itemClassPickaxe
.location locAmmoArrow1     item=itemClassAmmoArrow
.location locAmmoBomb1      item=itemClassAmmoBomb
.location locCoin1          item=itemClassCoin
.location locCapArrow1      item=itemClassCapArrow
.location locCapBomb1       item=itemClassCapBomb


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
    @p_hasitem      itemClassSwordUpgrade
    @jnz            map0_talk_gateguard_has_sword
    @saystr         "Guard: \"It is not safe beyond this point. You need to find yourself a sword.\""
    textbox
    ret
map0_talk_gateguard_has_sword:
    @p_hasitem      itemClassArmourUpgrade
    @jnz            map0_talk_gateguard_has_armour
    @saystr         "Guard: \"I see you've got a sword. Now you just need to find some armour.\""
    textbox
    ret
map0_talk_gateguard_has_armour:
    @saystr         "Guard: \"You look well prepared; good luck in there!\""
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
    @p_claimed      locSwordUpgrade1
    @jnz            map0_test_fight_already_gave
    @p_giveitem     locSwordUpgrade1
    @p_giveitem     locPickaxe
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
map0_test_fight_already_gave:
    @saystr         "Man: \"I've given you everything I can.\""
    textbox
    ret


map0_downToAcademyCellar:
    @warpto         21 49 1
    ret

map0_downToDungeon:
    @warpto         29 28 1
    ret

map0_talkSage:
    @p_giveitem_imm itemClassAmmoArrow
    @saystr     "Sage: Hello! It is a pleasant morning, is it not?"
    textbox
    ret

map0_npclist:
.npc npcProfessor      name="Jeffrey"      x=46 y=14 typeId=npcTypeSage  talkFunc=map0_talk_professor proper
.npc npcArchdean       name="archdean"     x=51 y=21 typeId=npcTypeSage  talkFunc=map0_talk_archdean
.npc npcStudent        name="student"      x=51 y=23 typeId=npcTypeGuard
.npc npcCow1                               x=19 y=11 typeId=npcTypeCow
.npc npcCow2                               x=24 y=14 typeId=npcTypeCow
.npc npcChicken1                           x=30 y=15 typeId=npcTypeChicken
.npc npcChicken2                           x=28 y=11 typeId=npcTypeChicken
.npc npcRooster                            x=33 y=13 typeId=npcTypeRooster
.npc npcStablemaster   name="stablemaster" x=27 y=18 typeId=npcTypeSage  talkFunc=map0_test_giveparty
.npc npcDungeonGuard   name="guard"        x=29 y=25 typeId=npcTypeGuard talkFunc=map0_talk_gateguard
.npc npcTownCivilian1  name="civilian"     x=28 y=21 typeId=npcTypeGuard talkFunc=map0_test_fight
.npc npcTownCivilian2  name="sage"         x=37 y=25 typeId=npcTypeSage  talkFunc=map0_talkSage
.npc townBoulder                           x=18 y=30 typeId=npcTypeBoulder
.npc townPot1                              x=20 y=23 typeId=npcTypePot
.npc townPot2                              x=40 y=16 typeId=npcTypePot
.npc townPot3                              x=48 y=16 typeId=npcTypePot
.npc townDummy1                            x=24 y=28 typeId=npcTypeTargetDummy
.npc townDummy2                            x=24 y=30 typeId=npcTypeTargetDummy
.word -1

map0_enter:
    ret


.string map0_name "0000.map"
map0_build:
    @mf_fromfile    map0_name
    ; the lab
    @mf_addactors   map0_npclist
    ; the academy
    @mf_addevent    24 50 map0_downToAcademyCellar eventManual
    @mf_addevent    25 47 map0_talk_academysign eventManual
    ; dungeon guard house
    @mf_addevent    29 28 map0_downToDungeon eventManual
    ; village green
    @mf_addevent    26 40 map0_talkwell eventManual
    @mf_addevent    28 36 map0_talkfire eventManual

    @mf_additem     28 28 itemCoin

    ret
