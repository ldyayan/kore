/*************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

/* preamble *************************************************************/

#define NOWHERE    -1    /* nil reference for room-database	*/
#define SOMEPLACE   0    /* someplace for room-database		*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)


/* zone-related defines *************************************************/

/* zone flags */
#define ZONE_NOTELEPORT         (1 << 0) /* cannot teleport into zone   */
#define ZONE_NOPHASEDOOR	(1 << 1) /* cannot phase door into zone */
#define ZONE_CHAOS		(1 << 2) /* chaos arena zone */
#define ZONE_GODZONE		(1 << 3) /* higher imms only */
#define ZONE_BATTLEOK		(1 << 4) /* You can go to the arena */
#define ZONE_ACTIVE		(1 << 5) /* Zone is active, connected */
#define NUM_ZONE_FLAGS          6



/* room-related defines *************************************************/


/* The cardinal directions: used as index to room_data.dir_option[]
  and additionally in dir_order[] used by the 'exits' command */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define NORTHEAST      6
#define SOUTHEAST      7
#define SOUTHWEST      8
#define NORTHWEST      9
#define SOMEWHERE      10


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		(1 << 0)   /* Dark			*/
#define ROOM_DEATH		(1 << 1)   /* Death trap		*/
#define ROOM_NOMOB		(1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		(1 << 3)   /* Indoors			*/
#define ROOM_PEACEFUL		(1 << 4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF		(1 << 5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK		(1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		(1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		(1 << 8)   /* Is a tunnel		*/
#define ROOM_PRIVATE		(1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM		(1 << 10)  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		(1 << 11)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH	(1 << 12)  /* (R) House needs saving	*/
#define ROOM_ATRIUM		(1 << 13)  /* (R) The door to a house	*/
#define ROOM_OLC		(1 << 14)  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK		(1 << 15)  /* (R) breath-first srch mrk	*/
#define ROOM_CHAOS              (1 << 16)  /* PK-OK room                */
#define ROOM_NOTELEPORT		(1 << 17)  /* Can't teleport in         */
#define ROOM_NOPHASEDOOR	(1 << 18)  /* Can't phase door in       */
#define ROOM_STORAGE		(1 << 19)  /* Is a storage room 	*/
#define ROOM_HYPERREGEN         (1 << 20)  /* Regen extra fast 		*/
#define ROOM_SOLITARY		(1 << 21)  /* Room for just one person	*/
#define ROOM_UNUSED22		(1 << 22)  /* UNUSED	 		*/
#define ROOM_UNUSED23		(1 << 23)  /* UNUSED	 		*/
#define ROOM_UNUSED24		(1 << 24)  /* UNUSED	 		*/
#define ROOM_UNUSED25		(1 << 25)  /* UNUSED	 		*/
#define ROOM_UNUSED26		(1 << 26)  /* UNUSED	 		*/
#define ROOM_UNUSED27		(1 << 27)  /* UNUSED	 		*/
#define ROOM_UNUSED28		(1 << 28)  /* UNUSED	 		*/
#define ROOM_UNUSED29		(1 << 29)  /* UNUSED	 		*/
#define ROOM_UNUSED30		(1 << 30)  /* UNUSED	 		*/
#define ROOM_UNUSED31		(1 << 31)  /* UNUSED	 		*/
#define NUM_ROOM_FLAGS          32


/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		(1 << 3)   /* Lock can't be picked	*/
#define EX_SECRET		(1 << 4)   /* The door is secret	*/
#define EX_BREAKABLE		(1 << 5)   /* You can fall through	*/


/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE		0	/* Indoors			*/
#define SECT_CITY		1	/* In a city			*/
#define SECT_FIELD		2	/* In a field			*/
#define SECT_FOREST		3	/* In a forest			*/
#define SECT_HILLS		4	/* In the hills			*/
#define SECT_MOUNTAIN		5	/* On a mountain		*/
#define SECT_WATER_SWIM		6	/* Swimmable water		*/
#define SECT_WATER_NOSWIM	7	/* Water - need a boat		*/
#define SECT_UNDERWATER		8	/* Underwater			*/
#define SECT_FLYING		9	/* Wheee!			*/
#define SECT_DESERT		10	/* In the desert		*/
#define SECT_ICELAND		11	/* Cold				*/
#define SECT_OCEAN		12	/* Water - need a ship		*/
#define SECT_LADDER		13	/* Things fall, no grapple	*/
#define SECT_TREE		14	/* In a tree			*/
#define SECT_ASTRAL		15	/* Yikes			*/
#define SECT_SWAMP		16	/* Yucky dismal swamp		*/
#define NUM_ROOM_SECTORS	17


/* char and mob-related defines *****************************************/

/* mob format in mobfile (read in db.c) */
#define MOBFORMAT_SIMPLE	0
#define MOBFORMAT_EXPANDED	1


/* races */
#define RACE_UNDEFINED		-1
#define RACE_HUMAN		0
#define RACE_ELF		1
#define RACE_HOBBIT		2
#define RACE_DWARF		3
#define RACE_ORC		4
#define RACE_DROW		5
#define RACE_BUGBEAR		6
#define RACE_MINOTAUR		7
#define RACE_TROLL		8
#define RACE_GIANT		9
#define RACE_DRAGON		10 
#define RACE_UNDEAD		11 
#define RACE_HALFELF		12
#define RACE_GNOME		13
#define RACE_ANGEL		14
#define RACE_DUERGAR		15
#define RACE_THRIKREEN		16
#define NUM_RACES		17 /* This must be the number of races!! */

#define MAX_RACE_STAT_ADJUST    14 /* this must be the number of stats
                                      that can be changed (6) plus the
                                      number of permaffects the race can
                                      have (4) plus the number of permaffect2's
                                      the race can have (4) (6 +4 +4 = 14) */

