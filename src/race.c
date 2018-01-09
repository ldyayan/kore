#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "handler.h"


extern unsigned long seed;
extern struct room_data *world;
extern const int race_allows_class[];
extern const char *pc_class_types[];

ACMD(do_say);
void my_srand(unsigned long initial_seed);


/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new races to be added.  If you're adding a new race,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new race.
 */


/* Names first */

const char *race_abbrevs[] = {
  "Human ",
  "Elf   ",
  "Hobbit",
  "Dwarf ",
  "Orc   ",
  "Drow  ",
  "Bgbear",
  "Mino  ",
  "Troll ",
  "Giant ",
  "Dragon",
  "Undead",
  "HlfElf",
  "Gnome ",
  "Angel ",
  "Duergr",
  "Thri  ",
  "\n"
};


const char *pc_race_types[] = {
  "Human",
  "Elf",
  "Hobbit",
  "Dwarf",
  "Orc",
  "Drow",
  "Bugbear",
  "Minotaur",
  "Troll",
  "Giant",
  "Dragon",
  "Undead",
  "Half-elf",
  "Gnome",
  "Angel",
  "Duergar",
  "Thri-Kreen",
  "\n"
};



/* the arrays of race stuff */

const int race_stat_adjust[NUM_RACES][MAX_RACE_STAT_ADJUST] = {
/* str int wis dex con cha,
   regular permaffects: permaffect1, permaffect2, permaffect3, permaffect4,
   affected_bit2 permaffects: 1, 2, 3, 4.. */
  { 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0},                                        /* Human */
  { -2, +2, 0, +2, -1, +1,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0},					/* Elf */
  { -4, +1, -1, +3, -1, 0,
    AFF_INFRAVISION, AFF_SNEAK, 0, 0,
    0, 0, 0, 0},					/* Hobbit */
  { +1, -1, +2, -1, +2, -2,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0},  		                        /* Dwarf */
  { +2, -3, -2, -1, +2, -3,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0}, 		                        /* Orc */
  { -2, +1, -1, +2, -2, -4,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0},		                        /* Drow */
  { +2, -2, -2, +2, +2, -3,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0},         	                        /* OLD Thri-Kreen */
  { +2, -2, -2, -1, 0, -3,
    AFF_INFRAVISION, AFF_SENSE_LIFE, 0, 0,
    0, 0, 0, 0},   			                /* Minotaur */
  { +2, -2, -2, -3, +3, -3,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0},     					/* Troll */
  { +4, -2, -2, -3, +2, 0,
    0, 0, 0, 0,
    0, 0, 0, 0},    		                        /* Giant */
  { +3, +1, +1, +2, +4, -2,
    AFF_INFRAVISION, AFF_MAGIC_RESIST, AFF_FLY, 0,
    0, 0, 0, 0},                                        /* Dragon */
  { +2, +1, +3, -2, +4, -3,
    AFF_INFRAVISION, AFF_SENSE_LIFE,
    AFF_MAGIC_RESIST, AFF_DETECT_INVIS,
    0, 0, 0, 0},                                        /* Undead */
  { -1, +1,  0, +1, -1,  0,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0},				        /* Half-elf */
  { -1, +2, +2, +1, -1, -1,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0},			         	/* Gnome */
  { -3, +5, +4, +3, -2, +4,
    AFF_INFRAVISION, AFF_MAGIC_RESIST,
    AFF_SNEAK, AFF_DETECT_INVIS,
    0, 0, 0, 0},			                /* Angel */
  { +1, -1, +1, -3, +2, -3,
    AFF_INFRAVISION, 0, 0, 0,
    0, 0, 0, 0}, 				        /* Duergar */
  { +2, +1, -1, +2,  0, -5,
    AFF_SENSE_LIFE, 0, 0, 0,
    0, 0, 0, 0}				/* Thri-Kreen */
};



const sh_int race_start_room[NUM_RACES] = {
  3001,		/* Human */
  3001,		/* Elf */
  3001,		/* Hobbit */
  3001,		/* Dwarf */
  3001,		/* Orc */
  3001,		/* Drow */
  3001,		/* Bugbear */
  3001,		/* Minotaur */
  3001,		/* Troll */
  3001,		/* Giant */
  3001,		/* Dragon */
  3001,		/* Undead */
  3001,		/* Half-elf */
  3001,		/* Gnome */
  3001,		/* Angel */
  3001,		/* Duergar */
  3001,		/* Thri-Kreen */
};



/*
 * The code to interpret a race letter (used in interpreter.c when a
 * new character is selecting a race).
 */
int parse_race(char *arg)
{
  int i;
  char buf[MAX_INPUT_LENGTH];

  sprintf(buf, "%c%s", UPPER(arg[0]), arg + 1);
  for (i = 0; *(pc_race_types[i]) != '\n'; i++)
    if (!strncasecmp(buf, pc_race_types[i], strlen(arg)))
      break;
  if (!strcmp(pc_race_types[i], "\n"))
    return RACE_UNDEFINED;
  else
    return i;
}



