
/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"


const char circlemud_version[] = {
  "Heroes of Kore\r\n"
  "based on CircleMUD version 3.00 beta patchlevel 6\r\n"
};



/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for race definitions in race.c instead of here) */
/* (Note: strings for class definitions in class.c instead of here) */


/* cardinal directions */
const char *dirs[] = {
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northeast",
  "southeast",
  "southwest",
  "northwest",
  "somewhere",
  "\n"
};


/* cardinal directions abbreviations */
const char *dir_abbrevs[] = {
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "ne",
  "se",
  "sw",
  "nw",
  "*",
  "\n"
};


/* cardinal directions display order
  ie, when you type 'exits' what the order should be */
const int dir_order[] = {
  NORTH,
  NORTHEAST,
  EAST,
  SOUTHEAST,
  SOUTH,
  SOUTHWEST,
  WEST,
  NORTHWEST,
  UP,
  DOWN,
  SOMEWHERE 
};


const int rev_dir[] =
{
  SOUTH,
  WEST,
  NORTH,
  EAST,
  DOWN,
  UP,
  SOUTHWEST,
  NORTHWEST,
  NORTHEAST,
  SOUTHEAST,
  SOMEWHERE
};


/* ZONE_x */
const char *zone_bits[] = {
  "!TELEPORT",
  "!PHASEDOOR",
  "CHAOS",
  "GODZONE",
  "BATTLE_OK",
  "ACTIVE",
  "\n"
};


/* ROOM_x */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "!MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "!TRACK",
  "!MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HOUSE-CRASH",
  "HOUSE-ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "CHAOS",
  "!TELEPORT",
  "!PHASEDOOR",
  "STORAGE",
  "HYPER-REGEN",
  "SOLITARY",
  "UNUSED22",
  "UNUSED23",
  "UNUSED24",
  "UNUSED25",
  "UNUSED26",
  "UNUSED27",
  "UNUSED28",
  "UNUSED29",
  "UNUSED30",
  "UNUSED31",
  "\n"
};

const ubyte room_bits_olc[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
                               1, 1, 1, 0, 0};


/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "SECRET",
  "\n"
};


/* SECT_ */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Swim",
  "!Swim",
  "Underwater",
  "Fly",
  "Desert",
  "Iceland",
  "Ocean",
  "Ladder",
  "Tree",
  "Astral",
  "Swamp",
  "\n"
};


/* SEX_x */
const char *genders[] =
{
  "Neutral",
  "Male",
  "Female",
  "\n"
};


/* POS_x */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "Searching",
  "\n"
};


/* PLR_x */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "!WIZL",
  "!DEL",
  "INVST",
  "CRYO",
  "MASTER",
  "\n"
};


/* MOB_x */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "!UNUSED!",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR-EVIL",
  "AGGR-GOOD",
  "AGGR-NEUTRAL",
  "MEMORY",
  "HELPER",
  "!CHARM",
  "!SUMMON",
  "!SLEEP",
  "!BASH",
  "!BLIND",
  "SHOPKEEPER",
  "!PLINK",
  "SWIMMER",
  "!WALK",
  "WILL-SELF-PURGE",
  "!THERE",
  "NOBLOCK",
  "SAFE",
  "DSHOPKEEPER",
  "QUESTMASTER",
  "\n"
};



/* PRF_x */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "!TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "!HASSLE",
  "MERCPROMPT",
  "SUMMONABLE",
  "!REP",
  "LIGHT",
  "COLOR",
  "COLOR-PROMPT",
  "!WIZ",
  "SYSLCMP",
  "SYSLNRM",
  "!AUC",
  "!GOS",
  "!GTZ",
  "RMFLG",
  "AUTOLOOT",
  "AUTOSAC",
  "AUTOGOLD",
  "AUTOSPLIT",
  "AUTODIRS",
  "D_DIAG",
  "AUTOASSIST",
  "D_MINMAX",
  "D_GOLD",
  "D_EXP",
  "\n"
};


/* PRF2_* */
const char *preference2_bits[] = {
  "!CLAN",
  "AWAY",
  "ANON",
  "SHOW_DAMAGE",
  "ANON3",
  "AUTOSCAN",
  "BATTLEBRIEF",
  "AUTOGROUP",
  "!MUSIC",
  "AUTOMAP",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "\n"
};