/* PC clans - clan defines attached to clan names are all in clan.c now */
#define CLAN_UNDEFINED		-1
#define CLAN_NOCLAN		0
#define CLAN_PLEDGE		3
#define CLAN_BLACKLISTED	4
#define NUM_CLANS		20


/* PC clan levels, just four for now */
/* ONE day, we oughtta add a bunch with custom names and special priveleges */
/* Make sure none of these are -1! */
#define CLAN_LVL_MEMBER		0
#define CLAN_LVL_LEADER		1
#define CLAN_LVL_PATRON		2
#define CLAN_LVL_CHAMPION	3


/* PC quest status */
#define QUEST_OFF		0
#define QUEST_NORMAL		1
#define QUEST_ENROLLED		2
#define QUEST_SURVIVAL		3
#define QUEST_PKQUEST		4
#define QUEST_DEATHQUEST	5


/* classes */
#define CLASS_UNDEFINED		-1		/* NPCs */
#define CLASS_MAGIC_USER	0
#define CLASS_CLERIC		1
#define CLASS_THIEF		2
#define CLASS_WARRIOR		3
#define CLASS_BARD       	4
#define CLASS_DEATHKNIGHT	5
#define CLASS_DRUID		6
#define CLASS_VAMPIRE		7
#define CLASS_MONK		8
#define CLASS_LICH		9
#define CLASS_SERAPH		10
#define CLASS_CHERUB		11
#define CLASS_VALKYRIE		12
#define CLASS_UNUSED13		13
#define CLASS_UNUSED14		14
#define CLASS_UNUSED15		15
#define CLASS_UNUSED16		16
#define CLASS_UNUSED17		17
#define CLASS_UNUSED18		18
#define CLASS_UNUSED19		19
#define CLASS_UNUSED20		20
#define NUM_CLASSES		21  /* This must be the number of classes!! */

#if(0)
#define NUM_NEWBIE_GEAR		20  /* coincidence */
#endif


/* NPC classes (currently unused - feel free to implement!) */
/*
#define CLASS_OTHER       0
#define CLASS_UNDEAD      1
#define CLASS_HUMANOID    2
#define CLASS_ANIMAL      3
#define CLASS_DRAGON      4
#define CLASS_GIANT       5
#define CLASS_ELEMENTAL   6
*/


/* Sex */
#define SEX_NEUTRAL	0
#define SEX_MALE	1
#define SEX_FEMALE	2
#define NUM_GENDERS	3


/* Positions */
#define POS_DEAD	0	/* dead			*/
#define POS_MORTALLYW	1	/* mortally wounded	*/
#define POS_INCAP	2	/* incapacitated	*/
#define POS_STUNNED	3	/* stunned		*/
#define POS_SLEEPING	4	/* sleeping		*/
#define POS_RESTING	5	/* resting		*/
#define POS_SITTING	6	/* sitting		*/
#define POS_FIGHTING	7	/* fighting		*/
#define POS_STANDING	8	/* standing		*/
#define POS_SEARCHING	9	/* searching		*/
#define NUM_POSITIONS	10

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_MASTER	(1 << 16)  /* Player is deathquest master       */
#define PLR_NOPAWN	(1 << 17)  /* Player abused pawn		*/

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_UNUSED4      (1 << 4)  /* Unused				*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_EVIL	 (1 << 8)  /* auto attack evil PC's		*/
#define MOB_AGGR_GOOD	 (1 << 9)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 10) /* auto attack neutral PC's		*/
#define MOB_MEMORY	 (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	 (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	 (1 << 13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (1 << 15) /* Mob can't be slept		*/
#define MOB_NOBASH	 (1 << 16) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	 (1 << 17) /* Mob can't be blinded		*/
#define MOB_SHOPKEEPER   (1 << 18) /* Mob sells items (experimental)	*/
#define MOB_NOPLINK      (1 << 19) /* Mob cant be plinked and moved     */
#define MOB_SWIMMER      (1 << 20) /* Mob can also swim in water        */
#define MOB_NOWALK       (1 << 21) /* Mob cannot walk on land           */
#define MOB_WILL_SELF_PURGE (1 << 22) /* Mob will self purge next hit   */
#define MOB_NOTTHERE     (1 << 23) /* Mob is not really there (roomprog)*/
#define MOB_NOBLOCK      (1 << 24) /* Mob is fucking unblockable */
#define MOB_SAFE         (1 << 25) /* Mob is not attackable */
#define MOB_DSHOPKEEPER  (1 << 26) /* Mob is a new-style shopkeeper	*/
#define MOB_QMASTER      (1 << 27) /* Mob is the Autoquestor	*/
#define NUM_MOB_FLAGS	 28


/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(1 << 3)  /* Can't receive tells		*/
#define PRF_DISPHP	(1 << 4)  /* Display hit points in prompt	*/
#define PRF_DISPMANA	(1 << 5)  /* Display mana points in prompt	*/
#define PRF_DISPMOVE	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_UNUSED_09	(1 << 9)  /* *** RESERVED ***			*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR       (1 << 13) /* Color				*/
#define PRF_UNUSED_14   (1 << 14) /* *** RESERVED ***			*/
#define PRF_NOWIZ	(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	(1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	(1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_AUTOLOOT	(1 << 22) /* Players who get the kill get items	*/
#define PRF_AUTOSAC	(1 << 23) /* Sacrifice empty corpses		*/
#define PRF_AUTOGOLD	(1 << 24) /* Get all gold corpse		*/
#define PRF_AUTOSPLIT	(1 << 25) /* Split gold with group		*/
#define PRF_AUTODIRS	(1 << 26) /* Show directions (Old Style Exits)	*/
#define PRF_DISPDIAG	(1 << 27) /* Display diagnose in the prompt	*/
#define PRF_AUTOASSIST	(1 << 28) /* Automatically assist grouped chars */
#define PRF_UNUSED_29	(1 << 29) /* *** RESERVED ***			*/
#define PRF_DISPGOLD	(1 << 30) /* See current gold on the prompt	*/
#define PRF_DISPEXP	(1 << 31) /* See the amount of exps to level	*/


