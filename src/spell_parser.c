/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"

struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

#define SINFO spell_info[spellnum]

extern struct room_data *world;
extern struct zone_data *zone_table;
bool is_safe( struct char_data *ch, struct char_data *victim, bool show_messg);
bool can_see( struct char_data *ch, struct char_data *victim );

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, this should provide
 * ample slots for skills.
 */

char *spells[] =
{
  "!RESERVED!",			/* 0 - reserved */

  /* SPELLS */

  "armor",			/* 1 */
  "teleport",
  "bless",
  "blindness",
  "burning hands",
  "call lightning",
  "charm person",
  "chill touch",
  "clone",
  "color spray",		/* 10 */
  "control weather",
  "create food",
  "create water",
  "cure blind",
  "cure critic",
  "cure light",
  "curse",
  "detect alignment",
  "detect invisibility",
  "detect magic",		/* 20 */
  "detect poison",
  "dispel evil",
  "earthquake",
  "enchant weapon",
  "energy drain",
  "fireball",
  "harm",
  "heal",
  "invisibility",
  "lightning bolt",		/* 30 */
  "locate object",
  "magic missile",
  "poison",
  "protection from evil",
  "remove curse",
  "sanctuary",
  "shocking grasp",
  "sleep",
  "strength",
  "summon",			/* 40 */
  "ventriloquate",
  "word of recall",
  "remove poison",
  "sense life",
  "animate dead",
  "dispel good",
  "defensive harmony",
  "group heal",
  "group recall",
  "infravision",		/* 50 */
  "waterwalk",
  "floating disc",
  "wizard lock",
  "barkskin",
  "!UNUSED!",			/* 55 */
  "knock",
  "feather fall",
  "mirror image",
  "phantasmal force",
  "web",			/* 60 */
  "clairvoyance",
  "dispel magic",
  "aid",
  "haste", 
  "slow",			/* 65 */
  "dispel sanctuary",
  "relocate",
  "refresh",
  "fly",
  "nuke",			/* 70 */
  "petrify",
  "word of death",
  "restore",
  "paralysis",
  "protection from good",	/* 75 */
  "confusion",
  "dimension door",
  "polymorph other",
  "polymorph self",
  "cloudkill",			/* 80 */
  "feeblemind",
  "magic jar",
  "telekinesis",
  "antimagic shell",
  "disintegrate",		/* 85 */
  "invisible stalker",
  "stinking cloud",
  "find familiar",
  "forget",
  "cantrip",			/* 90 */
  "remove fear",
  "scare",
  "resist fire",
  "resist cold",
  "resist lightning",		/* 95 */
  "resist poison",
  "resist acid",
  "find traps",
  "silence",
  "sticks to snakes",		/* 100 */
  "shillelagh",
  "insect plague",
  "ressurrect",
  "far see",
  "monster summoning one",	/* 105 */
  "monster summoning two",
  "monster summoning three",
  "gate one",
  "gate two",
  "gate three",			/* 110 */
  "conjure elemental",
  "aerial servant",
  "identify",
  "cause light",
  "cause serious",		/* 115 */
  "cause critic",
  "cone of cold",
  "chain lightning",
  "cure serious",
  "inspire",			/* 120 */
  "aid two",
  "inspire two",
  "continual light",
  "faerie fire",
  "group sanctuary",		/* 125 */
  "wild heal",
  "phase door",
  "area word of death",
  "area scare", 
  "portal",			/* 130 */
  "instill energy",
  "energy flux",
  "holy light",
  "blur",
  "calm",			/* 135 */
  "create spring", 
  "create blood", 
  "enchant armor", 
  "faerie fog",
  "mass invis",			/* 140 */
  "pass without trace", 
  "power word stun",
  "prayer",
  "ray of enfeeblement",
  "spiritual hammer",		/* 145 */
  "poison two",
  "stoneskin",
  "fist of earth",
  "fist of stone",
  "sunray",			/* 150 */
  "searing orb",
  "battle hymn",
  "windwalk",
  "earth elemental",
  "water elemental",		/* 155 */
  "air elemental",
  "fire elemental",
  "fire storm",
  "finger of death",
  "mass fly",   		/* 160 */
  "ice storm",
  "acid blast",
  "mass refresh",
  "ice shield",
  "gypsy dance",		/* 165 */
  "create weapon",
  "fire shield",
  "mana shell",
  "!UNUSED!",
  "!UNUSED!",			/* 170 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 200 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 300 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 400 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 500 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 600 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 700 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 800 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 900 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1000 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1100 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1200 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1300 */

  /* SKILLS */
  
  "backstab",			/* 1301 */
  "bash",
  "hide",
  "kick",
  "pick lock",
  "punch",
  "rescue",
  "sneak",
  "steal",
  "track",			/* 1310 */
  "disarm",
  "critical_hit",
  "circle",
  "turn",
  "breathe fire",		/* 1315 */
  "breathe gas",
  "breathe frost",
  "breathe acid",
  "breathe lightning",
  "berserk",			/* 1320 */
  "block",
  "retreat",
  "scatter",
  "gaze",
  "skin",			/* 1325 */
  "smith",
  "shapeshift",
  "disguise",
  "defend",
  "scan",			/* 1330 */
  "stun touch",
  "quickheal",
  "meditate",
  "scrounge",
  "judge",			/* 1335 */
  "gauge",
  "riposte",
  "rage",
  "switch",
  "twist",			/* 1340 */
  "quickdraw",
  "dual wield",
  "mpdamage",
  "heal",
  "avenging blow",		/* 1345 */
  "valour",
  "trip",
  "palm",
  "bandage",
  "!UNUSED!",			/* 1350 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 1360 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 1370 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 1400 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1500 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1600 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1700 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1800 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 1900 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",   /* 2000 */


  /* OBJECT SPELLS AND NPC SPELLS/SKILLS */

  "!UNUSED!",			/* 2001 */
  "fire breath",
  "gas breath",
  "frost breath",
  "acid breath",
  "lightning breath",
  "\n"				/* the end */
};


struct syllable {
  char *org;
  char *new;
};


struct syllable syls[] = {
  {" ", " "},
  {"ar", "abra"},
  {"ate", "i"},
  {"cau", "kada"},
  {"blind", "nose"},
  {"bur", "mosa"},
  {"cu", "judi"},
  {"de", "oculo"},
  {"dis", "mar"},
  {"ect", "kamina"},
  {"en", "uns"},
  {"gro", "cra"},
  {"light", "dies"},
  {"lo", "hi"},
  {"magi", "kari"},
  {"mon", "bar"},
  {"mor", "zak"},
  {"move", "sido"},
  {"ness", "lacri"},
  {"ning", "illa"},
  {"per", "duda"},
  {"ra", "gru"},
  {"re", "candus"},
  {"son", "sabru"},
  {"tect", "infra"},
  {"tri", "cula"},
  {"ven", "nofo"},
  {"word of", "inset"},
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"},
  {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"},
  {"m", "w"}, {"n", "b"}, {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"},
  {"s", "g"}, {"t", "h"}, {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"},
  {"y", "l"}, {"z", "k"}, {"", ""}
};


struct magic_act_struct {
  int spellnum;
  char *to_notvict;
  char *to_vict;
  char *to_self;
};


/*
 * spell,
 * what others in the room see
 * what the victim sees (if there is one)
 * what the caster sees
 */
struct magic_act_struct magic_act_bard[] = {
  {SPELL_MIRROR_IMAGE,
   "$n sings softly to $mself a brief but chaotic melody.",
   "$n sings softly to $mself a brief but chaotic melody.",
   "You sing softly to yourself a brief chaotic melody."},
  {SPELL_INSPIRE,
   "$n plays a ballad of the legends and Heroes of Kore.",
   "$n plays a ballad of the legends and Heroes of Kore.",
   "You play a ballad of the legends and Heroes of Kore."},
  {SPELL_BARKSKIN,
   "$n sings of the strength and beauty of the woods.",
   "$n sings to you of the strength and beauty of the woods.",
   "You sing to yourself of the strength and beauty of the woods."},
  {SPELL_FORGET,
   "$n stares intently into $N's eyes as they appear to glaze over.",
   "$n stares into your eyes and then everything's a blank.",
   "You blank out."},
  {SPELL_SLOW,
   "$n waves $s hands in a counter-clockwise motion while staring intensly at $N.",
   "You seem to move in slow motion!",
   "You seem to move in slow motion!"},
  {SPELL_BATTLE_HYMN,
   "$n vocalizes a chant that makes your blood boil!",
   "$n vocalizes a chant that makes your blood boil!",
   "You sing a battle chant loudly and incite blood lust!"},
  {-1, "", "", ""}  /* this must come last */
};


int mag_manacost(struct char_data * ch, int spellnum)
{
  int mana_spread;
  int over_level;
  int mana;

/* HACKED TO FIX MANACOST
  mana = MAX(SINFO.mana_max - (SINFO.mana_change *
		    (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
	     SINFO.mana_min);
*/

  mana_spread = SINFO.mana_max - SINFO.mana_min;
  over_level = MAX(GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)], 1);
  mana = MAX(( mana_spread / over_level ) + SINFO.mana_min, SINFO.mana_min);

  return mana;
}



/*
 * say_spell_bard does all the work if its a bard doing the casting
 * returns a 0 if say_spell should go ahead, or a 1 if it did all the work
 */
int say_spell_bard(struct char_data * ch, int spellnum, struct char_data * tch,
        struct obj_data * tobj)
{
  int j;