/* AFF_x */
const char *affected_bits[] =
{                 /* Corresponds to structs.h by */
  "BLIND",        /* 1... 1 << 0 (1) */  
  "INVIS",        /* 2... 1 << 1 (2) */
  "RAGE",         /* 3... 1 << 2 (4) */
  "DETECT-INVIS", /* 4... 1 << 3 (8) */
  "DETECT-MAGIC", /* 5... 1 << 4 (16) */
  "SENSE-LIFE",   /* 6... 1 << 5 (32) */
  "WATERWALK",    /* 7... 1 << 6 (64) */
  "SANCT",        /* 8... 1 << 7 (128) */
  "GROUP",        /* 9... 1 << 8 (256) */
  "CURSE",        /* 10.. 1 << 9 (512) */
  "INFRA",        /* 11.. 1 << 10 (1024) */
  "POISON",       /* 12.. 1 << 11 (2048) */
  "PROT-EVIL",    /* 13.. 1 << 12 (4096) */
  "PROT-GOOD",    /* 14.. 1 << 13 (8192) */
  "SLEEP",        /* 15.. 1 << 14 (16384) */
  "!TRACK",       /* 16.. 1 << 15 (32768) */
  "HASTE",        /* 17.. 1 << 16 (65536) */
  "SLOW",         /* 18.. 1 << 17 (131072) */
  "SNEAK",        /* 19.. 1 << 18 (262144) */
  "HIDE",         /* 20.. 1 << 19 (524288) */
  "AID",          /* 21.. 1 << 20 (1048576) */
  "CHARM",        /* 22.. 1 << 21 (2097152) */
  "FLY",          /* 23.. 1 << 22 (4194304) */
  "MAGIC-RESIST", /* 24.. 1 << 23 (big) */
  "STONESKIN",    /* 25.. 1 << 24 (bigger) */
  "INSPIRE",      /* 26.. 1 << 25 */
  "MIRROR-IMAGE", /* 27.. 1 << 26 */
  "BLESS",        /* 28.. 1 << 27 */
  "STUN",         /* 29.. 1 << 28 */
  "BLOCK",        /* 30.. 1 << 29 */
  "GAUGE",        /* 31.. 1 << 30 */
  "DETECT-ALIGN", /* 32.. 1 << 31 */
  "\n"
};

/* AFF2 */
const char *affected2_bits[] =
{                 /* Corresponds to structs.h */
  "JARRED",       /* 1... 1 << 0 (1) */  
  "POISON2",      /* 2... 1 << 1 (2) */
  "ICE-SHIELD",   /* 3... 1 << 2 (4) */
  "QUESTOR",	  /* 4... 1 << 3 (8) */
  "WASCHARMED",   /* 5... 1 << 4 (16) */
  "WEBBED",       /* 6... 1 << 5 (32) */
  "RES_FIRE",     /* 7... 1 << 6 (64) */
  "RES_COLD",     /* 8... 1 << 7 (128) */
  "RES_ELEC",     /* 9... 1 << 8 (256) */
  "RES_ACID",     /* 10.. 1 << 9 (512) */
  "FIRESHIELD",   /* 11.. 1 << 10 (1024) */
  "MANASHELL",    /* 12.. 1 << 11 (2048) */
  "UNUSED",       /* 13.. 1 << 12 (4096) */
  "UNUSED",       /* 14.. 1 << 13 (8192) */
  "UNUSED",       /* 15.. 1 << 14 (16384) */
  "UNUSED",       /* 16.. 1 << 15 (32768) */
  "UNUSED",       /* 17.. 1 << 16 (65536) */
  "UNUSED",       /* 18.. 1 << 17 (131072) */
  "UNUSED",       /* 19.. 1 << 18 (262144) */
  "UNUSED",       /* 20.. 1 << 19 (524288) */
  "UNUSED",       /* 21.. 1 << 20 (1048576) */
  "UNUSED",       /* 22.. 1 << 21 (2097152) */
  "UNUSED",       /* 23.. 1 << 22 (4194304) */
  "UNUSED",       /* 24.. 1 << 23 (big) */
  "UNUSED",       /* 25.. 1 << 24 (bigger) */
  "UNUSED",       /* 26.. 1 << 25 */
  "UNUSED",       /* 27.. 1 << 26 */
  "UNUSED",       /* 28.. 1 << 27 */
  "UNUSED",       /* 29.. 1 << 28 */
  "UNUSED",       /* 30.. 1 << 29 */
  "UNUSED",       /* 31.. 1 << 30 */
  "UNUSED",       /* 32.. 1 << 31 */
  "UNUSED",       /* 33.. 1 << 32 */
  "\n"
};

/* AFF3 */
const char *affected3_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */
  "\n"
};


/* AFF4 */
const char *affected4_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */ 
  "\n"
};

/* AFF5 */
const char *affected5_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */
  "\n"
};

/* AFF6 */
const char *affected6_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */
  "\n"
};

/* AFF7 */
const char *affected7_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */
  "\n"
};

/* AFF8 */
const char *affected8_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */
  "\n"
};

/* AFF9 */
const char *affected9_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */
  "\n"
};

/* AFF10 */
const char *affected10_bits[] =
{                 /* Corresponds to structs.h */
  "UNUSED",       /* 1... 1 << 0 (1) */
  "UNUSED",       /* 2... 1 << 1 (2) */
  "UNUSED",       /* 3... 1 << 2 (4) */
  "\n"
};   


/* CON_x */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Get race",
  "Select color",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Deaf",
  "Confirm battle",
  "Get email",
  "\n"
};


/* WEAR_x - for eq list, be sure to add an entry to wear_name[]  */
const char *where[] = {
  "<held as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded primary>    ",
  "<held>               ",
  "<readied>            ",
  "<wielded secondary>  ",
  "<floating about body>",
  "<worn on ears>       ",
  "<worn on face>       "
};