/* Preference2 flags: used by char_data.player_specials.pref2 */
#define PRF2_NOCLAN	(1 << 0)  /* Hear your clan channel		*/
#define PRF2_AWAY	(1 << 1)  /* Whether or not you are afk		*/
#define PRF2_ANONYMOUS	(1 << 2)
#define PRF2_SHOW_DAMAGE (1 << 3)
#define PRF2_ANONYMOUS3	(1 << 4)
#define PRF2_AUTOSCAN	(1 << 5)  /* Automatically scan when you look	*/
#define PRF2_BATTLEBRIEF (1 << 6) /* Keep the multiple attacks to one line */
#define PRF2_AUTOGROUP	(1 << 7)  /* See group report every round       */
#define PRF2_NOMUSIC	(1 << 8)  /* Don't hear the music channel	*/
#define PRF2_AUTOMAP    (1 << 9)  /* Map whereever you go               */

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_RAGE              (1 << 2)	   /* Char is raged             */
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1 << 6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed		*/
#define AFF_INFRAVISION       (1 << 10)	   /* Char can see in dark	*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1 << 13)	   /* Char protected from good  */
#define AFF_SLEEP             (1 << 14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK	      (1 << 15)	   /* Char can't be tracked	*/
#define AFF_HASTE 	      (1 << 16)	   /* Char is hasted      	*/
#define AFF_SLOW	      (1 << 17)	   /* Char is slowed    	*/
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly	*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden		*/
#define AFF_AID		      (1 << 20)	   /* Char is aided     	*/
#define AFF_CHARM             (1 << 21)	   /* Char is charmed		*/
#define AFF_FLY               (1 << 22)    /* Char is flying            */
#define AFF_MAGIC_RESIST      (1 << 23)    /* Char resists magic        */
#define AFF_STONESKIN         (1 << 24)    /* Char has stoneskin	*/
#define AFF_INSPIRE           (1 << 25)    /* Char is inspired          */
#define AFF_MIRROR_IMAGE      (1 << 26)    /* Char has images           */
#define AFF_BLESS             (1 << 27)    /* Char is blessed           */
#define AFF_STUN              (1 << 28)    /* Char is stunned           */
#define AFF_BLOCK             (1 << 29)    /* Char is blocking          */
#define AFF_GAUGE             (1 << 30)    /* Char is gauging           */
#define AFF_DETECT_ALIGN      (1 << 31)    /* Char is sensitive to align*/
#define NUM_AFF_FLAGS	      32

#define AFF2_JARRED	      (1 << 0)     /* Char is jarred 		*/
#define AFF2_POISON2          (1 << 1)
#define AFF2_ICE_SHIELD       (1 << 2)
#define AFF2_QUESTOR          (1 << 3)
#define AFF2_WASCHARMED       (1 << 4)
#define AFF2_WEBBED           (1 << 5)
#define AFF2_RES_FIRE         (1 << 6)
#define AFF2_RES_COLD         (1 << 7)
#define AFF2_RES_ELEC         (1 << 8)
#define AFF2_RES_ACID         (1 << 9)
#define AFF2_FIRESHIELD       (1 << 10)
#define AFF2_MANASHELL        (1 << 11)
#define NUM_AFF2_FLAGS        12

#define AFF3_TEST0            (1 << 0)
#define AFF3_TEST1            (1 << 1)
#define NUM_AFF3_FLAGS        2

#define AFF4_TEST0            (1 << 0)
#define AFF4_TEST1            (1 << 1)
#define NUM_AFF4_FLAGS        2

#define AFF5_TEST0            (1 << 0)
#define AFF5_TEST1            (1 << 1)
#define NUM_AFF5_FLAGS        2

#define AFF6_TEST0            (1 << 0)
#define AFF6_TEST1            (1 << 1)
#define NUM_AFF6_FLAGS        2

#define AFF7_TEST0            (1 << 0)
#define AFF7_TEST1            (1 << 1)
#define NUM_AFF7_FLAGS        2

#define AFF8_TEST0            (1 << 0)
#define AFF8_TEST1            (1 << 1)
#define NUM_AFF8_FLAGS        2

#define AFF9_TEST0            (1 << 0)
#define AFF9_TEST1            (1 << 1)
#define NUM_AFF9_FLAGS        2

#define AFF10_TEST0           (1 << 0)
#define AFF10_TEST1           (1 << 1)
#define NUM_AFF10_FLAGS       2


/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0		/* Playing - Nominal state	*/
#define CON_CLOSE	 1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	 7		/* Sex?				*/
#define CON_QCLASS	 8		/* Class?			*/
#define CON_RMOTD	 9		/* PRESS RETURN after MOTD	*/
#define CON_MENU	 10		/* Your choice: (main menu)	*/
#define CON_EXDESC	 11		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12		/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 13		/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   14		/* Verify new password		*/
#define CON_DELCNF1	 15		/* Delete confirmation 1	*/
#define CON_DELCNF2	 16		/* Delete confirmation 2	*/
#define CON_QRACE        17             /* Race?			*/
#define CON_QCOLOR       18		/* Ansi Color?			*/
/* HACKED to add oasis-olc */
#define CON_OEDIT        19             /* OLC mode - object edit       */
#define CON_REDIT        20             /* OLC mode - room edit         */
#define CON_ZEDIT        21             /* OLC mode - zone info edit    */
#define CON_MEDIT        22             /* OLC mode - mobile edit       */
#define CON_SEDIT        23             /* OLC mode - shop edit         */
/* end of hack */
#define CON_DEAF         24		/* MOBProg: for mpsilent	*/
#define CON_BATTLE_VRFY  25             /* Verify battle (PK aren)      */
#define CON_EMAIL        26		/* Entering e-mail addy		*/


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17
#define WEAR_READY     18
#define WEAR_WIELD_2   19
#define WEAR_PRIZE     20
#define WEAR_EARS      21
#define WEAR_FACE      22
#define NUM_WEARS      23	/* This must be the # of eq positions!! */

