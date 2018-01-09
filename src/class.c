/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "spells.h"
#include "interpreter.h"



/* external functions */
void clanlog(char *str, struct char_data * ch);



/* Names first */

const char *class_abbrevs[] = {
  "Mag",
  "Cle",
  "Thf",
  "War",
  "Bar",
  "DKn",
  "Dru",
  "Vam",
  "Mnk",
  "Lch",
  "Ser",
  "Chr",
  "Vlk",
  "Unu",
  "Unu",
  "Unu",
  "Unu",
  "Unu",
  "Unu",
  "Unu",
  "Unu",
  "\n"
};


/* HACKED to add immort abbreviations */
const char *immort_abbrevs[] = {
  "BUI",	/*  Builder     */	/* 51 */
  "^yIMM^r",	/*  Immortal	*/
  "AVA",	/*  Avatar	*/
  "SUP",	/*  Supreme	*/
  "LSG",	/*  Lesser God	*/	/* 55 */
  "DMG",	/*  Demigod	*/
  "^mDEI^n",	/*  Deity       */
  "^gGOD^n",	/*  God         */
  "^RGRG^n",        /*  Greater God */
  "^WIMP^n",	/*  Implementor */	/* 60 */
  "\n"
};

/*

#define CLASS_UNDEFINED         -1
#define CLASS_MAGIC_USER        0
#define CLASS_CLERIC            1
#define CLASS_THIEF             2
#define CLASS_WARRIOR           3
#define CLASS_BARD              4
#define CLASS_DEATHKNIGHT       5
#define CLASS_DRUID             6
#define CLASS_VAMPIRE           7
#define CLASS_MONK              8
#define CLASS_LICH              9
#define CLASS_SERAPH            10
#define CLASS_CHERUB            11
#define CLASS_VALKYRIE          12
#define CLASS_UNUSED13          13
#define CLASS_UNUSED14          14
#define CLASS_UNUSED15          15
#define CLASS_UNUSED16          16
#define CLASS_UNUSED17          17
#define CLASS_UNUSED18          18
#define CLASS_UNUSED19          19
#define CLASS_UNUSED20          20

*/


const char *pc_class_types[] = {
  "Mage",
  "Cleric",
  "Thief",
  "Warrior",
  "Bard",
  "Deathknight",
  "Druid",
  "Vampire",
  "Monk",
  "Lich",
  "Seraph",
  "Cherub",
  "Valkyrie",
  "Unused",
  "Unused",
  "Unused",
  "Unused",
  "Unused",
  "Unused",
  "Unused",
  "Unused",
  "\n"
};



/* KASMIR */
const int race_allows_class[] = {
  /* Human */		BIT(CLASS_MAGIC_USER) + BIT(CLASS_CLERIC) +
			BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR) + 
			BIT(CLASS_BARD) + BIT(CLASS_DRUID),
  /* Elf */		BIT(CLASS_MAGIC_USER) + BIT(CLASS_CLERIC) +
			BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR) +
			BIT(CLASS_BARD) + BIT(CLASS_DRUID),
  /* Hobbit */		BIT(CLASS_MAGIC_USER) + BIT(CLASS_CLERIC) +
			BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR) +
			BIT(CLASS_BARD) + BIT(CLASS_DRUID),
  /* Dwarf */		BIT(CLASS_CLERIC) +
			BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR),
  /* Orc */		BIT(CLASS_WARRIOR) + BIT(CLASS_THIEF),
  /* Drow */		BIT(CLASS_MAGIC_USER) + BIT(CLASS_CLERIC) +
			BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR),
  /* Insect */		BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR),
  /* Minotaur */	BIT(CLASS_WARRIOR),
  /* Troll */		BIT(CLASS_WARRIOR),
  /* Giant */		BIT(CLASS_WARRIOR),
  /* Dragon */		BIT(CLASS_WARRIOR),
  /* Undead */		BIT(CLASS_VAMPIRE),
  /* Half-elf */	BIT(CLASS_MAGIC_USER) + BIT(CLASS_CLERIC) +
                        BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR) +
                        BIT(CLASS_BARD),
  /* Gnome */		BIT(CLASS_MAGIC_USER) + BIT(CLASS_CLERIC) +
			BIT(CLASS_THIEF) + BIT(CLASS_WARRIOR) +
			BIT(CLASS_DRUID),
  /* Elemental */       BIT(CLASS_VAMPIRE) + BIT(CLASS_DRUID),
  /* Dueregar */        BIT(CLASS_CLERIC) + BIT(CLASS_WARRIOR),
  /* Thri-kreen*/	BIT(CLASS_WARRIOR),
  /* end of list */	0
};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu[] = {
"  ^RMage^n                 Arcane spell casters.\r\n",
"  ^RCleric^n               Healers and defenders of the faithful.\r\n",
"  ^RThief^n                Sneaky rogues.\r\n",
"  ^RWarrior^n              Skilled swords-for-hire.\r\n",
"  ^RBard^n                 Spell singers and troubadors.\r\n",
"  ^RDragon^n               Terrible winged lizards.\r\n",
"  ^RDruid^n                Nature priests of yore.\r\n",
"  ^RMage2^n\r\n",
"  ^RCleric2^n\r\n",
"  ^RThief2^n\r\n",
"  ^RVampire^n              Demonic blood drinkers, the foul undead.\r\n",
"  ^RLich^n                 Undead spell casters, soulless and full of malice.\r\n",
"  ^RGhoul^n                Skulking corpse-eaters.\r\n",
"  ^RDeath Knight^n         Undead warriors, who traded life for magic.\r\n",
"  ^RGhost^n                Spirits bound to Kore by tragedy in their lives.\r\n",
"  ^RWarrior2^n\r\n",
"  ^RBard2^n\r\n",
"  ^RCity^n\r\n",
"  ^RKnight^n               Champions of Kore.\r\n",
"  ^RMuppet^n\r\n",
"\r\n"
};

const bool monk_can_wear[NUM_ITEM_WEARS] = {
/* Take   Finger  Neck    Body   Head   Legs   Feet   Hands  Arms   Shield
   About  Waist   Wrist   Wield  Hold   Ready */
   TRUE,  TRUE,   TRUE,   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
   FALSE, TRUE,   TRUE,   TRUE,  TRUE,  TRUE
};

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char *arg)
{
  int i;
  char buf[MAX_STRING_LENGTH];

  sprintf(buf, "%c%s", UPPER(arg[0]), arg + 1);
  for (i = 0; *(pc_class_types[i]) != '\n'; i++)
    if (!strncasecmp(buf, pc_class_types[i], strlen(arg)))
      break;
  if (!strcmp(pc_class_types[i], "\n"))
    return CLASS_UNDEFINED;
  else
    return i;
}


/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */
long find_class_bitvector(char *arg)
{
  int i;
  char buf[MAX_INPUT_LENGTH];

  sprintf(buf, "%c%s", UPPER(arg[0]), arg + 1);
  for (i = 0; *(pc_class_types[i]) != '\n'; i++)
    if (!strncasecmp(buf, pc_class_types[i], strlen(arg)))
      break;
  if (!strcmp(pc_class_types[i], "\n"))
    return 0;	/* CLASS_UNDEFINED */
  else
    return (1 << i);
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		4  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
/* MAG	CLE	THE	WAR	BAR	DRA	DRU	u7	u8	u9	VAM	LIC	GHL	DKN	GHO	ANM	CUR	CIT	KNI	MUP	MON	*/
{95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95,	95},	/* learned level */
{100,	100,	12,	12,	100,	12,	100,	12,	12,	12,	12,	12,	12,	12,	12,	12,	12,	12,	12,	12,	12},	/* max per prac */
{25,	25,	0,	0,	25,	0,	25,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	10},	/* min per pac */
{SPELL,	SPELL,	SKILL,	SKILL,  SPELL,	SKILL,	SPELL,	SKILL,	SKILL,	SKILL,	SPELL,	SPELL,	SKILL,	SKILL,	SPELL,	SKILL,	SKILL,	SKILL,	SKILL,	SKILL,	SPELL}	/* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 */
/* HACKED to let remort races ie dragons and undead use the guilds */
int guild_info[][3] = {

/* Kore */
  {CLASS_MAGIC_USER,	3017,	SCMD_SOUTH},
  {CLASS_CLERIC,	3004,	SCMD_NORTH},
  {CLASS_THIEF,		3027,	SCMD_EAST},
  {CLASS_WARRIOR,	3021,	SCMD_EAST},
  {CLASS_BARD,		3081,	SCMD_NORTH},

/* Northern Kore */
  {CLASS_BARD,		3715,	SCMD_EAST},

/* Brass Dragon */
  {-999 /* all */ ,	5065,	SCMD_WEST},

/* Slums of Kore */
  {CLASS_THIEF,		12509,	SCMD_NORTH},

/* City of Eli */
  {CLASS_MAGIC_USER,	25543,	SCMD_NORTH},
  {CLASS_WARRIOR,	25548,	SCMD_EAST},
  {CLASS_THIEF,		25549,	SCMD_SOUTH},
  {CLASS_CLERIC,	25550,	SCMD_WEST},
  {CLASS_BARD,		25551,	SCMD_UP},

/* this must go last -- add new guards above! */
  {-1, -1, -1}
};



/* THAC0 for classes and levels.  (To Hit Armor Class 0) */

/* [class], [level] (all) */
const int thaco[NUM_CLASSES][LVL_IMPL + 1] = {

/* MAGE */
  {100,						/* 0 */
   20, 20, 19, 19, 19, 18, 18, 17, 17, 17,	/* 10 */
   16, 16, 15, 15, 15, 14, 14, 13, 13, 13,	/* 20 */
   12, 12, 11, 11, 11, 10, 10, 9, 9, 9,		/* 30 */
   8, 8, 7, 7, 7, 6, 6, 5, 5, 5,		/* 40 */
   4, 4, 3, 3, 3, 2, 2, 1, 1, 1,		/* 50 */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},		/* 60 */

/* CLERIC */
  {100,
   20, 20, 19, 19, 18, 18, 17, 17, 16, 16,
   15, 15, 14, 14, 13, 13, 12, 12, 11, 11,
   10, 10, 9, 9, 8, 8, 7, 7, 6, 6,
   5, 5, 4, 4, 3, 3, 2, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* THIEF */
  {100,
   20, 20, 19, 19, 18, 18, 17, 17, 16, 16,
   15, 15, 14, 14, 13, 13, 12, 12, 11, 11,
   10, 10, 9, 9, 8, 8, 7, 7, 6, 6,
   5, 5, 4, 4, 3, 3, 2, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  

/* WARRIOR */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* BARD */
  {100,
   20, 20, 19, 19, 18, 18, 17, 17, 16, 16,
   15, 15, 14, 14, 13, 13, 12, 12, 11, 11,
   10, 10, 9, 9, 8, 8, 7, 7, 6, 6,
   5, 5, 4, 4, 3, 3, 2, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* DEATHKNIGHT */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* DRUID */
  {100,
   20, 20, 19, 19, 18, 18, 17, 17, 16, 16,
   15, 15, 14, 14, 13, 13, 12, 12, 11, 11,
   10, 10, 9, 9, 8, 8, 7, 7, 6, 6,
   5, 5, 4, 4, 3, 3, 2, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* VAMPIRE */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* MONK */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* LICH */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* SERAPH */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* VALKYRIE */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

/* UNUSED */
  {100,
   20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
   13, 13, 12, 11, 11, 10, 9, 9, 8, 7,
   7, 6, 5, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

};


#define CLASS_UNDEFINED         -1              /* NPCs */
#define CLASS_MAGIC_USER        0
#define CLASS_CLERIC            1
#define CLASS_THIEF             2                       
#define CLASS_WARRIOR           3 
#define CLASS_BARD              4                                         
#define CLASS_DEATHKNIGHT       5
#define CLASS_DRUID             6                                         
#define CLASS_VAMPIRE           7
#define CLASS_MONK              8 
#define CLASS_LICH              9
#define CLASS_SERAPH            10
#define CLASS_CHERUB            11
#define CLASS_VALKYRIE          12


const int newbie_gear[NUM_CLASSES][10] = {
  { 101, 102, 103, 104, 105, 106, 131, 2098, 2099, 0 },		/* MAGE */
  { 107, 108, 109, 110, 111, 112, 2098, 2099, 0, 0 },		/* CLERIC */
  { 113, 154, 115, 116, 117, 118, 132, 2098, 2099, 0 },		/* THIEF */
  { 120, 23008, 135, 133, 117, 136, 2098, 2099, 0, 0 },		/* WARRIOR */
  { 23008, 122, 123, 124, 125, 126, 134, 2098, 2099, 0 },	/* BARD */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* DKNIGHT */
  { 107, 108, 109, 110, 111, 112, 2098, 2099, 0, 0 },           /* DRUID */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },				/* UNUSED */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }				/* UNUSED */

};


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
/* HACKED to add in racial adjustments */
void roll_real_abils(struct char_data * ch)
{
  int i, j, k, temp;
  int a;
  ubyte table[6];
  ubyte rolls[4];

  extern int race_stat_adjust[NUM_RACES][MAX_RACE_STAT_ADJUST];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch)) {
  case CLASS_MAGIC_USER:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_CLERIC:
  case CLASS_DRUID:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_THIEF:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_WARRIOR:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_BARD:
    ch->real_abils.cha = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.str = table[5];
    break;
  case CLASS_DEATHKNIGHT:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_VALKYRIE:
    ch->real_abils.str = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.intel = table[2];
    ch->real_abils.cha = table[3];
    ch->real_abils.dex = table[4];
    ch->real_abils.con = table[5];
    break;
  case CLASS_VAMPIRE:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_MONK:
    ch->real_abils.str = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.cha = table[4];
    ch->real_abils.intel = table[5];
    break;
  }

  ch->real_abils.str += race_stat_adjust[(int) GET_RACE(ch)][0];
  ch->real_abils.intel += race_stat_adjust[(int) GET_RACE(ch)][1];
  ch->real_abils.wis += race_stat_adjust[(int) GET_RACE(ch)][2];
  ch->real_abils.dex += race_stat_adjust[(int) GET_RACE(ch)][3];
  ch->real_abils.con += race_stat_adjust[(int) GET_RACE(ch)][4];
  ch->real_abils.cha += race_stat_adjust[(int) GET_RACE(ch)][5];

  if (ch->real_abils.str < 3)
    ch->real_abils.str = 3;
  if (ch->real_abils.str > 25)
    ch->real_abils.str = 25;
  if (ch->real_abils.intel < 3)
    ch->real_abils.intel = 3;
  if (ch->real_abils.intel > 25)
    ch->real_abils.intel = 25;
  if (ch->real_abils.wis < 3)
    ch->real_abils.wis = 3;
  if (ch->real_abils.wis > 25)
    ch->real_abils.wis = 25;
  if (ch->real_abils.dex < 3)
    ch->real_abils.dex = 3;
  if (ch->real_abils.dex > 25)
    ch->real_abils.dex = 25;
  if (ch->real_abils.con < 3)
    ch->real_abils.con = 3;
  if (ch->real_abils.con > 25)
    ch->real_abils.con = 25;
  if (ch->real_abils.cha < 3)
    ch->real_abils.cha = 3;
  if (ch->real_abils.cha > 25)
    ch->real_abils.cha = 25;

  /* set affected_bit */
  for (a = 0; a < 4; a++)
    SET_BIT(AFF_FLAGS(ch), race_stat_adjust[(int) GET_RACE(ch)][6 + a]);

  /* set affected2_bit */
  for (a = 0; a < 4; a++)
    SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[(int) GET_RACE(ch)][10 + a]);

  ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch)
{
  void advance_level(struct char_data * ch);

  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 0;

  set_title(ch, NULL);
  roll_real_abils(ch);
  ch->points.max_hit = 10;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    break;

  case CLASS_THIEF:
    SET_SKILL(ch, SKILL_SNEAK, 30);
    SET_SKILL(ch, SKILL_HIDE, 30);
    SET_SKILL(ch, SKILL_STEAL, 30);
    SET_SKILL(ch, SKILL_BACKSTAB, 30);
    SET_SKILL(ch, SKILL_TWIST, 100);	/* twist is just a message */
    SET_SKILL(ch, SKILL_PICK_LOCK, 30);
    SET_SKILL(ch, SKILL_TRACK, 30);
    break;

  case CLASS_WARRIOR:
  case CLASS_DEATHKNIGHT:
    SET_SKILL(ch, SKILL_RESCUE, 30);
    break;

  case CLASS_BARD:
    break;

  case CLASS_VAMPIRE:
    break;

  case CLASS_MONK:
    break;
    
  }

  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
}