/* The wear list for thri-kreens! */
const char *thri_where[] = {
  "<held as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<left-hand shield>   ",
  "<right-hand shield>  ",
  "<worn about waist>   ",
  "<held in left claw>  ",
  "<wielded, right claw>",
  "<held in right claw> ",
  "<readied>            ",
  "<wielded, left claw> ",
  "<circling thorax>    ",
  "<worn on face>       "
};

/* WEAR_x - for stat */
/* corresponds to wear[] */
const char *equipment_types[] = {
  "light",
  "left finger",
  "right finger",
  "neck 1",
  "neck 2",
  "body",
  "head",
  "legs",
  "feet",
  "hands",
  "arms",
  "shield",
  "about",
  "waist",
  "left wrist",
  "right wrist",
  "wield",
  "held",
  "ready",
  "wield 2",
  "floating",
  "ears",
  "face",
  "\n"
};

const char *thri_equipment_types[] = {
  "light",
  "upper left finger",
  "upper right finger",
  "neck 1",
  "neck 2",
  "lower left finger",
  "lower right finger",
  "left shield",
  "right shield",
  "waist",
  "held left",
  "wield right",
  "held right",
  "ready",
  "wield left",
  "floating",
  "face",
  "\n"
};


/* this only affects the order they're displayed in */
const int wear_display_order[] = {
  WEAR_LIGHT,
  WEAR_FINGER_R,
  WEAR_FINGER_L,
  WEAR_NECK_1,
  WEAR_NECK_2,
  WEAR_BODY,
  WEAR_HEAD,
  WEAR_LEGS,
  WEAR_FEET,
  WEAR_HANDS,
  WEAR_ARMS,
  WEAR_SHIELD,
  WEAR_ABOUT,
  WEAR_WAIST,
  WEAR_WRIST_R,
  WEAR_WRIST_L,
  WEAR_WIELD,
  WEAR_WIELD_2,
  WEAR_READY,
  WEAR_HOLD,
  WEAR_PRIZE,
  WEAR_EARS,
  WEAR_FACE
};

const int thri_wear_display_order[] = {
  THRI_WEAR_LIGHT,
  THRI_WEAR_FINGER_UR,
  THRI_WEAR_FINGER_UL,
  THRI_WEAR_FINGER_LR,
  THRI_WEAR_FINGER_LL,
  THRI_WEAR_NECK_1,
  THRI_WEAR_NECK_2,
  THRI_WEAR_SHIELD_R,
  THRI_WEAR_SHIELD_L,
  THRI_WEAR_WAIST,
  THRI_WEAR_WIELD_L,
  THRI_WEAR_WIELD_R,
  THRI_WEAR_READY,
  THRI_WEAR_HOLD_L,
  THRI_WEAR_HOLD_R,
  THRI_WEAR_PRIZE,
  THRI_WEAR_FACE
};

/* ITEM_x (ordinal object types) */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIREWEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ-CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "INSTRUMENT",
  "PILL",
  "SEED",
  "PORTAL",
  "\n"
};

const ubyte item_types_olc[] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "READY",
  "PRIZE",
  "EARS",
  "FACE",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESSED",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_MAGE",
  "ANTI_CLERIC",
  "ANTI_THIEF",
  "ANTI_WARRIOR",
  "NO_SELL",
  "ANTI_BARD",
  "ANTI_DRAGON",
  "VETERAN",
  "HERO",
  "CHAMPION",
  "CONCEALED",
  "ANTI_DRUID",
  "SUPERCURSED",
  "ANTI_MONK",
  "QUEST",
  "NEWBIE",
  "NO_LOCATE",
  "NO_REMOVE",
  "AUTOQUEST",
  "PRICEOK",
  "\n"
};


/* APPLY_x */
const char *apply_types[] = {
  "none",
  "str",
  "dex",
  "int",
  "wis",
  "con",
  "cha",
  "!UNUSED!",
  "!UNUSED!",
  "age",
  "weight",
  "height",
  "mana",
  "hps",
  "movement",
  "!UNUSED!",
  "!UNUSED!",
  "ac",
  "hit",
  "dam",
  "save vs paralysis",
  "save vs rod",
  "save vs petrification",
  "save vs breath",
  "save vs spell",
  "!UNUSED!",
  "!UNUSED!",
  "\n"
};

const ubyte apply_types_olc[] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1,
                                  0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "coke",
  "pepsi",
  "moonshine",
  "ice tea",
  "root beer",
  "veggie juice",
  "fruit juice",
  "tequila",
  "kahlua",
  "rum",
  "champagne",
  "dwarven spirits",
#ifdef DRAGONSLAVE
  "water",
#else
  "magical mystery juice",
#endif
  "slurpee",
  "\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "coke",
  "pepsi",
  "moonshine",
  "tea",
  "rootbeer",
  "juice",
  "juice",
  "tequila",
  "kahlua",
  "rum",
  "champagne",
  "spirits",