#define THRI_WEAR_LIGHT		0
#define THRI_WEAR_FINGER_UL	1
#define THRI_WEAR_FINGER_UR	2
#define THRI_WEAR_NECK_1	3
#define THRI_WEAR_NECK_2	4
#define THRI_WEAR_FINGER_LL	5
#define THRI_WEAR_FINGER_LR	6
#define THRI_WEAR_SHIELD_L	7
#define THRI_WEAR_SHIELD_R	8
#define THRI_WEAR_WAIST		9
#define THRI_WEAR_HOLD_L	10
#define THRI_WEAR_WIELD_R	11
#define THRI_WEAR_HOLD_R	12
#define THRI_WEAR_READY		13
#define THRI_WEAR_WIELD_L	14
#define THRI_WEAR_PRIZE 	15
#define THRI_WEAR_FACE		16
#define NUM_THRI_WEARS		17	/* This must be the # of eq positions!! */


/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT		1	/* Item is a light source	*/
#define ITEM_SCROLL		2	/* Item is a scroll		*/
#define ITEM_WAND		3	/* Item is a wand		*/
#define ITEM_STAFF		4	/* Item is a staff		*/
#define ITEM_WEAPON		5	/* Item is a weapon		*/
#define ITEM_FIREWEAPON		6	/* Ranged weapon 		*/
#define ITEM_MISSILE		7	/* Unimplemented		*/
#define ITEM_TREASURE		8	/* Item is a treasure, not gold	*/
#define ITEM_ARMOR		9	/* Item is armor		*/
#define ITEM_POTION		10 	/* Item is a potion		*/
#define ITEM_WORN		11	/* Unimplemented		*/
#define ITEM_OTHER		12	/* Misc object			*/
#define ITEM_TRASH		13	/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP		14	/* Unimplemented		*/
#define ITEM_CONTAINER		15	/* Item is a container		*/
#define ITEM_NOTE		16	/* Item is note 		*/
#define ITEM_DRINKCON		17	/* Item is a drink container	*/
#define ITEM_KEY		18	/* Item is a key		*/
#define ITEM_FOOD		19	/* Item is food			*/
#define ITEM_MONEY		20	/* Item is money (gold)		*/
#define ITEM_PEN		21	/* Item is a pen		*/
#define ITEM_BOAT		22	/* Item is a boat		*/
#define ITEM_FOUNTAIN		23	/* Item is a fountain		*/
#define ITEM_INSTRUMENT		24	/* Item is an instrument        */
#define ITEM_PILL		25	/* Item is a pill		*/
#define ITEM_SEED		26	/* Item is a seed		*/
#define ITEM_PORTAL		27	/* Item is a magical portal	*/
#define NUM_ITEM_TYPES		28


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/
#define ITEM_WEAR_READY         (1 << 15) /* Can be readied             */
#define ITEM_WEAR_PRIZE		(1 << 16) /* Is a quest-prize item	*/
#define ITEM_WEAR_EARS		(1 << 17) /* Can be worn on ears	*/
#define ITEM_WEAR_FACE		(1 << 18) /* Can be worn on face	*/
#define NUM_ITEM_WEARS		19


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW		(1 << 0)  /* Item is glowing		*/
#define ITEM_HUM		(1 << 1)  /* Item is humming		*/
#define ITEM_NORENT		(1 << 2)  /* Item cannot be rented	*/
#define ITEM_NODONATE		(1 << 3)  /* Item cannot be donated	*/
#define ITEM_NOINVIS		(1 << 4)  /* Item cannot be made invis	*/
#define ITEM_INVISIBLE		(1 << 5)  /* Item is invisible		*/
#define ITEM_MAGIC		(1 << 6)  /* Item is magical		*/
#define ITEM_NODROP		(1 << 7)  /* Item is cursed: can't drop	*/
#define ITEM_BLESS		(1 << 8)  /* Item is blessed		*/
#define ITEM_ANTI_GOOD		(1 << 9)  /* Not usable by good people	*/
#define ITEM_ANTI_EVIL		(1 << 10) /* Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL	(1 << 11) /* Not usable by neutral people */
#define ITEM_ANTI_MAGIC_USER	(1 << 12) /* Not usable by mages	*/
#define ITEM_ANTI_CLERIC	(1 << 13) /* Not usable by clerics	*/
#define ITEM_ANTI_THIEF		(1 << 14) /* Not usable by thieves	*/
#define ITEM_ANTI_WARRIOR	(1 << 15) /* Not usable by warriors	*/
#define ITEM_NOSELL		(1 << 16) /* Shopkeepers won't touch it	*/
#define ITEM_ANTI_BARD		(1 << 17) /* Not usable by bards	*/
#define ITEM_ANTI_DRAGON	(1 << 18) /* Not usable by dragons	*/
#define ITEM_VETERAN		(1 << 19) /* Must be 25th level		*/
#define ITEM_HERO		(1 << 20) /* Must be 35th level		*/
#define ITEM_CHAMPION		(1 << 21) /* Must be 42nd level		*/
#define ITEM_CONCEALED		(1 << 22) /* Item is concealed		*/
#define ITEM_ANTI_DRUID		(1 << 23) /* Not usable by druids	*/
#define ITEM_SUPERCURSED	(1 << 24) /* Doesnt ever come off	*/
#define ITEM_ANTI_MONK		(1 << 25) /* Monks can't use		*/
#define ITEM_QUEST		(1 << 26) /* Item is quest flagged	*/
#define ITEM_NEWBIE		(1 << 27) /* Must be below 16th level	*/
#define ITEM_NOLOCATE		(1 << 28) /* Can't use 'locate object'  */
#define ITEM_NOREMOVE		(1 << 29) /* Can't use 'locate object'  */
#define ITEM_AUTOQUEST		(1 << 30) /* Can't use 'locate object'  */
#define ITEM_PRICEOK		(1 << 31) /* Price has been validated   */
#define NUM_ITEM_FLAGS		32        /* EEEK it's full!            */

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to constitution	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/
/* 25 free for use */
#define APPLY_RACE             26       /* Added, apply to race		*/
#define APPLY_MAX              27	/* MUST be the last value	*/
#define NUM_APPLIES            27


