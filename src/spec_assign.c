/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern struct room_data *world;
extern int top_of_world;
extern int mini_mud;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
/* void assign_dynamic_rooms(void);       DYNAMIC */

/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname))
{
  if (real_mobile(mob) >= 0)
    mob_index[real_mobile(mob)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant mob #%d",
	    mob);
    log(buf);
  }
}

void ASSIGNOBJ(int obj, SPECIAL(fname))
{
  if (real_object(obj) >= 0)
    obj_index[real_object(obj)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant obj #%d",
	    obj);
    log(buf);
  }
}

void ASSIGNROOM(int room, SPECIAL(fname))
{
  if (real_room(room) >= 0)
    world[real_room(room)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant rm. #%d",
	    room);
    log(buf);
  }
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  SPECIAL(ronk);
  SPECIAL(temple_goddess);
  SPECIAL(armor);
  SPECIAL(remove_poison);
  SPECIAL(fly);
  SPECIAL(cure_critic);
  SPECIAL(infra);
  SPECIAL(invis);
  SPECIAL(postmaster);
  SPECIAL(cityguard);
  SPECIAL(receptionist);
  SPECIAL(cryogenicist);
  SPECIAL(guild_guard);
/*SPECIAL(guild);  hacked to end practice stuff */
/*SPECIAL(puff); */
  SPECIAL(fido);
  SPECIAL(janitor);
  SPECIAL(mayor);
  SPECIAL(snake);
  SPECIAL(thief);
  SPECIAL(magic_user);
  SPECIAL(color_spray_fireball);
/*SPECIAL(drain);  <-- crash bug */
/*SPECIAL(gate);  <-- crash bug */
  SPECIAL(warrior);
  SPECIAL(area_word_death);
  SPECIAL(area_dispel_magic);
  SPECIAL(slap_remove_eq);
  SPECIAL(bitch_queen);
  SPECIAL(area_scare_hunt);
  SPECIAL(spit_blind);
  SPECIAL(tyrant);
  SPECIAL(area_bash);
  SPECIAL(restore);
  SPECIAL(word_of_death);
  SPECIAL(disarm);
  SPECIAL(backstab);
  SPECIAL(bash);
  SPECIAL(zeus);
  SPECIAL(chasm);
  SPECIAL(iron);
  SPECIAL(stone);
  SPECIAL(golem);
  SPECIAL(vampire);
  SPECIAL(tiamat);
/*SPECIAL(antimagic_bugs); */
  SPECIAL(blue_dragon);
  SPECIAL(white_dragon);
  SPECIAL(red_dragon);
  SPECIAL(green_dragon);
  SPECIAL(black_dragon);
/*SPECIAL(newbie_guard); */
  SPECIAL(doom_orc);
  SPECIAL(doom_demon);
  SPECIAL(doom_beholder);
  SPECIAL(doom_cyberdaemon);
/* HACKED to take out the castle */
/*
  void assign_kings_castle(void);

  assign_kings_castle();
*/

  /* Limbo */
/*ASSIGNMOB(1, puff); */
  
  /* Training Area */
/*ASSIGNMOB(305, guild); */

  /* Olympus */
  ASSIGNMOB(401, zeus);
  ASSIGNMOB(430, temple_goddess);

  /* Clan Zone One */
  ASSIGNMOB(500, temple_goddess);
/*ASSIGNMOB(502, cure_critic); */
/*ASSIGNMOB(503, cure_critic); */
/*ASSIGNMOB(504, temple_goddess); */
/*ASSIGNMOB(505, cure_critic); */
/*ASSIGNMOB(506, cure_critic); */
/*ASSIGNMOB(507, temple_goddess); */
  ASSIGNMOB(508, receptionist);