#ifdef DRAGONSLAVE
  "water",
#else
  "magical mystery juice",
#endif
  "slurpee",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
const int drink_aff[][3] = {
  {0, 1, 10},		/* water */
  {3, 2, 5},		/* beer */
  {5, 2, 5},		/* wine */
  {2, 2, 5},		/* ale */
  {1, 2, 5},		/* ale */
  {6, 1, 4},		/* whiskey */
  {0, 1, 8},		/* lemonade */
  {10, 0, 0},		/* firebreather */
  {3, 3, 3},		/* local */
  {0, 4, -8},		/* juice */
  {0, 3, 6},		/* milk */
  {0, 1, 6},		/* tea */
  {0, 1, 6},		/* coffee */
  {0, 2, -1},		/* blood */
  {0, 1, -2},		/* salt */
  {0, 0, 13},		/* water */
  {-1, 0, 10},		/* coke */
  {-1, 0, 10},		/* pepsi */
  {6, 1, 4},		/* moonshine */
  {0, 1, 6},		/* iced tea */
  {-1, 0, 5},		/* root beer */
  {0, 4, 1},		/* veggie juice */
  {0, 3, 5},		/* fruit juice */
  {10, 0, 0},		/* tequila */
  {3, 0, 0},		/* kahlua */
  {5, 0, 0},		/* rum */
  {2, 0, 2},		/* champagne */
  {-6, 1, 4},		/* magical mystery juice */ /* (don't ask) */
  {0, -1, 30},		/* slurpee */
  {0, 1, 10}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "brown",
  "brown",
  "yellow",
  "light brown",
  "brown",
  "dark red",
  "purple",
  "clear",
  "cream",
  "brown",
  "golden",
  "plaid",
  "neon crystalline blue"
};


/* level of fullness for drink containers */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* seed codes */
const char *seeds[] = {
  "load obj",
  "load mob",
  "\n"
};


/* portal enter codes */
const char *portals[] = {
  "enter",
  "board",
  "push",
  "pull",
  "look (UNUSED)",
  "get (UNUSED)",
  "climb",
  "\n"
};


/* str, int, wis, dex, con applies **************************************/

/* +hit +dam max_carry max_wield */

/* [ch] strength apply (all) */
const struct str_app_type str_app[35] = {
  {-4, -4, 0, 0},
  {-4, -4, 3, 1},
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},  /* 5 */
  {-2, -1, 55, 5},
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 105, 9},  /* 10 */
  {0, 0, 115, 10},
  {0, 0, 125, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 160, 14},  /* 15 */
  {0, 0, 180, 15},
  {1, 1, 195, 16},
  {1, 1, 220, 18},
  {2, 2, 255, 19},
  {2, 2, 320, 20},  /* 20 */
  {3, 3, 440, 21},
  {3, 3, 550, 22},
  {4, 4, 660, 23},
  {4, 4, 720, 24},
  {5, 5, 740, 25},  /* 25 */
  {5, 5, 740, 25},
  {5, 5, 740, 25},
  {5, 5, 740, 25},
  {5, 5, 740, 25},
  {5, 5, 740, 25},  /* 30 */
  {5, 5, 740, 25},
  {5, 5, 740, 25},
  {5, 5, 740, 25},
  {5, 5, 740, 25},
  {5, 5, 740, 25}  /* 35 */
};



/* [dex] skill apply (thieves only) */
const struct dex_skill_type dex_app_skill[36] = {
  {-99, -99, -90, -99, -60},
  {-90, -90, -60, -90, -50},
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {30, 30, 20, 25, 25},
  {30, 30, 20, 25, 25},
  {30, 35, 20, 30, 30},
  {30, 35, 20, 30, 30},
  {35, 35, 25, 30, 30},
  {35, 35, 25, 35, 35},
  {35, 35, 25, 35, 35},
  {35, 40, 25, 35, 35},
  {35, 40, 30, 35, 35},
  {40, 40, 30, 35, 35}		/* 35 */
};



/* [level] backstab multiplyer (thieves only) */
const byte backstab_mult[LVL_IMPL + 1] = {
  1,				/* 0 */
  2,				/* 1 */
  2,
  2,
  2,
  2,				/* 5 */
  2,
  2,
  3,
  3,
  3,				/* 10 */
  3,
  3,
  3,
  4,
  4,				/* 15 */
  4,
  4,
  4,
  4,
  4,				/* 20 */
  5,
  5,
  5,
  5,
  5,				/* 25 */
  5,
  5,
  5,
  5,
  5,				/* 30 */
  5,
  5,
  5,
  5,
  5,				/* 35 */
  6,
  6,
  6,
  6,
  6,				/* 40 */
  7,
  7,
  7,
  7,
  7,				/* 45 */
  8,
  8,
  8,
  8,
  8, 				/* 50 */
  9,
  9,
  9,
  9,
  9,				/* 55 */
  9,
  9,
  9,
  9,
  10				/* 60 */
};


