/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define CAST_UNDEFINED	-1
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4
#define CAST_PILL	5

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)


#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD	     45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD	     46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DEFENSIVE_HARMONY	     47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL	     48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL	     49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION	     50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK		     51 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FLOATING_DISC          52 
#define SPELL_WIZARD_LOCK            53
#define SPELL_BARKSKIN               54
#define SPELL_UNUSED55               55
#define SPELL_KNOCK                  56
#define SPELL_FEATHER_FALL           57
#define SPELL_MIRROR_IMAGE           58
#define SPELL_PHANTASMAL_FORCE       59
#define SPELL_WEB                    60
#define SPELL_CLAIRVOYANCE           61
#define SPELL_DISPEL_MAGIC           62
#define SPELL_AID                    63 /* Added  */
#define SPELL_HASTE                  64 /* Added  */
#define SPELL_SLOW                   65 /* Added  */
#define SPELL_DISPEL_SANCTUARY       66 /* Added  */
#define SPELL_RELOCATE               67 /* Added  */
#define SPELL_REFRESH                68 /* Added  */
#define SPELL_FLY                    69 /* Added  */
#define SPELL_NUKE                   70 /* Added  */
#define SPELL_PETRIFY                71 /* Added  */
#define SPELL_WORD_OF_DEATH          72 /* Added  */
#define SPELL_RESTORE                73 /* Added  */
#define SPELL_PARALYSIS              74 /* Added  */
#define SPELL_PROTECTION_FROM_GOOD   75
#define SPELL_CONFUSION              76
#define SPELL_DIMENSION_DOOR         77
#define SPELL_POLYMORPH_OTHER        78
#define SPELL_POLYMORPH_SELF         79
#define SPELL_CLOUDKILL              80
#define SPELL_FEEBLEMIND             81
#define SPELL_MAGIC_JAR              82
#define SPELL_TELEKINESIS            83
#define SPELL_ANTIMAGIC_SHELL        84
#define SPELL_DISINTEGRATE           85
#define SPELL_INVISIBLE_STALKER      86
#define SPELL_STINKING_CLOUD         87
#define SPELL_FIND_FAMILIAR          88
#define SPELL_FORGET                 89
#define SPELL_CANTRIP                90
#define SPELL_REMOVE_FEAR            91
#define SPELL_SCARE                  92
#define SPELL_RESIST_FIRE            93
#define SPELL_RESIST_COLD            94
#define SPELL_RESIST_LIGHTNING       95
#define SPELL_RESIST_POISON          96
#define SPELL_RESIST_ACID            97
#define SPELL_FIND_TRAPS             98
#define SPELL_SILENCE                99
#define SPELL_STICKS_TO_SNAKES      100
#define SPELL_SHILLELAGH            101
#define SPELL_INSECT_PLAGUE         102
#define SPELL_RESSURRECT            103
#define SPELL_FAR_SEE               104
#define SPELL_MONSUM_I              105
#define SPELL_MONSUM_II             106
#define SPELL_MONSUM_III            107
#define SPELL_GATE_I                108
#define SPELL_GATE_II               109
#define SPELL_GATE_III              110
#define SPELL_CONJURE_ELEMENTAL     111
#define SPELL_AERIAL_SERVANT        112
#define SPELL_IDENTIFY              113
#define SPELL_CAUSE_LIGHT           114
#define SPELL_CAUSE_SERIOUS         115
#define SPELL_CAUSE_CRITIC          116
#define SPELL_CONE_OF_COLD          117
#define SPELL_CHAIN_LIGHTNING       118
#define SPELL_CURE_SERIOUS          119
#define SPELL_INSPIRE               120
#define SPELL_AID_2                 121   /*  Cheezy hack on spell aid */
#define SPELL_INSPIRE_2             122   /*  Same hack for inspire    */
#define SPELL_CONTINUAL_LIGHT       123
#define SPELL_FAERIE_FIRE           124
#define SPELL_GROUP_SANCTUARY       125
#define SPELL_WILD_HEAL             126
#define SPELL_PHASE_DOOR            127
#define SPELL_AREA_WORD_OF_DEATH    128
#define SPELL_AREA_SCARE            129
#define SPELL_PORTAL                130
#define SPELL_INSTILL_ENERGY        131
#define SPELL_ENERGY_FLUX           132
#define SPELL_HOLY_LIGHT            133
#define SPELL_BLUR                  134
#define SPELL_CALM                  135
#define SPELL_CREATE_SPRING         136
#define SPELL_CREATE_BLOOD          137
#define SPELL_ENCHANT_ARMOR         138
#define SPELL_FAERIE_FOG            139
#define SPELL_MASS_INVIS            140
#define SPELL_PASS_WITHOUT_TRACE    141
#define SPELL_POWER_WORD_STUN       142
#define SPELL_PRAYER                143
#define SPELL_RAY_OF_ENFEEBLEMENT   144
#define SPELL_SPIRITUAL_HAMMER      145
#define SPELL_POISON2               146
#define SPELL_STONESKIN             147
#define SPELL_FIST_OF_EARTH         148
#define SPELL_FIST_OF_STONE         149
#define SPELL_SUNRAY                150
#define SPELL_SEARING_ORB           151
#define SPELL_BATTLE_HYMN           152
#define SPELL_WINDWALK              153
#define SPELL_EARTH_ELEMENTAL       154
#define SPELL_WATER_ELEMENTAL       155
#define SPELL_AIR_ELEMENTAL         156
#define SPELL_FIRE_ELEMENTAL        157
#define SPELL_FIRESTORM             158
#define SPELL_FINGER_OF_DEATH       159
#define SPELL_MASS_FLY              160
#define SPELL_ICE_STORM             161
#define SPELL_ACID_BLAST            162
#define SPELL_MASS_REFRESH          163
#define SPELL_ICE_SHIELD            164
#define SPELL_GYPSY_DANCE           165
#define SPELL_CREATE_WEAPON         166
#define SPELL_FIRESHIELD            167
#define SPELL_MANASHELL             168
#define NUM_SPELLS                  169 	/* num spells really */
#define MAX_SPELLS		    1300        /* insert new spells above */

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB              1301 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  1302 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  1303 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  1304 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             1305 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PUNCH                 1306 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                1307 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 1308 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 1309 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		    1310 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_DISARM                1311
#define SKILL_CRITICAL_HIT          1312
#define SKILL_CIRCLE                1313
#define SKILL_TURN                  1314
#define SKILL_BREATHE_FIRE          1315
#define SKILL_BREATHE_GAS           1316
#define SKILL_BREATHE_FROST         1317
#define SKILL_BREATHE_ACID          1318
#define SKILL_BREATHE_LIGHTNING     1319
#define SKILL_BERSERK               1320
#define SKILL_BLOCK                 1321
#define SKILL_RETREAT               1322
#define SKILL_SCATTER               1323
#define SKILL_GAZE                  1324
#define SKILL_SKIN                  1325
#define SKILL_SMITH                 1326
#define SKILL_SHAPESHIFT            1327
#define SKILL_DISGUISE              1328
#define SKILL_DEFEND                1329
#define SKILL_SCAN                  1330
#define SKILL_STUN_TOUCH            1331
#define SKILL_QUICKHEAL             1332
#define SKILL_MEDITATE              1333
#define SKILL_SCROUNGE              1334
#define SKILL_JUDGE                 1335
#define SKILL_GAUGE                 1336
#define SKILL_RIPOSTE               1337
#define SKILL_RAGE                  1338
#define SKILL_SWITCH                1339
#define SKILL_TWIST                 1340
#define SKILL_QUICKDRAW             1341
#define SKILL_DUALWIELD             1342
#define SKILL_MPDAMAGE              1343
#define SKILL_HEAL                  1344
#define SKILL_AVENGING_BLOW         1345
#define SKILL_VALOUR                1346
#define SKILL_TRIP                  1347
#define SKILL_PALM                  1348
#define SKILL_BANDAGE               1349
/* New skills may be added here up to MAX_SKILLS (2000) */