long find_race_bitvector(char *arg)
{
  int i;
  char buf[MAX_INPUT_LENGTH];

  sprintf(buf, "%c%s", UPPER(arg[0]), arg + 1);
  for (i = 0; *(pc_race_types[i]) != '\n'; i++)
    if (!strncasecmp(buf, pc_race_types[i], strlen(arg)))
      break;
  if (!strcmp(pc_race_types[i], "\n"))
    return 0;	/* RACE_UNDEFINED */
  else
    return (1 << i);
}



int get_race_guess(struct char_data *ch) {

  if (!IS_NPC(ch))
    return GET_RACE(ch);

  if (isname("minotaur", ch->player.name))   return RACE_MINOTAUR;
  if (isname("drow", ch->player.name))       return RACE_DROW;
  if (isname("spider", ch->player.name))     return RACE_DROW;
  if (isname("drider", ch->player.name))     return RACE_DROW;
  if (isname("bugbear", ch->player.name))    return RACE_BUGBEAR;
  if (isname("orc", ch->player.name))        return RACE_ORC;
  if (isname("orcish", ch->player.name))     return RACE_ORC;
  if (isname("goblin", ch->player.name))     return RACE_ORC;
  if (isname("hobgoblin", ch->player.name))  return RACE_ORC;
  if (isname("dwarf", ch->player.name))      return RACE_DWARF;
  if (isname("dwarven", ch->player.name))    return RACE_DWARF;
  if (isname("gnome", ch->player.name))      return RACE_GNOME;
  if (isname("hobbit", ch->player.name))     return RACE_HOBBIT;
  if (isname("halfling", ch->player.name))   return RACE_HOBBIT;
  if (isname("angel", ch->player.name))      return RACE_ANGEL;
  if (isname("deva", ch->player.name))       return RACE_ANGEL;
  if (isname("cherub", ch->player.name))     return RACE_ANGEL;
  if (isname("valkyrie", ch->player.name))   return RACE_ANGEL;
  if (isname("seraph", ch->player.name))     return RACE_ANGEL;
  if (isname("elf", ch->player.name))        return RACE_ELF;
  if (isname("elven", ch->player.name))      return RACE_ELF;
  if (isname("troll", ch->player.name))      return RACE_TROLL;
  if (isname("dragon", ch->player.name))     return RACE_DRAGON;
  if (isname("dragonet", ch->player.name))   return RACE_DRAGON;
  if (isname("wyvern", ch->player.name))     return RACE_DRAGON;
  if (isname("duergar", ch->player.name))    return RACE_DUERGAR;
  if (isname("darkdwarf", ch->player.name))  return RACE_DUERGAR;
  if (isname("undead", ch->player.name))     return RACE_UNDEAD;
  if (isname("skeleton", ch->player.name))   return RACE_UNDEAD;
  if (isname("zombie", ch->player.name))     return RACE_UNDEAD;
  if (isname("ghoul", ch->player.name))      return RACE_UNDEAD;
  if (isname("shadow", ch->player.name))     return RACE_UNDEAD;
  if (isname("wight", ch->player.name))      return RACE_UNDEAD;
  if (isname("ghast", ch->player.name))      return RACE_UNDEAD;
  if (isname("wraith", ch->player.name))     return RACE_UNDEAD;
  if (isname("mummy", ch->player.name))      return RACE_UNDEAD;
  if (isname("spectre", ch->player.name))    return RACE_UNDEAD;
  if (isname("ghost", ch->player.name))      return RACE_UNDEAD;
  if (isname("lich", ch->player.name))       return RACE_UNDEAD;
  if (isname("vampire", ch->player.name))    return RACE_UNDEAD;
  if (isname("thrikreen", ch->player.name))  return RACE_THRIKREEN;
  if (isname("thri-kreen", ch->player.name)) return RACE_THRIKREEN;

  return RACE_HUMAN;
}



const char *race_kind[] = {
  "common",		/* Human, (do_say really) */
  "quenya",		/* Elf */
  "halfling",		/* Hobbit */
  "dwarven",		/* Dwarf */
  "orcish",		/* Orc */
  "drow elven",		/* Drow */
  "bugbearish",		/* OLD Thri-kreen */
  "minotaur",		/* Minotaur */
  "troll",		/* Troll */
  "giantish",		/* Giant */
  "dragon",		/* Dragon */
  "a whisper",		/* Undead */
  "quenya",		/* Half-elf */
  "gnomish",		/* Gnome */
  "an angelic voice",	/* Angel */
  "duergish",		/* Duergar */
  "thri-kreen"		/* Thri-kreen */
};



struct syllable {
  char *org;
  char *new;
};



/* a big empty array of 100 slots - useful */
#define EMPTY_LANG_SLOTS   {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, \
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}