/* [dex] apply (all) */
struct dex_app_type dex_app[36] = {
  {-7, -7, 6},
  {-6, -6, 5},
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},
  {3, 3, -4},
  {3, 3, -4},
  {4, 4, -5},
  {4, 4, -5},
  {4, 4, -5},
  {5, 5, -6},
  {5, 5, -6},
  {5, 5, -6},
  {5, 5, -6},
  {6, 6, -7},
  {6, 6, -7},
  {6, 6, -7},
  {6, 6, -7},
  {7, 7, -8},
  {7, 7, -8},
  {7, 7, -8},
  {7, 7, -6}			/* 35 */
};



/* [con] apply (all) */
struct con_app_type con_app[36] = {
  {-4, 20},
  {-3, 25},
  {-2, 30},
  {-2, 35},
  {-1, 40},
  {-1, 45},
  {-1, 50},
  {0, 55},
  {0, 60},
  {0, 65},
  {0, 70},
  {0, 75},
  {0, 80},
  {0, 85},
  {0, 88},
  {1, 90},
  {2, 95},
  {2, 97},
  {3, 99},
  {3, 99},
  {4, 99},
  {5, 99},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99},
  {6, 100},
  {7, 100},
  {7, 100},
  {7, 100},
  {7, 100},
  {8, 100},
  {8, 100},
  {8, 100},
  {8, 100},
  {9, 100}			/* 35 */
};



/* [int] apply (all) */
struct int_app_type int_app[36] = {
  {3},
  {5},				/* 1 */
  {7},
  {8},
  {9},
  {10},				/* 5 */
  {11},
  {12},
  {13},
  {15},
  {17},				/* 10 */
  {19},
  {22},
  {25},
  {30},
  {35},				/* 15 */
  {40},
  {45},
  {50},
  {53},
  {55},				/* 20 */
  {56},
  {57},
  {58},
  {59},
  {60},				/* 25 */
  {61},
  {62},
  {63},
  {64},
  {65},				/* 30 */
  {66},
  {69},
  {70},
  {80},
  {99}				/* 35 */
};


/* [wis] apply (all) */
/* HACKED a little to fall off differently for practices */
struct wis_app_type wis_app[36] = {
  {0},				/* 0 */
  {0},				/* 1 */
  {0},
  {0},
  {0},
  {0},				/* 5 */
  {0},
  {0},
  {2},
  {2},
  {2},				/* 10 */
  {3},
  {3},
  {3},
  {3},
  {4},				/* 15 */
  {4},
  {5},
  {5},				/* 18 */
  {6},
  {6},				/* 20 */
  {6},
  {6},
  {7},
  {7},
  {7},				/* 25 */
  {7},
  {8},
  {8},
  {8},
  {8},				/* 30 */
  {8},
  {8},
  {8},
  {9},
  {9}				/* 35 */
};
/* end of hack */



/* HACKED to add in charisma limits for summoned followers */
/* 25 is not some arbitrary number, its the limit of an attribute,
  25 is the maximum charisma a person could ever have (18 for mortals)
  and is set that way in handler.c   I add 1 (but dont need to) for
  the 0 that begins the array. */
const int cha_max_followers[26] = {
  1,				/* 0 */
  1,				/* 1 */
  1,
  1,
  1,
  1,				/* 5 */
  1,
  1,
  1,
  1,
  1,				/* 10 */
  1,
  1,
  1,
  2,
  2,				/* 15 */
  2,
  2,
  3,
  3,
  3,				/* 20 */
  3,
  4,
  4,
  4,
  4				/* 25 */
};
/* end of hack */