/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

#define UNUSED2001                   2001
#define SPELL_FIRE_BREATH            2002
#define SPELL_GAS_BREATH             2003
#define SPELL_FROST_BREATH           2004
#define SPELL_ACID_BREATH            2005
#define SPELL_LIGHTNING_BREATH       2006

#define TOP_SPELL_DEFINE	     2999
/* NEW NPC/OBJECT SPELLS can be inserted here up to 2999 */


/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     3000
#define TYPE_STING                   3001
#define TYPE_WHIP                    3002
#define TYPE_SLASH                   3003
#define TYPE_BITE                    3004
#define TYPE_BLUDGEON                3005
#define TYPE_CRUSH                   3006
#define TYPE_POUND                   3007
#define TYPE_CLAW                    3008
#define TYPE_MAUL                    3009
#define TYPE_THRASH                  3010
#define TYPE_PIERCE                  3011
#define TYPE_BLAST		     3012
#define TYPE_PUNCH		     3013
#define TYPE_STAB		     3014
#define TYPE_CHOP                    3015
#define TYPE_BREATHE_LIGHTNING       3016
#define TYPE_BREATHE_FROST           3017
#define TYPE_BREATHE_ACID            3018
#define TYPE_BREATHE_FIRE            3019
#define TYPE_BREATHE_GAS             3020

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING		     3999