/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER	0
#define LIQ_BEER	1
#define LIQ_WINE	2
#define LIQ_ALE		3
#define LIQ_DARKALE	4
#define LIQ_WHISKY	5
#define LIQ_LEMONADE	6
#define LIQ_FIREBRT	7
#define LIQ_LOCALSPC	8
#define LIQ_SLIME	9
#define LIQ_MILK	10
#define LIQ_TEA		11
#define LIQ_COFFE	12
#define LIQ_BLOOD	13
#define LIQ_SALTWATER	14
#define LIQ_CLEARWATER	15
#define LIQ_COKE	16
#define LIQ_PEPSI	17
#define LIQ_MOONSHINE	18
#define LIQ_ICETEA	19
#define LIQ_ROOTBEER	20
#define LIQ_VEGGIE	21
#define LIQ_JUICE	22
#define LIQ_TEQUILA	23
#define LIQ_KAHLUA	24
#define LIQ_RUM		25
#define LIQ_CHAMPAGNE	26
#define LIQ_DWARVEN	27
#define LIQ_FOUNTAIN	28
#define NUM_LIQ_TYPES	29

/* Seed types - value[0] */
#define SEED_LOAD_OBJ		0
#define SEED_LOAD_MOB		1


/* Portal trigger types - value[1] */
#define PORTAL_ENTER		0
#define PORTAL_BOARD		1
#define PORTAL_PUSH		2
#define PORTAL_PULL		3
#define PORTAL_LOOK		4
#define PORTAL_GET		5
#define PORTAL_CLIMB		6
#define NUM_PORTAL_TYPES	7


/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2

/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3


/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5

/* other #defined constants **********************************************/

#define LVL_IMPL	60	/* 54 */
#define LVL_GRGOD	59
#define LVL_GOD		58	/* 53 */
#define LVL_DEITY	57
#define LVL_DEMI	56	/* 52 */
#define LVL_LGOD	55
#define LVL_SUPR	54
#define LVL_AVTR	53
#define LVL_IMMORT	51	/* immortal and builder are the same */
#define LVL_BUILDER	51
#define LVL_LOWBIE	8

#define LVL_FREEZE	LVL_SUPR
#define LVL_DO_NOT_DISPLAY	-1	/* for mobprogs essentially */

#define NUM_OF_DIRS	11	/* number of directions in a room (nsewud) */

#define OPT_USEC	100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_ACT	(3 RL_SEC)
#define PULSE_MOBILE    (3 RL_SEC)
#define PULSE_VIOLENCE  (3 RL_SEC)
#define PULSE_SEARCH    (10 RL_SEC)

#define SMALL_BUFSIZE		1024
#define LARGE_BUFSIZE		(12 * 1024)
#define GARBAGE_SPACE		32

#define MAX_STRING_LENGTH	16384	/* Used to be 8192 */
#define MAX_INPUT_LENGTH	1024  /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	2048  /* Max size of *raw* input */
#define MAX_MESSAGES		60
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH		10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define PRETITLE_SEP_CHAR	'|' /* changing this will mess up pretitles! */
#define HOST_LENGTH		30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH		240 /* Used in char_file_u *DO*NOT*CHANGE* */
/* HACKED to add more languages (was 3) */
#define MAX_TONGUE		50   /* Used in char_file_u *DO*NOT*CHANGE* */
/* end of hack */
#define MAX_SKILLS		2000 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		32  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT		6 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_SPELL_AFFECT	3 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define NUM_SAVING_THROW_TYPES 5

/* PETS */
#define MAX_PET_SKILLS		100 /* DO NOT CHANGE THIS */
/* END of PETS */

#define PROTO_POTION 21 /* Used in do_brew, DO NOT CHANGE! */

#define MIN_PAWN_PRICE 100
#define PAWN_COMMISSION 15
#define PAWNSHOP_VNUM 3060
#define PAWNSHOP_KEEPER_VNUM 16299
#define PAWNSHOP_IDENT 500


/***********************************************************************
 * Structures                                                          *
 **********************************************************************/


typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
typedef char			byte;

typedef sh_int	room_num;
typedef sh_int	obj_num;


/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};


/* object-related structures ******************************************/


/* object flags; used in obj_data */
struct obj_flag_data {
   int	value[4];	/* Values of the item (see list)    */
   byte type_flag;	/* Type of item			    */
   int	wear_flags;	/* Where you can wear it	    */
   int	extra_flags;	/* If it hums, glows, etc.	    */
   int	weight;		/* Weight what else                 */
   int	cost;		/* Value when sold (gp.)            */
   int	cost_per_day;	/* Cost to keep pr. real day        */
   int	timer;		/* Timer for object                 */
   long	bitvector;	/* To set chars bits                */
   long bitvector2;
   long bitvector3;
   long bitvector4;
   long bitvector5;
   long bitvector6;
   long bitvector7;
   long bitvector8;
   long bitvector9;
   long bitvector10;
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
   byte location;       /* Which ability to change (APPLY_XXX) */
   sbyte modifier;      /* How much it changes by              */
};