#define MAX_SYLLABLES 100
struct syllable languages[NUM_RACES][500] = {
/* HUMAN (Westron/Common) */
  {{"a", "a"}, {"b", "b"}, {"c", "c"}, {"d", "d"}, {"e", "e"},
  {"f", "f"}, {"g", "g"}, {"h", "h"}, {"i", "i"}, {"j", "j"},
  {"k", "k"}, {"l", "l"}, {"m", "m"}, {"n", "n"}, {"o", "o"},
  {"p", "p"}, {"q", "q"}, {"r", "w"}, {"s", "s"}, {"t", "t"},
  {"u", "u"}, {"v", "v"}, {"w", "w"}, {"x", "x"}, {"y", "y"},
  {"z", "z"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* ELVEN (Sindarin) */
  {{"tower", "minas"}, {"sorcery", "morgul"}, {"moonday", "orithil"},
  {"metal", "naur"}, {"land", "dor"},
  {"wine", "winion"}, {"black", "mor"}, {"red", "caras"},
  {"island", "tol"}, {"chamber", "sammath"},
  {"hall", "thrond"}, {"white", "nimf"}, {"friend", "mellon"},
  {"host", "ath"}, {"platform", "talan"},
  {"high", "andon"}, {"sky", "vilya"}, {"bridge", "ant"},
  {"brown", "baran"}, {"sunlight", "aur"},
  {"guest", "mellon"}, {"bird", "awie"}, {"art", "echen"},
  {"ring", "cor"}, {"beyond", "pellon"},
  {"doors", "ennyn"}, {"chasm", "ia"}, {"hills", "emyn"},
  {"stair", "pinnath"}, {"rush", "agon"},
  {"fire", "carna"}, {"elf", "eldar"}, {"elves", "edhil"},
  {"elven", "tengwar"}, {"room", "talan"},
  {"iron", "ang"}, {"sudden", "bragol"}, {"horns", "rais"},
  {"silver", "celeb"}, {"pass", "cirith"},
  {"darkness", "dur"}, {"cloudy", "fanuid"}, {"head", "hol"},
  {"golden", "mal"}, {"en ", "orn "},
  {"grey", "mith"}, {"waterfall", "lanthir"}, {"copper", "paer"},
  {"stream", "sir"}, {"gate", "annon"},
  {"starhost", "giliath"}, {"lord", "aran"}, {"bronze", "evyth"},
  {"fire", "naur"}, {"willow", "tasar"},
  {"great", "lim"}, {"meal", "sang"}, {"cave", "grod"},
  {"wing", "agar"}, {"death", "gurth"},
  {"wolf", "thaur"}, {"wolve", "theyr"}, {"dire", "hudrun"},
  {"hawk", "lomin"}, {"vault", "nalaew"},
  {"people", "waith"}, {"jewel", "mir"}, {"city", "ost"},
  {"of the", "en"}, {"misty", "hithui"},
  {"mountains", "ered"}, {"fortress", "gost"}, {"might", "beleg"},
  {"dwarves", "naugrim"}, {"dwarf", "naug"},
  {"gift", "anna"}, {"gray", "mith"}, {"wild", "rhovan"},
  {"solar", "anor"}, {"void", "ia"},
  {"rock", "ond"}, {"gaze", "diriel"}, {"orcs", "yrch"},
  {"trolls", "tereg"}, {"troll", "torog"},
  {"prison", "band"}, {"hells", "band"}, {"aule", "aule"},
  {"halt", "daro"}, {"cleaver", "rist"},
  {"swordsman", "vagor"}, {"hammer", "dring"}, {"foe", "glam"},
  {"orc", "orch"}, {"home", "mar"},
  {"spider", "ungol"}, {"horror", "deloth"}, {"far", "palan"},
  {"gaze", "tir"}, {"wrath", "ruth"},
  {"ash", "lith"}, {"autumn", "iavas"}, {"awe", "gaya"},
  {"bald", "rudh"}, {"bane", "dagnir"},
  {"barrows", "tyrn"}, {"barrow", "tur"}, {"battle", "dagnir"},
  {"beech", "neldor"}, {"bend", "lok"},
  {"between", "im"}, {"birch", "brethil"}, {"blessed", "aman"},
  {"bloodstained", "agarwaen"}, {"blood", "agar"},
  {"blossom", "loth"}, {"blue", "lhun"}, {"bow", "cu"},
  {"brilliance", "ril"}, {"bull", "mundo"},
  {"chant", "lin"}, {"children", "hin"}, {"child", "hin"},
  {"cleft", "cirith"}, {"cloak", "gollo"},
  {"coast", "falas"}, {"course", "rant"}, {"cold", "ring"},
  {"command", "gon"}, {"cool", "him"},
  {"crows", "crebain"}, {"crow", "craban"}, {"cry", "nall"},
  {"cut", "cir"}, {"dark", "dur"},
  {"dauntless", "thalion"}, {"dead", "firm"}, {"demon", "raug"},
  {"devoted", "dil"}, {"dim", "du"},
  {"dog", "huan"}, {"doom", "amarth"}, {"door", "annon"},
  {"double", "adu"}, {"draw", "teith"},
  {"dread", "gor"}, {"dusk", "dome"}, {"towers", "bereid"},
  {"eagle", "thoron"}, {"earth", "arda"},
  {"ear", "lhaw"}, {"east", "rhun"}, {"echo", "lom"},
  {"empty", "lost"}, {"encircle", "echor"},
  {"circle", "echor"}, {"end", "met"}, {"treants", "enyd"},
  {"treant", "onod"}, {"enter", "minn"},
  {"eye", "hen"}, {"fading", "beleth"}, {"fade", "beleth"},
  {"fangs", "carach"}, {"farsee", "palan"},
  {"far", "palan"}, {"fate", "amarth"}, {"father", "adar"},
  {"feast", "mereth"}, {"fence", "iath"},
  {"fist", "paur"}, {"flame", "lach"}, {"plains", "talath"},
  {"flow", "dui"}, {"flower", "loth"},
  {"lorien", "dream"}, {"flow", "sirith"}, {"foam", "ros"},
  {"gape", "faug"}, {"gazing", "diriel"},
  {"glass", "heled"}, {"gleam", "glin"}, {"glitter", "bril"},
  {"sparkle", "silivren"}, {"gloomy", "dim"},
  {"gloom", "fuin"}, {"glory", "aglar"}, {"glorious", "aglareb"},
  {"goblins", "glamhoth"}, {"goblin", "glam"},
  {"golds", "mel"}, {"gold", "mal"}, {"good", "man"},
  {"gravel", "brith"}, {"greens", "gelin"},
  {"green", "galen"}, {"growth", "loa"}, {"hair", "fin"},
  {"half", "pher"}, {"harbor", "lond"},
  {"haven", "lond"}, {"hearing", "lhaw"}, {"heaven", "menel"},
  {"heavy", "blung"}, {"heir", "chil"},
  {"helm", "thol"}, {"here", "si"}, {"hidden", "dolen"},
  {"hiding", "esgal"}, {"hill", "amon"},
  {"hither", "nev"}, {"hollow", "nov"}, {"holly", "ereg"},
  {"land", "ion"}, {"home", "bar"},
  {"hope", "estel"}, {"horn", "ras"}, {"horde", "hoth"},
  {"horse", "roch"}, {"hound", "huan"},
  {"hour", "lumenn"}, {"howl", "ngwaw"}, {"hunt", "faroth"},
  {"ice", "khelek"}, {"jewel", "mir"},
  {"keep", "cheb"}, {"kindler", "thoniel"}, {"kindle", "thonie"},
  {"kings", "erain"}, {"king", "aran"},
  {"knowledge", "golodh"}, {"lady", "hiril"}, {"lair", "torech"},
  {"lake", "aelin"}, {"last", "vedui"},
  {"laughter", "lalaith"}, {"laugh", "lala"}, {"lawn", "parth"},
  {"letter", "tiw"}, {"life", "coi"},
  {"light", "galad"}, {"listen", "last"}, {"lofty", "tar"},
  {"lonely", "ereb"}, {"mountain", "orod"},
  {"maiden", "riel"}, {"men", "edain"}, {"loop", "tok"},
  {"lord", "hir"}, {"love", "mel"},
  {"master", "tur"}, {"meeting", "omentielvo"}, {"mesh", "rem"},
  {"armor", "ennon"}, {"mortal", "fir"},
  {"mounds", "tyrn"}, {"mound", "tur"}, {"nectar", "miruvor"},
  {"new", "vinya"}, {"night", "du"},
  {"nightingale", "dulin"}, {"noble", "ar"}, {"noise", "glam"},
  {"north", "forod"}, {"number", "rim"},
  {"ocean", "aear"}, {"old", "iar"}, {"elephants", "mumakil"},
  {"elephant", "mumak"}, {"oliphants", "mumakil"},
  {"oliphant", "mumak"}, {"open", "edr"}, {"oppression", "thang"},
  {"oppress", "thang"}, {"outflow", "ethir"},
  {"out", "eth"}, {"petty", "nibin"}, {"gnomes", "nibinaugrim"},
  {"gnome", "nibinaug"}, {"pillar", "tarma"},
  {"pine", "thon"}, {"point", "aeg"}, {"prince", "ernil"},
  {"hell", "band"}, {"pursue", "faroth"},
  {"queen", "bereth"}, {"radiance", "galad"}, {"rainbow", "ninniach"},
  {"ransom", "danwedh"}, {"refuser", "avar"},
  {"realm", "arth"}, {"ride", "nor"}, {"ridge", "pinnath"},
  {"root", "thond"}, {"royal", "ar"},
  {"runes", "certhas"}, {"rune", "cirth"}, {"rushing", "alag"},
  {"screen", "esgal"}, {"serpents", "lyg"},
  {"seven", "odo"}, {"shadow", "dae"}, {"shadow", "morchaint"},
  {"sharp", "maeg"}, {"ship", "cair"},
  {"shore", "falas"}, {"shudder", "girith"}, {"sickly", "engwa"},
  {"sick", "engwa"}, {"sign", "thiw"},
  {"singer", "linde"}, {"skill", "curu"}, {"slant", "penn"},
  {"slender", "fim"}, {"slim", "fim"},
  {"snakes", "lyg"}, {"snake", "lhug"}, {"serpent", "lhug"},
  {"snowy", "lossen"}, {"snow", "loss"},
  {"south", "harad"}, {"spiderweb", "ungwe"}, {"spring", "ethuil"},
  {"stakes", "cebir"}, {"stake", "ceber"},
  {"stone", "rond"}, {"stray", "raen"}, {"stream", "hir"},
  {"street", "rath"}, {"strong", "thalion"},
  {"summer", "laer"}, {"sunrise", "run"}, {"sunset", "annun"},
  {"sunday", "oranor"}, {"swan", "alph"},
  {"sword", "megil"}, {"thorn", "ereg"}, {"thousand", "mene"},
  {"thread", "lain"}, {"three", "nelde"},
  {"tower", "barad"}, {"dwelling", "bar"}, {"torrent", "thor"},
  {"treesday", "orgaladhad"}, {"trees", "galadhad"},
  {"valiant", "astaldo"}, {"valley", "imlad"}, {"veil", "fana"},
  {"voice", "lammen"}, {"wall", "ram"},
  {"water", "nen"}, {"way", "bad"}, {"well", "eithel"},
  {"werewolf", "ngaur"}, {"west", "annun"},
  {"window", "henneth"}, {"wind", "gwae"}, {"winter", "rhiw"},
  {"woods", "eryn"}, {"wood", "erin"},
  {"word", "beth"}, {"wose", "dru"}, {"wrights", "dain"},
  {"wright", "dan"}, {"year", "loa"},
  {"region", "arth"}, {"black", "rast"}, {"cape", "vorn"},
  {"angle", "egladil"}, {"north", "for"},
  {"stone", "gond"}, {"west", "numen"}, {"east", "rhud"},
  {"mountains", "aeglir"}, {"spear", "aeg"},
  {"sea", "gaer"}, {"holy", "aina"}, {"gift", "ian"}, 
  {"anger", "ruth"}, {"wolf", "draug"},
  {"starsday", "orgilion"}, {"cleaver", "cir"}, {"breeze", "hesta"},
  {"rauko", "demon"}, {"cleaver", "kir"},
  {"starjewel", "elmir"}, {"sun", "anor"}, {"moon", "orithil"},
  {"monday", "orithil"}, {"tree", "galadh"},
  {"tuesday", "orgaladhad"}, {"january", "narwain"}, {"february", "ninui"},
  {"march", "gwaeron"}, {"april", "gwirith"},
  {"may", "lothron"}, {"june", "norui"}, {"july", "cerveth"},
  {"auguest", "urui"}, {"hot", "urui"},
  {"september", "ivanneth"}, {"october", "narbeleth"}, {"november", "hithui"},
  {"mist", "hith"}, {"december", "girithron"},
  {"fruitful", "iavas"}, {"fall", "iavas"}, {"fading", "firith"},
  {"stirring", "echuir"}, {"chill", "ring"},
  {"fire", "ruin"}, {"chill", "rin"}, {"fist", "bor"},
  {"cloak", "gol"}, {"middle", "en"},
  {"watch", "antir"}, {"mighty", "beleg"}, {"hand", "cam"},
  {"star", "gil"}, {"sea", "aer"},
  {"gift", "tar"}, {"wind", "suth"}, {"cloak", "collo"},
  {"shadow", "ath"}, {"feet", "dali"},
  {"foot", "dal"}, {"necklace", "lamir"}, {"ever", "ui"},
  {"forever", "ui"}, {"golden", "glor"},
  {"hollen", "close"}, {"path", "nilon"}, {"flames", "lhach"},
  {"flame", "lhach"}, {"cloud", "fanuid"},
  {"cloudy", "fanui"}, {"constrainer", "bauglir"}, {"dusk", "moth"},
  {"fire", "nar"}, {"sea", "endil"},
  {"sul", "friend"}, {"long thread", "laen"}, {"titanium steel", "adarcer"},
  {"tin", "alcam"}, {"steel", "borang"},
  {"burning silver", "celebur"}, {"ang eol", "eog"}, {"aluminum", "galnin"},
  {"shining black", "galvorn"}, {"moonstar", "ithildin"},
  {"platinum", "mithglin"}, {"beryllium", "mithin"}, {"of", "o"},
  {"the", "en"}, {"point", "dil"},
  {"peaks", "aeglir"}, {"peak", "aeglir"}, {"", ""},
  {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  },
/* HALFLING */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* DWARVEN (Khuzdul) */
  {{"dwarf", "khazad"}, {"mansion", "dum"}, {"glass", "kheled"},
  {"lake", "zaram"}, {"e", "om"},
  {"records", "mazar"}, {"hall", "dim"}, {"lone", "ere"},
  {"mountain", "bor"}, {"cavern", "felik"},
  {"flowers", "bulum"}, {"barag", "green"}, {"cave", "felak"},
  {"bear", "buarndur"}, {"maker", "mahal"},
  {"aule", "mahal"}, {"axes", "baruk"}, {"dwarvish", "khuzdul"},
  {"dwarves", "khazad"}, {"dim", "azan"},
  {"red", "baraz"}, {"vale", "bizar"}, {"flower", "bulum"}, 
  {"clouded", "bundus"}, {"cloudy", "bundus"},
  {"cloud", "bundus"}, {"cold", "kibil"}, {"head", "hathur"},
  {"pass", "lagil"}, {"black", "narag"},
  {"spring", "nala"}, {"streams", "nul"}, {"stream", "nul"},
  {"pool", "zaram"}, {"mirror", "zigil"},
  {"mere", "zaram"}, {"horn", "zinbar"}, {"zirak", "silver"},
  {"watch", "garul"}, {"weapon", "vabnad"},
  {"host", "hur"}, {"guard", "virdhir"}, {"lane", "gehil"},
  {"magic", "tharka"}, {"mystic", "tharka"},
  {"warder", "arul"}, {"unit", "menig"}, {"attack", "ashoken"},
  {"archer", "bogazad"}, {"battle", "huval"},
  {"water", "zirim"}, {"passage", "ligil"}, {"balrog", "valarauko"},
  {"fire", "durbag"}, {"eat", "hash"},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* ORCISH (Black Speech) */
  {{"dung", "bag"}, {"hole", "ronk"}, {"filth", "glob"},
  {"five", "crak"}, {"ing", "dog"},
  {"one", "ash"}, {"pit", "ronk"}, {"ringwraith", "nazgul"},
  {"ring", "nazg"}, {"shit", "bag"},
  {"slop", "glob"}, {"stink", "push"}, {"to bring them all and in the darkness bind them", "agh burzum-ishi krimpatul"},
  {"to find them", "gimbatul"}, {"to rule them all", "durbatuluuk"},
  {"to the", "u"}, {"with", "sha"}, {"", ""},
  {"", ""}, {"", ""},
  {"gah", "gah"}, {"st", "ash"}, {"fire", "ghash"},
  {"ate", "kash"}, {"ing", "ksh"},
  {"ion", "nahk"}, {"pre", "dikh"}, {"el", "mar"},
  {"al", "mil"}, {"ect", "kesh"},
  {"en", "kil"}, {"gro", "dar"}, {"lord", "sauron"},
  {"lo", "ish"}, {"mag", "mor"},
  {"mon", "bar"}, {"mor", "zak"}, {"move", "sikh"},
  {"ness", "yak"}, {"ning", "bahk"},
  {"per", "gresh"}, {"ra", "gak"}, {"re", "bak"},
  {"son", "kshi"}, {"ect", "arkh"},
  {"tri", "kardks"}, {"ven", "mofo"}, {"word of", "kahn"},
  {"a", "ahk"}, {"b", "bik"},
  {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"},
  {"m", "w"}, {"n", "b"}, {"o", "a"}, {"p", "s"}, {"q", "k"},
  {"r", "a"}, {"s", "y"}, {"t", "h"}, {"u", "e"}, {"v", "z"},
  {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* DROW ELVEN */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* OLD THRI-KREEN */
  {{" ", " "}, {"a", "clk'"}, {"b", "tic'"}, {"c", "buzz"}, {"d", "rik'"},
  {"e", "ok"}, {"f", "kik"}, {"g", "tik'"}, {"h", "z"}, {"i", "vic'"},
  {"j", "krrk'"}, {"k", "t"}, {"l", "rzt"}, {"m", "wee-oo"}, {"n", "bzz"},
  {"o", "aiik"}, {"p", "sip!"}, {"q", "krick"}, {"r", "ai"}, {"s", "s"},
  {"t", "v"}, {"u", "eeek"}, {"v", "z'"}, {"w", "xic"}, {"x", "nii"},
  {"y", "lo!"}, {"z", "k"}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* MINOTAUR */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* TROLL */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, 
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* GIANT */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* DRAGON */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* UNDEAD */
  {{" ", " "}, {"a", "a"}, {"b", "b"}, {"c", "c"}, {"d", "d"},
  {"e", "e"}, {"f", "f"}, {"g", "g"}, {"h", "h"}, {"i", "i"},
  {"j", "j"}, {"k", "k"}, {"l", "l"}, {"m", "m"}, {"n", "n"},
  {"o", "o"}, {"p", "p"}, {"q", "q"}, {"r", "r"}, {"s", "ss"},
  {"t", "t"}, {"u", "u"}, {"v", "v"}, {"w", "w"}, {"x", "x"},
  {"y", "y"}, {"z", "z"}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* HALFELF */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* GNOMISH */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* ELEMENTAL */
  {{"a", "foo"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* DUERGAR */
  {{"a", "ahk"}, {"b", "bik"}, {"c", "ckh"}, {"d", "r"}, {"e", "om"},
  {"f", "ky"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
  {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
  {"p", "s"}, {"q", "k"}, {"r", "a"}, {"s", "y"}, {"t", "h"},
  {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
  {"z", "k"}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
  EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
  EMPTY_LANG_SLOTS },
/* THRI-KREEN */
  { EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS,
    EMPTY_LANG_SLOTS, EMPTY_LANG_SLOTS }
};



/* EXPERIMENTAL language system */
#define MAX_LANGUAGES2          3
#define SYLLABLE_PROFILE        10
#define SOUNDS_PER_LANG         15
const int syllable_profile[MAX_LANGUAGES2][SYLLABLE_PROFILE] = {
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 2, 2, 2, 2, 3 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};
const char *sounds[MAX_LANGUAGES2][SOUNDS_PER_LANG] = {
/* DEFAULT */
  { "ah",   "gah",  "feh",   "heh",   "bwa",
    "geh",  "neh",  "ra",    "mm",    "o",
    "jeh",  "gleh", "fle",   "ss",    "gr" },
/* DIRTY */
  { "fuck", "piss", "shit",  "damn",  "cunt",
    "dick", "slut", "bitch", "whore", "jesus",
    "me",   "suck", "turd",  "god",   "you" },
/* SNUGGLE */
  { "aww",     "cuddle", "snuggle", "lovey", "bunny",
    "love",    "kisses", "miss",    "you",   "snuggloe",
    "huggles", "hug",    "luv",     "aww",   "her" }
};

/*
 * this is a utility to make up a new word
 * it returns the strlen() of the made up word
 */
char *made_up_word(int race, char *word)
{
  static char buf[MAX_STRING_LENGTH];
  static char buf2[MAX_STRING_LENGTH];
  unsigned long oldseed;
  int wordlen;
  int i;
  unsigned long wordtotal;
  int lang;


  /* figure out what language of sounds to use */
  lang = 0; /* default */

  /* save the seed */
  oldseed = seed;

  /* reseed the random number generator with the words ascii chars totalled */
  wordlen = strlen(word);
  for (i = 0, wordtotal = 0; i < wordlen; i++)
    wordtotal += word[i];
  my_srand(wordtotal);

  /* spew out the made up word (same for a given word every time) */
  *buf = '\0';
  for (i = 0; i < syllable_profile[lang][number(0, SYLLABLE_PROFILE - 1)]; i++) {
    sprintf(buf2, "%s", sounds[lang][number(0, SOUNDS_PER_LANG - 1)]);
    strcat(buf, buf2);
  }

  /* restore the seed */
  my_srand(oldseed);

  return buf;
}



ACMD(do_speak)
{
  char lbuf[256];
  char tbuf[256];
  struct char_data *i;
  int j, ofs = 0;
  int race_guess;
  int found;


  skip_spaces(&argument);

  if (IS_NPC(ch)) {
    GET_MOB_RACE(ch) = get_race_guess(ch);
  } else {
    GET_PC_RACE(ch) = get_race_guess(ch);
  }

  if (GET_RACE(ch) == RACE_UNDEFINED) {
    send_to_char("Sorry, can't tell what language you speak.\r\n", ch);
    return;
  }

  race_guess = GET_RACE(ch);
 
  if (!*argument) {
    send_to_char("Yes, but WHAT do you want to speak?\r\n", ch);
  } else if ((race_guess < 0) || (race_guess >= NUM_RACES)) {
    do_say(ch, argument, 0, 0);
  } else {
    *buf = '\0';
    strcpy(lbuf, argument);

    /* first turn upper into lower case */
    for (j = 0; j < strlen(lbuf); j++) {
      lbuf[j] = tolower(lbuf[j]);
    }

    while (*(lbuf + ofs)) {
      if (!isalpha(*(lbuf + ofs))) {	/* skip non alphanumeric */
        sprintf(tbuf, "%c", *(lbuf + ofs));
        strcat(buf, tbuf);
        ofs++;
        continue;
      }
      found = 0;
      for (j = 0; *(languages[race_guess][j].org); j++) {
        if (!strncmp(languages[race_guess][j].org, lbuf + ofs,
                     strlen(languages[race_guess][j].org))) {
          strcat(buf, languages[race_guess][j].new);
          ofs += strlen(languages[race_guess][j].org);
          found = 1;
        }
      }
      /* totally make up a word */
      if (!found) {
        *buf2 = '\0';
        while (isalpha(*(lbuf + ofs))) {
          sprintf(tbuf, "%c", *(lbuf + ofs));
          strcat(buf2, tbuf);
          ofs++;
        }
        strcat(buf, made_up_word(race_guess, buf2));
      }
      /* end of totally made up words */
    }

    /* buf3 = who said it */
    sprintf(buf3, "%s", GET_NAME(ch));
    buf3[0] = toupper(buf3[0]);
    /* buf1 = what was said (normal) */
    sprintf(buf1, "in %s, '%s'\r\n",
        race_kind[race_guess], argument);
    /* buf2 = what was said (munged) */
    sprintf(buf2, "in %s, '%s'\r\n",
        race_kind[race_guess], buf);

    for (i = world[ch->in_room].people; i; i = i->next_in_room) {
      if (i == ch) {
        send_to_char("You say ", i);
        send_to_char(buf1, i);
/*
      } else if (GET_RACE(i) == race_guess) {
*/
      } else if (race_kind[GET_RACE(ch)] == race_kind[GET_RACE(i)]) {
        send_to_char(buf3, i);
        send_to_char(" says ", i);
        send_to_char(buf1, i);
      } else {
        send_to_char(buf3, i);
        send_to_char(" says ", i);
        send_to_char(buf2, i);
      }
    }
  }

  return;
}



ACMD(do_translate)
{
  char lbuf[256];
  char tbuf[256];
  struct char_data *i;
  int j, ofs = 0;
  int race_guess;
  int found;

 
  skip_spaces(&argument);

  if (GET_RACE(ch) == RACE_UNDEFINED) {
    send_to_char("Sorry, can't tell what language you speak.\r\n", ch);
    return;
  }

  race_guess = GET_RACE(ch);

  if (!*argument) {
    send_to_char("Yes, but WHAT do you want to translate?\r\n", ch);
/* humans now translate, like other races */
/*
  } else if (race_guess == RACE_HUMAN) {
    do_say(ch, argument, 0, 0);
*/
  } else if ((race_guess < 0) || (race_guess >= NUM_RACES)) {
    do_say(ch, argument, 0, 0);
  } else {
    *buf = '\0';
    strcpy(lbuf, argument);

    /* first turn upper into lower case */
    for (j = 0; j < strlen(lbuf); j++) {
      lbuf[j] = tolower(lbuf[j]);
    }

    while (*(lbuf + ofs)) {
      if (!isalpha(*(lbuf + ofs))) {    /* skip non alphanumeric */
        sprintf(tbuf, "%c", *(lbuf + ofs));
        strcat(buf, tbuf);
        ofs++;
        continue;
      }
      found = 0;
      for (j = 0; *(languages[race_guess][j].new); j++) {
        if (!strncmp(languages[race_guess][j].new, lbuf + ofs,
                     strlen(languages[race_guess][j].new))) {
          strcat(buf, languages[race_guess][j].org);
          ofs += strlen(languages[race_guess][j].new);
          found = 1;
        }
      }
      /* just add the letters */
      if (!found) {
        *buf2 = '\0';
        while (isalpha(*(lbuf + ofs))) {
          sprintf(tbuf, "%c", *(lbuf + ofs));
          strcat(buf2, tbuf);
          ofs++;
        }
        strcat(buf, buf2);
      }
      /* added letters */
    }

    /* buf3 = who said it */
    sprintf(buf3, "%s", GET_NAME(ch));
    buf3[0] = toupper(buf3[0]);
    /* buf1 = what was said (translated) */
    sprintf(buf1, "'%s'\r\n", buf);

    for (i = world[ch->in_room].people; i; i = i->next_in_room) {
      if (i == ch) {
        send_to_char("You say, ", i);
        send_to_char(buf1, i);
      } else {
        send_to_char(buf3, i);
        send_to_char(" translates, ", i);
        send_to_char(buf1, i);
      }
    }
  }

  return;
}



/* These are the races that people can choose from */
const int race_choice_list[] = {
  RACE_HUMAN,
  RACE_ELF,
  RACE_HOBBIT,
  RACE_DWARF,
  RACE_DROW,
/*
  RACE_BUGBEAR,
*/
  RACE_ORC,
  RACE_TROLL,
  RACE_MINOTAUR,
  RACE_GIANT,
  RACE_HALFELF,
  RACE_GNOME,
  RACE_DUERGAR,
  -1
};

const int class_choice_list[] = {
  CLASS_MAGIC_USER,
  CLASS_CLERIC,
  CLASS_THIEF,
  CLASS_WARRIOR,
  CLASS_BARD,
  CLASS_DRUID,
  -1
};

/*
 * Because the classes available to each race change all the time,
 * the code that creates the menu of what is available is all in here.
 * This menu is used to choose a class by race in interpreter.c
 */

char *race_menu;

void setup_race_menu(void)
{
  int i, j;


  strcpy(buf, "\r\n"
              "^ySelect a^n ^Crace^n^y:^n          ^y[Allowed classes]^n\r\n");
  for (i = 0; race_choice_list[i] != -1; i++) {
    sprintf(buf2, "  ^R%-17s^n", pc_race_types[race_choice_list[i]]);
    strcat(buf, buf2);
    for (j = 0; class_choice_list[j] != -1; j++) {
      if (IS_SET(race_allows_class[race_choice_list[i]],
                 BIT(class_choice_list[j]))) {
        sprintf(buf2, "%s  ", pc_class_types[class_choice_list[j]]);
        strcat(buf, buf2);
      } else {
        sprintf(buf2, "%*s  ",
            strlen(pc_class_types[class_choice_list[j]]), "");
        strcat(buf, buf2);
      }
    }
    strcat(buf, "\r\n");
  } 

  CREATE(race_menu, char, strlen(buf) + 1);
  strcpy(race_menu, buf);
}