/*ASSIGNMOB(509, cure_critic); */
/*ASSIGNMOB(510, cure_critic); */
  ASSIGNMOB(511, invis);
  ASSIGNMOB(512, cure_critic);
  ASSIGNMOB(513, postmaster);
  ASSIGNMOB(514, cure_critic);
  ASSIGNMOB(515, cryogenicist);

  /* Clan Zone Two */
/*ASSIGNMOB(600, temple_goddess); */
/*ASSIGNMOB(601, temple_goddess); */
/*ASSIGNMOB(602, temple_goddess); */
/*ASSIGNMOB(603, armor); */
/*ASSIGNMOB(604, infra); */
/*ASSIGNMOB(605, remove_poison); */
/*ASSIGNMOB(606, fly); */
  ASSIGNMOB(607, receptionist);
  ASSIGNMOB(608, cryogenicist);
  ASSIGNMOB(609, postmaster);
  ASSIGNMOB(613, receptionist);
  ASSIGNMOB(629, cryogenicist);

  /* Tower of Dh'Vral */
  ASSIGNMOB(1003, magic_user);

  /* Immortal Zone */
  ASSIGNMOB(1200, receptionist);
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(1202, janitor);

  /* Remort Gauntlet */
  ASSIGNMOB(1424, doom_beholder);
  ASSIGNMOB(1425, doom_orc);
  ASSIGNMOB(1427, doom_demon);
  ASSIGNMOB(1428, doom_cyberdaemon);

  /* Avernus */
  ASSIGNMOB(2001, tiamat);
  ASSIGNMOB(2010, black_dragon);
  ASSIGNMOB(2011, red_dragon);
  ASSIGNMOB(2012, green_dragon);
  ASSIGNMOB(2013, blue_dragon);
  ASSIGNMOB(2014, white_dragon);

  /* Housing */
  ASSIGNMOB(2301, receptionist);

  /* High Tower Of Sorcery */
  ASSIGNMOB(2501, magic_user); /* should likely be cleric */
  ASSIGNMOB(2504, magic_user);
  ASSIGNMOB(2507, magic_user);
  ASSIGNMOB(2508, magic_user);
  ASSIGNMOB(2510, magic_user);
  ASSIGNMOB(2511, thief);
  ASSIGNMOB(2514, magic_user);
  ASSIGNMOB(2515, magic_user);
  ASSIGNMOB(2516, magic_user);
  ASSIGNMOB(2517, magic_user);
  ASSIGNMOB(2518, magic_user);
  ASSIGNMOB(2520, magic_user);
  ASSIGNMOB(2521, magic_user);
  ASSIGNMOB(2522, magic_user);
  ASSIGNMOB(2523, magic_user);
  ASSIGNMOB(2524, magic_user);
  ASSIGNMOB(2525, magic_user);
  ASSIGNMOB(2526, magic_user);
  ASSIGNMOB(2527, magic_user);
  ASSIGNMOB(2528, magic_user);
  ASSIGNMOB(2529, magic_user);
  ASSIGNMOB(2530, magic_user);
  ASSIGNMOB(2531, magic_user);
  ASSIGNMOB(2532, magic_user);
  ASSIGNMOB(2533, magic_user);
  ASSIGNMOB(2534, magic_user);
  ASSIGNMOB(2536, magic_user);
  ASSIGNMOB(2537, magic_user);
  ASSIGNMOB(2538, magic_user);
  ASSIGNMOB(2540, magic_user);
  ASSIGNMOB(2541, magic_user);
  ASSIGNMOB(2548, magic_user);
  ASSIGNMOB(2549, magic_user);
  ASSIGNMOB(2552, magic_user);
  ASSIGNMOB(2553, magic_user);
  ASSIGNMOB(2554, magic_user);
  ASSIGNMOB(2556, magic_user);
  ASSIGNMOB(2557, magic_user);
  ASSIGNMOB(2559, magic_user);
  ASSIGNMOB(2560, magic_user);
  ASSIGNMOB(2562, magic_user);
  ASSIGNMOB(2564, magic_user);

  /* Kore */
  ASSIGNMOB(3005, receptionist);
  ASSIGNMOB(3010, postmaster);
  ASSIGNMOB(3011, temple_goddess);