  for (j = 0; magic_act_bard[j].spellnum != -1; j++) {
    if (magic_act_bard[j].spellnum == spellnum) {
      act(magic_act_bard[j].to_notvict, FALSE, ch, tobj, tch, TO_NOTVICT);
      if (ch == tch)
        act(magic_act_bard[j].to_self, FALSE, ch, tobj, tch, TO_CHAR);
      else
        act(magic_act_bard[j].to_vict, FALSE, ch, tobj, tch, TO_VICT);
      return 1;
    }
  }

  return 0;
}



/*
 * say_spell erodes buf, buf1, buf2
 *
 * bards are a special case because sometimes they act out their spells
 * instead of say them out loud like other classes
 */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch,
	            struct obj_data * tobj)
{
  char lbuf[256];
  struct char_data *i;
  int j, ofs = 0;
  int bard_found = 0;


  if (GET_CLASS(ch) == CLASS_BARD) {
    bard_found = say_spell_bard(ch, spellnum, tch, tobj);
    if (bard_found)
      return;
  }

  *buf = '\0';
  strcpy(lbuf, spells[spellnum]);

  while (*(lbuf + ofs)) {
    for (j = 0; *(syls[j].org); j++) {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
        strcat(buf, syls[j].new);
        ofs += strlen(syls[j].org);
      }
    }
  }

  if (tch != NULL && tch->in_room == ch->in_room) {
    if (tch == ch)
      sprintf(lbuf, "$n closes $s eyes and utters the words, '%%s'.");
    else
      sprintf(lbuf, "$n stares at $N and utters the words, '%%s'.");
  } else if (tobj != NULL && tobj->in_room == ch->in_room)
    sprintf(lbuf, "$n stares at $p and utters the words, '%%s'.");
  else
    sprintf(lbuf, "$n utters the words, '%%s'.");

  sprintf(buf1, lbuf, spells[spellnum]);
  sprintf(buf2, lbuf, buf);

  for (i = world[ch->in_room].people; i; i = i->next_in_room) {
    if (i == ch || i == tch || !i->desc || !AWAKE(i))
      continue;
    if (GET_CLASS(ch) == GET_CLASS(i))
      perform_act(buf1, ch, tobj, tch, i);
    else
      perform_act(buf2, ch, tobj, tch, i);
  }

  if (tch != NULL && tch != ch) {
    sprintf(buf1, "$n stares at you and utters the words, '%s'.",
        GET_CLASS(ch) == GET_CLASS(tch) ? spells[spellnum] : buf);
    act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }
}



int find_skill_num(char *name)
{
  int index = 0, ok;
  char *temp, *temp2;
  char first[256], first2[256];

  while (*spells[++index] != '\n') {
    if (is_abbrev(name, spells[index]))
      return index;

    ok = 1;
    temp = any_one_arg(spells[index], first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first, first2))
	ok = 0;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return index;
  }

  return -1;
}



/*
 * All invocations of any spell must come through this function,
 * call_magic(). This is also the entry point for non-spoken or unrestricted
 * spells. Spellnum 0 is legal but silently ignored here, to make callers
 * simpler.
 */
int call_magic(struct char_data * caster, struct char_data * cvict,
	     struct obj_data * ovict, char * argument,
             int spellnum, int level, int casttype)
{
  int savetype;
  int spell_is_violent = 0;
  int caster_room_is_peaceful = 0;
  int cvict_room_is_peaceful = 0;

#if(0)
  sprintf(buf, "Call to magic by %s for spellnum: %d.", GET_NAME(caster), spellnum);
  mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(caster)), TRUE);