const char *spell_wear_off_msg[] = {
  "RESERVED DB.C",		/* 0 */
  "You feel less protected.",	/* 1 */
  "!Teleport!",
  "You feel less righteous.",
  "You feel a cloak of blindness disolve.",
  "!Burning Hands!",		/* 5 */
  "!Call Lightning",
  "You feel more self-confident.",
  "You feel your strength return.",
  "!Clone!",
  "!Color Spray!",		/* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",		/* 15 */
  "!Cure Light!",
  "You feel more optimistic.",
  "You feel less aware.",
  "Your eyes stop tingling.",
  "The detect magic wears off.",/* 20 */
  "The detect poison wears off.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",		/* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal!",
  "You feel yourself exposed.",
  "!Lightning Bolt!",		/* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "You feel less sick.",
  "You feel less protected.",
  "!Remove Curse!",		/* 35 */
  "The white aura around your body fades.",
  "!Shocking Grasp!",
  "You feel less tired.",
  "You feel weaker.",
  "!Summon!",			/* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your suroundings.",
  "!Animate Dead!",		/* 45 */
  "!Dispel Good!",
  "!Group Armor!",
  "!Group Heal!",
  "!Group Recall!",
  "Your night vision seems to fade.",	/* 50 */
  "Your feet seem less buoyant.",
  "You can no longer carry as much weight.",
  "!Wizard Lock!",
  "Your skin loses it's bark-like texture.",
  "You feel less aware of your surroundings.",            /* 55 */
  "!Knock!",
  "!Feather Fall!",
  "The last of your images dissipates, leaving you unprotected.",
  "!Phantasmal Force!",
  "The webs that bind you fall away.",                    /* 60 */
  "!Clairvoyance!",
  "!Dispel Magic!",
  "You no longer feel aided.",
  "You feel your incredible speed wear off.",
  "You shrug off your sluggishness.",      /* 65 */
  "!Dispel Sanctuary!",
  "!Relocate!",
  "!Rejuvinate!",
  "You find that your wings have disappeared.",
  "!Nuke!",				/* 70 */
  "!Petrify!",
  "!Word of Death!",
  "!Restore!",
  "You feel supple again, like you can move!",
  "You now feel vulernable to good.",		/* 75 */
  "You no longer feel so confused.",
  "!Dimension Door!",
  "You feel like your old self again, whew.",
  "You feel like your old self again.",
  "!Cloudkill!",			/* 80 */
  "You feel your mind sharpen.",
  "!Magic Jar!",
  "You feel your Gelleric powers wearing off.",
  "You feel vulnerable to magic again.",
  "!Disintegrate!",			/* 85 */
  "!Invisible Stalker!",
  "!Stinking Cloud!",
  "!Find Familiar!",
  "!Forget!",
  "!Cantrip!",				/* 90 */
  "!Remove Fear!",
  "!Scare!",
  "You feel sensitive to heat again.",
  "You feel sensitive to cold again.",
  "You feel sensitive to electricity again.",			/* 95 */
  "You feel sensitive to poisons again.",
  "You feel sensitive to acids again.",
  "You feel your ability to sense traps wear off.",
  "You find yourself able to speak again.",
  "!Sticks to Snakes!",			/* 100 */
  "!Shillelagh!",
  "!Insect Plague!",
  "!Ressurrect!",
  "!Far See!",
  "!Monster Summoning One!",		/* 105 */
  "!Monster Summoning Two!",
  "!Monster Summoning Three!",
  "!Gate One!",
  "!Gate Two!",
  "!Gate Three!",			/* 110 */
  "!Conjure Elemental!",
  "!Aerial Servant!",
  "!Identify!",
  "!Cause Light!",
  "!Cause Serious!",			/* 115 */
  "!Cause Critic!",
  "!Cone of Cold!",
  "!Chain Lightning!",
  "!Cure Serious",
  "You no longer feel inspired!",       /* 120 */
  "",   /* AID_2 hack - don't want a message */
  "You no longer feel inspired.",
  "!Continual Light!",
  "The faerie fire around you dims...",
  "!Group Sanctuary!",			/* 125 */
  "!Wild Heal!",
  "!Phase Door!",
  "!Area Word of Death!",
  "!Area Scare!",
  "!Portal!",				/* 130 */
  "!Instill Energy!",
  "!Energy Flux!",
  "!Holy Light!",
  "You no longer feel so quick.",
  "!Calm!",				/* 135 */
  "!Create Spring!",
  "!Create Blood!",
  "!Enchant Armor!",
  "!Faerie Fog!",
  "!Mass Invis!",			/* 140 */
  "You feel clumsy and leadfooted again.",
  "!Power Word Stun!",
  "!Prayer!",
  "!Ray of Enfeeblement!",
  "!Spritual Hammer!",			/* 145 */
  "You feel less sick.",
  "Your skin loses its stony texture.",
  "!Fist of Earth!",
  "!Fist of Stone!",
  "!Sunray!",				/* 150 */
  "!Searing Orb!",
  "You no longer feel inspired!",
  "!Transport Via Plant!",
  "!Earth Elemental!",
  "!Water Elemental!",			/* 155 */
  "!Air Elemental!",
  "!Fire Elemental!",
  "!Fire storm!",
  "!Finger of Death!",
  "!Mass Fly!",				/* 160 */
  "!Ice storm!",
  "!Acid Blast!",
  "!Mass Refresh!",
  "Your Ice Shield melts away.",
  "!Gypsy Dance!",			/* 165 */
  "!Create Weapon!",
  "Your fireshield flickers and dies.",
  "Your manashell slowly fades away.",
  "\n"
};