/*  ASSIGNMOB(3020, guild);
  ASSIGNMOB(3021, guild);
  ASSIGNMOB(3022, guild);
  ASSIGNMOB(3023, guild);
  ASSIGNMOB(3070, guild); */
  ASSIGNMOB(3024, guild_guard);
  ASSIGNMOB(3025, guild_guard);
  ASSIGNMOB(3026, guild_guard);
  ASSIGNMOB(3027, guild_guard);
  ASSIGNMOB(3060, warrior);
  ASSIGNMOB(3061, janitor);
  ASSIGNMOB(3063, cityguard);
  ASSIGNMOB(3067, cityguard);
  ASSIGNMOB(3071, guild_guard);
  ASSIGNMOB(3072, guild_guard);
  ASSIGNMOB(3088, ronk);
  ASSIGNMOB(3095, cryogenicist);

  /* Palace of the Tyrant */  
  ASSIGNMOB(3100, warrior);
  ASSIGNMOB(3102, tyrant);
  ASSIGNMOB(3104, stone);
  ASSIGNMOB(3105, stone);
  ASSIGNMOB(3106, iron);

  ASSIGNMOB(12516, stone);
/*ASSIGNMOB(8262, guild); hacked to end practice stuff */

  /* Gol'gorgoth */
  ASSIGNMOB(3505, receptionist);

  /* Graveyard */
  ASSIGNMOB(3800, vampire);
 
  /* Moria */
  ASSIGNMOB(4000, snake);
  ASSIGNMOB(4001, snake);
  ASSIGNMOB(4053, snake);
/*  ASSIGNMOB(4100, magic_user);
  ASSIGNMOB(4102, snake);
  ASSIGNMOB(4103, thief); */

  /* Desert */
  ASSIGNMOB(5004, magic_user);
  ASSIGNMOB(5005, guild_guard); /* brass dragon */
  ASSIGNMOB(5010, magic_user);
  ASSIGNMOB(5014, magic_user);

  /* Drow City */
  ASSIGNMOB(5103, magic_user);
  ASSIGNMOB(5104, magic_user);
  ASSIGNMOB(5107, magic_user);
  ASSIGNMOB(5108, magic_user);

  /* Agarost */
  ASSIGNMOB(5200, receptionist);
  ASSIGNMOB(5202, temple_goddess);
/*
  ASSIGNMOB(5211, guild);
  ASSIGNMOB(5221, guild);
  ASSIGNMOB(5220, guild); hacked to end practice stuff */

  /* Pyramid - area removed.
  ASSIGNMOB(5300, snake);
  ASSIGNMOB(5301, snake);
  ASSIGNMOB(5304, thief);
  ASSIGNMOB(5305, thief);
  ASSIGNMOB(5309, magic_user);
  ASSIGNMOB(5311, magic_user);
  ASSIGNMOB(5313, magic_user);
  ASSIGNMOB(5314, magic_user);
  ASSIGNMOB(5315, magic_user);
  ASSIGNMOB(5316, magic_user);
  ASSIGNMOB(5317, magic_user);
  */

  /* New Thalos */
  ASSIGNMOB(5404, receptionist);
/* 5481 - Cleric (or Mage... but he IS a high priest... *shrug*) */
  ASSIGNMOB(5421, magic_user);
  ASSIGNMOB(5422, magic_user);
  ASSIGNMOB(5423, magic_user);
  ASSIGNMOB(5424, magic_user);
  ASSIGNMOB(5425, magic_user);
  ASSIGNMOB(5426, magic_user);
  ASSIGNMOB(5427, magic_user);
  ASSIGNMOB(5428, magic_user);
  ASSIGNMOB(5434, cityguard);
  ASSIGNMOB(5440, magic_user);
  ASSIGNMOB(5455, magic_user);
  ASSIGNMOB(5461, cityguard);
  ASSIGNMOB(5462, cityguard);
  ASSIGNMOB(5463, cityguard);
  ASSIGNMOB(5482, cityguard);