#endif

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
    return 0;

  if (ROOM_FLAGGED(caster->in_room, ROOM_NOMAGIC) ||
      (cvict && ROOM_FLAGGED(cvict->in_room, ROOM_NOMAGIC))) {
    send_to_char("You reach for the one source, but grasp nothing.\r\n", caster);
    return 0;
  }

  if ( (!ZONE_FLAGGED(caster->in_room, ZONE_ACTIVE) ||
		(cvict && !ZONE_FLAGGED(cvict->in_room, ZONE_ACTIVE))) &&
	GET_LEVEL(caster) < LVL_GRGOD) {
    send_to_char("You reach for the one source, but grasp nothing.\r\n", caster);
    if(cvict)
	send_to_char("You should not be here, please ask an immortal to retrieve you.\r\n", cvict);
    return 0;
  }


  /*
   * leading up to a check for a peaceful room stopping the spell,
   * to fix an old bug: if a player spammed the casting of a violent
   * spell in a peaceful room, they could get a mob to attack.
   */
  if (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))
    spell_is_violent = 1;
  if(cvict && spell_is_violent) {
    if (is_safe(caster, cvict, TRUE))
	return 0;
  }
  if (ROOM_FLAGGED(caster->in_room, ROOM_PEACEFUL))
    caster_room_is_peaceful = 1;
  if (cvict && ROOM_FLAGGED(cvict->in_room, ROOM_PEACEFUL))
    cvict_room_is_peaceful = 1;

  if ( spell_is_violent && cvict &&
       (caster_room_is_peaceful || cvict_room_is_peaceful) ) {
    send_to_char("A flash of white light fills the room, dispelling your "
                 "violent magic!\r\n", caster);
    act("White light from no particular source suddenly fills the room, "
        "then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
    return 0;
  }

  /* fufu */
/*
  if ( spell_is_violent && cvict && (can_see( cvict, caster ))) {
    set_fighting(cvict, caster);
  }
*/

  /* determine the type of saving throw */
  switch (casttype) {
    case CAST_STAFF:
    case CAST_SCROLL:
    case CAST_POTION:
    case CAST_PILL:
    case CAST_WAND:
        savetype = SAVING_ROD;
        break;
    case CAST_SPELL:
        savetype = SAVING_SPELL;
        break;
    default:
        savetype = SAVING_BREATH;
        break;
  }


  if (IS_SET(SINFO.routines, MAG_DAMAGE))
    mag_damage(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_AFFECTS))
    mag_affects(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
    mag_unaffects(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_POINTS))
    mag_points(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
    mag_alter_objs(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_GROUPS))
    mag_groups(level, caster, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_MASSES))
    mag_masses(level, caster, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_AREAS))
    mag_areas(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_SUMMONS))
    mag_summons(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_CREATIONS))
    mag_creations(level, caster, spellnum);

  /* be sure to give any new spells an ASPELL in spells.h */
  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum) {
    case SPELL_AREA_SCARE:
      MANUAL_SPELL(spell_area_scare);
      break;
    case SPELL_CALM:
      MANUAL_SPELL(spell_calm);
      break;
    case SPELL_CANTRIP:
      MANUAL_SPELL(spell_cantrip);
      break;
    case SPELL_CHARM:
      MANUAL_SPELL(spell_charm);
      break;
    case SPELL_CLAIRVOYANCE:
      MANUAL_SPELL(spell_clairvoyance);
      break;
    case SPELL_CREATE_WATER:
      MANUAL_SPELL(spell_create_water);
      break;
    case SPELL_DIMENSION_DOOR:
      MANUAL_SPELL(spell_dimension_door);
      break;
    case SPELL_DISPEL_MAGIC:
      MANUAL_SPELL(spell_dispel_magic);
      break;
    case SPELL_ENCHANT_ARMOR:
      MANUAL_SPELL(spell_enchant_armor);
      break;
    case SPELL_ENCHANT_WEAPON:
      MANUAL_SPELL(spell_enchant_weapon);
      break;
    case SPELL_FAR_SEE:
      MANUAL_SPELL(spell_far_see);
      break;
    case SPELL_FORGET:
      MANUAL_SPELL(spell_forget);
      break;
    case SPELL_GYPSY_DANCE:
      MANUAL_SPELL(spell_gypsy_dance);
      break;
    case SPELL_IDENTIFY:
      MANUAL_SPELL(spell_identify);
      break;
    case SPELL_KNOCK:
      MANUAL_SPELL(spell_knock);
      break;
    case SPELL_LOCATE_OBJECT:
      MANUAL_SPELL(spell_locate_object);
      break;
    case SPELL_MAGIC_JAR:
      MANUAL_SPELL(spell_magic_jar);
      break;
    case SPELL_PORTAL:
      MANUAL_SPELL(spell_portal);
      break;
    case SPELL_SCARE:
      MANUAL_SPELL(spell_scare);
      break;
    case SPELL_RELOCATE:
      MANUAL_SPELL(spell_relocate);
      break;
    case SPELL_SUMMON:
      MANUAL_SPELL(spell_summon);
      break;
    case SPELL_TELEPORT:
      MANUAL_SPELL(spell_teleport);
      break;
    case SPELL_WINDWALK:
      MANUAL_SPELL(spell_windwalk);
      break;
    case SPELL_PHASE_DOOR:
      MANUAL_SPELL(spell_phase_door);
      break;
    case SPELL_WORD_OF_RECALL:
      MANUAL_SPELL(spell_recall);
      break;
    case SPELL_WIZARD_LOCK:
      MANUAL_SPELL(spell_wizard_lock);
      break;
    case SPELL_CREATE_WEAPON:
      MANUAL_SPELL(spell_create_weapon);
      break;
    case SPELL_REMOVE_CURSE:
      MANUAL_SPELL(spell_remove_curse);
      break;
/*
    case SPELL_MANUAL_DAMAGE:
      MANUAL_SPELL(spell_manual_damage);
      break;
*/
    }

  return 1;
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.
 *
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 * pill   - [0] level   [1] spell num   [2] spell num   [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified.
 */

void mag_objectmagic(struct char_data * ch, struct obj_data * obj,
		          char *argument)
{
  int i, k;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_CHAR_NEARBY
                        | FIND_OBJ_ROOM | FIND_OBJ_EQUIP
                        | FIND_CHAR_DIR | FIND_OBJ_DIR,
		   ch, &tch, &tobj);
  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_STAFF:
    act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
    } else {
      GET_OBJ_VAL(obj, 2)--;
      for (tch = world[ch->in_room].people; tch; tch = next_tch) {
	next_tch = tch->next_in_room;
	if (ch == tch)
	  continue;
	if (GET_OBJ_VAL(obj, 0))
	  call_magic(ch, tch, NULL, NULL, GET_OBJ_VAL(obj, 3),
		     GET_OBJ_VAL(obj, 0), CAST_STAFF);
	else
	  call_magic(ch, tch, NULL, NULL, GET_OBJ_VAL(obj, 3),
		     DEFAULT_STAFF_LVL, CAST_STAFF);
      }
    }
    break;
  case ITEM_WAND:
    if ((k == FIND_CHAR_ROOM) || (k == FIND_CHAR_NEARBY)
        || (k == FIND_CHAR_DIR)) {
      if (tch == ch) {
	act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
      } else {
	act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
	if (obj->action_description != NULL) {
/* 
	  act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
*/
          act(obj->action_description, FALSE, ch, obj, tch, TO_VICT);
          act(obj->action_description, FALSE, ch, obj, tch, TO_NOTVICT);
        } else {
/*
	  act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
*/
          act("$n points $p at you.", TRUE, ch, obj, tch, TO_VICT);
          act("$n points $p at $N.", TRUE, ch, obj, tch, TO_NOTVICT);
        }
      }
    } else if (tobj != NULL) {
      act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
      if (obj->action_description != NULL) {
/*
	act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
*/
        act(obj->action_description, FALSE, ch, obj, tobj, TO_VICT);
        act(obj->action_description, FALSE, ch, obj, tobj, TO_NOTVICT);
      } else {
/*
	act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
*/
        act("$n points $p at you.", TRUE, ch, obj, tobj, TO_VICT);
        act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_NOTVICT);
      }
    } else {
      act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
      return;
    }

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    GET_OBJ_VAL(obj, 2)--;
    if (GET_OBJ_VAL(obj, 0))
      call_magic(ch, tch, tobj, NULL, GET_OBJ_VAL(obj, 3),
		 GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
      call_magic(ch, tch, tobj, NULL, GET_OBJ_VAL(obj, 3),
		 DEFAULT_WAND_LVL, CAST_WAND);
    break;
  case ITEM_SCROLL:
    if (*arg) {
      if (!k) {
	act("There is nothing to here to affect with $p.", FALSE,
	    ch, obj, NULL, TO_CHAR);
	return;
      }
    } else
      tch = ch;

    if (obj->worn_by) {
      send_to_char("How about unequipping it first?\r\n", ch);
      return;
    }

    act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

    obj_from_char(obj);

    for (i = 1; i < 4; i++)
      if (!(call_magic(ch, tch, tobj, NULL, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_SCROLL)))
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_POTION:
    if (obj->worn_by) {
      send_to_char("How about unequipping it first?\r\n", ch);
      return;
    }

    tch = ch;
    act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);
    
    /* The character isn't going to have the potion after ANYWAY. This
       prevents death-junk/quest potions from being free'd twice. */
    obj_from_char(obj);

    for (i = 1; i < 4; i++)
      if (!(call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_POTION)))
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_PILL:
    if (obj->worn_by) {
      send_to_char("How about unequipping it first?\r\n", ch);
      return;
    }

    tch = ch;
    act("You eat $p.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n eats $p.", TRUE, ch, obj, NULL, TO_ROOM);

    obj_from_char(obj);

    for (i = 1; i < 4; i++)
      if (!(call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, i),
                       GET_OBJ_VAL(obj, 0), CAST_PILL)))
        break;
    
    if (obj != NULL)
      extract_obj(obj);
    break;
  default:
    log("SYSERR: Unknown object_type in mag_objectmagic");
    break;
  }
  WAIT_STATE(ch, PULSE_VIOLENCE);
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.
 */

int cast_spell(struct char_data * ch, struct char_data * tch,
	           struct obj_data * tobj, char * argument, int spellnum)
{
  if (GET_POS(ch) < SINFO.min_position) {
    switch (GET_POS(ch)) {
      case POS_SLEEPING:
      send_to_char("You dream about great magical powers.\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("You cannot concentrate while resting.\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("You can't do this sitting!\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("Impossible!  You can't concentrate enough!\r\n", ch);
      break;
    default:
      send_to_char("You can't do much of anything like this!\r\n", ch);
      break;
    }
    return 0;
  }
  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char("You are afraid you might hurt your master!\r\n", ch);
    return 0;
  }
  if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY)) {
    send_to_char("You can only cast this spell upon yourself!\r\n", ch);
    return 0;
  }
  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
    send_to_char("You cannot cast this spell upon yourself!\r\n", ch);
    return 0;
  }
  send_to_char(OK, ch);
  say_spell(ch, spellnum, tch, tobj);

  return (call_magic(ch, tch, tobj, argument, 
             spellnum, GET_LEVEL(ch), CAST_SPELL));
}



/*
 * spell skill is a base chance of 95%
 * multiplied by a casters prime requisite
 * (ie int for mages, wis for clerics, cha for bards)
 * over the max of that attribute (here on Kore 25)
 */
int check_spell_skill(struct char_data * ch, int spellnum)
{
  int skill;
  int prime_req;


  /* mobs have 100% skill */
  if (IS_NPC(ch))
    return 100;

  /* if they dont have the spell yet (not high enough level etc)
    then their 'skill' in it is 0 */
  if (GET_SKILL(ch, spellnum) == 0)
    return 0;

  /* they look ok go ahead and calc it out */
  switch (GET_CLASS(ch)) {
    case CLASS_MAGIC_USER:  prime_req = GET_INT(ch); break;
    case CLASS_CLERIC:      prime_req = GET_WIS(ch); break;
    case CLASS_BARD:        prime_req = GET_CHA(ch); break;
    case CLASS_DRUID:       prime_req = GET_WIS(ch); break;
    default:                prime_req = GET_INT(ch); break;
  }

  /* no parenthesis so that this doesnt get cut down */
  /* and not more than 100% skill */
  /* skill = MIN(95 * prime_req / 25, 100); */
  skill = MIN(75 + prime_req, 100);

  return skill;
}



/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 *
 * HACKED in many places to let NPCs cast spells too, but not charmed mobs
 * or mobs that someone has switched into.
 */
ACMD(do_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char *s, *d, *t;
  int mana, spellnum, i, target = 0;
  struct char_data *k, *next_k;
  int skip = 0;

/* HACKED to take out a crash bug in skip spaces over whole_target */
/*
  char orig_argument[256];
  char *whole_target;
*/
/* end of hack */
  int mprog_spell_trigger(struct char_data *ch, char *arg);

/*
  if (IS_NPC(ch))
    return;
*/
#if 0
  sprintf(buf, "%s casting %s.", GET_NAME(ch), argument);
  mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
#endif /* 0 */

  if (IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM))) {
    if (ch->desc)
      send_to_char("Huh?!?\r\n", ch);
    return;
  }