/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_spell_type {
   ush_int spelltype;   /*number of spell*/
   ush_int level;       /*level of spell*/
   ush_int percentage;  /*percentage of success*/
};

#define OBJ_GET_PROG 	0
#define OBJ_USE_PROG 	1
#define OBJ_ROT_PROG 	2
#define OBJ_DROP_PROG 	3
#define OBJ_IDENT_PROG	4
#define OBJ_WEAR_PROG	5
#define OBJ_WORN_PROG	6

struct obj_prog_data {
  ubyte progtype;		/* What type of prog is this? */
  char *prog;			/* The prog (lines sep'd by '\n') */
  struct obj_prog_data *next;
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_num item_number;		/* Where in data-base			*/
   room_num in_room;		/* In what room -1 when conta/carr	*/
   int count;			/* for keeping count in coalescing,
				   temporary variable, 0 means
				   the obj has been counted already
				   and can be skipped  */

   struct obj_flag_data obj_flags;/* Object information               */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */
   struct obj_spell_type spell_affect[MAX_SPELL_AFFECT];

   char	*name;                    /* Title of object :get etc.        */
   char *description;             /* When in room                     */
   char *short_description;       /* when worn/carry/in cont.         */
   char *action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;     /* Worn by?                         */
   sh_int worn_on;                /* Worn where?                      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */

   int dynamic_x;                 /* DYNAMIC */
   int dynamic_y;
   int dynamic_z;
   
   long progtypes;                /* What kind of progs do we have?   */
   struct obj_prog_data *progs;   /* Linked list of objprogs          */
   char *op_strs[10];             /* String vars for progs            */
   long op_nums[10];              /* Numeric vars for progs           */
};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
struct obj_file_elem {
   obj_num item_number;

   int	value[4];
   int	extra_flags;
   int	weight;
   int	timer;
   long	bitvector;
   long	bitvector2;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
   struct obj_spell_type spell_affect[MAX_SPELL_AFFECT];
};


/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
   int	time;
   int	rentcode;
   int	net_cost_per_diem;
   int	gold;
   int	account;
   int	nitems;
   int	spare0;
   int	spare1;
   int	spare2;
   int	spare3;
   int	spare4;
   int	spare5;
   int	spare6;
   int	spare7;
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
   char	*general_description;       /* When look DIR.			*/

   char	*keyword;		/* for open/close			*/

   sh_int exit_info;		/* Exit info				*/
   obj_num key;			/* Key's number (-1 for no key)		*/
   room_num to_room;		/* Where direction leads (NOWHERE)	*/
};


/* ================== Memory Structure for room ======================= */
struct room_data {
   room_num number;		/* Rooms number	(vnum)		      */
   sh_int zone;                 /* Room zone (for resetting)          */
   int	sector_type;            /* sector type (move/hide)            */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   int room_flags;		/* DEATH,DARK ... etc                 */

   byte light;                  /* Number of lightsources in room     */
   SPECIAL(*func);

   struct obj_data *contents;   /* List of items in room              */
   struct obj_data *hidden;	/* List of hidden items in room       */
   struct char_data *people;    /* List of NPC / PC in room           */
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;

/* MOBProgram foo */
struct mob_prog_act_list {
  struct mob_prog_act_list *next;
  char *buf;
  struct char_data *ch;
  struct obj_data *obj;
  void *vo;
};

typedef struct mob_prog_act_list MPROG_ACT_LIST;

struct mob_prog_data {
  struct mob_prog_data *next;
  int type;
  char *arglist;
  char *comlist;
};

typedef struct mob_prog_data MPROG_DATA;

extern bool MOBTrigger;
extern int MOBHandled;

#define ERROR_PROG		-1
#define IN_FILE_PROG		0
#define ACT_PROG		(1 << 0)
#define SPEECH_PROG		(1 << 1)
#define RAND_PROG		(1 << 2)
#define FIGHT_PROG		(1 << 3)
#define DEATH_PROG		(1 << 4)
#define HITPRCNT_PROG		(1 << 5)
#define ENTRY_PROG		(1 << 6)
#define GREET_PROG		(1 << 7)
#define ALL_GREET_PROG		(1 << 8)
#define GIVE_PROG		(1 << 9)
#define BRIBE_PROG		(1 << 10)
#define SOCIAL_PROG		(1 << 11)
#define COMMAND_PROG		(1 << 12)
#define SCRIPT_PROG		(1 << 13)
#define TIME_PROG		(1 << 14)
#define KILL_PROG		(1 << 15)
#define ALL_SPEECH_PROG		(1 << 16)
#define GREET_EVERY_PROG	(1 << 17)
#define ALL_GREET_EVERY_PROG	(1 << 18)
#define BROADCAST_PROG		(1 << 19)
#define SPELL_PROG		(1 << 20)
#define LOAD_PROG		(1 << 21)

/* end of MOBProg foo */
/* HACKED for MOBProg global vars */
struct mobprog_var_data {
  char *name;
  int val;
  struct mobprog_var_data *next;
};
/* END of HACK */

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   byte hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int	played;     /* This is the total accumulated time played in secs */
};