/*ASSIGNMOB(5400, guild);
  ASSIGNMOB(5401, guild);
  ASSIGNMOB(5402, guild);
  ASSIGNMOB(5403, guild); hacked to end practice junk */
  ASSIGNMOB(5456, guild_guard);
  ASSIGNMOB(5457, guild_guard);
  ASSIGNMOB(5458, guild_guard);
  ASSIGNMOB(5459, guild_guard);

  /* Forest */
  ASSIGNMOB(6112, magic_user);
  ASSIGNMOB(6113, snake);
  ASSIGNMOB(6114, magic_user);
  ASSIGNMOB(6115, magic_user);
  ASSIGNMOB(6116, magic_user); /* should be a cleric */
  ASSIGNMOB(6117, magic_user);

  /* Arachnos */
  ASSIGNMOB(6302, magic_user);
  ASSIGNMOB(6309, magic_user);
  ASSIGNMOB(6312, magic_user);
  ASSIGNMOB(6314, magic_user);
  ASSIGNMOB(6315, magic_user);

  /* Dwarven Kingdom */
/*  ASSIGNMOB(6500, cityguard);
  ASSIGNMOB(6502, magic_user);
  ASSIGNMOB(6509, magic_user);
  ASSIGNMOB(6516, magic_user); */

  /* The Abyss */
/*ASSIGNMOB(6601, drain); <-- crash bug */
/*ASSIGNMOB(6611, gate);  <-- crash bug */

  /* Sewers */
  ASSIGNMOB(7006, snake);
  ASSIGNMOB(7009, magic_user);
  ASSIGNMOB(7200, magic_user);
  ASSIGNMOB(7201, magic_user);
  ASSIGNMOB(7202, magic_user);

  /* Redferne's */
/*  ASSIGNMOB(7900, cityguard); */

  /* Night Kore */
  ASSIGNMOB(8205, receptionist);
/*ASSIGNMOB(8262, tyrant_statue); */

  /* Shade's Tower */
  ASSIGNMOB(10002, snake);
  ASSIGNMOB(10008, stone);
  ASSIGNMOB(10009, golem);
  ASSIGNMOB(10012, stone);
/*ASSIGNMOB(10026, zeus); */
/*ASSIGNMOB(10027, magic_user);*/

  /* Ruins of Trokair */
/*ASSIGNMOB(11002, area_word_death); */
/*ASSIGNMOB(11007, spit_blind); */
/*ASSIGNMOB(11008, area_scare_hunt); */
  ASSIGNMOB(11017, bitch_queen);

  /* Rome */
  ASSIGNMOB(12009, magic_user);
  ASSIGNMOB(12018, cityguard);
  ASSIGNMOB(12020, magic_user);
  ASSIGNMOB(12021, cityguard);
  ASSIGNMOB(12025, magic_user);
  ASSIGNMOB(12030, magic_user);
  ASSIGNMOB(12031, magic_user);
  ASSIGNMOB(12032, magic_user);

  /* Castle of King Kyrellia */
  ASSIGNMOB(12101, stone);

  /* Slums of Kore */
  ASSIGNMOB(12515, vampire);
  ASSIGNMOB(12516, stone);
/*ASSIGNMOB(12523, guild); hacked to end practice stuff */
  ASSIGNMOB(12524, snake);
  ASSIGNMOB(12525, guild_guard);

/* HACKED to take out castle.c */
  /* King Welmar's Castle (not covered in castle.c) */