/* HACKED to take out a crash bug in skip spaces over whole_target */
/*
  sprintf(orig_argument, "%s", argument);
  whole_target = strtok(orig_argument, "'");
  whole_target = strtok(NULL, "'"); 
  whole_target = strtok(NULL, "\0");
  skip_spaces(&whole_target);
*/
/* end of hack */

  /* get: blank, spell name, direction + target name */
  s = strtok(argument, "'");

  if (s == NULL) {
    if (IS_NPC(ch))
      return;
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("Cast what where?\r\n", ch);
        break;
      case CLASS_CLERIC:
        send_to_char("Pray for what where?\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("Sing for what where?\r\n", ch);
        break;
      default:
        send_to_char("Best leave the spell-casting to those trained.\r\n", ch);
        break;
    }
    return;
  }

  s = strtok(NULL, "'");
  if (s == NULL) {
    if (IS_NPC(ch))
      return;
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("Spell names must be enclosed in the Magic Symbols: '\r\n",
            ch);
        break;
      case CLASS_CLERIC:
        send_to_char("Prayers must be enclosed in the Holy Symbols: '\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("Songs must be sung within the Musical Braces: '\r\n", ch);
        break;
      default:
        send_to_char("Spell names must be enclosed in the Magic Symbols: '\r\n",
            ch);
        break;
    }
    return;
  }
  t = strtok(NULL, "\0");

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum > MAX_SPELLS)) {
    if (IS_NPC(ch))
      return;
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("Cast what?!?\r\n", ch);
        break;
      case CLASS_CLERIC:
        send_to_char("Pray for what?!?\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("Sing about what?!?\r\n", ch);
        break;
      default:
        send_to_char("Cast what?!?\r\n", ch);
        break;
    }
    return;
  }

  if (!IS_NPC(ch) && (GET_CLASS(ch) == CLASS_BARD)) {
     if (ch->equipment[GET_WEAR_WIELD(ch)] == NULL) {
       send_to_char("Bards must wield an instrument to "
                    "sing their songs!\r\n", ch);
       return;
     }
     if (GET_OBJ_TYPE(ch->equipment[GET_WEAR_WIELD(ch)]) != ITEM_INSTRUMENT) {
       send_to_char("Bards must wield an instrument to "
                    "sing their songs!\r\n", ch);
       return;
     }
  }

  if (!IS_NPC(ch) && (GET_LEVEL(ch) < SINFO.min_level[(int) GET_CLASS(ch)])) {
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("You do not know that spell!\r\n", ch);
        break;
      case CLASS_CLERIC:
        send_to_char("Your deity has not granted you that spell!\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("You have not learned the lyrics to that song!\r\n", ch);
        break;
      default:
        send_to_char("You do not know that spell!\r\n", ch);
        break;
    }
    return;
  }

  if (!IS_NPC(ch) && (GET_SKILL(ch, spellnum) == 0)) {
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("You are unfamiliar with that spell.\r\n", ch);
        break;
      case CLASS_CLERIC:
        send_to_char("You almost could mumble the prayer, but fail.\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("You dont know the lyrics well enough.\r\n", ch);
        break;
      default:
        send_to_char("You are unfamiliar with that spell.\r\n", ch);
        break;
    }
    return;
  }

  /* check for a direction */
  d = t;

  /* Find the target */
  if (t != NULL) {
    one_argument(strcpy(arg, t), t);
    skip_spaces(&t);
  }
  if (IS_SET(SINFO.targets, TAR_IGNORE)) {
    target = TRUE;
  } else if (t != NULL && *t) {
    if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
      if ((tch = get_char_room_vis(ch, t)) != NULL)
	target = TRUE;
  }

/* HACKED to not let players cast spells on !plink mobs */
/* nor to let players cast spells on mobs in other zones */
/*
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_NEARBY)) {
      if ((tch = get_char_nearby_vis(ch, t)) != NULL) {
        target = TRUE;
        if (ch->in_room != tch->in_room) {
          if (MOB_FLAGGED(tch, MOB_NOPLINK) && IS_NPC(tch)) {
            send_to_char("You can't get a good shot!\r\n", ch);
            return;
          }
        }
      }
    }
*/
/* end of hack */

    if (!target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, t)))
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, t, ch->carrying)))
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
      for (i = 0; !target && i < GET_NUM_WEARS(ch); i++)
	if (ch->equipment[i] && !str_cmp(t, ch->equipment[i]->name)) {
	  tobj = ch->equipment[i];
	  target = TRUE;
	}
    }
    if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents)))
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_NEARBY))
      if ((tobj = get_obj_nearby_vis(ch, t)))
        target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD)) {
      if ((tobj = get_obj_vis(ch, t))) {
	target = TRUE;

 /*
  * This is a hack to make locate object, which is the only TAR_OBJ_WORLD spell
  * still use mana and return a value if it can't find the desired object.
  */
      } else {
	target = TRUE;
      }
    }
  } else {			/* if target string is empty */
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL) {
	tch = ch;
	target = TRUE;
      }

    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL) {
	tch = FIGHTING(ch);
	target = TRUE;
        if (ch->in_room != tch->in_room) {
          if (MOB_FLAGGED(tch, MOB_NOPLINK) && IS_NPC(tch)) {
            send_to_char("You can't get a good shot!\r\n", ch);
            return;
          }
        }
      }

    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
	!SINFO.violent) {
      tch = ch;
      target = TRUE;
    }

    if (!target && IS_SET(SINFO.routines, MAG_AREAS)) {
	for (k = world[ch->in_room].people; k; k = next_k) {

	    next_k = k->next_in_room;

       /*
	* The skips: 1) immortals && self 2) mobs only hit charmed mobs 3)
	* players can only hit players in CRIMEOK rooms 4) players can only hit
	* charmed mobs in CRIMEOK rooms
	* MOBProg foo: 5) skip mobs that are MOB_NOTTHERE
	* 6) skips players if the caster is a player
	* 7) skips pets
	*/

	skip = 0;
	if (IS_NPC(k) && MOB_FLAGGED(k, MOB_SAFE))
	    skip = 1;
	if (IS_NPC(k) && MOB_FLAGGED(k, MOB_NOTTHERE))
	    skip = 1;
	if (k == ch)
	    skip = 1;
	if (IS_NPC(ch) && IS_NPC(k) && !IS_AFFECTED(k, AFF_CHARM))
	    skip = 1;
	if (!IS_NPC(k) && GET_LEVEL(k) >= LVL_IMMORT)
	    skip = 1;
	if (!IS_NPC(ch) && !IS_NPC(k))
	    skip = 1;
	if (!IS_NPC(ch) && IS_NPC(k) && IS_AFFECTED(k, AFF_CHARM))
	    skip = 1;
	if (!IS_NPC(ch) && !IS_NPC(k))
	    skip = 1;
	if (IS_PET(k))
	    skip = 1;

	if (skip)
	    continue;

	tch = k;
	target = TRUE;

	}
    }


/* END */
#if 0
    if (!target) {
      if (!strcmp(t, "all") && GET_LEVEL(ch) >= LVL_GOD) {
        for (tch = character_list; tch; tch = tch->next) {
          if (!IS_NPC(tch) && tch->desc)
            cast_spell(ch, tch, tobj, t, spellnum);
        }
        return;
      }
    }
    if (!target) {
      if (!strcmp(t, "room") && GET_LEVEL(ch) >= LVL_GOD) {
        for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
          cast_spell(ch, tch, tobj, t, spellnum);
        }
        return;
      }
    }