/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(struct char_data *ch, struct obj_data *obj) {

/* NOTE: Code for monks' weapons is in perform_wear() in act.obj.c.
 *       Code for restricting weapon slots is at the bottom of this
 *       file in class_can_wear().
 */

  if ((IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
      ((GET_OBJ_VAL(obj, 3) == (TYPE_SLASH - TYPE_HIT)) && IS_MAGIC_USER(ch)) ||
      ((GET_OBJ_VAL(obj, 3) == (TYPE_THRASH - TYPE_HIT)) && IS_MAGIC_USER(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch)) ||
      ((GET_OBJ_VAL(obj, 3) == (TYPE_PIERCE - TYPE_HIT)) && IS_CLERIC(ch)) ||
      ((GET_OBJ_VAL(obj, 3) == (TYPE_SLASH - TYPE_HIT)) && IS_CLERIC(ch)) ||
      ((GET_OBJ_VAL(obj, 3) == (TYPE_STAB - TYPE_HIT)) && IS_CLERIC(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_BARD) && IS_BARD(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && IS_VAMPIRE(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && IS_DEATHKNIGHT(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_MONK) && IS_MONK(ch)))
	return 1;

  return 0;
}



/*
 * invalid_level is used by handler.c to determine if a piece of equipment is
 * too high level for a character to use.
 * the levels of eq are: veteran (25), hero (35), and champion (42)
 *
 * also now there is newbie equipment, levels 1-15
 */
int invalid_level(struct char_data *ch, struct obj_data *obj) {
  if ((IS_OBJ_STAT(obj, ITEM_VETERAN) && (GET_LEVEL(ch) < 25)) ||
      (IS_OBJ_STAT(obj, ITEM_HERO) && (GET_LEVEL(ch) < 35)) ||
      (IS_OBJ_STAT(obj, ITEM_CHAMPION) && (GET_LEVEL(ch) < 42)))
        return 1;
  
  if (IS_OBJ_STAT(obj, ITEM_NEWBIE) && (GET_LEVEL(ch) > LVL_LOWBIE)) return 2;

  return 0;
}



/* Names of class/levels and exp required for each level */

/* NOTE: the experience at the end of the titles is unused.  It is only
 * a placeholder.  The experience tables are generated by the function
 * generate_experience_tables which overrides the values for title.exp
 */

struct title_type titles[NUM_CLASSES][LVL_IMPL + 1] = {

/* MAGIC_USER */
  {{"the Man", "the Woman", 0},
  {"the Stage Magician", "the Stage Magician", 1},
  {"the Spell Student", "the Spell Student", 2500},
  {"the Scholar of Magic", "the Scholar of Magic", 5000},
  {"the Delver in Spells", "the Delveress in Spells", 10000},
  {"the Medium of Magic", "the Medium of Magic", 20000},
  {"the Scribe of Magic", "the Scribess of Magic", 40000},
  {"the Seer", "the Seeress", 60000},
  {"the Summoner", "the Summoner", 90000},
  {"the Illusionist", "the Illusionist", 135000},
  {"the Abjurer", "the Abjuress", 250000},
  {"the Invoker", "the Invoker", 375000},
  {"the Enchanter", "the Enchantress", 750000},
  {"the Conjurer", "the Conjuress", 1125000},
  {"the Magician", "the Witch", 1500000},
  {"the Creator", "the Creator", 1875000},
  {"the Savant", "the Savant", 2250000},
  {"the Magus", "the Craftess", 2625000},
  {"the Wizard", "the Wizard", 3000000},
  {"the Warlock", "the War Witch", 3375000},
  {"the Sorcerer", "the Sorceress", 3750000},
  {"the Necromancer", "the Necromancress", 4000000},
  {"the Thaumaturge", "the Thaumaturgess", 4300000},
  {"the Student of the Occult", "the Student of the Occult", 4600000},
  {"the Disciple of the Uncanny", "the Disciple of the Uncanny", 4900000},
  {"the Minor Elemental", "the Minor Elementress", 5200000},
  {"the Greater Elemental", "the Greater Elementress", 5500000},
  {"the Master of Portals", "the Mistress of Portals", 5950000},
  {"the Shaman", "Shaman", 6400000},
  {"the Master of Illusions", "the Mistress of Illusions", 6850000},
  {"the Transmuter", "Transmuter", 7400001},
  {"the Weather Warlock", "Weather Witch", 7400002},
  {"the Lesser Golem Maker", "the Greater Golem Maker", 7400003},
  {"the Master of Air", "the Mistress of Air", 7400004},
  {"the Master of Water", "the Mistress of Water", 7400005},
  {"the Master of Earth", "the Mistress of Earth", 7400006},
  {"the Master of Ice", "the Mistress of Ice", 7400007},
  {"the Master of Fire", "the Mistress of Fire", 7400008},
  {"the Master of Demons", "the Mistress of Demons", 7400009},
  {"the Animator", "the Animator", 7400010},
  {"the Melder", "the Melder", 7400011},
  {"the Controller", "the Controller", 7400012},
  {"the Shapeshifter", "Shapeshifter", 7400013},
  {"the Walker of the Planes", "the Walker of the Planes", 7400014},
  {"the Master of the Occult", "the Mistress of the Occult", 7400015},
  {"the Keeper of Talismans", "the Keeper of Talismans", 7400016},
  {"the Archmage", "Archwitch", 7400017},
  {"the Weaver of Time", "Weaver of Time", 7400018},
  {"the Sage", "the Sage", 7400019},
  {"the Crafter of Magics", "the Craftess of Magics", 7400020},
  {"the Archmage", "the Archwitch", 7400021},
  {"the Builder of Magics", "the Builder of Magics", 8000000},
  {"the Immortal Warlock", "the Immortal Enchantress", 8000001},
  {"the Avatar of Magic", "the Avatar of Magic", 8000002},
  {"the Supreme of Magic", "the Supreme of Magic", 8000003},
  {"the Lesser God of Magic", "the Lesser Goddess of Magic", 8000004},
  {"the Demigod of Magic", "the Demigod of Magic", 8000005},
  {"the Magical Questor", "the Magical Questor", 8000006},
  {"the Deity of Magic", "the Deity of Magic", 9000000},
  {"the God of Magic", "the Goddess of Magic", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* CLERIC */
  {{"the Man", "the Woman", 0},
  {"the Altar Boy", "the Altar Girl", 1},
  {"the Meditator", "the Meditator", 1500},
  {"the Acolyte", "the Acolyte", 3000},
  {"the Novice", "the Novice", 6000},
  {"the Missionary", "the Missionary", 13000},
  {"the Adept", "the Adept", 27500},
  {"the Deacon", "the Deaconess", 55000},
  {"the Vicar", "the Vicaress", 110000},
  {"the Priest", "the Priestess", 225000},
  {"the Minister", "the Lady Minister", 450000},
  {"the Canon", "the Canon", 675000},
  {"the Enlightened", "the Enlightened", 900000},
  {"the Curate", "the Curess", 1125000},
  {"the Monk", "the Nunne", 1350000},
  {"the Healer", "the Healer", 1575000},
  {"the Chaplain", "the Chaplain", 1800000},
  {"the Expositor", "the Expositress", 2100000},
  {"the Bishop", "the Bishop", 2400000},
  {"the Protector", "the Protector", 2700000},
  {"the Learned", "the Learned", 3000000},
  {"the Skillfull", "the Skillful", 3250000},
  {"the Soother", "the Soother", 3500000},
  {"the Herbalist", "the Herbalist", 3800000},
  {"the Diviner", "the Diviner", 4100000},
  {"the Atoned", "the Atoned", 4400000},
  {"the Forgiver", "the Forgiver", 4800000},
  {"the Astral", "the Astral", 5200000},
  {"the Astrologer", "the Astrologer", 5600000},
  {"the Saint", "the Saint", 6000000},
  {"the Chaste", "the Chaste", 6400000},
  {"the Witch Hunter", "the Witch Hunter", 6400001},
  {"the Possessor", "the Possessor", 6400002},
  {"the Saver of Souls", "the Saver of Souls", 6400003},
  {"the Sacred Heart", "the Sacred Heart", 6400004},
  {"the Dispeller of Demons", "the Dispeller of Demons", 6400005},
  {"the Exorcist", "the Exorcist", 6400006},
  {"the Cleanser of Sin", "the Cleanser of Sin", 6400007},
  {"the Godlike", "the Godlike", 6400008},
  {"the Master Monk", "the Reverend Mother", 6400009},
  {"the Guardian of Chapels", "the Guardian of Chapels", 6400010},
  {"the Speaker of the Dead", "the Speaker of the Dead", 6400011},
  {"the Reader of the Heavens", "the Reader of the Heavens", 6400012},
  {"the Reanimator", "the Reanimator", 6400013},
  {"the Pope", "the Pope", 6400014},
  {"the Easer of Burdens", "the Easer of Burdens", 6400015},
  {"the Banisher of Evil", "the Banisher of Evil", 6400016},
  {"the Ressurector", "the Ressurector", 6400017},
  {"the Arch Bishop", "the Arch Lady of the Church", 6400018},
  {"the Grand Master", "the Grand Mistress", 6400019},
  {"the Patriarch", "the Matriarch", 6400020},
  {"the Builder of Temples", "the Builder of Temples", 7000000},
  {"the Immortal Cardinal", "the Immortal Priestess", 7000001},
  {"the Avatar", "the Avatar", 7000002},
  {"the Supreme Cardinal", "the Supreme Priestess", 7000003},
  {"the Lesser God of Prayer", "the Lesser Goddess of Prayer", 7000004},
  {"the Demigod of Prayer", "the Demigod of Prayer", 7000005},
  {"the Holy Questor", "the Holy Questor", 7000006},
  {"the Deity of Prayer", "the Deity or Prayer", 9000000},
  {"the God of good and evil", "the Goddess of good and evil", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* THIEF */
  {{"the Man", "the Woman", 0},
  {"the Candy Snatcher", "the Candy Snatcher", 1},
  {"the Footpad", "the Footpad", 1250},
  {"the Filcher", "the Filcheress", 2500},
  {"the Pick-Pocket", "the Pick-Pocket", 5000},
  {"the Sneak", "the Sneak", 10000},
  {"the Pincher", "the Pincheress", 20000},
  {"the Cut-Purse", "the Cut-Purse", 30000},
  {"the Snatcher", "the Snatcheress", 70000},
  {"the Sharper", "the Sharpress", 110000},
  {"the Rogue", "the Rogue", 160000},
  {"the Robber", "the Robber", 220000},
  {"the Magsman", "the Magswoman", 440000},
  {"the Highwayman", "the Highwaywoman", 660000},
  {"the Burglar", "the Burglaress", 880000},
  {"the Silent", "the Silent", 1100000},
  {"the Knifer", "the Knifer", 1500000},
  {"the Quick-Blade", "the Quick-Blade", 2000000},
  {"the Fence", "the Fence", 2500000},
  {"the Brigand", "the Brigand", 3000000},
  {"the Impersonator", "the Impersonator", 3500000},
  {"the Sly", "the Sly", 3650000},
  {"the Unseen", "the Unseen", 3800000},
  {"the Master of Smell", "the Mistress of Smell", 4100000},
  {"the Infiltrator", "the Infiltrator", 4400000},
  {"the Vandal", "the Vandal", 4700000},
  {"the Bank Robber", "the Bank Robber", 5100000},
  {"the Seeker of Crowds", "the Seeker of Crowds", 5500000},
  {"the Killer", "the Murderess", 5900000},
  {"the Dextrous", "the Dextrous", 6300000},
  {"the Cut-Throat", "the Cut-Throat", 6650000},
  {"the Escapist", "the Escapist", 6650001},
  {"the Vaporous Stalker", "the Vaporous Stalker", 6650002},
  {"the Lethal Assassin", "the Lethal Assassin", 6650003},
  {"the Stalker of Shadows", "the Stalker of Shadows", 6650004},
  {"the Underling", "the Underling", 6650005},
  {"the Hired Killer", "the Hired Killer", 6650006},
  {"the Masked", "the Masked", 6650007},
  {"the Silencer", "the Silencer", 6650008},
  {"the Unknown Force", "the Unknown Force", 6650009},
  {"the Master of Locks", "the Mistress of Locks", 6650010},
  {"the Master of Escape", "the Mistress of Escape", 6650011},
  {"the Master of Knives", "the Mistress of Knives", 6650012},
  {"the Master of Disguise", "the Mistress of Disguise", 6650013},
  {"the Master of Death", "the Mistress of Death", 6650014},
  {"the Bitter End", "the Bitter End", 6650015},
  {"the Filler of Coffins", "the Filler of Coffins", 6650016},
  {"the Death Shroud", "the Death Shroud", 6650017},
  {"the Master of Theft", "the Master of Theft", 6650018},
  {"the Grand Master Assassin", "the Grand Mistress Assassin", 6650019},
  {"the Grand Master Thief", "the Grand Mistress Thief", 6650020},
  {"the Unseen Builder", "the Unseen Builder", 7000000},
  {"the Immortal Assasin", "the Immortal Assasin", 7000001},
  {"the Deadly Avatar", "the Deadly Avatar", 7000002},
  {"the Supreme Assasin", "the Supreme Assasin", 7000003},
  {"the Lesser God of Thieves", "the Lesser Goddess of Thieves", 7000004},
  {"the Demigod of Thieves", "the Demigod of Thieves", 7000005},
  {"the Lethal Questor", "the Lethal Questor", 7000006},
  {"the Deity of thieves", "the Deity of thieves", 9000000},
  {"the God of Thieves", "the Goddess of Thieves", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* WARRIOR */
  {{"the Man", "the Woman", 0},
  {"the Swordpupil", "the Swordpupil", 1},
  {"the Recruit", "the Swordpupil", 2000},
  {"the Sentry", "the Sentress", 4000},
  {"the Fighter", "the Fighter", 8000},
  {"the Soldier", "the Soldier", 16000},
  {"the Warrior", "the Warrior", 32000},
  {"the Veteran", "the Veteran", 64000},
  {"the Swordsman", "the Swordswoman", 125000},
  {"the Fencer", "the Fenceress", 250000},
  {"the Combatant", "the Combatess", 500000},
  {"the Hero", "the Heroine", 750000},
  {"the Myrmidon", "the Myrmidon", 1000000},
  {"the Swashbuckler", "the Swashbuckleress", 1250000},
  {"the Mercenary", "the Mercenaress", 1500000},
  {"the Swordmaster", "the Swordmistress", 1850000},
  {"the Lieutenant", "the Lieutenant", 2200000},
  {"the Champion", "the Lady Champion", 2550000},
  {"the Dragoon", "the Lady Dragoon", 2900000},
  {"the Cavalier", "the Cavalier", 3250000},
  {"the Knight Hopeful", "the Knight Hopeful", 3600000},
  {"the Warlord", "the Warlord", 3900000},
  {"the Extirpator",  "the Extirpator", 4200000},
  {"the Scourge", "the Scourge", 4500000},
  {"the Master Fighter", "the Master Fighter", 4800000},
  {"the Swordmaster in Training", "the Swordmaster in Training", 5150000},
  {"the Swordmaster", "the Swordmaster", 5500000},
  {"the Grand Swordmaster", "the Grand Swordmaster", 5950000},
  {"the Lesser Knight", "the Lesser Lady Knight", 6400000},
  {"the Knight", "the Lady Knight", 6850000},
  {"the Grand Knight", "the Grand Lady Knight", 7400000},
  {"the Weaponsmaster in Training", "the Weaponsmistress in Training", 7400001},
  {"the Weaponsmaster", "the Weaponsmistress", 7400002},
  {"the Berserker", "the Berserker", 7400003},
  {"the Oppressor", "the Oppressor", 7400004},
  {"the Duke", "the Duchess", 7400005},
  {"the Lord", "the Lady", 7400006},
  {"the Finger of Death", "the Finger of Death", 7400007},
  {"the Hand of Death", "the Hand of Death", 7400008},
  {"the Duke of the Sheath", "the Duchess of the Sheath", 7400009},
  {"the Duke of the Blade", "the Duchess of the Blade", 7400010},
  {"the Duke of Fire", "the Duchess of Fire", 7400011},
  {"the Duke of the Nexus", "the Duchess of the Nexus", 7400012},
  {"the Lord of the Sheath", "the Lady of the Sheath", 7400013},
  {"the Lord of the Blade", "the Lady of the Blade", 7400014},
  {"the Lord of Fire", "the Lady of Fire", 7400015},
  {"the Lord of the Nexus", "the Lady of the Nexus", 7400016},
  {"the Warrior at the Oak", "the Warrior at the Oak", 7400017},
  {"the Warrior at the Bridge", "the Warrior at the Bridge", 7400018},
  {"the Warrior at the Gate", "the Warrior at the Gate", 7400019},
  {"the Keeper of the Keys", "the Keeper of the Keys", 7400020},
  {"the Warlord Builder", "the Warlord Builder", 8000000},
  {"the Immortal Warlord", "the Immortal Lady of War", 8000001},
  {"the Avatar Warrior", "the Avatar Warrior", 8000002},
  {"the Supreme Warlord", "the Supreme Warrior", 8000003},
  {"the Lesser God of War", "the Lesser Goddess of War", 8000004},
  {"the Demigod of War", "the Demigod of War", 8000005},
  {"the Warrior Questor", "the Warrior Questor", 8000006},
  {"the Deity of Destruction", "the Deity of Destruction", 9000000},
  {"the God of war", "the Goddess of war", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* BARD */
  {{"the Man", "the Woman", 0},
  {"the Rhymer", "the Lady Rhymer", 1},
  {"the Blank Versist", "the Lady of Blank Verse", 2000},
  {"the Lyrist", "the Lady Lyrist", 4000},
  {"the Singer", "the Singer", 8000},
  {"the Sonnateer", "the Sonnatess", 16000},
  {"the Performer", "the Performer", 32000},
  {"the Skald", "the Lady Skald", 64000},
  {"the Declaimer", "the Lady Declaimer", 125000},
  {"the Racaraide", "the Lady Racaraide", 250000},
  {"the Melodian", "the Melodious Lady", 500000},
  {"the Joungleur", "the Lady Joungleur", 750000},
  {"the Versache", "the Versache", 1000000},
  {"the Troubador", "the Lady Troubador", 1250000},
  {"the Composer", "the Lady Composer", 1500000},
  {"the Minstrel", "the Lady Minstrel", 1850000},
  {"the Poet", "the Poetess", 2200000},
  {"the Muse", "the Beatific Muse", 2550000},
  {"the Musician", "the Musician", 2900000},
  {"the Lorist", "the Wondrous Lorist", 3250000},
  {"the Epicist", "the Uncanny Epicist", 3600000},
  {"the Bard", "the Bard", 3900000},
  {"the Master Bard (22)", "the Lady Bard (22)", 4200000},
  {"the Master Bard (23)", "the Lady Bard (23)", 4500000},
  {"the Master Bard (24)", "the Lady Bard (24)", 4800000},
  {"the Master Bard (25)", "the Lady Bard (25)", 5150000},
  {"the Master Bard (26)", "the Lady Bard (26)", 5500000},
  {"the Master Bard (27)", "the Lady Bard (27)", 5950000},
  {"the Master Bard (28)", "the Lady Bard (28)", 6400000},
  {"the Master Bard (29)", "the Lady Bard (29)", 6850000},
  {"the Master Bard (30)", "the Lady Bard (30)", 7400000},
  {"the Master Bard (31)", "the Lady Bard (31)", 7400001},
  {"the Master Bard (32)", "the Lady Bard (32)", 7400002},
  {"the Master Bard (33)", "the Lady Bard (33)", 7400003},
  {"the Master Bard (34)", "the Lady Bard (34)", 7400004},
  {"the Master Bard (35)", "the Lady Bard (35)", 7400005},
  {"the Master Bard (36)", "the Lady Bard (36)", 7400006},
  {"the Master Bard (37)", "the Lady Bard (37)", 7400007},
  {"the Master Bard (38)", "the Lady Bard (38)", 7400008},
  {"the Master Bard (39)", "the Lady Bard (39)", 7400009},
  {"the Master Bard (40)", "the Lady Bard (40)", 7400010},
  {"the Master Bard (41)", "the Lady Bard (41)", 7400011},
  {"the Master Bard (42)", "the Lady Bard (42)", 7400012},
  {"the Master Bard (43)", "the Lady Bard (43)", 7400013},
  {"the Master Bard (44)", "the Lady Bard (44)", 7400014},
  {"the Master Bard (45)", "the Lady Bard (45)", 7400015},
  {"the Master Bard (46)", "the Lady Bard (46)", 7400016},
  {"the Master Bard (47)", "the Lady Bard (47)", 7400017},
  {"the Master Bard (48)", "the Lady Bard (48)", 7400018},
  {"the Master Bard (49)", "the Lady Bard (49)", 7400019},
  {"the Master Bard (50)", "the Lady Bard (50)", 7400020},
  {"the Builder of Lyrics", "the Builder of Lyrics", 8000000},
  {"the Immortal Bard", "the Immortal Lady Bard", 8000001},
  {"the Avatar Bard", "the Avatar Bard", 8000002},
  {"the Supreme Bard", "the Supreme Bard", 8000003},
  {"the Lesser God of Bards", "the Lesser Godess of Bards", 8000004},
  {"the Demigod of Bards", "the Demigod of Bards", 8000005},
  {"the Singing Questor", "the Singing Questor", 8000006},
  {"the Deity of Poetry", "the Deity of Poetry", 9000000},
  {"the God of Song", "the Goddess of Song", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* DEATHKNIGHT */
  {{"the Incorruptable", "the Incorruptable", 0},
  {"the Humble", "the Humble", 1},
  {"the White Rose", "the White Rose", 2000},
  {"the Faithful", "the Faithful", 4000},
  {"the Worthy", "the Worthy", 8000},
  {"the Oathtaker", "the Oathtaker", 16000},
  {"the Hawk", "the Sparrow", 32000},
  {"the Truthteller", "the Truthteller", 64000},
  {"the Stout", "the Stout", 125000},
  {"the Risen", "the Risen", 250000},
  {"the Hero", "the Hero", 500000},
  {"the Sword", "the Sworn", 750000},
  {"the Sword of Light", "the Eyes of Light", 1000000},
  {"the Life Sword", "the Eyes of Life", 1250000},
  {"the Newly Born", "the Newly Born", 1500000},
  {"the Yellow Rose", "the Yellow Rose", 1850000},
  {"the Champion", "the Companion", 2200000},
  {"the White Knight", "the White Lady", 2550000},
  {"the Rider in White", "the Rider in White", 2900000},
  {"the Protector of the Innocent", "the Protector of the Innocent", 3250000},
  {"the Knight", "the Lady Knight", 3600000},
  {"the Knight (21)", "the Lady Knight (21)", 3900000},
  {"the Knight (22)", "the Lady Knight (22)", 4200000},
  {"the Knight (23)", "the Lady Knight (23)", 4500000},
  {"the Knight (24)", "the Lady Knight (24)", 4800000},
  {"the Knight (25)", "the Lady Knight (25)", 5150000},
  {"the Knight (26)", "the Lady Knight (26)", 5500000},
  {"the Knight (27)", "the Lady Knight (27)", 5950000},
  {"the Knight (28)", "the Lady Knight (28)", 6400000},
  {"the Knight (29)", "the Lady Knight (29)", 6850000},
  {"the Knight (30)", "the Lady Knight (30)", 7400000},
  {"the Knight (31)", "the Lady Knight (31)", 7400001},
  {"the Knight (32)", "the Lady Knight (32)", 7400002},
  {"the Knight (33)", "the Lady Knight (33)", 7400003},
  {"the Knight (34)", "the Lady Knight (34)", 7400004},
  {"the Knight (35)", "the Lady Knight (35)", 7400005},
  {"the Knight (36)", "the Lady Knight (36)", 7400006},
  {"the Knight (37)", "the Lady Knight (37)", 7400007},
  {"the Knight (38)", "the Lady Knight (38)", 7400008},
  {"the Knight (39)", "the Lady Knight (39)", 7400009},
  {"the Knight (40)", "the Lady Knight (40)", 7400010},
  {"the Knight (41)", "the Lady Knight (41)", 7400011},
  {"the Knight (42)", "the Lady Knight (42)", 7400012},
  {"the Knight (43)", "the Lady Knight (43)", 7400013},
  {"the Knight (44)", "the Lady Knight (44)", 7400014},
  {"the Knight (45)", "the Lady Knight (45)", 7400015},
  {"the Knight (46)", "the Lady Knight (46)", 7400016},
  {"the Knight (47)", "the Lady Knight (47)", 7400017},
  {"the Knight (48)", "the Lady Knight (48)", 7400018},
  {"the Knight (49)", "the Lady Knight (49)", 7400019},
  {"the Knight (50)", "the Lady Knight (50)", 7400020},
  {"the Avenger (51)", "the Avenger (51)", 8000000},
  {"the Avenger (52)", "the Avenger (52)", 8000001},
  {"the Avenger (53)", "the Avenger (53)", 8000002},
  {"the Avenger (54)", "the Avenger (54)", 8000003},
  {"the Avenger (55)", "the Avenger (55)", 8000004},
  {"the Avenger (56)", "the Avenger (56)", 8000005},
  {"the Avenger (57)", "the Avenger (57)", 8000006},
  {"the Avenger (58)", "the Avenger (58)", 9000000},
  {"the Avenger (59)", "the Avenger (59)", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* DRUID */
  {{"the man", "the woman", 0},
  {"the Altar Boy", "the Altar Girl", 1},
  {"the Meditator", "the Meditator", 1500},
  {"the Acolyte", "the Acolyte", 3000},
  {"the Novice", "the Novice", 6000},
  {"the Missionary", "the Missionary", 13000},
  {"the Adept", "the Adept", 27500},
  {"the Deacon", "the Deaconess", 55000},
  {"the Vicar", "the Vicaress", 110000},
  {"the Priest", "the Priestess", 225000},
  {"the Minister", "the Lady Minister", 450000},
  {"the Canon", "the Canon", 675000},
  {"the Enlightened", "the Enlightened", 900000},
  {"the Curate", "the Curess", 1125000},
  {"the Monk", "the Nunne", 1350000},
  {"the Healer", "the Healer", 1575000},
  {"the Chaplain", "the Chaplain", 1800000},
  {"the Expositor", "the Expositress", 2100000},
  {"the Bishop", "the Bishop", 2400000},
  {"the Protector", "the Protector", 2700000},
  {"the Learned", "the Learned", 3000000},
  {"the Skillfull", "the Skillful", 3250000},
  {"the Soother", "the Soother", 3500000},
  {"the Herbalist", "the Herbalist", 3800000},
  {"the Diviner", "the Diviner", 4100000},
  {"the Atoned", "the Atoned", 4400000},
  {"the Forgiver", "the Forgiver", 4800000},
  {"the Astral", "the Astral", 5200000},
  {"the Astrologer", "the Astrologer", 5600000},
  {"the Saint", "the Saint", 6000000},
  {"the Chaste", "the Chaste", 6400000},
  {"the Witch Hunter", "the Witch Hunter", 6400001},
  {"the Possessor", "the Possessor", 6400002},
  {"the Saver of Souls", "the Saver of Souls", 6400003},
  {"the Sacred Heart", "the Sacred Heart", 6400004},
  {"the Dispeller of Demons", "the Dispeller of Demons", 6400005},
  {"the Exorcist", "the Exorcist", 6400006},
  {"the Cleanser of Sin", "the Cleanser of Sin", 6400007},
  {"the Godlike", "the Godlike", 6400008},
  {"the Master Monk", "the Reverend Mother", 6400009},
  {"the Guardian of Chapels", "the Guardian of Chapels", 6400010},
  {"the Speaker of the Dead", "the Speaker of the Dead", 6400011},
  {"the Reader of the Heavens", "the Reader of the Heavens", 6400012},
  {"the Reanimator", "the Reanimator", 6400013},
  {"the Pope", "the Pope", 6400014},
  {"the Easer of Burdens", "the Easer of Burdens", 6400015},
  {"the Banisher of Evil", "the Banisher of Evil", 6400016},
  {"the Ressurector", "the Ressurector", 6400017},
  {"the Arch Bishop", "the Arch Lady of the Church", 6400018},
  {"the Grand Master", "the Grand Mistress", 6400019},
  {"the Patriarch", "the Matriarch", 6400020},
  {"the Builder of Temples", "the Builder of Temples", 7000000},
  {"the Immortal Cardinal", "the Immortal Priestess", 7000001},
  {"the Avatar", "the Avatar", 7000002},
  {"the Supreme Cardinal", "the Supreme Priestess", 7000003},
  {"the Lesser God of Prayer", "the Lesser Goddess of Prayer", 7000004},
  {"the Demigod of Prayer", "the Demigod of Prayer", 7000005},
  {"the Holy Questor", "the Holy Questor", 7000006},
  {"the Deity of Prayer", "the Deity or Prayer", 9000000},
  {"the God of good and evil", "the Goddess of good and evil", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* VAMPIRE */
  {{"the Drained", "the Drained", 0},
  {"the Thirsty Corpse", "the Thirsty Corpse", 1},
  {"the Leech", "the Leech", 2000},
  {"the Tick", "the Tick", 4000},
  {"the Blood Slave", "the Blood Slave", 8000},
  {"the Blood Drinker", "the Blood Drinker", 16000},
  {"the Life Sucker", "the Life Sucker", 32000},
  {"the Seducer", "the Sultress", 64000},
  {"the Manipulator", "the Manipulator", 125000},
  {"the Corruptor", "the Corruptor", 250000},
  {"the Ruler", "the Ruler", 500000},
  {"the Dominator", "the Dominator", 750000},
  {"the Drainer", "the Drainer", 1000000},
  {"the Impaler", "the Impaler", 1250000},
  {"the Damned", "the Damned", 1500000},
  {"the Imp", "the Imp", 1850000},
  {"the Incubus", "the Succubus", 2200000},
  {"the Damned", "the Damned", 2550000},
  {"the Master", "the Master", 2900000},
  {"the Blood Lord", "the Blood Lady", 3250000},
  {"the Drinker of Babes", "the Drinker of Babes", 3600000},
  {"the Coffin Dweller", "the Coffin Dweller", 3900000},
  {"the Vampire (22)", "the Vampiress (22)", 4200000},
  {"the Vampire (23)", "the Vampiress (23)", 4500000},
  {"the Vampire (24)", "the Vampiress (24)", 4800000},
  {"the Vampire (25)", "the Vampiress (25)", 5150000},
  {"the Vampire (26)", "the Vampiress (26)", 5500000},
  {"the Vampire (27)", "the Vampiress (27)", 5950000},
  {"the Vampire (28)", "the Vampiress (28)", 6400000},
  {"the Vampire (29)", "the Vampiress (29)", 6850000},
  {"the Vampire (30)", "the Vampiress (30)", 7400000},
  {"the Vampire (31)", "the Vampiress (31)", 7400001},
  {"the Vampire (32)", "the Vampiress (32)", 7400002},
  {"the Vampire (33)", "the Vampiress (33)", 7400003},
  {"the Vampire (34)", "the Vampiress (34)", 7400004},
  {"the Vampire (35)", "the Vampiress (35)", 7400005},
  {"the Vampire (36)", "the Vampiress (36)", 7400006},
  {"the Vampire (37)", "the Vampiress (37)", 7400007},
  {"the Vampire (38)", "the Vampiress (38)", 7400008},
  {"the Vampire (39)", "the Vampiress (39)", 7400009},
  {"the Vampire (40)", "the Vampiress (40)", 7400010},
  {"the Vampire (41)", "the Vampiress (41)", 7400011},
  {"the Vampire (42)", "the Vampiress (42)", 7400012},
  {"the Vampire (43)", "the Vampiress (43)", 7400013},
  {"the Vampire (44)", "the Vampiress (44)", 7400014},
  {"the Vampire (45)", "the Vampiress (45)", 7400015},
  {"the Taste of Vitae (46)", "the Taste of Vitae (46)", 7400016},
  {"the Deadly Temptor (47)", "the Deadly Temptress (47)", 7400017},
  {"the Mocker of God (48)", "the Mocker of God (48)", 7400018},
  {"the Kiss of Death (49)", "the Kiss of Death (49)", 7400019},
  {"the Vampire (50)", "the Vampiress (50)", 7400020},
  {"the Immortal Bleeder (51)", "the Immortal Bleeder (51)", 8000000},
  {"the Vampire Prince (52)", "the Coven Vampiress (52)", 8000001},
  {"the Vampire Prince (53)", "the Coven Vampiress (53)", 8000002},
  {"the Vampire Prince (54)", "the Coven Vampiress (54)", 8000003},
  {"the Vampire Prince (55)", "the Coven Vampiress (55)", 8000004},
  {"the Vampire Prince (56)", "the Coven Vampiress (56)", 8000005},
  {"the Vampire Prince (57)", "the Coven Vampiress (57)", 8000006},
  {"the Vampire King", "the Queen of Vampires", 9000000},
  {"the First Son of Cain", "the First Daughter of Cain", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },

/* UNUSED7 */
  {{"is just a Black Egg", "is just a Black Egg", 0},
  {"the Black Hatchling", "the Black Hatchling", 1},
  {"the Black Worm", "the Black Worm", 2000},
  {"the Black Wyrm", "the Black Wyrm", 4000},
  {"the Caustic Black Wyrm", "the Caustic Black Wyrm", 8000},
  {"the Young Black Drake", "the Young Black Drake", 16000},
  {"the Young Black Dragon", "the Young Black Dragon", 32000},
  {"the Young Black Worm", "the Young Black Worm", 64000},
  {"the Young Black Wyrm", "the Young Black Wyrm", 125000},
  {"the Adult Black Drake", "the Adult Black Drake", 250000},
  {"the Adult Black Dragon", "the Adult Black Dragon", 500000},
  {"the Adult Black Worm", "the Adult Black Worm", 750000},
  {"the Adult Black Wyrm", "the Adult Black Wyrm", 1000000},
  {"the Mature Black Drake", "the Mature Black Drake", 1250000},
  {"the Mature Black Dragon", "the Mature Black Dragon", 1500000},
  {"the Mature Black Worm", "the Mature Black Worm", 1850000},
  {"the Mature Black Wyrm", "the Mature Black Wyrm", 2200000},
  {"the Ancient Black Drake", "the Ancient Black Drake", 2550000},
  {"the Ancient Black Dragon", "the Ancient Black Dragon", 2900000},
  {"the Ancient Black Worm", "the Ancient Black Worm", 3250000},
  {"the Ancient Black Wyrm", "the Ancient Black Wyrm", 3600000},
  {"the Great Black Drake (21)", "the Great Black Drake (21)", 3900000},
  {"the Great Black Dragon (22)", "the Great Black Dragon (22)", 4200000},
  {"the Great Black Worm (23)", "the Great Black Worm (23)", 4500000},
  {"the Great Black Wyrm (24)", "the Great Black Wyrm (24)", 4800000},
  {"the Caustic Spitter (25)", "the Caustic Spitter (25)", 5150000},
  {"the Alkali (26)", "the Alkali (26)", 5500000},
  {"the Black Spitter (27)", "the Black Spitter (27)", 5950000},
  {"the Caustic Worm (28)", "the Caustic Worm (28)", 6400000},
  {"the Fen Wyrm (29)", "the Fen Wyrm (29)", 6850000},
  {"the Black (30)", "the Black (30)", 7400000},
  {"the Black (31)", "the Black (31)", 7400001},
  {"the Black (32)", "the Black (32)", 7400002},
  {"the Black (33)", "the Black (33)", 7400003},
  {"the Black (34)", "the Black (34)", 7400004},
  {"the Black (35)", "the Black (35)", 7400005},
  {"the Black (36)", "the Black (36)", 7400006},
  {"the Black (37)", "the Black (37)", 7400007},
  {"the Black (38)", "the Black (38)", 7400008},
  {"the Black (39)", "the Black (39)", 7400009},
  {"the Black (40)", "the Black (40)", 7400010},
  {"the Black (41)", "the Black (41)", 7400011},
  {"the Black (42)", "the Black (42)", 7400012},
  {"the Black (43)", "the Black (43)", 7400013},
  {"the Black (44)", "the Black (44)", 7400014},
  {"the Black (45)", "the Black (45)", 7400015},
  {"the Black (46)", "the Black (46)", 7400016},
  {"the Black (47)", "the Black (47)", 7400017},
  {"the Black (48)", "the Black (48)", 7400018},
  {"the Black (49)", "the Black (49)", 7400019},
  {"the Black (50)", "the Black (50)", 7400020},
  {"the King of Black Dragons (51)", "the Queen of Black Dragons (51)", 8000000},
  {"the King of Black Dragons (52)", "the Queen of Black Dragons (52)", 8000001},
  {"the King of Black Dragons (53)", "the Queen of Black Dragons (53)", 8000002},
  {"the King of Black Dragons (54)", "the Queen of Black Dragons (54)", 8000003},
  {"the King of Black Dragons (55)", "the Queen of Black Dragons (55)", 8000004},
  {"the King of Black Dragons (56)", "the Queen of Black Dragons (56)", 8000005},
  {"the King of Black Dragons (57)", "the Queen of Black Dragons (57)", 8000006},
  {"the King of All Dragons", "the Queen of All Dragons", 9000000},
  {"the God of Dragons", "the Goddess of Dragons", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* DRAGON */
  {{"is just a Egg", "is just a Egg", 0},
  {"the Hatchling", "the Hatchling", 1},
  {"the Little Worm", "the Little Worm", 2000},
  {"the Little Wyrm", "the Little Wyrm", 4000},
  {"the Tiny Wyrm", "the Tiny Wyrm", 8000},
  {"the Young Drake", "the Young Drake", 16000},
  {"the Young Dragon", "the Young Dragon", 32000},
  {"the Young Worm", "the Young Worm", 64000},
  {"the Young Wyrm", "the Young Wyrm", 125000},
  {"the Adult Drake", "the Adult Drake", 250000},
  {"the Adult Dragon", "the Adult Dragon", 500000},
  {"the Adult Worm", "the Adult Worm", 750000},
  {"the Adult Wyrm", "the Adult Wyrm", 1000000},
  {"the Mature Drake", "the Mature Drake", 1250000},
  {"the Mature Dragon", "the Mature Dragon", 1500000},
  {"the Mature Worm", "the Mature Worm", 1850000},
  {"the Mature Wyrm", "the Mature Wyrm", 2200000},
  {"the Ancient Drake", "the Ancient Drake", 2550000},
  {"the Ancient Dragon", "the Ancient Dragon", 2900000},
  {"the Ancient Worm", "the Ancient Worm", 3250000},
  {"the Ancient Wyrm", "the Ancient Wyrm", 3600000},
  {"the Great Drake (21)", "the Great Drake (21)", 3900000},
  {"the Great Dragon (22)", "the Great Dragon (22)", 4200000},
  {"the Great Worm (23)", "the Great Worm (23)", 4500000},
  {"the Great Wyrm (24)", "the Great Wyrm (24)", 4800000},
  {"the Flash of Lightning (25)", "the Flash of Lightning (25)", 5150000},
  {"the Desert Storm (26)", "the Desert Storm (26)", 5500000},
  {"the Thunderer (27)", "the Thunderess (27)", 5950000},
  {"the Lighting Worm (28)", "the Lightning Worm (28)", 6400000},
  {"the Blitz Wyrm (29)", "the Blitz Wyrm (29)", 6850000},
  {"the Dragon (30)", "the Dragon (30)", 7400000},
  {"the Dragon (31)", "the Dragon (31)", 7400001},
  {"the Dragon (32)", "the Dragon (32)", 7400002},
  {"the Dragon (33)", "the Dragon (33)", 7400003},
  {"the Dragon (34)", "the Dragon (34)", 7400004},
  {"the Dragon (35)", "the Dragon (35)", 7400005},
  {"the Dragon (36)", "the Dragon (36)", 7400006},
  {"the Dragon (37)", "the Dragon (37)", 7400007},
  {"the Dragon (38)", "the Dragon (38)", 7400008},
  {"the Dragon (39)", "the Dragon (39)", 7400009},
  {"the Dragon (40)", "the Dragon (40)", 7400010},
  {"the Dragon (41)", "the Dragon (41)", 7400011},
  {"the Dragon (42)", "the Dragon (42)", 7400012},
  {"the Dragon (43)", "the Dragon (43)", 7400013},
  {"the Dragon (44)", "the Dragon (44)", 7400014},
  {"the Dragon (45)", "the Dragon (45)", 7400015},
  {"the Dragon (46)", "the Dragon (46)", 7400016},
  {"the Dragon (47)", "the Dragon (47)", 7400017},
  {"the Dragon (48)", "the Dragon (48)", 7400018},
  {"the Dragon (49)", "the Dragon (49)", 7400019},
  {"the Dragon (50)", "the Dragon (50)", 7400020},
  {"the King of Dragons (51)", "the Queen of Dragons (51)", 8000000},
  {"the King of Dragons (52)", "the Queen of Dragons (52)", 8000001},
  {"the King of Dragons (53)", "the Queen of Dragons (53)", 8000002},
  {"the King of Dragons (54)", "the Queen of Dragons (54)", 8000003},
  {"the King of Dragons (55)", "the Queen of Dragons (55)", 8000004},
  {"the King of Dragons (56)", "the Queen of Dragons (56)", 8000005},
  {"the King of Dragons (57)", "the Queen of Dragons (57)", 8000006},
  {"the King of All Dragons", "the Queen of All Dragons", 9000000},
  {"the God of Dragons", "the Goddess of Dragons", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* UNUSED8 */
  {{"is just a Red Egg", "is just a Red Egg", 0},
  {"the Red Hatchling", "the Red Hatchling", 1},
  {"the Red Worm", "the Red Worm", 2000},
  {"the Red Wyrm", "the Red Wyrm", 4000},
  {"the Fiery Red Wyrm", "the Fiery Red Wyrm", 8000},
  {"the Young Red Drake", "the Young Red Drake", 16000},
  {"the Young Red Dragon", "the Young Red Dragon", 32000},
  {"the Young Red Worm", "the Young Red Worm", 64000},
  {"the Young Red Wyrm", "the Young Red Wyrm", 125000},
  {"the Adult Red Drake", "the Adult Red Drake", 250000},
  {"the Adult Red Dragon", "the Adult Red Dragon", 500000},
  {"the Adult Red Worm", "the Adult Red Worm", 750000},
  {"the Adult Red Wyrm", "the Adult Red Wyrm", 1000000},
  {"the Mature Red Drake", "the Mature Red Drake", 1250000},
  {"the Mature Red Dragon", "the Mature Red Dragon", 1500000},
  {"the Mature Red Worm", "the Mature Red Worm", 1850000},
  {"the Mature Red Wyrm", "the Mature Red Wyrm", 2200000},
  {"the Ancient Red Drake", "the Ancient Red Drake", 2550000},
  {"the Ancient Red Dragon", "the Ancient Red Dragon", 2900000},
  {"the Ancient Red Worm", "the Ancient Red Worm", 3250000},
  {"the Ancient Red Wyrm", "the Ancient Red Wyrm", 3600000},
  {"the Great Red Drake (21)", "the Great Red Drake (21)", 3900000},
  {"the Great Red Dragon (22)", "the Great Red Dragon (22)", 4200000},
  {"the Great Red Worm (23)", "the Great Red Worm (23)", 4500000},
  {"the Great Red Wyrm (24)", "the Great Red Wyrm (24)", 4800000},
  {"the Blast of Fire (25)", "the Blast of Fire (25)", 5150000},
  {"the Burning Flame (26)", "the Burning Flame (26)", 5500000},
  {"the Firebrand (27)", "the Firebrand (27)", 5950000},
  {"the Fire Worm (28)", "the Fire Worm (28)", 6400000},
  {"the Flame Wyrm (29)", "the Flame Wyrm (29)", 6850000},
  {"the Red (30)", "the Red (30)", 7400000},
  {"the Red (31)", "the Red (31)", 7400001},
  {"the Red (32)", "the Red (32)", 7400002},
  {"the Red (33)", "the Red (33)", 7400003},
  {"the Red (34)", "the Red (34)", 7400004},
  {"the Red (35)", "the Red (35)", 7400005},
  {"the Red (36)", "the Red (36)", 7400006},
  {"the Red (37)", "the Red (37)", 7400007},
  {"the Red (38)", "the Red (38)", 7400008},
  {"the Red (39)", "the Red (39)", 7400009},
  {"the Red (40)", "the Red (40)", 7400010},
  {"the Red (41)", "the Red (41)", 7400011},
  {"the Red (42)", "the Red (42)", 7400012},
  {"the Red (43)", "the Red (43)", 7400013},
  {"the Red (44)", "the Red (44)", 7400014},
  {"the Red (45)", "the Red (45)", 7400015},
  {"the Red (46)", "the Red (46)", 7400016},
  {"the Red (47)", "the Red (47)", 7400017},
  {"the Red (48)", "the Red (48)", 7400018},
  {"the Red (49)", "the Red (49)", 7400019},
  {"the Red (50)", "the Red (50)", 7400020},
  {"the King of Red Dragons (51)", "the Queen of Red Dragons (51)", 8000000},
  {"the King of Red Dragons (52)", "the Queen of Red Dragons (52)", 8000001},
  {"the King of Red Dragons (53)", "the Queen of Red Dragons (53)", 8000002},
  {"the King of Red Dragons (54)", "the Queen of Red Dragons (54)", 8000003},
  {"the King of Red Dragons (55)", "the Queen of Red Dragons (55)", 8000004},
  {"the King of Red Dragons (56)", "the Queen of Red Dragons (56)", 8000005},
  {"the King of Red Dragons (57)", "the Queen of Red Dragons (57)", 8000006},
  {"the King of All Dragons", "the Queen of All Dragons", 9000000},
  {"the God of Dragons", "the Goddess of Dragons", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* UNUSED9 */
  {{"is just a Green Egg", "is just a Green Egg", 0},
  {"the Green Hatchling", "the Green Hatchling", 1},
  {"the Green Worm", "the Green Worm", 2000},
  {"the Green Wyrm", "the Green Wyrm", 4000},
  {"the Choking Green Wyrm", "the Choking Green Wyrm", 8000},
  {"the Young Green Drake", "the Young Green Drake", 16000},
  {"the Young Green Dragon", "the Young Green Dragon", 32000},
  {"the Young Green Worm", "the Young Green Worm", 64000},
  {"the Young Green Wyrm", "the Young Green Wyrm", 125000},
  {"the Adult Green Drake", "the Adult Green Drake", 250000},
  {"the Adult Green Dragon", "the Adult Green Dragon", 500000},
  {"the Adult Green Worm", "the Adult Green Worm", 750000},
  {"the Adult Green Wyrm", "the Adult Green Wyrm", 1000000},
  {"the Mature Green Drake", "the Mature Green Drake", 1250000},
  {"the Mature Green Dragon", "the Mature Green Dragon", 1500000},
  {"the Mature Green Worm", "the Mature Green Worm", 1850000},
  {"the Mature Green Wyrm", "the Mature Green Wyrm", 2200000},
  {"the Ancient Green Drake", "the Ancient Green Drake", 2550000},
  {"the Ancient Green Dragon", "the Ancient Green Dragon", 2900000},
  {"the Ancient Green Worm", "the Ancient Green Worm", 3250000},
  {"the Ancient Green Wyrm", "the Ancient Green Wyrm", 3600000},
  {"the Great Green Drake (21)", "the Great Green Drake (21)", 3900000},
  {"the Great Green Dragon (22)", "the Great Green Dragon (22)", 4200000},
  {"the Great Green Worm (23)", "the Great Green Worm (23)", 4500000},
  {"the Great Green Wyrm (24)", "the Great Green Wyrm (24)", 4800000},
  {"the Stifling Green Cloud (25)", "the Stifling Green Cloud (25)", 5150000},
  {"the Green Flame (26)", "the Green Flame (26)", 5500000},
  {"the Unbreathable (27)", "the Unbreathable (27)", 5950000},
  {"the Noxious Worm (28)", "the Noxious Worm (28)", 6400000},
  {"the Poison Wyrm (29)", "the Poison Wyrm (29)", 6850000},
  {"the Green (30)", "the Green (30)", 7400000},
  {"the Green (31)", "the Green (31)", 7400001},
  {"the Green (32)", "the Green (32)", 7400002},
  {"the Green (33)", "the Green (33)", 7400003},
  {"the Green (34)", "the Green (34)", 7400004},
  {"the Green (35)", "the Green (35)", 7400005},
  {"the Green (36)", "the Green (36)", 7400006},
  {"the Green (37)", "the Green (37)", 7400007},
  {"the Green (38)", "the Green (38)", 7400008},
  {"the Green (39)", "the Green (39)", 7400009},
  {"the Green (40)", "the Green (40)", 7400010},
  {"the Green (41)", "the Green (41)", 7400011},
  {"the Green (42)", "the Green (42)", 7400012},
  {"the Green (43)", "the Green (43)", 7400013},
  {"the Green (44)", "the Green (44)", 7400014},
  {"the Green (45)", "the Green (45)", 7400015},
  {"the Green (46)", "the Green (46)", 7400016},
  {"the Green (47)", "the Green (47)", 7400017},
  {"the Green (48)", "the Green (48)", 7400018},
  {"the Green (49)", "the Green (49)", 7400019},
  {"the Green (50)", "the Green (50)", 7400020},
  {"the King of Green Dragons (51)", "the Queen of Green Dragons (51)", 8000000},
  {"the King of Green Dragons (52)", "the Queen of Green Dragons (52)", 8000001},
  {"the King of Green Dragons (53)", "the Queen of Green Dragons (53)", 8000002},
  {"the King of Green Dragons (54)", "the Queen of Green Dragons (54)", 8000003},
  {"the King of Green Dragons (55)", "the Queen of Green Dragons (55)", 8000004},
  {"the King of Green Dragons (56)", "the Queen of Green Dragons (56)", 8000005},
  {"the King of Green Dragons (57)", "the Queen of Green Dragons (57)", 8000006},
  {"the King of All Dragons", "the Queen of All Dragons", 9000000},
  {"the God of Dragons", "the Goddess of Dragons", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* LICH */
  {{"the Fool", "the Fool", 0},
  {"the Cultist", "the Cult Virgin", 1},
  {"the Excommunicated", "the Excommunicated", 2000},
  {"the Damned", "the Damned", 4000},
  {"the Spat Upon", "the Spat Upon", 8000},
  {"the Forgotten", "the Forgotten", 16000},
  {"the Corrupted", "the Corrupted", 32000},
  {"the Initiate", "the Initiate Virgin", 64000},
  {"the Black Cowl", "the Coven Virgin", 125000},
  {"the Petty Necromancer", "the Petty Necromancer", 250000},
  {"the Splifficate", "the Splifficate Whore", 500000},
  {"the Bargainer", "the Bargainer", 750000},
  {"the Lost Soul", "the Lost Soul", 1000000},
  {"the Unholy", "the Unholy", 1250000},
  {"the Larvae", "the Larvae", 1500000},
  {"the Quasit", "the Quasit", 1850000},
  {"the Fiend's Chained Pet", "the Fiend's Plaything", 2200000},
  {"the Eye", "the Eye", 2550000},
  {"the Hand", "the Hand", 2900000},
  {"the Dark", "the Dark", 3250000},
  {"the Lich", "the Lich", 3600000},
  {"the Lich (21)", "the Lich (21)", 3900000},
  {"the Lich (22)", "the Lich (22)", 4200000},
  {"the Lich (23)", "the Lich (23)", 4500000},
  {"the Lich (24)", "the Lich (24)", 4800000},
  {"the Lich (25)", "the Lich (25)", 5150000},
  {"the Lich (26)", "the Lich (26)", 5500000},
  {"the Lich (27)", "the Lich (27)", 5950000},
  {"the Lich (28)", "the Lich (28)", 6400000},
  {"the Lich (29)", "the Lich (29)", 6850000},
  {"the Lich (30)", "the Lich (30)", 7400000},
  {"the Lich (31)", "the Lich (31)", 7400001},
  {"the Lich (32)", "the Lich (32)", 7400002},
  {"the Lich (33)", "the Lich (33)", 7400003},
  {"the Lich (34)", "the Lich (34)", 7400004},
  {"the Lich (35)", "the Lich (35)", 7400005},
  {"the Lich (36)", "the Lich (36)", 7400006},
  {"the Lich (37)", "the Lich (37)", 7400007},
  {"the Lich (38)", "the Lich (38)", 7400008},
  {"the Lich (39)", "the Lich (39)", 7400009},
  {"the Lich (40)", "the Lich (40)", 7400010},
  {"the Lich (41)", "the Lich (41)", 7400011},
  {"the Lich (42)", "the Lich (42)", 7400012},
  {"the Lich (43)", "the Lich (43)", 7400013},
  {"the Lich (44)", "the Lich (44)", 7400014},
  {"the Lich (45)", "the Lich (45)", 7400015},
  {"the Lich (46)", "the Lich (46)", 7400016},
  {"the Lich (47)", "the Lich (47)", 7400017},
  {"the Lich (48)", "the Lich (48)", 7400018},
  {"the Lich (49)", "the Lich (49)", 7400019},
  {"the Lich (50)", "the Lich (50)", 7400020},
  {"the Infernal (51)", "the Infernal (51)", 8000000},
  {"the Infernal (52)", "the Infernal (52)", 8000001},
  {"the Infernal (53)", "the Infernal (53)", 8000002},
  {"the Infernal (54)", "the Infernal (54)", 8000003},
  {"the Infernal (55)", "the Infernal (55)", 8000004},
  {"the Infernal (56)", "the Infernal (56)", 8000005},
  {"the Infernal (57)", "the Infernal (57)", 8000006},
  {"the Greater Infernal", "the Greater Infernal", 9000000},
  {"the Yama King", "the Queen of Hell", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* GHOUL */
  {{"the Bookworm", "the Bookworm", 0},
  {"the Charnal Corpse", "the Charnal Corpse", 1},
  {"the Ghoul Slave", "the Ghoul Slave", 2000},
  {"the Baby Snatcher", "the Baby Snatcher", 4000},
  {"the Flesheater", "the Fleshcooker", 8000},
  {"the Cannibal", "the Cannibal", 16000},
  {"the Grave Robber", "the Grave Robber", 32000},
  {"the Corpse Thief", "the Corpse Thief", 64000},
  {"the Corpse Stealer", "the Corpse Stealer", 125000},
  {"the Corpse Eater", "the Corpse Eater", 250000},
  {"the Corpse Monkey", "the Corpse Monkey", 500000},
  {"the Meeper", "the Meeper", 750000},
  {"the Gibberer", "the Gibberer", 1000000},
  {"the Howler", "the Howler", 1250000},
  {"the Grue", "the Grue", 1500000},
  {"the Retriever", "the Retriever", 1850000},
  {"the Grave Defiler", "the Grave Defiler", 2200000},
  {"the Crypt Beast", "the Crypt Beast", 2550000},
  {"the Graveyard Shadow", "the Lamplight Shadow", 2900000},
  {"the Night Prowler", "the Night Prowler", 3250000},
  {"the Ghoul", "the Ghoul", 3600000},
  {"the Ghoul (21)", "the Ghoul (21)", 3900000},
  {"the Ghoul (22)", "the Ghoul (22)", 4200000},
  {"the Ghoul (23)", "the Ghoul (23)", 4500000},
  {"the Ghoul (24)", "the Ghoul (24)", 4800000},
  {"the Ghoul (25)", "the Ghoul (25)", 5150000},
  {"the Ghoul (26)", "the Ghoul (26)", 5500000},
  {"the Ghoul (27)", "the Ghoul (27)", 5950000},
  {"the Ghoul (28)", "the Ghoul (28)", 6400000},
  {"the Ghoul (29)", "the Ghoul (29)", 6850000},
  {"the Ghoul (30)", "the Ghoul (30)", 7400000},
  {"the Ghoul (31)", "the Ghoul (31)", 7400001},
  {"the Ghoul (32)", "the Ghoul (32)", 7400002},
  {"the Ghoul (33)", "the Ghoul (33)", 7400003},
  {"the Ghoul (34)", "the Ghoul (34)", 7400004},
  {"the Ghoul (35)", "the Ghoul (35)", 7400005},
  {"the Ghoul (36)", "the Ghoul (36)", 7400006},
  {"the Ghoul (37)", "the Ghoul (37)", 7400007},
  {"the Ghoul (38)", "the Ghoul (38)", 7400008},
  {"the Ghoul (39)", "the Ghoul (39)", 7400009},
  {"the Ghoul (40)", "the Ghoul (40)", 7400010},
  {"the Ghoul (41)", "the Ghoul (41)", 7400011},
  {"the Ghoul (42)", "the Ghoul (42)", 7400012},
  {"the Ghoul (43)", "the Ghoul (43)", 7400013},
  {"the Ghoul (44)", "the Ghoul (44)", 7400014},
  {"the Ghoul (45)", "the Ghoul (45)", 7400015},
  {"the Ghoul (46)", "the Ghoul (46)", 7400016},
  {"the Ghoul (47)", "the Ghoul (47)", 7400017},
  {"the Ghoul (48)", "the Ghoul (48)", 7400018},
  {"the Ghoul (49)", "the Ghoul (49)", 7400019},
  {"the Ghoul (50)", "the Ghoul (50)", 7400020},
  {"the Lector (51)", "the Infamous Childeater (51)", 8000000},  
  {"the Lector (52)", "the Infamous Childeater (52)", 8000001},
  {"the Lector (53)", "the Infamous Childeater (53)", 8000002}, 
  {"the Lector (54)", "the Infamous Childeater (54)", 8000003},
  {"the Lector (55)", "the Infamous Childeater (55)", 8000004},
  {"the Lector (56)", "the Infamous Childeater (56)", 8000005},
  {"the Lector (57)", "the Infamous Childeater (57)", 8000006},
  {"the Ghoul King", "the Queen of Ghouls", 9000000},
  {"the Death Monkey", "the Death Monkey", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* DEATH KNIGHT */
  {{"the Corruptable", "the Corruptable", 0},
  {"the Vain", "the Vain", 1},
  {"the Red Rose", "the Red Rose", 2000},
  {"the Traitor", "the Traitress", 4000},
  {"the Betrayer", "the Betrayed", 8000},
  {"the Oathbreaker", "the Oathbreaker", 16000},
  {"the Raven", "the Ravenhaired", 32000},
  {"the Snaketongue", "the Snaketongue", 64000},
  {"the Corrupt", "the Corrupt", 125000},
  {"the Fallen", "the Fallen", 250000},
  {"the Hero Slayer", "the Hero Slayer", 500000},
  {"the Reaver", "the Poisoner", 750000},
  {"the Dark Sword", "the Dark Eyed", 1000000},
  {"the Death Sword", "the Death Eyed", 1250000},
  {"the Red Death", "the Red Death", 1500000},
  {"the Black Rose", "the Black Rose", 1850000},
  {"the Death Champion", "the Death Companion", 2200000},
  {"the Black Knight", "the Black Lady", 2550000},
  {"the Dark Rider", "the Dark Rider", 2900000},
  {"the Hiepslayer", "the Betrayer of Hiep", 3250000},
  {"the Death Knight", "the Death Lady ", 3600000},
  {"the Death Knight (21)", "the Death Lady (21)", 3900000},
  {"the Death Knight (22)", "the Death Lady (22)", 4200000},
  {"the Death Knight (23)", "the Death Lady (23)", 4500000},
  {"the Death Knight (24)", "the Death Lady (24)", 4800000},
  {"the Death Knight (25)", "the Death Lady (25)", 5150000},
  {"the Death Knight (26)", "the Death Lady (26)", 5500000},
  {"the Death Knight (27)", "the Death Lady (27)", 5950000},
  {"the Death Knight (28)", "the Death Lady (28)", 6400000},
  {"the Death Knight (29)", "the Death Lady (29)", 6850000},
  {"the Death Knight (30)", "the Death Lady (30)", 7400000},
  {"the Death Knight (31)", "the Death Lady (31)", 7400001},
  {"the Death Knight (32)", "the Death Lady (32)", 7400002},
  {"the Death Knight (33)", "the Death Lady (33)", 7400003},
  {"the Death Knight (34)", "the Death Lady (34)", 7400004},
  {"the Death Knight (35)", "the Death Lady (35)", 7400005},
  {"the Death Knight (36)", "the Death Lady (36)", 7400006},
  {"the Death Knight (37)", "the Death Lady (37)", 7400007},
  {"the Death Knight (38)", "the Death Lady (38)", 7400008},
  {"the Death Knight (39)", "the Death Lady (39)", 7400009},
  {"the Death Knight (40)", "the Death Lady (40)", 7400010},
  {"the Death Knight (41)", "the Death Lady (41)", 7400011},
  {"the Death Knight (42)", "the Death Lady (42)", 7400012},
  {"the Death Knight (43)", "the Death Lady (43)", 7400013},
  {"the Death Knight (44)", "the Death Lady (44)", 7400014},
  {"the Death Knight (45)", "the Death Lady (45)", 7400015},
  {"the Death Knight (46)", "the Death Lady (46)", 7400016},
  {"the Death Knight (47)", "the Death Lady (47)", 7400017},
  {"the Death Knight (48)", "the Death Lady (48)", 7400018},
  {"the Death Knight (49)", "the Death Lady (49)", 7400019},
  {"the Death Knight (50)", "the Death Lady (50)", 7400020},
  {"the Fourth Horseman (51)", "the Sweet Kiss of Death (51)", 8000000},  
  {"the Fourth Horseman (52)", "the Sweet Kiss of Death (52)", 8000001},
  {"the Fourth Horseman (53)", "the Sweet Kiss of Death (53)", 8000002}, 
  {"the Fourth Horseman (54)", "the Sweet Kiss of Death (54)", 8000003},
  {"the Fourth Horseman (55)", "the Sweet Kiss of Death (55)", 8000004},
  {"the Fourth Horseman (56)", "the Sweet Kiss of Death (56)", 8000005},
  {"the Fourth Horseman (57)", "the Sweet Kiss of Death (57)", 8000006},
  {"the Grim Reaper", "the Earth Mother", 9000000},
  {"the God of Death", "the Goddess of Death", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* GHOST */
  {{"the Murdered", "the Murdered", 0},
  {"the Recently Deceased", "the Recently Deceased", 1},
  {"the Victim", "the Victim", 2000},
  {"the Slain", "the Slain", 4000},
  {"the Witness", "the Witness", 8000},
  {"the Risen", "the Risen", 16000},
  {"the Unseen", "the Unseen", 32000},
  {"the Poltergeist", "the Poltergeist", 64000},
  {"the Spirit", "the Spirit", 125000},
  {"the Haunter", "the Haunter", 250000},
  {"the Chained", "the Chained", 500000},
  {"the Cursed", "the Cursed", 750000},
  {"the Warner", "the Warner", 1000000},
  {"the Moaner", "the Moaner", 1250000},
  {"the Doom Speaker", "the Doom Speaker", 1500000},
  {"the Wailer", "the Wailer", 1850000},
  {"the Banshee", "the Banshee", 2200000},
  {"the Shining", "the Shining", 2550000},
  {"the Spectre", "the Spectre", 2900000},
  {"the Phantom", "the Phantom", 3250000},
  {"the Ghost", "the Ghost", 3600000},
  {"the Ghost (21)", "the Ghost (21)", 3900000},
  {"the Ghost (22)", "the Ghost (22)", 4200000},
  {"the Ghost (23)", "the Ghost (23)", 4500000},
  {"the Ghost (24)", "the Ghost (24)", 4800000},
  {"the Ghost (25)", "the Ghost (25)", 5150000},
  {"the Ghost (26)", "the Ghost (26)", 5500000},
  {"the Ghost (27)", "the Ghost (27)", 5950000},
  {"the Ghost (28)", "the Ghost (28)", 6400000},
  {"the Ghost (29)", "the Ghost (29)", 6850000},
  {"the Ghost (30)", "the Ghost (30)", 7400000},
  {"the Ghost (31)", "the Ghost (31)", 7400001},
  {"the Ghost (32)", "the Ghost (32)", 7400002},
  {"the Ghost (33)", "the Ghost (33)", 7400003},
  {"the Ghost (34)", "the Ghost (34)", 7400004},
  {"the Ghost (35)", "the Ghost (35)", 7400005},
  {"the Ghost (36)", "the Ghost (36)", 7400006},
  {"the Ghost (37)", "the Ghost (37)", 7400007},
  {"the Ghost (38)", "the Ghost (38)", 7400008},
  {"the Ghost (39)", "the Ghost (39)", 7400009},
  {"the Ghost (40)", "the Ghost (40)", 7400010},
  {"the Ghost (41)", "the Ghost (41)", 7400011},
  {"the Ghost (42)", "the Ghost (42)", 7400012},
  {"the Ghost (43)", "the Ghost (43)", 7400013},
  {"the Ghost (44)", "the Ghost (44)", 7400014},
  {"the Ghost (45)", "the Ghost (45)", 7400015},
  {"the Ghost (46)", "the Ghost (46)", 7400016},
  {"the Ghost (47)", "the Ghost (47)", 7400017},
  {"the Ghost (48)", "the Ghost (48)", 7400018},
  {"the Ghost (49)", "the Ghost (49)", 7400019},
  {"the Ghost (50)", "the Ghost (50)", 7400020},
  {"the Restless Ghost (51)", "the Restless Ghost (51)", 8000000},  
  {"the Restless Ghost (52)", "the Restless Ghost (52)", 8000001},
  {"the Restless Ghost (53)", "the Restless Ghost (53)", 8000002}, 
  {"the Restless Ghost (54)", "the Restless Ghost (54)", 8000003},
  {"the Restless Ghost (55)", "the Restless Ghost (55)", 8000004},
  {"the Restless Ghost (56)", "the Restless Ghost (56)", 8000005},
  {"the Restless Ghost (57)", "the Restless Ghost (57)", 8000006},
  {"the Most Foul Ghost", "the Most Foul Ghost", 9000000},
  {"the Greatest Shade", "the Greatest Shade", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* ANIMAL */
  {{"the Man", "the Woman", 0},
  {"the Swordpupil", "the Swordpupil", 1},
  {"the Recruit", "the Recruit", 2000},
  {"the Sentry", "the Sentress", 4000},
  {"the Fighter", "the Fighter", 8000},
  {"the Soldier", "the Soldier", 16000},
  {"the Warrior", "the Warrior", 32000},
  {"the Veteran", "the Veteran", 64000},
  {"the Swordsman", "the Swordswoman", 125000},
  {"the Fencer", "the Fenceress", 250000},
  {"the Combatant", "the Combatess", 500000},
  {"the Hero", "the Heroine", 750000},
  {"the Myrmidon", "the Myrmidon", 1000000},
  {"the Swashbuckler", "the Swashbuckleress", 1250000},
  {"the Mercenary", "the Mercenaress", 1500000},
  {"the Swordmaster", "the Swordmistress", 1850000},
  {"the Lieutenant", "the Lieutenant", 2200000},
  {"the Champion", "the Lady Champion", 2550000},
  {"the Dragoon", "the Lady Dragoon", 2900000},
  {"the Cavalier", "the Cavalier", 3250000},
  {"the Knight", "the Lady Knight", 3600000},
  {"the Knight (21)", "the Lady Knight (21)", 3900000},
  {"the Knight (22)", "the Lady Knight (22)", 4200000},
  {"the Knight (23)", "the Lady Knight (23)", 4500000},
  {"the Knight (24)", "the Lady Knight (24)", 4800000},
  {"the Knight (25)", "the Lady Knight (25)", 5150000},
  {"the Knight (26)", "the Lady Knight (26)", 5500000},
  {"the Knight (27)", "the Lady Knight (27)", 5950000},
  {"the Knight (28)", "the Lady Knight (28)", 6400000},
  {"the Knight (29)", "the Lady Knight (29)", 6850000},
  {"the Knight (30)", "the Lady Knight (30)", 7400000},
  {"the Knight (31)", "the Lady Knight (31)", 7400001},
  {"the Knight (32)", "the Lady Knight (32)", 7400002},
  {"the Knight (33)", "the Lady Knight (33)", 7400003},
  {"the Knight (34)", "the Lady Knight (34)", 7400004},
  {"the Knight (35)", "the Lady Knight (35)", 7400005},
  {"the Knight (36)", "the Lady Knight (36)", 7400006},
  {"the Knight (37)", "the Lady Knight (37)", 7400007},
  {"the Knight (38)", "the Lady Knight (38)", 7400008},
  {"the Knight (39)", "the Lady Knight (39)", 7400009},
  {"the Knight (40)", "the Lady Knight (40)", 7400010},
  {"the Knight (41)", "the Lady Knight (41)", 7400011},
  {"the Knight (42)", "the Lady Knight (42)", 7400012},
  {"the Knight (43)", "the Lady Knight (43)", 7400013},
  {"the Knight (44)", "the Lady Knight (44)", 7400014},
  {"the Knight (45)", "the Lady Knight (45)", 7400015},
  {"the Knight (46)", "the Lady Knight (46)", 7400016},
  {"the Knight (47)", "the Lady Knight (47)", 7400017},
  {"the Knight (48)", "the Lady Knight (48)", 7400018},
  {"the Knight (49)", "the Lady Knight (49)", 7400019},
  {"the Knight (50)", "the Lady Knight (50)", 7400020},
  {"the Immortal Warlord (51)", "the Immortal Lady of War (51)", 8000000},
  {"the Immortal Warlord (52)", "the Immortal Lady of War (52)", 8000001},
  {"the Immortal Warlord (53)", "the Immortal Lady of War (53)", 8000002},
  {"the Immortal Warlord (54)", "the Immortal Lady of War (54)", 8000003},
  {"the Immortal Warlord (55)", "the Immortal Lady of War (55)", 8000004},
  {"the Immortal Warlord (56)", "the Immortal Lady of War (56)", 8000005},
  {"the Immortal Warlord (57)", "the Immortal Lady of War (57)", 8000006},
  {"the Extirpator", "the Queen of Destruction", 9000000},
  {"the God of war", "the Goddess of war", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* CURRENCY */
  {{"the Man", "the Woman", 0},
  {"the Swordpupil", "the Swordpupil", 1},
  {"the Recruit", "the Recruit", 2000},
  {"the Sentry", "the Sentress", 4000},
  {"the Fighter", "the Fighter", 8000},
  {"the Soldier", "the Soldier", 16000},
  {"the Warrior", "the Warrior", 32000},
  {"the Veteran", "the Veteran", 64000},
  {"the Swordsman", "the Swordswoman", 125000},
  {"the Fencer", "the Fenceress", 250000},
  {"the Combatant", "the Combatess", 500000},
  {"the Hero", "the Heroine", 750000},
  {"the Myrmidon", "the Myrmidon", 1000000},
  {"the Swashbuckler", "the Swashbuckleress", 1250000},
  {"the Mercenary", "the Mercenaress", 1500000},
  {"the Swordmaster", "the Swordmistress", 1850000},
  {"the Lieutenant", "the Lieutenant", 2200000},
  {"the Champion", "the Lady Champion", 2550000},
  {"the Dragoon", "the Lady Dragoon", 2900000},
  {"the Cavalier", "the Cavalier", 3250000},
  {"the Knight", "the Lady Knight", 3600000},
  {"the Knight (21)", "the Lady Knight (21)", 3900000},
  {"the Knight (22)", "the Lady Knight (22)", 4200000},
  {"the Knight (23)", "the Lady Knight (23)", 4500000},
  {"the Knight (24)", "the Lady Knight (24)", 4800000},
  {"the Knight (25)", "the Lady Knight (25)", 5150000},
  {"the Knight (26)", "the Lady Knight (26)", 5500000},
  {"the Knight (27)", "the Lady Knight (27)", 5950000},
  {"the Knight (28)", "the Lady Knight (28)", 6400000},
  {"the Knight (29)", "the Lady Knight (29)", 6850000},
  {"the Knight (30)", "the Lady Knight (30)", 7400000},
  {"the Knight (31)", "the Lady Knight (31)", 7400001},
  {"the Knight (32)", "the Lady Knight (32)", 7400002},
  {"the Knight (33)", "the Lady Knight (33)", 7400003}, 
  {"the Knight (34)", "the Lady Knight (34)", 7400004},
  {"the Knight (35)", "the Lady Knight (35)", 7400005},
  {"the Knight (36)", "the Lady Knight (36)", 7400006},
  {"the Knight (37)", "the Lady Knight (37)", 7400007},
  {"the Knight (38)", "the Lady Knight (38)", 7400008},
  {"the Knight (39)", "the Lady Knight (39)", 7400009},
  {"the Knight (40)", "the Lady Knight (40)", 7400010},
  {"the Knight (41)", "the Lady Knight (41)", 7400011},
  {"the Knight (42)", "the Lady Knight (42)", 7400012},
  {"the Knight (43)", "the Lady Knight (43)", 7400013},
  {"the Knight (44)", "the Lady Knight (44)", 7400014},
  {"the Knight (45)", "the Lady Knight (45)", 7400015},
  {"the Knight (46)", "the Lady Knight (46)", 7400016},
  {"the Knight (47)", "the Lady Knight (47)", 7400017},
  {"the Knight (48)", "the Lady Knight (48)", 7400018},
  {"the Knight (49)", "the Lady Knight (49)", 7400019},
  {"the Knight (50)", "the Lady Knight (50)", 7400020},
  {"the Immortal Warlord (51)", "the Immortal Lady of War (51)", 8000000},
  {"the Immortal Warlord (52)", "the Immortal Lady of War (52)", 8000001},
  {"the Immortal Warlord (53)", "the Immortal Lady of War (53)", 8000002},
  {"the Immortal Warlord (54)", "the Immortal Lady of War (54)", 8000003},
  {"the Immortal Warlord (55)", "the Immortal Lady of War (55)", 8000004},
  {"the Immortal Warlord (56)", "the Immortal Lady of War (56)", 8000005},
  {"the Immortal Warlord (57)", "the Immortal Lady of War (57)", 8000006},
  {"the Extirpator", "the Queen of Destruction", 9000000},
  {"the God of war", "the Goddess of war", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* CITY */
  {{"the Man", "the Woman", 0},
  {"the Swordpupil", "the Swordpupil", 1},
  {"the Recruit", "the Recruit", 2000},
  {"the Sentry", "the Sentress", 4000},
  {"the Fighter", "the Fighter", 8000},
  {"the Soldier", "the Soldier", 16000},
  {"the Warrior", "the Warrior", 32000},
  {"the Veteran", "the Veteran", 64000},
  {"the Swordsman", "the Swordswoman", 125000},
  {"the Fencer", "the Fenceress", 250000},
  {"the Combatant", "the Combatess", 500000},
  {"the Hero", "the Heroine", 750000},
  {"the Myrmidon", "the Myrmidon", 1000000},
  {"the Swashbuckler", "the Swashbuckleress", 1250000},
  {"the Mercenary", "the Mercenaress", 1500000},
  {"the Swordmaster", "the Swordmistress", 1850000},
  {"the Lieutenant", "the Lieutenant", 2200000},
  {"the Champion", "the Lady Champion", 2550000},
  {"the Dragoon", "the Lady Dragoon", 2900000},
  {"the Cavalier", "the Cavalier", 3250000},
  {"the Knight", "the Lady Knight", 3600000},
  {"the Knight (21)", "the Lady Knight (21)", 3900000},
  {"the Knight (22)", "the Lady Knight (22)", 4200000},
  {"the Knight (23)", "the Lady Knight (23)", 4500000},
  {"the Knight (24)", "the Lady Knight (24)", 4800000},
  {"the Knight (25)", "the Lady Knight (25)", 5150000},
  {"the Knight (26)", "the Lady Knight (26)", 5500000},
  {"the Knight (27)", "the Lady Knight (27)", 5950000},
  {"the Knight (28)", "the Lady Knight (28)", 6400000},
  {"the Knight (29)", "the Lady Knight (29)", 6850000},
  {"the Knight (30)", "the Lady Knight (30)", 7400000},
  {"the Knight (31)", "the Lady Knight (31)", 7400001},
  {"the Knight (32)", "the Lady Knight (32)", 7400002},
  {"the Knight (33)", "the Lady Knight (33)", 7400003}, 
  {"the Knight (34)", "the Lady Knight (34)", 7400004},
  {"the Knight (35)", "the Lady Knight (35)", 7400005},
  {"the Knight (36)", "the Lady Knight (36)", 7400006},
  {"the Knight (37)", "the Lady Knight (37)", 7400007},
  {"the Knight (38)", "the Lady Knight (38)", 7400008},
  {"the Knight (39)", "the Lady Knight (39)", 7400009},
  {"the Knight (40)", "the Lady Knight (40)", 7400010},
  {"the Knight (41)", "the Lady Knight (41)", 7400011},
  {"the Knight (42)", "the Lady Knight (42)", 7400012},
  {"the Knight (43)", "the Lady Knight (43)", 7400013},
  {"the Knight (44)", "the Lady Knight (44)", 7400014},
  {"the Knight (45)", "the Lady Knight (45)", 7400015},
  {"the Knight (46)", "the Lady Knight (46)", 7400016},
  {"the Knight (47)", "the Lady Knight (47)", 7400017},
  {"the Knight (48)", "the Lady Knight (48)", 7400018},
  {"the Knight (49)", "the Lady Knight (49)", 7400019},
  {"the Knight (50)", "the Lady Knight (50)", 7400020},
  {"the Immortal Warlord (51)", "the Immortal Lady of War (51)", 8000000},
  {"the Immortal Warlord (52)", "the Immortal Lady of War (52)", 8000001},
  {"the Immortal Warlord (53)", "the Immortal Lady of War (53)", 8000002},
  {"the Immortal Warlord (54)", "the Immortal Lady of War (54)", 8000003},
  {"the Immortal Warlord (55)", "the Immortal Lady of War (55)", 8000004},
  {"the Immortal Warlord (56)", "the Immortal Lady of War (56)", 8000005},
  {"the Immortal Warlord (57)", "the Immortal Lady of War (57)", 8000006},
  {"the Extirpator", "the Queen of Destruction", 9000000},
  {"the God of war", "the Goddess of war", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },
/* MUPPET */
  {{"the Man", "the Woman", 0},
  {"the Swordpupil", "the Swordpupil", 1},
  {"the Recruit", "the Recruit", 2000},
  {"the Sentry", "the Sentress", 4000},
  {"the Fighter", "the Fighter", 8000},
  {"the Soldier", "the Soldier", 16000},
  {"the Warrior", "the Warrior", 32000},
  {"the Veteran", "the Veteran", 64000},
  {"the Swordsman", "the Swordswoman", 125000},
  {"the Fencer", "the Fenceress", 250000},
  {"the Combatant", "the Combatess", 500000},
  {"the Hero", "the Heroine", 750000},
  {"the Myrmidon", "the Myrmidon", 1000000},
  {"the Swashbuckler", "the Swashbuckleress", 1250000},
  {"the Mercenary", "the Mercenaress", 1500000},
  {"the Swordmaster", "the Swordmistress", 1850000},
  {"the Lieutenant", "the Lieutenant", 2200000},
  {"the Champion", "the Lady Champion", 2550000},
  {"the Dragoon", "the Lady Dragoon", 2900000},
  {"the Cavalier", "the Cavalier", 3250000},
  {"the Knight", "the Lady Knight", 3600000},
  {"the Knight (21)", "the Lady Knight (21)", 3900000},
  {"the Knight (22)", "the Lady Knight (22)", 4200000},
  {"the Knight (23)", "the Lady Knight (23)", 4500000},
  {"the Knight (24)", "the Lady Knight (24)", 4800000},
  {"the Knight (25)", "the Lady Knight (25)", 5150000},
  {"the Knight (26)", "the Lady Knight (26)", 5500000},
  {"the Knight (27)", "the Lady Knight (27)", 5950000},
  {"the Knight (28)", "the Lady Knight (28)", 6400000},
  {"the Knight (29)", "the Lady Knight (29)", 6850000},
  {"the Knight (30)", "the Lady Knight (30)", 7400000},
  {"the Knight (31)", "the Lady Knight (31)", 7400001},
  {"the Knight (32)", "the Lady Knight (32)", 7400002},
  {"the Knight (33)", "the Lady Knight (33)", 7400003}, 
  {"the Knight (34)", "the Lady Knight (34)", 7400004},
  {"the Knight (35)", "the Lady Knight (35)", 7400005},
  {"the Knight (36)", "the Lady Knight (36)", 7400006},
  {"the Knight (37)", "the Lady Knight (37)", 7400007},
  {"the Knight (38)", "the Lady Knight (38)", 7400008},
  {"the Knight (39)", "the Lady Knight (39)", 7400009},
  {"the Knight (40)", "the Lady Knight (40)", 7400010},
  {"the Knight (41)", "the Lady Knight (41)", 7400011},
  {"the Knight (42)", "the Lady Knight (42)", 7400012},
  {"the Knight (43)", "the Lady Knight (43)", 7400013},
  {"the Knight (44)", "the Lady Knight (44)", 7400014},
  {"the Knight (45)", "the Lady Knight (45)", 7400015},
  {"the Knight (46)", "the Lady Knight (46)", 7400016},
  {"the Knight (47)", "the Lady Knight (47)", 7400017},
  {"the Knight (48)", "the Lady Knight (48)", 7400018},
  {"the Knight (49)", "the Lady Knight (49)", 7400019},
  {"the Knight (50)", "the Lady Knight (50)", 7400020},
  {"the Immortal Warlord (51)", "the Immortal Lady of War (51)", 8000000},
  {"the Immortal Warlord (52)", "the Immortal Lady of War (52)", 8000001},
  {"the Immortal Warlord (53)", "the Immortal Lady of War (53)", 8000002},
  {"the Immortal Warlord (54)", "the Immortal Lady of War (54)", 8000003},
  {"the Immortal Warlord (55)", "the Immortal Lady of War (55)", 8000004},
  {"the Immortal Warlord (56)", "the Immortal Lady of War (56)", 8000005},
  {"the Immortal Warlord (57)", "the Immortal Lady of War (57)", 8000006},
  {"the Extirpator", "the Queen of Destruction", 9000000},
  {"the God of war", "the Goddess of war", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  },


/* MONK */
  {{"the Man", "the Woman", 0},
  {"the Acolyte", "the Acolyte", 1},
  {"the Student", "the Student", 2000},
  {"the White Belt", "the White Belt", 4000},
  {"the Combatant", "the Combatant", 8000},
  {"the Yellow Belt", "the Yellow Belt", 16000},
  {"the Kung-Fu Fighter", "the Kung-Fu Fighter", 32000},
  {"the Red Belt", "the Red Belt", 64000},
  {"the Monk", "the Monk", 125000},
  {"the Fencer", "the Fenceress", 250000},
  {"the Combatant", "the Combatess", 500000},
  {"the Hero", "the Heroine", 750000},
  {"the Myrmidon", "the Myrmidon", 1000000},
  {"the Swashbuckler", "the Swashbuckleress", 1250000},
  {"the Mercenary", "the Mercenaress", 1500000},
  {"the Swordmaster", "the Swordmistress", 1850000},
  {"the Lieutenant", "the Lieutenant", 2200000},
  {"the Champion", "the Lady Champion", 2550000},
  {"the Dragoon", "the Lady Dragoon", 2900000},
  {"the Cavalier", "the Cavalier", 3250000},
  {"the Knight", "the Lady Knight", 3600000},
  {"the Knight (21)", "the Lady Knight (21)", 3900000},
  {"the Knight (22)", "the Lady Knight (22)", 4200000},
  {"the Knight (23)", "the Lady Knight (23)", 4500000},
  {"the Knight (24)", "the Lady Knight (24)", 4800000},
  {"the Knight (25)", "the Lady Knight (25)", 5150000},
  {"the Knight (26)", "the Lady Knight (26)", 5500000},
  {"the Knight (27)", "the Lady Knight (27)", 5950000},
  {"the Knight (28)", "the Lady Knight (28)", 6400000},
  {"the Knight (29)", "the Lady Knight (29)", 6850000},
  {"the Knight (30)", "the Lady Knight (30)", 7400000},
  {"the Knight (31)", "the Lady Knight (31)", 7400001},
  {"the Knight (32)", "the Lady Knight (32)", 7400002},
  {"the Knight (33)", "the Lady Knight (33)", 7400003}, 
  {"the Knight (34)", "the Lady Knight (34)", 7400004},
  {"the Knight (35)", "the Lady Knight (35)", 7400005},
  {"the Knight (36)", "the Lady Knight (36)", 7400006},
  {"the Knight (37)", "the Lady Knight (37)", 7400007},
  {"the Knight (38)", "the Lady Knight (38)", 7400008},
  {"the Knight (39)", "the Lady Knight (39)", 7400009},
  {"the Knight (40)", "the Lady Knight (40)", 7400010},
  {"the Knight (41)", "the Lady Knight (41)", 7400011},
  {"the Knight (42)", "the Lady Knight (42)", 7400012},
  {"the Knight (43)", "the Lady Knight (43)", 7400013},
  {"the Knight (44)", "the Lady Knight (44)", 7400014},
  {"the Knight (45)", "the Lady Knight (45)", 7400015},
  {"the Knight (46)", "the Lady Knight (46)", 7400016},
  {"the Knight (47)", "the Lady Knight (47)", 7400017},
  {"the Knight (48)", "the Lady Knight (48)", 7400018},
  {"the Knight (49)", "the Lady Knight (49)", 7400019},
  {"the Knight (50)", "the Lady Knight (50)", 7400020},
  {"the Immortal Warlord (51)", "the Immortal Lady of War (51)", 8000000},
  {"the Immortal Warlord (52)", "the Immortal Lady of War (52)", 8000001},
  {"the Immortal Warlord (53)", "the Immortal Lady of War (53)", 8000002},
  {"the Immortal Warlord (54)", "the Immortal Lady of War (54)", 8000003},
  {"the Immortal Warlord (55)", "the Immortal Lady of War (55)", 8000004},
  {"the Immortal Warlord (56)", "the Immortal Lady of War (56)", 8000005},
  {"the Immortal Warlord (57)", "the Immortal Lady of War (57)", 8000006},
  {"the Extirpator", "the Queen of Destruction", 9000000},
  {"the God of war", "the Goddess of war", 9500000},
  {"the Implementor", "the Implementress", 10000000}
  }


};


/* This exp array is called by generate_experience_tables to override
 * the values for titles[lvl].exp for each class
 */

const int experience_table[LVL_IMPL + 1] = {
            0,		/* 0 */
            1,
         1500,
         3000,
         5000,
        11000,		/* 5 */
        23000,
        40000,
        70000,
       100000,
       200000,		/* 10 */
       350000,
       500000,
       900000,
      1300000,
      2000000,		/* 15 */
      3000000,
      4000000,
      5000000,
      6200000,
      7500000,		/* 20 */
      9000000,
     11000000,
     13500000,
     16000000,
     19000000,		/* 25 */
     22000000,
     25000000,
     28000000,
     32000000,
     37000000,		/* 30 */
     42000000,
     47000000,
     52000000,
     58000000,
     64000000,		/* 35 */
     70000000,
     76000000,
     83000000,
     90000000,
     97000000,		/* 40 */
    115000000,
    123000000,
    131000000,
    139000000,
    149000000,		/* 45 */
    160000000,
    170000000,
    180000000,
    190000000,
    200000000,		/* 50 */
    220000000,
    270000000,
    330000000,
    400000000,
    500000000,		/* 55 */
    620000000,
    750000000,
    900000000,
   1000000000,
   1100000000,		/* 60 */
};

/* This array is for experience multipliers that are applied to the classes */
const int experience_mult[NUM_CLASSES + 1] = {
   110,		/* Magic-User */
   120,		/* Cleric */
    90,		/* Thief */
   110,		/* Warrior */
   110,		/* Bard */
   300,		/* Deathknight */
   110,		/* Druid */
   300,		/* Vampire */
   300, 	/* Monk */
   300,		/* Lich */
   300,		/* Seraph */
   300,		/* Cherub */
   300,		/* Valkyrie */
   100,		/* Unused */
   100,		/* Unused */
   100,		/* Unused */
   100,		/* Unused */
   100,		/* Unused */
   100,		/* Unused */
   100, 	/* Unused */
   100,         /* Unused */
     0
};
/* end of hack */

int get_experience_to_level(int class, int level) 
{
  int exp;

  if (level < 0 || level > LVL_IMPL) {
    return -1;
  }

  exp = experience_table[level] / 100 * experience_mult[class];

  return exp;
}



void generate_experience_tables(void)
{
  int i, j;

  for (i = 0; i < NUM_CLASSES; i++) {
    for (j = 0; j < LVL_IMPL + 1; j++) {
      titles[i][j].exp = get_experience_to_level(i, j); 
    }
  }

  return;
}


/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data * ch)
{
  int add_hp = 0, add_mana = 0, add_move = 0, i;

/* HACKED to take out practices */
/*  extern struct wis_app_type wis_app[]; */
/* end of hack */
  extern struct con_app_type con_app[];

  add_hp = con_app[GET_CON(ch)].hitp;

/* HACKED to make addmana decent */
  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    add_hp += number(3, 8);
    add_move = number(0, 2);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_hp += number(5, 9);
    add_move = number(0, 2);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_THIEF:
    add_hp += number(7, 10);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_WARRIOR:
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_VALKYRIE:
    add_hp += number(8, 11);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_BARD:                                                          
    add_hp += number(6, 9);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_DEATHKNIGHT:
    add_hp += number(11, 16);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_VAMPIRE:                                                          
    add_hp += number(11, 16);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_MONK:
    add_hp += number(8, 12);
    add_move = number(2, 5);
    add_move += number(1, (GET_DEX(ch) / 3));

/*
  case CLASS_DRAGON:                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_MAGE2:                                      
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_CLERIC2:                                           
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_THIEF2:                                  
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_LICH:                                                          
    add_hp += number(5, 10);
    add_move = number(0, 2);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_GHOUL:                                                          
    add_hp += number(7, 13);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_DEATH_KNIGHT:                                     
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_GHOST:                                                          
    add_hp += number(6, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_WARRIOR2:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_BARD2:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_CITY:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_MUPPET:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
*/
  
  }
  
  if (IS_THRIKREEN(ch)) {
    add_move *= 1.4;
    add_move += number(1, (GET_DEX(ch) / 3));
  }

  add_mana = 10 + dice(1, 6);
  switch (GET_CLASS(ch)) {
    case CLASS_MAGIC_USER:
            add_mana += GET_INT(ch) - 16;
            add_mana = MAX(5, add_mana);
            break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
            add_mana += GET_WIS(ch) - 16;
            add_mana = MAX(5, add_mana);
            break;
    case CLASS_BARD:
            add_mana += GET_CHA(ch) - 16;
            add_mana = MAX(5, add_mana);
            add_mana = add_mana / 2;		/* bards get 1/2 mana */
            break;
    case CLASS_VAMPIRE:
            add_mana += GET_INT(ch) - 8;
            add_mana = MAX(5, add_mana);
            break;
    case CLASS_DEATHKNIGHT:
            add_mana += GET_WIS(ch) - 15;
            add_mana = MAX(5, add_mana);
            add_mana = add_mana / 3;		/* knights get 1/3 mana */
            break;
    case CLASS_VALKYRIE:
            add_mana += GET_WIS(ch) - 15;
            add_mana = MAX(5, add_mana);
            add_mana = add_mana / 3;		/* Valkyries get 1/3 mana */
            break;
    case CLASS_MONK:
            add_mana += GET_WIS(ch) - 14;
            add_mana = MAX(5, add_mana);
            add_mana = add_mana * 2 / 3;
    default:
            add_mana = 0;
            break;
  }
/* end of hack */

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

/* HACKED to end practices */
/*  GET_PRACTICES(ch) += MAX(1, wis_app[GET_WIS(ch)].bonus); */
/* end of hack */

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  save_char(ch, NOWHERE);

  sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
/* HACKED to add clanlog */
  sprintf(buf, "%s levels '%s %s'.",
      GET_NAME(ch), GET_NAME(ch),
      (GET_SEX(ch) == SEX_FEMALE) ?
        titles[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)].title_f :
        titles[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)].title_m);
  clanlog(buf, ch);
/* end of hack */
}

/* Level loss formula */
void demote_level(struct char_data * ch)
{
  int add_hp = 0, add_mana = 0, add_move = 0, i;

  extern struct con_app_type con_app[];

  add_hp = con_app[25].hitp;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    add_hp += number(3, 8);
    add_move = number(0, 2);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_hp += number(5, 9);
    add_move = number(0, 2);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_THIEF:
    add_hp += number(7, 10);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_WARRIOR:
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_BARD:                                                          
    add_hp += number(6, 9);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_DEATHKNIGHT:
    add_hp += number(11, 16);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_VAMPIRE:                                                          
    add_hp += number(11, 16);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;
  
  case CLASS_MONK:
    add_hp += number(8, 12);
    add_move = number(2, 5);
    add_move += number(1, (GET_DEX(ch) / 3));

/*

  case CLASS_DRAGON:                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_MAGE2:                                      
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_CLERIC2:                                           
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_THIEF2:                                  
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_LICH:                                                          
    add_hp += number(5, 10);
    add_move = number(0, 2);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_GHOUL:                                                          
    add_hp += number(7, 13);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_UNUSED:                                     
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_GHOST:                                                          
    add_hp += number(6, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_WARRIOR2:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_BARD2:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_CITY:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
    break;

  case CLASS_MUPPET:                                                          
    add_hp += number(10, 15);
    add_move = number(1, 3);
    add_move += number(1, (GET_DEX(ch) / 3));
*/

  }
  
  if (IS_THRIKREEN(ch)) {
    add_move *= 1.4;
    add_move += number(1, (GET_DEX(ch) / 3));
  }

  add_mana = 10 + dice(1, 6);
  switch (GET_CLASS(ch)) {
    case CLASS_MAGIC_USER:
/*
            add_mana += GET_INT(ch) - 16;
*/
            add_mana += 9;
            add_mana = MAX(5, add_mana);
            break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
/*
            add_mana += GET_WIS(ch) - 16;
*/
            add_mana += 9;
            add_mana = MAX(5, add_mana);
            break;
    case CLASS_BARD:
/*
            add_mana += GET_CHA(ch) - 16;
*/
            add_mana += 9;
            add_mana = MAX(5, add_mana);
            add_mana = add_mana / 2;		/* bards get 1/2 mana */
            break;
    case CLASS_VAMPIRE:
/*
            add_mana += GET_INT(ch) - 8;
*/
            add_mana += 17;
            add_mana = MAX(5, add_mana);
            break;
    case CLASS_DEATHKNIGHT:
/*
            add_mana += GET_WIS(ch) - 15;
*/
	    add_mana += 10;
            add_mana = MAX(5, add_mana);
            add_mana = add_mana / 2;		/* knights get 1/3 mana */
            break;
    case CLASS_MONK:
/*
            add_mana += GET_WIS(ch) - 14;
*/
	    add_mana += 11;
            add_mana = MAX(5, add_mana);
            add_mana = add_mana * 2 / 3;
    default:
            add_mana = 0;
            break;
  }

  ch->points.max_hit -= MAX(1, add_hp);
  ch->points.max_move -= MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana -= add_mana;

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  save_char(ch, NOWHERE);

  sprintf(buf, "%s demoted to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
  sprintf(buf, "%s loses a level down to '%s %s'.",
      GET_NAME(ch), GET_NAME(ch),
      (GET_SEX(ch) == SEX_FEMALE) ?
        titles[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)].title_f :
        titles[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)].title_m);
  clanlog(buf, ch);
}

bool class_can_wear( struct char_data *ch, struct obj_data *obj ) {
  /* Code added by Culvan to see if the class can use the object's slot.
     Currently only used for monks.
     Minor bug: Unpredictible for items with multiple wear slots!
  */
  int i;

  if (!IS_MONK(ch)) return TRUE;
  for ( i = 1; i < NUM_ITEM_WEARS; i++ )
    if (monk_can_wear[i])
      if (CAN_WEAR(obj, 1 << i)) return TRUE;
  return FALSE;
}

int get_class_guess(struct char_data *ch) {

  if (!IS_NPC(ch))
    return GET_CLASS(ch);

  if (isname("warrior", ch->player.name))	return CLASS_WARRIOR;
  if (isname("cleric", ch->player.name))	return CLASS_CLERIC;
  if (isname("mage", ch->player.name))		return CLASS_MAGIC_USER;
  if (isname("thief", ch->player.name))		return CLASS_THIEF;
  if (isname("bard", ch->player.name))		return CLASS_BARD;
  if (isname("druid", ch->player.name))		return CLASS_DRUID;

  return CLASS_UNDEFINED;
}