/*  ASSIGNMOB(15015, thief);      */ /* Ergan... have a better idea? */
/*  ASSIGNMOB(15032, magic_user); */ /* Pit Fiend, have something better?  Use it */
/* end of hack */
/*
  ASSIGNMOB(15000, warrior);
  ASSIGNMOB(15001, tyrant);
  ASSIGNMOB(15003, warrior);
  ASSIGNMOB(15004, warrior);
  ASSIGNMOB(15005, warrior);
  ASSIGNMOB(15006, stone);
>>>  ASSIGNMOB(15007, drain);   <-- crash bug?  <<<
  ASSIGNMOB(15008, warrior);
  ASSIGNMOB(15009, warrior);
  ASSIGNMOB(15010, warrior);
  ASSIGNMOB(15011, snake);
  ASSIGNMOB(15012, warrior);
  ASSIGNMOB(15013, guild);
  ASSIGNMOB(15016, warrior);
  ASSIGNMOB(15017, warrior);
  ASSIGNMOB(15020, green_dragon);
  ASSIGNMOB(15021, tiamat);
  ASSIGNMOB(15024, warrior);
  ASSIGNMOB(15025, warrior);
  ASSIGNMOB(15026, snake);
  ASSIGNMOB(15027, magic_user);
  ASSIGNMOB(15028, magic_user);
  ASSIGNMOB(15029, magic_user);
*/

  /* Beanstalk Castle */
  ASSIGNMOB(15211, color_spray_fireball);
  ASSIGNMOB(15216, area_bash);

  /* New Kore */
  ASSIGNMOB(16032, receptionist);
  ASSIGNMOB(16033, cryogenicist);
  ASSIGNMOB(16034, postmaster);

  /* Bone Hill */
  ASSIGNMOB(18605, magic_user);
  ASSIGNMOB(18613, magic_user);
  ASSIGNMOB(18615, magic_user);
/*ASSIGNMOB(18610, drain);   <--- crash bug */
  ASSIGNMOB(18608, snake);

  /* Elven */
/*  ASSIGNMOB(19109, receptionist); */

  /* Grey Marshes */
/*ASSIGNMOB(22002, antimagic_bugs); */
/*ASSIGNMOB(22003, antimagic_bugs); */
  ASSIGNMOB(22041, receptionist);

  /* Ghost Town */
  ASSIGNMOB(6813, receptionist);

  /* City of Eli */
  ASSIGNMOB(25537, word_of_death);
  ASSIGNMOB(25538, bash);
  ASSIGNMOB(25540, restore);
  ASSIGNMOB(25539, backstab);
  ASSIGNMOB(25541, disarm);
/*ASSIGNMOB(25537, guild);
  ASSIGNMOB(25538, guild);
  ASSIGNMOB(25539, guild);
  ASSIGNMOB(25540, guild);
  ASSIGNMOB(25541, guild); hacked to end practice stuff */
/*ASSIGNMOB(25542, word_of_death); */
  ASSIGNMOB(25543, guild_guard);
  ASSIGNMOB(25548, guild_guard);
  ASSIGNMOB(25549, guild_guard);
  ASSIGNMOB(25550, guild_guard);
  ASSIGNMOB(25551, guild_guard);
}