/* general player-related info, usually PC's and NPC's */
struct char_player_data {
   char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
   char	*name;	       /* PC / NPC s name (kill ...  )         */
   char	*short_descr;  /* for NPC 'actions'                    */
   char	*long_descr;   /* for 'look'			       */
   char	*description;  /* Extra descriptions                   */
   char	*title;        /* PC / NPC's title                     */
   byte sex;           /* PC / NPC's sex                       */
   byte class;         /* PC / NPC's class		       */
   byte level;         /* PC / NPC's level                     */
   int	hometown;      /* PC s Hometown (zone)                 */
   struct time_data time;  /* PC's AGE in days                 */
   ubyte weight;       /* PC / NPC's weight                    */
   ubyte height;       /* PC / NPC's height                    */
   int recallpoint;    /* Room PC will recall into             */
};


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
   sbyte str;
   sbyte str_add;      /* 000 - 100 if strength 18             */
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
   sh_int mana;
   sh_int max_mana;     /* Max move for PC/NPC			   */
   sh_int hit;
   sh_int max_hit;      /* Max hit for PC/NPC                      */
   sh_int move;
   sh_int max_move;     /* Max move for PC/NPC                     */

   sh_int armor;        /* Internal -100..100, external -10..10 AC */
   int	gold;           /* Money carried                           */
   int	bank_gold;	/* Gold the char has in a bank account	   */
   int	exp;            /* The experience of the player            */

   sbyte hitroll;       /* Any bonus or penalty to the hit roll    */
   sbyte damroll;       /* Any bonus or penalty to the damage roll */
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
   int	alignment;		/* +-1000 for alignments                */
   long	idnum;			/* player's idnum; -1 for mobiles	*/
   long	act;			/* act flag for NPC's; player flag for PC's */

   long	affected_by;		/* Bitvector for spells/skills affected by */
   long affected_by2;
   long affected_by3;
   long affected_by4;
   long affected_by5;
   long affected_by6;
   long affected_by7;
   long affected_by8;
   long affected_by9;
   long affected_by10;

   sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)		*/
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
   struct char_data *fighting;	/* Opponent				*/
   struct char_data *hunting;	/* Char hunted by this char		*/

   byte position;		/* Standing, fighting, sleeping, etc.	*/
   byte blocked;		/* Attacks are blocked			*/
   byte cant_wimpy;		/* For one reason or another cant flee  */

   int	carry_weight;		/* Carried weight			*/
   byte carry_items;		/* Number of items carried		*/
   int	timer;			/* Timer for update			*/

   int color_stack[10];		/* color to reset back to		*/
				/* if they blow over 4 colors, the set	*/
				/* goes to 0, and the stack pointer to 0 */
				/* the extra value is to take the extra */
				/* increment of the stack before being  */
				/* fixed.				*/
   int color_stack_index;	/* index to the stack			*/

   int dynamic_x;		/* DYNAMIC */
   int dynamic_y;
   int dynamic_z;

   int num_images;		/* mirror image				*/
   int num_layers;		/* stoneskin				*/
   int num_specials;

   long tempbestkillrank;        /* for best kill */
   int tempbestkill;            /*               */
   long temparchfoerank;         /* arch foe */
   int temparchfoe;              /*          */

   struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
   byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
   byte spells_to_learn;	/* How many can you learn yet this level*/
   bool talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
   int	wimp_level;		/* Below this # of hit points, flee!	*/
   byte freeze_level;		/* Level of god who froze char, if any	*/
   sh_int invis_level;		/* level of invisibility		*/
   room_num load_room;		/* Which room to place char in		*/
   long	pref;			/* preference flags for PC's.		*/
   ubyte bad_pws;		/* number of bad password attemps	*/
   sbyte conditions[3];         /* Drunk, full, thirsty			*/

   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */

   ubyte spare1;
   byte race;			/* race number */
   byte clan;			/* clan number */
   byte quest_enroll;		/* for level of quest enrollment */
   byte clan_level;		/* clan level */
   int nextquest;			/* for MIRROR_IMAGE */
   long pref2;			/* extended preference flags for PC's	*/
   int olc_zone;		/* HACKED to add oasis-olc, was spare8 */
/* AUTOQUEST */
   int countdown;			/* archfoe */
   int questobj; 		/* best kill */
   int color_prefs[40];		/* color preferences */
   int questmob;
   long questpoints;
   long	bestkillrank;
   long	spare19;
   long	spare20;
   long spare21;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
   struct player_special_data_saved saved;

   char	*poofin;		/* Description on arrival of a god.     */
   char	*poofout;		/* Description upon a god's exit.       */
   struct alias *aliases;	/* Character's aliases			*/
   long last_tell;		/* idnum of last tell from		*/
   long killer_to_forgive;	/* idnum of person who tried to kill them */
   void *last_olc_targ;		/* olc control				*/
   int last_olc_mode;		/* olc control				*/
   char *email;			/* email address			*/
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
   byte last_direction;     /* The last direction the monster went     */
   int	attack_type;        /* The Attack Type Bitvector for NPC's     */
   byte default_pos;        /* Default position for NPC                */
   memory_rec *memory;	    /* List of attackers to remember	       */
   byte damnodice;          /* The number of damage dice's	       */
   byte damsizedice;        /* The size of the damage dice's           */
   int  mobformat;          /* Is the mob complex instead of simple?   */
                            /* Only complex mobs have race/class/stats */
   byte race;               /* Usually RACE_UNDEFINED, but sometimes.. */
   int kills, deaths;       /* Track kills & deaths for this mob       */
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
   sh_int type;          /* The type of spell that caused this      */
   sh_int duration;      /* For how long its effects will last      */
   sbyte modifier;       /* This is added to apropriate ability     */
   byte location;        /* Tells which ability to change(APPLY_XXX)*/
   long	bitvector;       /* Tells which bits to set (AFF_XXX)       */
   long bitvector2;      /* Tells which bits to set (AFF2_XXX)      */
   long bitvector3;
   long bitvector4;
   long bitvector5;
   long bitvector6;
   long bitvector7;
   long bitvector8;
   long bitvector9;
   long bitvector10;
   struct affected_type *next;
};