#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4
#define SAVING_NO_SAVE 5	/* this must come last! */
#define MAX_SAVE_THROW 5	/* wierd but true, this is one less */

#define TAR_IGNORE	(1 << 0)
#define TAR_CHAR_ROOM	(1 << 1)
#define TAR_CHAR_WORLD	(1 << 2)
#define TAR_FIGHT_SELF	(1 << 3)
#define TAR_FIGHT_VICT	(1 << 4)
#define TAR_SELF_ONLY	(1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV	(1 << 7)
#define TAR_OBJ_ROOM	(1 << 8)
#define TAR_OBJ_WORLD	(1 << 9)
#define TAR_OBJ_EQUIP	(1 << 10)
#define TAR_CHAR_NEARBY	(1 << 11)
#define TAR_OBJ_NEARBY	(1 << 12)
#define TAR_CHAR_DIR	(1 << 13)
#define TAR_OBJ_DIR	(1 << 14)
#define TAR_UNUSED	(1 << 15) /* area spells can be cast w/o an arg */


struct spell_info_type {
   byte min_position;	/* Position for caster	 */
/* HACKED to make the mana min and max broader */
   int mana_min;
   int mana_max;
   int mana_change;
/*   byte mana_min;	*/	/* Min amount of mana used by a spell (highest lev) */
/*   byte mana_max;	*/	/* Max amount of mana used by a spell (lowest lev) */
/*   byte mana_change;	*/	/* Change in mana used by spell from lev to lev */

   byte min_level[NUM_CLASSES];
   int routines;
   byte violent;
   sh_int targets;         /* See below for use with TAR_XXX  */
};


#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4
#define SPELL_TYPE_PILL	   5


/* Attacktypes with grammar */

struct attack_hit_type {
   char	*singular;
   char	*plural;
};


#define ASPELL(spellname) \
void	spellname(byte level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj, \
                  char *argument)

#define MANUAL_SPELL(spellname) \
	spellname(level, caster, cvict, ovict, argument);

/* be sure to define all of these as MAG_MANUAL spells in spell_parser.c */
ASPELL(spell_area_scare);
ASPELL(spell_calm);
ASPELL(spell_cantrip);
ASPELL(spell_charm);
ASPELL(spell_clairvoyance);
ASPELL(spell_create_blood);
ASPELL(spell_create_water);
ASPELL(spell_dimension_door);
ASPELL(spell_dispel_magic);
ASPELL(spell_enchant_weapon);
ASPELL(spell_enchant_armor);
ASPELL(spell_far_see);
ASPELL(spell_fireball);
ASPELL(spell_forget);
ASPELL(spell_gypsy_dance);
ASPELL(spell_identify);
ASPELL(spell_information);
ASPELL(spell_knock);
ASPELL(spell_locate_object);
ASPELL(spell_magic_jar);
ASPELL(spell_phase_door);
ASPELL(spell_portal);
ASPELL(spell_recall);
ASPELL(spell_scare);
ASPELL(spell_relocate);
ASPELL(spell_summon);
ASPELL(spell_teleport);
ASPELL(spell_wizard_lock);
ASPELL(spell_windwalk);
ASPELL(spell_create_weapon);
ASPELL(spell_remove_curse);

/* basic magic calling functions */

int find_skill_num(char *name);

void mag_damage(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

void mag_group_switch(int level, struct char_data *ch, struct char_data *tch, 
  int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

void mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(byte level, struct char_data *ch, struct char_data *victim,
 int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 int spellnum, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
 int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
  int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

int	call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, char *argument, 
  int spellnum, int level, int casttype);

void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
  char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,
  struct obj_data *tobj, char *argument, int spellnum);