/* assign special procedures to objects */
void assign_objects(void)
{
  SPECIAL(gen_board);
  SPECIAL(bank);
  SPECIAL(bank_expensive);
  SPECIAL(pop_dispenser);
  SPECIAL(ship_tiller);

  /* Clans */
  ASSIGNOBJ(500, bank);
  ASSIGNOBJ(501, gen_board);
  ASSIGNOBJ(528, gen_board);
  ASSIGNOBJ(529, gen_board);
  ASSIGNOBJ(531, gen_board);
  ASSIGNOBJ(532, gen_board);
  ASSIGNOBJ(533, gen_board); /* Toot */
  ASSIGNOBJ(534, gen_board); /* Madmen */
  ASSIGNOBJ(535, gen_board);
  ASSIGNOBJ(539, gen_board);
  ASSIGNOBJ(540, gen_board);
  ASSIGNOBJ(566, gen_board); /* Emerald Enclave */
  ASSIGNOBJ(613, gen_board); /* Dragonliege */
  ASSIGNOBJ(600, gen_board); /* Coven */
  ASSIGNOBJ(601, bank);


  /* God Rooms */
  ASSIGNOBJ(1217, pop_dispenser);

  /* Avernus */
  ASSIGNOBJ(2095, ship_tiller);

  /* Kore */
  ASSIGNOBJ(3088, gen_board);	/* idea board */
  ASSIGNOBJ(3095, gen_board);	/* imp board */
  ASSIGNOBJ(3096, gen_board);	/* social board */
  ASSIGNOBJ(3097, gen_board);	/* freeze board */
  ASSIGNOBJ(3094, gen_board);
  ASSIGNOBJ(3098, gen_board);	/* immortal board */
  ASSIGNOBJ(3099, gen_board);	/* mortal board */

  ASSIGNOBJ(3724, bank);               /* evil money changer */

  /* Greza Ship */
  ASSIGNOBJ(5701, ship_tiller);

  /* Ar-Pharazon Ship */
  ASSIGNOBJ(5801, ship_tiller);

  /* New Kore */
  ASSIGNOBJ(16052, bank_expensive);     /* bank balance sheet */

  /* Grey Marshes */
  ASSIGNOBJ(22022, ship_tiller);
  
  /* Ghost Town */
  ASSIGNOBJ(6823, bank);
}



/* assign special procedures to clan atriums, called by assign_rooms */
void assign_clan_atriums(void)
{
  int i;
  extern int clan_info[][3];

  SPECIAL(clan_atrium);

  for (i = 0; clan_info[i][0] != -1; i++) {
    if (real_room(clan_info[i][1]) < 0)
      log("SYSERR:  Clan atrium does not exist.  Change in clan.c.");
    else
      world[real_room(clan_info[i][1])].func = clan_atrium;
  }
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  extern int dts_are_dumps;
  int i;

  SPECIAL(dump);
  SPECIAL(pet_shops);
  SPECIAL(pray_for_items);
  SPECIAL(corpse_shop);
  SPECIAL(ship_deck);
  SPECIAL(lounge);


  ASSIGNROOM(1204, lounge);

  ASSIGNROOM(2092, ship_deck);

  ASSIGNROOM(16210, pet_shops);

/*ASSIGNROOM(3880, corpse_shop); */

/*
  ASSIGNROOM(3052, night_west_gate);
  ASSIGNROOM(3053, night_east_gate);
  ASSIGNROOM(3067, night_north_gate);
  ASSIGNROOM(3830, night_south_gate);
  ASSIGNROOM(3204, night_south_gate);
  ASSIGNROOM(3100, night_up_gate);
*/

  ASSIGNROOM(5700, ship_deck);
  ASSIGNROOM(5701, ship_deck);
  ASSIGNROOM(5702, ship_deck);
  ASSIGNROOM(5703, ship_deck);
  ASSIGNROOM(5704, ship_deck);
  ASSIGNROOM(5705, ship_deck);
  ASSIGNROOM(5706, ship_deck);
  ASSIGNROOM(5707, ship_deck);
  ASSIGNROOM(5708, ship_deck);
  ASSIGNROOM(5709, ship_deck);
  ASSIGNROOM(5710, ship_deck);

  ASSIGNROOM(5800, ship_deck);
  ASSIGNROOM(5801, ship_deck);
  ASSIGNROOM(5802, ship_deck);
  ASSIGNROOM(5803, ship_deck);

  ASSIGNROOM(22026, ship_deck);

  if (dts_are_dumps)
    for (i = 0; i < top_of_world; i++)
      if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH))
	world[i].func = dump;

  assign_clan_atriums();

  /* DYNAMIC assign_dynamic_rooms(); */
}