const char *spell_dispel_msg[] = {
  "RESERVED DB.C",		/* 0 */
  "is less protected.",	/* 1 */
  "!Teleport!",
  "shivers slightly.",
  "is no longer blinded.",
  "!Burning Hands!",		/* 5 */
  "!Call Lightning",
  "looks more self-confident.",
  "looks stronger.",
  "!Clone!",
  "!Color Spray!",		/* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",		/* 15 */
  "!Cure Light!",
  "shivers slightly.", /* detect hidden */
  "shivers slightly.", /* detect invis */
  "shivers slightly.",
  "shivers slightly.",/* 20 */
  "shivers slightly.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",		/* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal!",
  "fades into existence.",
  "!Lightning Bolt!",		/* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "looks less sick.",
  "shivers slightly.",
  "!Remove Curse!",		/* 35 */
  "is no longer surrounded by a white aura.",
  "!Shocking Grasp!",
  "looks less tired.",
  "looks less mighty.",
  "!Summon!",			/* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "shivers slightly.",
  "!Animate Dead!",		/* 45 */
  "!Dispel Good!",
  "!Group Armor!",
  "!Group Heal!",
  "!Group Recall!",
  "shivers slightly.",	/* 50 */
  "appears less buoyant.",
  "shivers slightly.",
  "!Wizard Lock!",
  "is no longer covered with a bark-like texture",
  "shivers slightly.",            /* 55 */
  "!Knock!",
  "!Feather Fall!",
  "is no longer surrounded by images.",
  "!Phantasmal Force!",
  "is no longer webbed.",                    /* 60 */
  "!Clairvoyance!",
  "!Dispel Magic!",
  "shivers slightly.",
  "is no longer moving so quickly.",
  "begins to move at a normal speed.",      /* 65 */
  "!Dispel Sanctuary!",
  "!Relocate!",
  "!Rejuvinate!",
  "falls to the ground.",
  "!Nuke!",				/* 70 */
  "!Petrify!",
  "!Word of Death!",
  "!Restore!",
  "is no longer paralyzed.",
  "shivers slightly.",		/* 75 */
  "appears less confused.",
  "!Dimension Door!",
  "resumes a more natural form.",
  "resumes a more natural form.",
  "!Cloudkill!",			/* 80 */
  "appears more intiutive.",
  "!Magic Jar!",
  "shivers slightly.",
  "shivers slightly.",
  "!Disintegrate!",			/* 85 */
  "!Invisible Stalker!",
  "!Stinking Cloud!",
  "!Find Familiar!",
  "!Forget!",
  "!Cantrip!",				/* 90 */
  "!Remove Fear!",
  "!Scare!",
  "shivers slightly.",
  "shivers slightly.",
  "shivers slightly.",
  "shivers slightly.",
  "shivers slightly.",
  "shivers slightly.",
  "shivers slightly.",
  "!Sticks to Snakes!",			/* 100 */
  "!Shillelagh!",
  "!Insect Plague!",
  "!Ressurrect!",
  "!Far See!",
  "!Monster Summoning One!",		/* 105 */
  "!Monster Summoning Two!",
  "!Monster Summoning Three!",
  "!Gate One!",
  "!Gate Two!",
  "!Gate Three!",			/* 110 */
  "!Conjure Elemental!",
  "!Aerial Servant!",
  "!Identify!",
  "!Cause Light!",
  "!Cause Serious!",			/* 115 */
  "!Cause Critic!",
  "!Cone of Cold!",
  "!Chain Lightning!",
  "!Cure Serious",
  "looks less inspired.",       /* 120 */
  "",   /* AID_2 hack - don't want a message */
  "looks less inspired.",
  "!Continual Light!",
  "no longer glows with a pink aura.",
  "!Group Sanctuary!",			/* 125 */
  "!Wild Heal!",
  "!Phase Door!",
  "!Area Word of Death!",
  "!Area Scare!",
  "!Portal!",				/* 130 */
  "!Instill Energy!",
  "!Energy Flux!",
  "!Holy Light!",
  "comes into focus.",
  "!Calm!",				/* 135 */
  "!Create Spring!",
  "!Create Blood!",
  "!Enchant Armor!",
  "!Faerie Fog!",
  "!Mass Invis!",			/* 140 */
  "shivers slightly.",
  "!Power Word Stun!",
  "!Prayer!",
  "!Ray of Enfeeblement!",
  "!Spritual Hammer!",			/* 145 */
  "looks less sick.",
  "is no longer covered by stone.",
  "!Fist of Earth!",
  "!Fist of Stone!",
  "!Sunray!",				/* 150 */
  "!Searing Orb!",
  "looks less inspired.",
  "!Transport Via Plant!",
  "!Earth Elemental!",
  "!Water Elemental!",			/* 155 */
  "!Air Elemental!",
  "!Fire Elemental!",
  "!Fire storm!",
  "!Finger of Death!",
  "!Mass Fly!",				/* 160 */
  "!Ice storm!",
  "!Acid Blast!",
  "!Mass Refresh!",
  "is no longer protected by Ice Shield.",
  "!Gypsy Dance!",			/* 165 */
  "!Create Weapon!",
  "is no longer shielded in flames.",
  "is no longer protected by magic.",
  "\n"
};