#endif


    if (!target) {
      if (IS_NPC(ch))
        return;
      switch (GET_CLASS(ch)) {
        case CLASS_MAGIC_USER:
          sprintf(buf, "Upon %s should the spell be cast?\r\n",
              IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_CHAR_NEARBY
                   | TAR_CHAR_DIR | TAR_OBJ_INV | TAR_OBJ_WORLD) ?
                   "what" : "who");
          break;
        case CLASS_CLERIC:
          sprintf(buf, "For %s should the prayer be recited?\r\n",
              IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_CHAR_NEARBY
                   | TAR_CHAR_DIR | TAR_OBJ_INV | TAR_OBJ_WORLD) ?
                   "what" : "who");
          break;
        case CLASS_BARD:
          sprintf(buf, "About %s should the song be sung?\r\n",
              IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_CHAR_NEARBY
                   | TAR_CHAR_DIR | TAR_OBJ_INV | TAR_OBJ_WORLD) ?
                   "what" : "who");
          break;
        default:
          sprintf(buf, "Upon %s should the spell be cast?\r\n",
              IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_CHAR_NEARBY
                   | TAR_CHAR_DIR | TAR_OBJ_INV | TAR_OBJ_WORLD) ?
                   "what" : "who");
          break;
      }
      send_to_char(buf, ch);
      return;
    }
  }

  if (target && (tch == ch) && SINFO.violent) {
    if (IS_NPC(ch))
      return;
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("You shouldn't cast that on yourself", ch);
        send_to_char(" -- could be bad for your health!\r\n", ch);
        break;
      case CLASS_CLERIC:
        send_to_char("You shouldn't pray for that to happen to you", ch);
        send_to_char(" -- your god is an angry god!\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("You shouldn't take song lyrics so seriously", ch);
        send_to_char(" -- you could die from depression!\r\n", ch);
        break;
      default:
        send_to_char("You shouldn't cast that on yourself", ch);
        send_to_char(" -- could be bad for your health!\r\n", ch);
        break;
    }
    return;
  }

  if (!target) {
    if (IS_NPC(ch))
      return;
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("Cannot find the target of your spell!\r\n", ch);
        break;
      case CLASS_CLERIC:
        send_to_char("Cannot find the object of your prayers!\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("Cannot find the audience for your song!\r\n", ch);
        break;
      default:
        send_to_char("Cannot find the target of your spell!\r\n", ch);
        break;
    }
    return;
  }

  /* NPCs stop here */
  if (IS_NPC(ch)) {
    cast_spell(ch, tch, tobj, t, spellnum);
    return;
  }

  mana = mag_manacost(ch, spellnum);

  if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
        send_to_char("You haven't the energy to cast that spell!\r\n", ch);
        break;
      case CLASS_CLERIC:
        send_to_char("You haven't the inner spirit to pray anymore!\r\n", ch);
        break;
      case CLASS_BARD:
        send_to_char("You haven't the strength to sing that song!\r\n", ch);
        break;
      default:
        send_to_char("You haven't the energy to cast that spell!\r\n", ch);
        break;
    }
    return;
  }

  /* You throws the dice and you takes your chances.. 101% is total failure */
  if (number(0, 101) > check_spell_skill(ch, spellnum) 
	&& (GET_LEVEL(ch) < LVL_IMMORT)) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
/*    if (!tch || !skill_message(0, ch, tch, spellnum)) */
      switch (GET_CLASS(ch)) {
        case CLASS_MAGIC_USER:
          send_to_char("You lost your concentration!\r\n", ch);
          break;
        case CLASS_CLERIC:
          send_to_char("Your prayer goes unanswered!\r\n", ch);
          break;
        case CLASS_BARD:
          send_to_char("Your song is off key!\r\n", ch);
          break;
        default:
          send_to_char("You lost your concentration!\r\n", ch);
          break;
      }
    if (mana > 0)
      GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
    
    if (SINFO.violent && tch && IS_NPC(tch))
      hit(tch, ch, TYPE_UNDEFINED);
  } else {

/* HACKED for spell_progs - they trigger here */
    if (mprog_spell_trigger(ch, spells[spellnum])) return;
/* END of HACK */

/* HACKED to take out a crash bug in skip spaces over whole_target */
    if (cast_spell(ch, tch, tobj, t, spellnum)) {
/* end of hack */
	if (GET_LEVEL(ch) < LVL_IMMORT)
	    WAIT_STATE(ch, PULSE_VIOLENCE);
	if (mana > 0)
	    GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
    }
  }
}


/* Assign the spells on boot up */
void spello(int spl,
		int magicuserlev, int clericlev, int thieflev, int warriorlev,
		int bardlev, int deathknightlev, int druidlev, 
		int vampirelev, int monklev, int lichlev,
		int seraphlev, int cherublev,
		int valklev, int unusedlev, int ghostlev, int warriorlev2,
		int bardlev2, int citylev, int unused2lev, int muppetlev,
		int clericlev2,
		int max_mana, int min_mana, int mana_change, int minpos,
		int targets, int violent, int routines)
{
  spell_info[spl].min_level[CLASS_MAGIC_USER] = magicuserlev;
  spell_info[spl].min_level[CLASS_CLERIC] = clericlev;
  spell_info[spl].min_level[CLASS_THIEF] = thieflev;
  spell_info[spl].min_level[CLASS_WARRIOR] = warriorlev;
  spell_info[spl].min_level[CLASS_BARD] = bardlev;
  spell_info[spl].min_level[CLASS_DEATHKNIGHT] = deathknightlev;
  spell_info[spl].min_level[CLASS_DRUID] = druidlev;
  spell_info[spl].min_level[CLASS_VAMPIRE] = vampirelev;
  spell_info[spl].min_level[CLASS_MONK] = monklev;
  spell_info[spl].min_level[CLASS_LICH] = lichlev;
  spell_info[spl].min_level[CLASS_SERAPH] = seraphlev;
  spell_info[spl].min_level[CLASS_CHERUB] = cherublev;
  spell_info[spl].min_level[CLASS_VALKYRIE] = valklev;
  spell_info[spl].mana_max = max_mana;
  spell_info[spl].mana_min = min_mana;
  spell_info[spl].mana_change = mana_change;
  spell_info[spl].min_position = minpos;
  spell_info[spl].targets = targets;
  spell_info[spl].violent = violent;
  spell_info[spl].routines = routines;
}

/*
 * Arguments for spello calls:
 *
 * spellnum, levels (MCTW), maxmana, minmana, manachng, minpos, targets,
 * violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL). levels  :  Minimum level (mage, cleric,
 * thief, warrior) a player must be to cast this spell.  Use 'X' for immortal
 * only. maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell). minmana :  The minimum
 * mana this spell will take, no matter how high level the caster is.
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|'). violent :  TRUE or FALSE,
 * depending on if this is considered a violent spell and should not be cast
 * in PEACEFUL rooms or on yourself. routines:  A list of magic routines
 * which are associated with this spell. Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

#define UU (LVL_IMPL+1)
#define UNUSED UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,0,0,0,0,0,0,0

  /* MAGIC_USER CLERIC THIEF WARRIOR BARD 
     Deathknight DRUID vampire monk lich
     seraph cherub valkyrie COLOR THING
     ANIMAL CURRENCY CITY unused MUPPET MONK
     Max-mana Min-mana Change */

#define X LVL_IMMORT