/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};

/* PETS */
struct pet_specials {
  int loyalty;				/* How much do you like your owner? */
  int skills[MAX_PET_SKILLS];		/* What can you do?                 */
  int petnum;				/* What pet vnum?                   */
  bool mounted;
};
/* END of PETS */

/* Structures related to new-style shops */
struct unlimited_sell_info {
  int vnum;
  struct unlimited_sell_info *next;
};

struct limited_sell_info {
  struct obj_data *obj;
  struct limited_sell_info *next;
};

struct shopkeeper_info {
  float buy_rate, sell_rate;
  int flags, types;
  int gold;
  int open, close, shop, home;
  struct unlimited_sell_info *unlimited;
  struct limited_sell_info *limited;
  char *msgs[24];
};
/* end of new-style shop structs */

/* ================== Structure for player/non-player ===================== */
struct char_data {
   int pfilepos;			 /* playerfile pos		  */
   sh_int nr;                            /* Mob's rnum			  */
   room_num in_room;                     /* Location (real room number)	  */
   room_num was_in_room;		 /* location for linkdead people  */

   struct char_player_data player;       /* Normal data                   */
   struct char_ability_data real_abils;	 /* Abilities without modifiers   */
   struct char_ability_data aff_abils;	 /* Abils with spells/stones/etc  */
   struct char_point_data points;        /* Points                        */
   struct char_special_data char_specials;	/* PC/NPC specials	  */
   struct player_special_data *player_specials; /* PC specials		  */
   struct mob_special_data mob_specials;	/* NPC specials		  */

   struct affected_type *affected;       /* affected by what spells         */
                                         /* DO NOT EXPAND FOR AFF2_XXX etc: */
                                         /* this is the list of all affects,*/
                                         /* regardless of what bitvectors   */
                                         /* are set                         */
 
   struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

   struct obj_data *carrying;            /* Head of list                  */
   struct descriptor_data *desc;         /* NULL for mobiles              */

   struct char_data *next_in_room;     /* For room->people - list         */
   struct char_data *next;             /* For either monster or ppl-list  */
   struct char_data *next_fighting;    /* For fighting list               */

   struct follow_type *followers;        /* List of chars followers       */
   struct char_data *master;             /* Who is char following?        */
   
   /* PETS */
   struct descriptor_data *ownerd;	/* Who's pet is this?		*/
   struct char_data *owner;
   struct char_data *pet;		/* Your pet - also in desc	*/
   struct pet_specials *petdata;	/* Special pet info */
   /* END of PETS */

   struct shopkeeper_info *shopinfo;	/* New-style shopkeeper info	*/
   
   /* MOBprog foo */
   MPROG_ACT_LIST *mpact;
   int mpactnum;
   int mpscriptnum;                      /* which script is running       */
   int mpscriptstep;                     /* what step of the script       */ 
   struct char_data *mpscriptactor;      /* who triggered script          */
   struct char_data *mpnextscript;       /* for script list               */
};
/* ====================================================================== */


/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[EXDSCR_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   byte sex;
   byte class;
   byte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int	played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;

   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];
   struct affected_type affected2[MAX_AFFECT];
   struct affected_type affected3[MAX_AFFECT];
   struct affected_type affected4[MAX_AFFECT];
   struct affected_type affected5[MAX_AFFECT];
   struct affected_type affected6[MAX_AFFECT];
   struct affected_type affected7[MAX_AFFECT];
   struct affected_type affected8[MAX_AFFECT];
   struct affected_type affected9[MAX_AFFECT];
   struct affected_type affected10[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	/* host of last logon */
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct descriptor_data {
   int	descriptor;		/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   int	connected;		/* mode of 'connectedness'		*/
   int	wait;			/* wait for how many loops		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char	*showstr_head;		/* for paging through texts		*/
   char	*showstr_point;		/*		-			*/
   char	**str;			/* for the modify-str system		*/
   int	max_str;		/*		-			*/
   long	mail_to;		/* name for mail system			*/
   int prompt_mode;            /* control of prompt-printing           */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
/* HACKED to add oasis-olc */
   struct olc_data *olc;        /* OLC info - defined in olc.h          */
/* end of hack */
/* PETS */
   struct char_data *pet;	/* Who your pet is - duped in char_data	*/
/* END of PETS */
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};


struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
};


struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
};


struct str_app_type {
   sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   sh_int todam;    /* Damage Bonus/Penalty                */
   sh_int carry_w;  /* Maximum weight that can be carrried */
   sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
   byte bonus;       /* how many practices player gains per lev */
};


struct int_app_type {
   byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
   sh_int hitp;
   sh_int shock;
};


struct weather_data {
   int	pressure;	/* How is the pressure ( Mb ) */
   int	change;	/* How fast and what way does it change. */
   int	sky;	/* How is the sky. */
   int	sunlight;	/* And how much sun. */
};


struct title_type {
   char	*title_m;
   char	*title_f;
   int	exp;
};


/* element in monster and object index-tables   */
struct index_data {
   int	virtual;    /* virtual number of this mob/obj           */
   int	number;     /* number of existing units of this mob/obj	*/
   int  progtypes;  /* program types for MOBProg		*/
   MPROG_DATA *mobprogs; /* programs for MOBProg		*/
   SPECIAL(*func);
};

struct dm_score_data {
  char plrname[MAX_NAME_LENGTH];
  int kills;
  int deaths;
  int killscore;
  struct dm_score_data *next;
};

struct social_messg {
  int act_nr;
  int hide;
  int min_victim_position;	/* Position of victim */

  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;		/* if NULL, read no further, ignore args */
  char *others_found;
  char *vict_found;

  /* An argument was there, but no victim was found */
  char *not_found;

  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;
};