/* Spell list for reference 

  "armor",			/= 1 =/
  "teleport",
  "bless",
  "blindness",
  "burning hands",
  "call lightning",
  "charm person",
  "chill touch",
  "clone",
  "color spray",		/= 10 =/
  "control weather",
  "create food",
  "create water",
  "cure blind",
  "cure critic",
  "cure light",
  "curse",
  "detect alignment",
  "detect invisibility",
  "detect magic",		/= 20 =/
  "detect poison",
  "dispel evil",
  "earthquake",
  "enchant weapon",
  "energy drain",
  "fireball",
  "harm",
  "heal",
  "invisibility",
  "lightning bolt",		/= 30 =/
  "locate object",
  "magic missile",
  "poison",
  "protection from evil",
  "remove curse",
  "sanctuary",
  "shocking grasp",
  "sleep",
  "strength",
  "summon",			/= 40 =/
  "ventriloquate",
  "word of recall",
  "remove poison",
  "sense life",
  "animate dead",
  "dispel good",
  "defensive harmony",
  "group heal",
  "group recall",
  "infravision",		/= 50 =/
  "waterwalk",
  "floating disc",
  "wizard lock",
  "barkskin",
  "!UNUSED!",			/= 55 =/
  "knock",
  "feather fall",
  "mirror image",
  "phantasmal force",
  "web",			/= 60 =/
  "clairvoyance",
  "dispel magic",
  "aid",
  "haste", 
  "slow",			/= 65 =/
  "dispel sanctuary",
  "relocate",
  "refresh",
  "fly",
  "nuke",			/= 70 =/
  "petrify",
  "word of death",
  "restore",
  "paralysis",
  "protection from good",	/= 75 =/
  "confusion",
  "dimension door",
  "polymorph other",
  "polymorph self",
  "cloudkill",			/= 80 =/
  "feeblemind",
  "magic jar",
  "telekinesis",
  "antimagic shell",
  "disintegrate",		/= 85 =/
  "invisible stalker",
  "stinking cloud",
  "find familiar",
  "forget",
  "cantrip",			/= 90 =/
  "remove fear",
  "scare",
  "resist fire",
  "resist cold",
  "resist lightning",		/= 95 =/
  "resist poison",
  "resist acid",
  "find traps",
  "silence",
  "sticks to snakes",		/= 100 =/
  "shillelagh",
  "insect plague",
  "ressurrect",
  "far see",
  "monster summoning one",	/= 105 =/
  "monster summoning two",
  "monster summoning three",
  "gate one",
  "gate two",
  "gate three",			/= 110 =/
  "conjure elemental",
  "aerial servant",
  "identify",
  "cause light",
  "cause serious",		/= 115 =/
  "cause critic",
  "cone of cold",
  "chain lightning",
  "cure serious",
  "inspire",			/= 120 =/
  "aid two",
  "inspire two",
  "continual light",
  "faerie fire",
  "group sanctuary",		/= 125 =/
  "wild heal",
  "phase door",
  "area word of death",
  "area scare", 
  "portal",			/= 130 =/
  "instill energy",
  "energy flux",
  "holy light",
  "blur",
  "calm",			/= 135 =/
  "create spring", 
  "create blood", 
  "enchant armor", 
  "faerie fog",
  "mass invis",			/= 140 =/
  "pass without trace", 
  "power word stun",
  "prayer",
  "ray of enfeeblement",
  "spiritual hammer",		/= 145 =/
  "poison two",
  "stoneskin",
  "fist of earth",
  "fist of stone",
  "sunray",			/= 150 =/
  "searing orb",
  "battle hymn",
  "windwalk",
  "earth elemental",
  "water elemental",		/= 155 =/
  "air elemental",
  "fire elemental",
  "fire storm",
  "finger of death",
  "mass fly",   		/= 160 =/
  "ice storm",
  "acid blast",
  "mass refresh",
  "ice shield",
  "gypsy dance",		/= 165 =/
  "create weapon",
  "fireshield",
  "mana shell",
*/ 


const int movement_loss[] =
{
  1,				/* Inside     */
  1,				/* City       */
  3,				/* Field      */
  5,				/* Forest     */
  7,				/* Hills      */
  8,				/* Mountains  */
  4,				/* Swimming   */
  1,				/* Unswimable */
  6,                            /* Underwater */
  0,                            /* Flying     */
  10,                           /* Desert     */
  8,                            /* Iceland    */
  1,                            /* Ocean      */
  1,                            /* Ladder     */
  4,                            /* Tree       */
  0,                            /* Astral     */ 
  12                            /* Swamp      */
};


const char *weekdays[7] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};


const char *month_name[17] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};


/* clerics cant use sharp weapons */
const int sharp[] = {
  0,				/* Hitting */
  1,				/* Stinging */
  0,				/* Whipping */
  1,				/* Slashing */
  0,				/* Biting */
  0,				/* Bludgeoning */
  0,				/* Crushing */
  0,				/* Pounding */
  0,				/* Clawing */
  0,				/* Mauling */
  1,				/* Thrashing */
  1,				/* Piercing */
  0, 				/* Blasting */
  0,				/* Punching */
  1,				/* Stabbing */
  1,				/* Chopping */
  0,				/* Breathe Lightning */
  0,				/* Breathe Frost */
  0,				/* Breathe Acid */
  0,				/* Breathe Fire */
  0				/* Breathe Gas */
};



const char *mobformats[] = {
  "Simple",
  "Expanded",
  "\n"
};

char *objprognames[] = {
  "get_prog",
  "use_prog",
  "rot_prog",
  "drop_prog",
  "identify_prog",
  "wear_prog",
  "worn_prog",
  "\n"
};