void mag_assign_spells(void)
{
  int i;

  for (i = 1; i <= TOP_SPELL_DEFINE; i++)
    spello(i, UNUSED);
  /* C L A S S E S   (21 classes in all)   and  M A N A   */

  spello(SPELL_ACID_BLAST,
	 X, X, X, X, X, X, 21, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 60, 35, 1,
	 POS_FIGHTING, 
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_AERIAL_SERVANT,
         3, 9, X, X, X, X, X, 3, X, X,
         3, X, X, X, X, X, X, X, X, X, X,
         70, 30, 3,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_AID,
         X, 20, X, X, X, X, X, X, X, X,
         X, 1, X, X, X, X, X, X, X, X, X,
         50, 15, 1,
         POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_AIR_ELEMENTAL,
         X, X, X, X, X, X, 40, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         200, 200, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_ANIMATE_DEAD,
	 15, 21, X, X, X, X, X, 15, X, X,
	 15, X, X, X, X, X, X, X, X, X, X,
	 100, 50, 3,
	 POS_STANDING,
	 TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_SUMMONS);

/*
  spello(SPELL_ANTIMAGIC_SHELL,
	 26, X, X, X, X, 26, X, X, X, X,
	 26, X, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
*/

  spello(SPELL_AREA_SCARE,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         300, 200, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_AREA_WORD_OF_DEATH,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         300, 250, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
         TRUE, MAG_AREAS);

  spello(SPELL_ARMOR, 
	 2, 2, X, X, X, X, X, 2, X, X,
         2, 2, X, X, X, X, X, X, X, X, X,
	 50, 10, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_BARKSKIN,
         X, X, X, X, 10, X, 1, X, X, 10,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_BATTLE_HYMN,
         X, X, X, X, 38, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 50, 1,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_MASSES);

  spello(SPELL_BLESS, 
	 X, 8, X, X, X, X, X, X, X, X,
	 X, 3, X, X, X, X, X, X, X, X, X,
	 50, 15, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_BLINDNESS, 
	 27, 19, X, X, X, X, X, 27, X, X,
	 27, X, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS);

  spello(SPELL_BLUR,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 25, 3,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_BURNING_HANDS,
	 4, X, X, X, X, X, X, 4, X, X,
	 4, X, X,10, X, X, X, X, X, X, X,
	 50, 15, 3,
         POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AREAS);

  spello(SPELL_CAUSE_CRITIC,
         X, 18, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CAUSE_LIGHT,
         X, 3, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 10, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
  
  spello(SPELL_CAUSE_SERIOUS,
         X, 7, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 15, 2,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CALL_LIGHTNING,
	 X, 37, X, X, X, X, X, X, X, X,
	 X, 4, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_FIGHTING, 
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CALM,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_CANTRIP,
	 X, X, X, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 10, 10, 1,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_CHAIN_LIGHTNING,
         40, X, X, X, X, X, X, 40, X, X,
         40, X, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
         TRUE, MAG_AREAS);

  spello(SPELL_CHARM,
	 X, X, X, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 70, 70, 1,
	 POS_STANDING,
	 TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL);

  spello(SPELL_CHILL_TOUCH,
	 16, X, X, X, X, X, X, 16, X, X,
	 16, X, 1, 1, 1, X, X, X, X, X, X,
	 50, 15, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS);

  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */

  spello(SPELL_CLAIRVOYANCE,
	 2, X, X, X, X, X, X, 5, X, X,
         5, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_STANDING,
	 TAR_CHAR_WORLD, FALSE, MAG_MANUAL);

  spello(SPELL_COLOR_SPRAY,
	 26, X, X, X, X, X, X, 26, X, X,
	 26, X, X, X, X, X, X, X, X, X, X,
	 50, 20, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CONE_OF_COLD,
         31, X, X, X, X, X, X, 31, X, X,
         31, X, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
         TRUE, MAG_AREAS);

  spello(SPELL_CONJURE_ELEMENTAL,
         32, X, X, X, X, X, X, 32, X, X,
         32, X, X, X, X, X, X, X, X, X, X,
         150, 100, 5,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_CONTINUAL_LIGHT, 
	 1, 1, X, X, X, X, X, 1, 1, 1,
	 1, 5, X, X, X, X, X, X, X, X, X,
	 20, 5, 3,
	 POS_STANDING,
	 TAR_IGNORE, FALSE, MAG_CREATIONS);

  spello(SPELL_CONTROL_WEATHER,
	 X, 36, X, X, X, X, X, X, X, X,
	 X, 6, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_STANDING,
	 TAR_IGNORE, FALSE, MAG_MANUAL);

  spello(SPELL_CREATE_BLOOD,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         40, 5, 3,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS);

  spello(SPELL_CREATE_FOOD, 
	 X, 4, X, X, X, X, 9, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 40, 5, 3,
	 POS_STANDING,
	 TAR_IGNORE, FALSE, MAG_CREATIONS);

  spello(SPELL_CREATE_SPRING,
         X, X, X, X, X, X, 11, X, X, X,
         X, 7, X, X, X, X, X, X, X, X, X,
         40, 5, 3,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS);

  spello(SPELL_CREATE_WATER,
	 X, 5, X, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 40, 5, 3,
	 POS_STANDING,
	 TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL);

  spello(SPELL_CURE_BLIND,
	 X, 11, X, X, X, X, X, X, X, X,
	 X, 8, X, X, X, X, X, X, X, X, X,
	 50, 10, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS);

  spello(SPELL_CURE_CRITIC,
	 X, 17, X, X, X, X, 35, X, X, X,
	 X, 11, X, X, X, X, X, X, X, X, X,
	 50, 20, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */

  spello(SPELL_CURE_LIGHT,
	 X, 1, X, X, X, X, 3, X, X, X,
	 X, 9, X, X, X, X, X, X, X, X, X,
	 50, 10, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_CURE_SERIOUS,
         X, 6, X, X, X, X, 15, X, X, X,
         X, 10, X, X, X, X, X, X, X, X, X,
         50, 15, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_CURSE,
	 29, 25, X, X, X, X, X, 29, X, X,
	 29, X, X, X, X, X, X, X, X, X, X,
	 50, 30, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV | TAR_FIGHT_VICT,
	 TRUE, MAG_AFFECTS);

  spello(SPELL_DEFENSIVE_HARMONY,
         X, 13, X, X, X, X, X, X, X, X,
         X, 12, X, X, X, X, X, X, X, X, X,
         60, 30, 3,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_DETECT_INVIS,
	 8, 12, X, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 50, 15, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_DETECT_MAGIC,
	 6, X, X, X, X, X, X, 6, X, X,
	 6, X, X, X, X, X, X, X, X, X, X,
	 50, 10, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */

  spello(SPELL_DETECT_POISON,
	 X, 3, X, X, X, X, 3, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 50, 10, 3,
	 POS_STANDING,
	 TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_DISINTEGRATE,
         42, X, X, X, X, X, X, 42, X, X,
         42, X, X, X, X, X, X, X, X, X, X,
         60, 60, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
	 TRUE, MAG_DAMAGE);

  spello(SPELL_DISPEL_EVIL,
	 X, 23, X, X, X, X, X, X, X, X,
	 X, 13, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_DISPEL_GOOD,
	 X, 23, X, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_DISPEL_MAGIC,
	 38, X, X, X, X, X, X, 38, X, X,
	 38, 13, X, X, X, X, X, X, X, X, X,
	 50, 50, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_MANUAL);

  spello(SPELL_DISPEL_SANCTUARY,
         X, 49, X, X, X, X, X, X, X, X,
         X, 14, X, X, X, X, X, X, X, X, X,
         100, 100, 1,
         POS_STANDING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_UNAFFECTS);

  spello(SPELL_EARTHQUAKE,
	 X, 35, X, X, X, X, X, X, X, X,
	 X, 15, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AREAS);

  spello(SPELL_EARTH_ELEMENTAL,
         X, X, X, X, X, X, 26, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 100, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);
  
  spello(SPELL_ENCHANT_ARMOR,
         30, X, X, X, X, X, X, 30, X, X,
         30, X, X, X, X, X, X, X, X, X, X,
         150, 75, 2,
         POS_STANDING,
         TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL);

  spello(SPELL_ENCHANT_WEAPON,
	 38, X, X, X, X, X, X, 38, X, X,
	 38, X, X, X, X, X, X, X, X, X, X,
	 150, 150, 1,
	 POS_STANDING,
	 TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL);

  spello(SPELL_ENERGY_DRAIN,
	 20, X, X, X, X, X, X, 20, X, X,
	 20, X, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL);

  spello(SPELL_FAERIE_FIRE, 
	 4, 4, X, X, 6, X, 4, 4, X, X,
         4, 16, X, X, 3, X, X, X, X, X, X,
	 50, 10, 3,
         POS_STANDING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS);

  spello(SPELL_FAERIE_FOG,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 10, 3,
         POS_STANDING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_UNAFFECTS);

  spello(SPELL_FEEBLEMIND,
         7, X, X, X, 17, X, X, 7, X, 10,
         7, X, X, X, X, X, X, X, X, X, X,
         50, 10, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_POINTS | MAG_AFFECTS);

  spello(SPELL_FIND_FAMILIAR,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         40, 40, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);
/*
  spello(SPELL_FIND_FAMILIAR,
         5, X, X, X, X, X, X, 5, X, X,
         5, X, X, X, X, X, X, X, X, X, X,
         40, 40, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);
*/

  spello(SPELL_FINGER_OF_DEATH,
	 X, X, X, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_FIREBALL,
         36, X, X, X, X, X, X, 36, X, X,
         36, X, X, X, X, X, X, X, X, X, X,
         50, 50, 2,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
	 TRUE, MAG_AREAS);

  spello(SPELL_FIRESHIELD,
         43, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 100, 1,
         POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_FIRESTORM,
	 X, X, X, X, X, X, 50, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 80, 80, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AREAS);

  spello(SPELL_FIRE_ELEMENTAL,
         X, X, X, X, X, X, 48, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         250, 250, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_FIST_OF_EARTH,
	 X, X, X, X, X, X, 36, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_FIGHTING, 
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_FIST_OF_STONE,
	 X, X, X, X, X, X, 41, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_FIGHTING, 
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_FLOATING_DISC,
         4, X, X, X, X, X, X, 4, X, X,
         4, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_STANDING,
	 TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_FLY,
         25, X, X, X, 12, X, X, 25, X, 10,
         25, 17, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_FORGET,
	 24, X, X, X, 10, X, X, 24, X, 10,
	 24, 18, X, X, X, X, X, X, X, X, X,
	 50, 20, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_GATE_I,
         X, 40, X, X, X, X, X, X, X, X,
         X, 23, X, X, X, X, X, X, X, X, X,
         150, 100, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_GATE_II,
         X, 45, X, X, X, X, X, X, X, X,
         X, 25, X, X, X, X, X, X, X, X, X,
         200, 150, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_GATE_III,
         X, 49, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         250, 200, 50,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */

  spello(SPELL_GROUP_HEAL,
	 X, 42, X, X, X, X, X, X, X, X,
	 X, 19, X, X, X, X, 25, X, X, X, X,
	 150, 90, 1,
	 POS_FIGHTING,
	 TAR_IGNORE, FALSE, MAG_GROUPS);
  
  spello(SPELL_GROUP_RECALL,
         X, 20, X, X, X, X, X, X, X, X,
         X, 20, X, X, X, X, 25, X, X, X, X,
         50, 15, 1,
         POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS);

   spello(SPELL_GROUP_SANCTUARY,
	 X, 50, X, X, X, X, X, X, X, X,
	 X, 21, X, X, X, X, 25, X, X, X, X,
	 200, 200, 1,
	 POS_STANDING,
	 TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_GYPSY_DANCE,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 50, 2,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL);

  spello(SPELL_HARM,
	 X, 28, X, X, X, X, X, X, X, X,
	 X, 22, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_HASTE,
         35, X, X, X, X, X, X, 32, X, X,
         32, X, X, X, X, X, X, X, X, X, X,
         75, 75, 1,
         POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS );

  spello(SPELL_HEAL,
	 X, 24, X, X, X, X, X, X, X, X,
	 X, 23, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_POINTS );

  spello(SPELL_ENERGY_FLUX,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS );

  spello(SPELL_HOLY_LIGHT,
         X, 35, X, X, X, X, X, X, X, X,
         X, 24, X, X, X, X, X, X, X, X, X,
         60, 30, 3,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_ICE_SHIELD,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 30, 1,
         POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_ICE_STORM,
         X, X, X, X, X, X, 30, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         70, 50, 2,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
	 TRUE, MAG_AREAS);

  spello(SPELL_IDENTIFY,
         21, X, X, X, X, X, X, 21, X, X,
	 21, X, X, X, X, X, X, X, X, X, X,
         50, 25, 1,
         POS_STANDING,
	 TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_INFRAVISION,
	 23, 14, X, X, X, X, X, 23, X, X,
	 23, X, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_INSECT_PLAGUE,
         X, 26, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         120, 120, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_INSPIRE,
         X, X, X, X, 18, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 10, 1,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_INSTILL_ENERGY,
         50, X, X, X, X, X, X, 49, X, X,
         49, X, X, X, X, X, X, X, X, X, X,
         200, 200, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_INVISIBLE,
	 6, X, X, X, X, X, X, 6, X, X,
	 6, X, X, X, X, X, X, X, X, X, X,
	 30, 30, 1,
	 POS_STANDING,
	 TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
	 FALSE, MAG_AFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_INVISIBLE_STALKER,
	 22, X, X, X, X, X, X, 22, X, X,
	 22, X, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_STANDING,
	 TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_KNOCK,
         46, X, X, X, X, X, X, 46, X, X,
         46, X, X, X, X, X, X, X, X, X, X,
         80, 80, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL);

  spello(SPELL_LIGHTNING_BOLT,
	 34, X, X, X, X, X, X, 34, X, X,
	 34, X, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_FIGHTING, 
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */

  spello(SPELL_LOCATE_OBJECT,
	 13, 21, X, X, X, X, X, 13, X, X,
	 13, X, X, X, X, X, X, X, X, X, X,
	 30, 30, 1,
	 POS_STANDING,
	 TAR_OBJ_WORLD, FALSE, MAG_MANUAL);

  spello(SPELL_MAGIC_JAR,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         200, 200, 1,
         POS_DEAD,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL);
/*
  spello(SPELL_MAGIC_JAR,
         48, X, X, X, X, X, X, 50, X, X,
         50, X, X, X, X, X, X, X, X, X, X,
         200, 200, 1,
         POS_DEAD,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL);
*/

  spello(SPELL_MAGIC_MISSILE,
	 1, X, X, X, 1, X, X, 1, X, X,
	 1, X, X, X, X, X, X, X, X, X, X,
	 50, 10, 3,
	 POS_FIGHTING, 
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_MANASHELL,
         28, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         150, 150, 1,
         POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_MASS_INVIS,
         20, X, X, X, X, X, X, 18, X, X,
         18, X, X, X, X, X, X, X, X, X, X,
         80, 40, 2,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_GROUPS);

  spello(SPELL_MASS_FLY,
         X, X, X, X, X, X, 20, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         60, 35, 2,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_GROUPS);

  spello(SPELL_MASS_REFRESH,
         X, X, X, X, X, X, 43, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         60, 35, 2,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_GROUPS);

  spello(SPELL_MIRROR_IMAGE,
         X, X, X, X, 20, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 30, 1,
         POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_MONSUM_I,
         20, X, X, X, X, X, X, 20, X, X,
         20, X, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_MONSUM_II,
         32, X, X, X, X, X, X, 32, X, X,
         32, X, X, X, X, X, X, X, X, X, X,
         100, 100, 3,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_MONSUM_III,
         39, X, X, X, X, X, X, 39, X, X,
         39, X, X, X, X, X, X, X, X, X, X,
         150, 150, 3,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_WINDWALK,
         X, X, X, X, X, X, 33, X, X, X,
         X, 33, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_STANDING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);

  spello(SPELL_PASS_WITHOUT_TRACE,
         12, X, X, X, X, X, 18, 12, X, X,
         12, X, X, X, X, X, X, X, X, X, X,
         50, 10, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);


  spello(SPELL_PETRIFY,
	 45, 41, X, X, X, X, X, 45, X, X,
	 45, X, X, X, X, X, X, X, X, X, X,
	 100, 100, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_PORTAL,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 50, 0,
         POS_STANDING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);

  spello(SPELL_PRAYER,
         X, 33, X, X, X, X, X, X, X, X,
         X, 25, X, X, X, X, X, X, X, X, X,
         90, 60, 2,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_GROUPS);


  spello(SPELL_PROTECTION_FROM_GOOD,
         X, 15, X, X, X, X, X, 20, X, X,
         20, X, X, X, X, X, X, X, X, X, X,
         40, 40, 1,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_REFRESH,
         X, X, X, X, X, X, 22, X, X, X,
         X, 26, X, X, X, X, X, X, X, X, X,
         40, 20, 1,
         POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_RESTORE,
	 X, 45, X, X, X, X, X, X, X, X,
	 X, 27, X, X, X, X, X, X, X, X, X,
	 350, 250, 1,
         POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_POINTS );
   
  spello(SPELL_POISON,
	 X, 15, X, X, X, X, X, 28, X, X,
	 28, X, X, X, X, X, X, X, X, X, X,
	 50, 20, 3,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV,
         TRUE, MAG_AFFECTS);

  spello(SPELL_POISON2,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV,
         TRUE, MAG_AFFECTS);

  spello(SPELL_POWER_WORD_STUN,
         X, X, X, X, X, X, 45, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         70, 40, 3,
         POS_STANDING,
         TAR_CHAR_ROOM, TRUE, MAG_AFFECTS);

  spello(SPELL_PROT_FROM_EVIL,
	 X, 16, X, X, X, X, X, X, X, X,
	 X, 28, X, X, X, X, X, X, X, X, X,
	 30, 30, 1,
	 POS_STANDING,
	 TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_RAY_OF_ENFEEBLEMENT,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV,
         TRUE, MAG_AFFECTS);

  spello(SPELL_REMOVE_CURSE,
	 X, 30, X, X, X, X, X, X, X, X,
	 X, 29, X, X, X, X, X, X, X, X, X,
	 40, 30, 1,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_MANUAL );

  spello(SPELL_REMOVE_POISON,
         X, 14, X, X, X, X, 5, X, X, X,
         X, 30, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS);

  spello(SPELL_SANCTUARY,
	 X, 24, X, X, X, X, X, X, X, X,
	 X, 31, X, X, X, X, X, X, X, X, X,
	 100, 70, 2,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_SCARE,
	 X, X, X, X, 21, X, X, 20, X, X,
	 20, X, X, X, X, X, X, X, X, X, X,
	 100, 100, 1,
	 POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_SEARING_ORB,
	 X, X, X, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 50, 50, 1,
	 POS_FIGHTING, 
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
	
  spello(SPELL_SENSE_LIFE,
         8, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 20, 3,
         POS_STANDING,
	 TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_SHOCKING_GRASP,
	 12, X, X, X, X, X, X, 14, X, X,
	 14, X, X, 8, X, X, X, X, X, X, X,
	 50, 15, 3, 
	 POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */

  spello(SPELL_SLEEP,
	 19, X, X, X, 15, X, X, 18, X, X,	
	 18, X, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM, TRUE, MAG_AFFECTS);

  spello(SPELL_SLOW,
         X, X, X, X, 25, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 50, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS );

  spello(SPELL_SPIRITUAL_HAMMER,
         X, 41, X, X, X, X, X, X, X, X,
         X, 32, X, X, X, X, X, X, X, X, X,
         100, 55, 2,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS);

  spello(SPELL_STICKS_TO_SNAKES,
         X, 10, X, X, 16, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         80, 50, 1,
         POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);

  spello(SPELL_STONESKIN,
         X, X, X, X, X, X, 25, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 30, 1,
         POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_STRENGTH,
	 9, X, X, X, 26, X, X, 9, X, X,
	 9, X, X, X, X, X, X, X, X, X, X,
	 50, 25, 3,
	 POS_STANDING,
	 TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_RELOCATE,
         49, 47, X, X, 50, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_STANDING,
	 TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);

  spello(SPELL_SUMMON,
	 X, 32, X, X, 48, X, 38, X, X, X,
	 X, 34, X, X, X, X, X, X, X, X, X,
	 100, 50, 3,
	 POS_STANDING,
	 TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);

  spello(SPELL_SUNRAY,
         X, X, X, X, X, X, 42, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         100, 100, 2,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
         TRUE, MAG_AREAS);

  spello(SPELL_TELEPORT,
         18, X, X, X, 30, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 25, 3,
         POS_STANDING,
	 TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL);

  spello(SPELL_PHASE_DOOR,
         X, X, X, X, X, X, X, 39, X, X,
         39, X, X, X, X, X, X, X, X, X, X,
         50, 50, 3,
         POS_STANDING,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL);

  spello(SPELL_DIMENSION_DOOR,
         X, X, X, X, X, X, X, 15, X, X,
         15, X, 15, 15, 15, X, X, X, X, X, X,
         50, 25, 3,
         POS_STANDING,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL);

  spello(SPELL_WATERWALK,
         15, 11, X, X, X, X, 17, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 25, 4,
         POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_WATER_ELEMENTAL,
         X, X, X, X, X, X, 32, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         150, 150, 1,
         POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS);


  spello(SPELL_WILD_HEAL,
         X, 20, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 50, 3,
         POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS);

  /* MAGIC_USER CLERIC THIEF WARRIOR BARD 
     Deathknight DRUID vampire monk lich
     seraph cherub valkyrie COLOR THING
     ANIMAL CURRENCY CITY unused MUPPET MONK
     Max-mana Min-mana Change */

  spello(SPELL_WEB,
         37, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 25, 3,
         POS_FIGHTING,
	 TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,
	 TRUE, MAG_AFFECTS);

  spello(SPELL_WIZARD_LOCK,
         17, X, X, X, 42, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 50, 1,
         POS_STANDING,
	 TAR_IGNORE, FALSE, MAG_MANUAL);

  spello(SPELL_WORD_OF_DEATH,
         48, X, X, X, 50, X, X, 44, X, X,
         44, X, X, 30, X, X, X, X, X, X, X,
         100, 100, 1,
         POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,
         TRUE, MAG_DAMAGE);

  spello(SPELL_WORD_OF_RECALL,
         X, 5, X, X, 46, X, X, 24, X, X,
         24, 35, X, X, X, X, X, X, X, X, X,
         50, 5, 1,
         POS_FIGHTING,
	 TAR_CHAR_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_VENTRILOQUATE,
         2, X, X, X, X, X, X, 1, X, X,
         1, X, X, X, X, X, X, X, X, X, X,
	 50, 10, 3,
         POS_FIGHTING,
         TAR_OBJ_ROOM | TAR_OBJ_NEARBY | TAR_CHAR_ROOM ,
	 FALSE, MAG_MANUAL);

  spello(SPELL_CREATE_WEAPON,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         50, 50, 2,
         POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL);

  /*
   * SKILLS
   * 
   * The only parameters needed for skills are only the minimum levels for each
   * class.  The remaining 8 fields of the structure should be filled with
   * 0's.
   */

  /* Ma  Cl  Th  Wa  */
  spello(SKILL_AVENGING_BLOW,
         X, X, X, X, X, 30, X, X, X, X,
         X, X, X, X, X, X, X, X, 30, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BACKSTAB,
	 X, X, 3, X, 10, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);
  
  spello(SKILL_BASH,
	 X, X, X, 2, 2, 2, X, 6, 20, 0,
	 2, X, 2, 2, 2, 2, 2, 2, 1, 2, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BERSERK,
	 X, X, X, 15, X, 5, X, X, X, X,
	 X, X, X, X, X, X, X, X, 5, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BLOCK,
         X, X, X, 15, 5, 5, X, X, X, X,
         X, X, X, X, X, X, X, X, 5, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BREATHE_ACID,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BREATHE_FIRE,
	 X, X, X, X, X, X, X, X, 10, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BREATHE_FROST,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BREATHE_GAS,
	 X, X, X, X, X, X, X, X, X, 10,
         X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BREATHE_LIGHTNING,
         X, X, X, X, X, 10, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_CIRCLE,
         X, X, 20, X, X, X, X, X, X, X,
         X, X, 4, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_DEFEND,
         X, X, X, 15, X, 3, X, X, 15, 15,
         X, X, X, 12, X, X, X, X, 3, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_DISARM,
         X, X, X, 10, 10, 3, X, 10, 10, 10,
         10, X, 10, 10, 10, X, X, X, 3, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_DUALWIELD,
	 X, X, X, 25, X, 12, X, X, X, X,
	 X, X, X, X, X, X, X, X, 12, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_GAUGE,
         X, X, 30, X, X, X, X, X, X, X, 
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_HEAL,
         X, X, X, X, X, 18, X, X, X, X,
         X, X, X, X, X, X, X, X, 18, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_HIDE,
	 X, X, 9, X, X, X, 22, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_JUDGE,
         X, X, X, 1, X, 1, X, X, X, X,
         X, X, X, X, X, X, X, X, 1, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_KICK,
	 X, X, X, 1, 1, 1, X, 1, 1, 1,
	 1, X, 1, 1, 1, X, X, X, 1, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_MEDITATE,
         10, 10, X, X, 20, X, 24, 1, X, X,
         1, 36, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  /* not a real skill, just a mobprogramming thing */
  spello(SKILL_MPDAMAGE,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_PALM,
         X, X, 10, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_PICK_LOCK,
	 X, X, 7, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_QUICKDRAW,
	 X, X, X, 36, X, 17, X, X, X, X,
	 X, X, X, X, X, X, X, X, 17, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_QUICKHEAL,
         X, X, 10, 10, X, 8, X, X, X, X,
         X, X, X, X, X, X, X, X, 8, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_RAGE,
         X, X, X, 18, X, 13, X, X, X, X,
         X, X, X, X, X, X, X, X, 13, X, X,
         0, 0, 0, 0, 0, 0, 0);


  spello(SKILL_RESCUE,
	 X, X, X, 3, 8, 3, X, X, 3, 3,
	 X, X, X, 2, 5, X, X, X, 8, X, X,
	 0, 0, 0, 0, 0, 0, 0);
  
  spello(SKILL_RETREAT,
         X, X, 22, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_RIPOSTE,
         X, X, X, 40, X, 20, X, X, X, X,
         X, X, X, X, X, X, X, X, 20, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_SCAN,
	 X, X, 15, X, X, 1, X, X, 1, 1,
         X, X, 12, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_SCROUNGE,
         X, X, 1, X, X, X, 1, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_CRITICAL_HIT,
         X, X, X, 10, X, 5, X, X, X, X,
         X, X, X, X, X, X, X, X, 5, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_SNEAK,
	 X, X, 1, X, X, X, 12, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_STEAL,
	 X, X, 5, X, X, X, X, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_STUN_TOUCH,
         X, X, 20, X, X, X, X, X, X, X,
         X, X, 4, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_SWITCH,
         X, X, X, 20, X, 9, X, X, X, X,
         X, X, X, X, X, X, X, X, 9, X, X,
         0, 0, 0, 0, 0, 0, 0);  

  /* Ma  Cl  Th  Wa  */

  spello(SKILL_TRACK,
	 X, X, 25, 9, 9, 10, 26, X, X, X,
	 X, X, X, X, X, X, X, X, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_TRIP,
         X, X, 35, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  /* twist is not a real 'skill' just a message in a sense */
  spello(SKILL_TWIST,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_VALOUR,
         X, X, X, X, X, 30, X, X, X, X,
         X, X, X, X, X, X, X, X, 30, X, X,
         0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BANDAGE,
         X, X, X, X, X, X, X, X, X, X,
         X, X, X, X, X, X, X, X, X, X, 1,
         0, 0, 0, 0, 0, 0, 0);

}
