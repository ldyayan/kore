/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"

void mprog_read_programs(FILE * fp, struct index_data * pMobIndex);
void mprog_boot(void);
void reload_mprogs(void);
void boot_pawnshop(void);
void boot_pets(void);
void boot_event_queue();
void boot_objprogs();
void reload_objprogs();

char err_buf[MAX_STRING_LENGTH];

#define bug(x, y) {sprintf(err_buf, (x), (y)); log(err_buf); }

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

struct room_data *world = NULL;	/* array of rooms		 */
int top_of_world = 0;		/* ref to top element of world	 */

struct char_data *character_list = NULL;	/* global linked list of
						 * chars	 */
struct index_data *mob_index;	/* index table for mobile file	 */
struct char_data *mob_proto;	/* prototypes for mobs		 */
int top_of_mobt = 0;		/* top of mobile index table	 */

struct obj_data *object_list = NULL;	/* global linked list of objs	 */
struct index_data *obj_index;	/* index table for object file	 */
struct obj_data *obj_proto;	/* prototypes for objs		 */
int top_of_objt = 0;		/* top of object index table	 */

struct zone_data *zone_table;	/* zone table			 */
int top_of_zone_table = 0;	/* top element of zone tab	 */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */

struct player_index_element *player_table = NULL;	/* index to plr file	 */
FILE *player_fl = NULL;		/* file desc of player file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;		/* ref of size of p file	 */
long top_idnum = 0;		/* highest idnum in use		 */

int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
sh_int r_mortal_start_room;	/* rnum of mortal start room	 */
sh_int r_immort_start_room;	/* rnum of immort start room	 */
sh_int r_frozen_start_room;	/* rnum of frozen start room	 */
sh_int r_mortuary_start_room;	/* rnum of mortuary start room   */
sh_int r_race_start_room[NUM_RACES];	/* rnums of race start rooms */
sh_int r_lowbie_start_room;	/* rnum of lowbie start room	 */
sh_int r_clan_start_room[NUM_CLANS];	/* rnums of clan start rooms */

char *credits = NULL;		/* game credits			*/
char *news = NULL;		/* mud news			*/
char *motd = NULL;		/* message of the day - mortals */
char *imotd = NULL;		/* message of the day - immorts */
char *help = NULL;		/* help screen			*/
char *clanhelp = NULL;		/* clanhelp screen		*/
char *areahelp = NULL;		/* areahelp screen		*/
char *wizhelp = NULL;		/* wizhelp screen		*/
char *info = NULL;		/* info page			*/
char *wizlist = NULL;		/* list of higher gods		*/
char *immlist = NULL;		/* list of peon gods		*/
/* HACKED to add clanlist */
char *clanlist = NULL;		/* list of clan members		*/
/* end of hack */
char *background = NULL;	/* background story		*/
char *handbook = NULL;		/* handbook for new immortals	*/
char *policies = NULL;		/* policies page		*/

FILE *help_fl = NULL;		/* file for help text		*/
struct help_index_element *help_index = 0;	/* the help table	*/
int top_of_helpt;		/* top of help index table	*/
FILE *clanhelp_fl = NULL;	/* file for clanhelp text */
struct help_index_element *clanhelp_index = 0;	/* the clanhelp table	*/
int top_of_clanhelpt;		/* top of clanhelp index table  */
FILE *areahelp_fl = NULL;	/* file for areahelp text */
struct help_index_element *areahelp_index = 0;  /* the areahelp table   */
int top_of_areahelpt;           /* top of areahelp index table  */
FILE *wizhelp_fl = NULL;	/* file for wizhelp text	*/
struct help_index_element *wizhelp_index = 0;	/* the wizhelp table	*/
int top_of_wizhelpt;		/* top of wizhelp index table	*/

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	 */
struct reset_q_type reset_q;	/* queue of zones to be reset	 */
struct olc_perm_data  *olc_perms;	/* DRANOR olc global */
char olc_onoff=OFF;			/* DRANOR olc global */

extern int get_race_guess(struct char_data *ch);
extern int get_class_guess(struct char_data *ch);
extern char *pc_race_types[];
extern char *pc_class_types[];


/* local functions */
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
void char_to_store(struct char_data * ch, struct char_file_u * st);
void store_to_char(struct char_file_u * st, struct char_data * ch);
int is_empty(int zone_nr);
void reset_zone(int zone);
int file_to_string(char *name, char *buf);
int file_to_string_alloc(char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(int zone, int cmd_no, char *message);
void reset_time(void);
void clear_char(struct char_data * ch);

/* external functions */
extern struct descriptor_data *descriptor_list;
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);	/* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
struct help_index_element *build_help_index(FILE * fl, int *num);
void generate_experience_tables(void);
void load_ship(int number, int room);
void Ship_boot(void);
extern char *affected_bits[];
extern char *affected2_bits[];
extern char *apply_types[];
extern char *spells[];
void auction_reset(void);
void setup_race_menu(void);
void dspace_object_boot(void);
void load_storage_rooms(void);
void boot_storage_rooms(void);
void boot_restricted_rooms(void);
void boot_clan_level_text();
void reload_clan_text();

/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */

/* Let's start a clanlist boots search */
/* HACKED by Brian to begin */

void reboot_clanlists(void)
{
  file_to_string_alloc(CLANLIST_FILE, &clanlist);
}

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}


ACMD(do_reboot)
{
  int i;
  char *options[] = {	/* just for show */
    "news",
    "credits",
    "motd",
    "imotd",
    "helpscreen",
    "clanhelpscreen",
    "areahelpscreen",
    "wizhelpscreen",
    "info",
    "wizlist",
    "immlist",
    "policy",
    "handbook",
    "background",
    "help",
    "clanhelp",
    "areahelp",
    "wizhelp",
    "mobprogs",
    "objprogs",
    "clanlevels",
    "\n"
  };

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(CLANHELP_PAGE_FILE, &clanhelp);
    file_to_string_alloc(AREAHELP_PAGE_FILE, &areahelp);
    file_to_string_alloc(WIZHELP_PAGE_FILE, &wizhelp);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
/* HACKED for clanlist */
    file_to_string_alloc(CLANLIST_FILE, &clanlist);
/* end of hack */
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
  } else if (!str_cmp(arg, "wizlist"))
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
  else if (!str_cmp(arg, "immlist"))
    file_to_string_alloc(IMMLIST_FILE, &immlist);
/* HACKED for clanlist */
  else if (!str_cmp(arg, "clanlist"))
    file_to_string_alloc(CLANLIST_FILE, &clanlist);
/* end of hack */
  else if (!str_cmp(arg, "news"))
    file_to_string_alloc(NEWS_FILE, &news);
  else if (!str_cmp(arg, "credits"))
    file_to_string_alloc(CREDITS_FILE, &credits);
  else if (!str_cmp(arg, "motd"))
    file_to_string_alloc(MOTD_FILE, &motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string_alloc(IMOTD_FILE, &imotd);
  else if (!str_cmp(arg, "helpscreen"))
    file_to_string_alloc(HELP_PAGE_FILE, &help);
  else if (!str_cmp(arg, "clanhelpscreen"))
    file_to_string_alloc(CLANHELP_PAGE_FILE, &clanhelp);
  else if (!str_cmp(arg, "areahelpscreen"))
    file_to_string_alloc(AREAHELP_PAGE_FILE, &areahelp);
  else if (!str_cmp(arg, "wizhelpscreen"))
    file_to_string_alloc(WIZHELP_PAGE_FILE, &wizhelp);
  else if (!str_cmp(arg, "info"))
    file_to_string_alloc(INFO_FILE, &info);
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "background"))
    file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "help")) {
    if (help_fl)
      fclose(help_fl);
    if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
      return;
    else {
      for (i = 0; i < top_of_helpt; i++)
	free(help_index[i].keyword);
      free(help_index);
      help_index = build_help_index(help_fl, &top_of_helpt);
    }
  } else if (!str_cmp(arg, "clanhelp")) {
    if (clanhelp_fl)
      fclose(clanhelp_fl);
    if (!(clanhelp_fl = fopen(CLANHELP_KWRD_FILE, "r")))
      return;
    else {
      for (i = 0; i < top_of_clanhelpt; i++)
        free(clanhelp_index[i].keyword);
      free(clanhelp_index);
      clanhelp_index = build_help_index(clanhelp_fl, &top_of_clanhelpt);
    }
  } else if (!str_cmp(arg, "areahelp")) {
    if (areahelp_fl)
      fclose(areahelp_fl);
    if (!(areahelp_fl = fopen(AREAHELP_KWRD_FILE, "r")))
      return;
    else {
      for (i = 0; i < top_of_areahelpt; i++)
        free(areahelp_index[i].keyword);
      free(areahelp_index);
      areahelp_index = build_help_index(areahelp_fl, &top_of_areahelpt);
    }
  } else if (!str_cmp(arg, "wizhelp")) {
    if (wizhelp_fl)
      fclose(wizhelp_fl);
    if (!(wizhelp_fl = fopen(WIZHELP_KWRD_FILE, "r")))
      return;
    else {
      for (i = 0; i < top_of_wizhelpt; i++)
        free(wizhelp_index[i].keyword);
      free(wizhelp_index);
      wizhelp_index = build_help_index(wizhelp_fl, &top_of_wizhelpt);
    }
  } else if (!str_cmp(arg, "mobprogs")) {
    reload_mprogs();
  } else if (!str_cmp(arg, "objprogs")) {
    reload_objprogs();
  } else if (!str_cmp(arg, "clanlevels")) {
    reload_clan_text();
  } else {
    sprintf(buf, "Unknown reload option, choose one of:");
    for (i = 0; str_cmp(options[i], "\n"); i++) {
      if (i % 6 == 0)
        strcat(buf, "\r\n");
      sprintf(buf1, "%-12s", options[i]);
      strcat(buf, buf1);
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  send_to_char(OK, ch);
}


/* body of the booting system */
void boot_db(void)
{
  int i;
  extern int no_specials;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  /* moved reading news credits help bground info and motds down */
  /* moved opening help file and the building of its index down */

  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading mobprogs.");
  mprog_boot();

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);
  
  log("Loading objprogs.");
  boot_objprogs();

  log("Renumbering zone table.");
  renum_zone_table();

  log("Generating player index.");
  build_player_index();

  log("Generating player experience tables.");
  generate_experience_tables();

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();
  
  log("Loading custom clan text.");
  boot_clan_level_text();

  if (!no_specials) {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }
  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
  }
  log("   Spells.");
  mag_assign_spells();

  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  log("Booting mail system.");
  if (!scan_file()) {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }

  log("Reading news, credits, helpscreens, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(CLANHELP_PAGE_FILE, &clanhelp);
  file_to_string_alloc(AREAHELP_PAGE_FILE, &areahelp);
  file_to_string_alloc(WIZHELP_PAGE_FILE, &wizhelp);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
/* HACKED to add clanlist */
  file_to_string_alloc(CLANLIST_FILE, &clanlist);
/* end of hack */
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);

  log("Opening help file.");
  if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
    log("   Could not open help file.");
  else
    help_index = build_help_index(help_fl, &top_of_helpt);
  log("Opening clanhelp file.");
  if (!(clanhelp_fl = fopen(CLANHELP_KWRD_FILE, "r")))
    log("   Could not open clanhelp file.");
  else
    clanhelp_index = build_help_index(clanhelp_fl, &top_of_clanhelpt);
  log("Opening areahelp file.");
  if (!(areahelp_fl = fopen(AREAHELP_KWRD_FILE, "r")))
    log("   Could not open areahelp file.");
  else
    areahelp_index = build_help_index(areahelp_fl, &top_of_areahelpt);
  log("Opening wizhelp file.");
  if (!(wizhelp_fl = fopen(WIZHELP_KWRD_FILE, "r")))
    log("   Could not open wizhelp file.");
  else
    wizhelp_index = build_help_index(wizhelp_fl, &top_of_wizhelpt);

  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("Done.");
  }
  for (i = 0; i <= top_of_zone_table; i++) {
    sprintf(buf2, "Resetting %s (rooms %d-%d).",
	    zone_table[i].name, (i ? (zone_table[i - 1].top + 1) : 0),
	    zone_table[i].top);
    log(buf2);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

/* HACKED to not boot houses */
/*
  if (!mini_mud) {
    log("Booting houses.");
    House_boot();
  }
*/
/* end of hack */
  log("Booting crash-saved rooms.");
  boot_storage_rooms();
  
  log("Loading crash-saved rooms.");
  load_storage_rooms();
  
  log("Booting restricted-access rooms.");
  boot_restricted_rooms();
  
  log("Booting pawnshop.");
  boot_pawnshop();
  
  log ("Booting pets.");
  boot_pets();

  if (!mini_mud) {
    log("Booting ships.");
    Ship_boot();
  }

/*
  if (!mini_mud) {
    log("Booting DSpace objects.");
    dspace_object_boot();
  }
*/

  /* auction foo */
  log("Auction system reset.");
  auction_reset();     
  
  log("Event queue reset.");
  boot_event_queue();

  log("Race menu set.");
  setup_race_menu();

  boot_time = time(0);

  MOBTrigger = TRUE;

  log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
  long beginning_of_time = 650336715;
  struct time_info_data mud_time_passed(time_t t2, time_t t1);

  time_info = mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  sprintf(buf, "   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);
  log(buf);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}



/* generate index table for the player file */
void build_player_index(void)
{
  int nr = -1, i;
  long size, recs;
  struct char_file_u dummy;

  if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
    if (errno != ENOENT) {
      perror("fatal error opening playerfile");
      exit(1);
    } else {
      log("No playerfile.  Creating a new one.");
      touch(PLAYER_FILE);
      if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
	perror("fatal error opening playerfile");
	exit(1);
      }
    }
  }

  fseek(player_fl, 0L, SEEK_END);
  size = ftell(player_fl);
  rewind(player_fl);
  if (size % sizeof(struct char_file_u))
    fprintf(stderr, "\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!\n");
  recs = size / sizeof(struct char_file_u);
  if (recs) {
    sprintf(buf, "   %ld players in database.", recs);
    log(buf);
    CREATE(player_table, struct player_index_element, recs);
  } else {
    player_table = NULL;
    top_of_p_file = top_of_p_table = -1;
    return;
  }

  for (; !feof(player_fl);) {
    fread(&dummy, sizeof(struct char_file_u), 1, player_fl);
    if (!feof(player_fl)) {	/* new record */
      nr++;
      CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
      for (i = 0;
	   (*(player_table[nr].name + i) = LOWER(*(dummy.name + i))); i++);
      player_table[nr].id = dummy.char_specials_saved.idnum;
      player_table[nr].level = dummy.level;
      if (player_table[nr].level >= LVL_IMMORT) {
        sprintf(buf, "   WIZLIST: Level %d %s",
            player_table[nr].level, player_table[nr].name);
        log(buf);
      }
/* HACKED to add clanlist, don't think this will work first try
      if (player_table[nr].level >= LVL_IMMORT) {
        sprintf(buf, "   WIZLIST: Level %d %s",
            player_table[nr].level, player_table[nr].name);
        log(buf);
      }
      if (player_table[nr].level >= LVL_IMMORT) {
        sprintf(buf, "   WIZLIST: Level %d %s",
            player_table[nr].level, player_table[nr].name);
        log(buf);
      }
 end of hack */
      top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum);
    }
  }

  top_of_p_file = top_of_p_table = nr;
}



/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}



void index_boot(int mode)
{
  char *index_filename, *prefix;
  FILE *index, *db_file;
  int rec_count = 0;

  switch (mode) {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand to index_boot!");
    exit(1);
    break;
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s/%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    sprintf(buf1, "Error opening index file '%s'", buf2);
    perror(buf1);
    exit(1);
  }
  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      exit(1);
    } else {
      if (mode == DB_BOOT_ZON)
	rec_count++;
      else
	rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  if (!rec_count) {
    log("SYSERR: boot error - 0 records counted");
    exit(1);
  }
  rec_count++;

  switch (mode) {
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    break;
  }

  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
      discrete_load(db_file, mode);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }
}


void discrete_load(FILE * fl, int mode)
{
  int nr = -1, last = 0;
  char line[256];

  char *modes[] = {"world", "mob", "obj"};

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	fprintf(stderr, "Format error #1 after %s #%d\n", modes[mode], nr);
	exit(1);
      }

    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	fprintf(stderr, "Format error #2 after %s #%d\n", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
	case DB_BOOT_WLD:
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr);
	  break;
	case DB_BOOT_OBJ:
	  strcpy(line, parse_object(fl, nr));
	  break;
	}
    } else {
      fprintf(stderr, "Format error in %s file near %s #%d\n",
	      modes[mode], modes[mode], nr);
      fprintf(stderr, "Offending line: '%s'\n", line);
      exit(1);
    }
  }
}


long asciiflag_conv(char *flag)
{
  long flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return flags;
}


/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256], flags[128];
  struct extra_descr_data *new_descr;

  sprintf(buf2, "room #%d", virtual_nr);

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    fprintf(stderr, "Room #%d is below zone %d.\n", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      fprintf(stderr, "Room %d is outside of any zone.\n", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (!get_line(fl, line) || sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
    fprintf(stderr, "Format error in room #%d\n", virtual_nr);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
  world[room_nr].room_flags = asciiflag_conv(flags);
  world[room_nr].sector_type = t[2];

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;	/* Zero light sources */

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  sprintf(buf, "Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      fprintf(stderr, "%s\n", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S':			/* end of room */
      top_of_world = room_nr++;
      return;
      break;
    default:
      fprintf(stderr, "%s\n", buf);
      exit(1);
      break;
    }
  }
}



/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int t[5];
  char line[256];

  sprintf(buf2, "room #%d, direction D%d", world[room].number, dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  switch (t[0]) {
      case 1:   world[room].dir_option[dir]->exit_info = EX_ISDOOR;
                break;
      case 2:   world[room].dir_option[dir]->exit_info = EX_ISDOOR |
                    EX_PICKPROOF;
                break;
      case 3:	world[room].dir_option[dir]->exit_info = EX_ISDOOR |
		    EX_SECRET;
		break;
      case 4:	world[room].dir_option[dir]->exit_info = EX_ISDOOR |
		    EX_PICKPROOF | EX_SECRET;
		break;
      case 5:	world[room].dir_option[dir]->exit_info = EX_BREAKABLE;
      		break;
      default:  world[room].dir_option[dir]->exit_info = 0;
		break;
  }

  world[room].dir_option[dir]->key = t[1];
  world[room].dir_option[dir]->to_room = t[2];
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  extern sh_int mortal_start_room;
  extern sh_int immort_start_room;
  extern sh_int frozen_start_room;
  extern sh_int mortuary_start_room;
  int i;
  extern sh_int race_start_room[NUM_RACES];
  extern sh_int lowbie_start_room;
  extern sh_int clan_start_room[NUM_CLANS];
 

  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
  if ((r_mortuary_start_room = real_room(mortuary_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Mortuary start room does not exist.  Change in config.c.");
    r_mortuary_start_room = r_mortal_start_room;
  }
  for (i = 0; i < NUM_RACES; i++)
    if ((r_race_start_room[i] = real_room(race_start_room[i])) < 0) {
      if (!mini_mud) {
        sprintf(buf, 
          "SYSERR:  Warning: Race start room %d does not exist.  Change in race.c.",
          race_start_room[i]);
        log(buf);
      } 
      r_race_start_room[i] = r_mortal_start_room;
    }
  if ((r_lowbie_start_room = real_room(lowbie_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Lowbie start room does not exist.  Change in config.c.");
    r_lowbie_start_room = r_mortal_start_room;
  }
  for (i = 0; i < NUM_CLANS; i++)
    if ((r_clan_start_room[i] = real_room(clan_start_room[i])) < 0) {
      if (!mini_mud) {
        sprintf(buf,
          "SYSERR:  Warning: Clan start room %d does not exist.  Change in clan.c", clan_start_room[i]);
        log(buf);
      }
      r_clan_start_room[i] = r_mortal_start_room;
    }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  register int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
	if (world[room].dir_option[door]->to_room != NOWHERE)
	  world[room].dir_option[door]->to_room =
	    real_room(world[room].dir_option[door]->to_room);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int zone, cmd_no, a, b;
  int arg1, arg2, arg3;

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = 0;
/* HACKED to save the original info for debugging */
      arg1 = ZCMD.arg1;
      arg2 = ZCMD.arg2;
      arg3 = ZCMD.arg3;
/* end of hack */
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	b = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  b = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	b = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
	b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      }
      if (a < 0 || b < 0) {
	if (!mini_mud)
/* HACKED to give more info */
/*	  log_zone_error(zone, cmd_no, "Invalid vnum, cmd disabled"); */
          if (a < 0)
            sprintf(buf, "Invalid vnum a, cmd disabled");
          else
            sprintf(buf, "Invalid vnum b, cmd disabled");
          log_zone_error(zone, cmd_no, buf);
          sprintf(buf, "...ZCMD command %c %d %d %d", ZCMD.command,
              arg1, arg2, arg3);
          log_zone_error(zone, cmd_no, buf);
/* end of hack */
	ZCMD.command = '*';
      }
    }
}



char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);
  } while (isspace(c));
  return c;
}

 

/*
 * All of these PARSE_XXX #defines require:
 *     char *val;
 */
#define PARSE_NUMBER(string, field)				\
		if (!str_cmp(string, attribute)) {		\
		  field = atoi(val);				\
                  found = 1;					\
                  break;					\
		}
#define PARSE_LINE(string, field)				\
		if (!str_cmp(string, attribute)) {		\
		  skip_spaces(&val);				\
                  if (field)					\
                    free(field);				\
		  CREATE(field, char, strlen(val) + 1);		\
		  strcpy(field, val);				\
		  found = 1;					\
		  break;					\
		}
/*
 * warning PARSE_LINES needs a filename to read from and 
 * uses buf2 to hold any errors
 */
#define PARSE_LINES(string, file, field)                        \
                if (!str_cmp(string, attribute)) {              \
                  if (field)                                    \
                    free(field);                                \
		  field = fread_string(file, buf2);		\
                  found = 1;                                    \
                  break;                                        \
                }

/* this requires 'int i;' be defined */
/* and buf2 for errors */
/* if it cant find a match, no load the mud */
#define PARSE_TYPE(string, field, list)				\
                skip_spaces(&val);				\
		if (!str_cmp(string, attribute)) {		\
                  for (i = 0; *(list[i]) != '\n'; i++)		\
                    if (!strncasecmp(val, list[i], strlen(val)))	\
                      break;					\
                  if (!strcmp(list[i], "\n")) {			\
                    field = -1;					\
                    fprintf(stderr,				\
                      "Unable to find a match for '%s' in list.\n", val); \
                    exit(1);					\
                  } else					\
    		    field = i;					\
		  found = 1;					\
		  break;					\
		}
void parse_mobile_attributes(FILE * mob_f, int nr)
{
  char line[MAX_STRING_LENGTH];
  char attribute[MAX_STRING_LENGTH];      /* attribute */
  char *val;                              /* value */
  char letter;
  int found;
  int i;


  while (1) {
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
    if (letter == '#') {
      break;
    }
    
    get_line(mob_f, line);
    val = one_argument(line, attribute);

    found = 0;
    switch (attribute[0]) {
      case 'c': case 'C':
          PARSE_NUMBER("cha", mob_proto[nr].real_abils.cha);
          PARSE_TYPE("class", mob_proto[nr].player.class, pc_class_types);
          PARSE_NUMBER("con", mob_proto[nr].real_abils.con);
          break;
      case 'd': case 'D':
          PARSE_LINES("desc", mob_f, mob_proto[nr].player.description);
          PARSE_NUMBER("dex", mob_proto[nr].real_abils.dex);
          break;
      case 'i': case 'I':
          PARSE_NUMBER("int", mob_proto[nr].real_abils.dex);
          break;
      case 'k': case 'K':
          PARSE_LINE("keywords", mob_proto[nr].player.name);
          break;
      case 'l': case 'L':
          PARSE_NUMBER("level", mob_proto[nr].player.level);
          PARSE_LINES("look", mob_f, mob_proto[nr].player.long_descr);
          break;
      case 'n': case 'N':
          PARSE_LINE("name", mob_proto[nr].player.short_descr);
          break;
      case 'r': case 'R':
          PARSE_TYPE("race", mob_proto[nr].mob_specials.race, pc_race_types);
          break;
      case 's': case 'S':
          PARSE_NUMBER("str", mob_proto[nr].real_abils.str);
          PARSE_NUMBER("stradd", mob_proto[nr].real_abils.str_add);
          break;
      case 'w': case 'W':
          PARSE_NUMBER("wis", mob_proto[nr].real_abils.wis);
          break;
      default:
          break;
    }

    if (!found) {
      sprintf(buf, "SYSERR:  Unknown attribute '%s' mob #%d.",
          attribute, mob_index[nr].virtual);
      log(buf);
    } else {
      mob_proto[nr].mob_specials.mobformat = MOBFORMAT_EXPANDED;
    }
  }
}



void parse_mobile(FILE * mob_f, int nr)
{
  static int i = 0;
  int j, t[10];
  char line[256], *tmpptr, letter;
  char f1[128], f2[128];


  mob_index[i].virtual = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  /***** Default data *** */
  mob_proto[i].player_specials = &dummy_mob;
  mob_proto[i].mob_specials.race = RACE_UNDEFINED;
  mob_proto[i].player.class = CLASS_UNDEFINED;
  mob_proto[i].real_abils.str = 11;
  mob_proto[i].real_abils.intel = 11;
  mob_proto[i].real_abils.wis = 11;
  mob_proto[i].real_abils.dex = 11;
  mob_proto[i].real_abils.con = 11;
  mob_proto[i].real_abils.cha = 11;
  mob_proto[i].player.weight = 200;
  mob_proto[i].player.height = 198;

  sprintf(buf2, "mob vnum %d", nr);

  /***** String data *** */
  mob_proto[i].player.name = fread_string(mob_f, buf2);
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.description = fread_string(mob_f, buf2);
  mob_proto[i].player.title = NULL;

  /* *** Numeric data *** */
  get_line(mob_f, line);
  sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter);
  MOB_FLAGS(mob_proto + i) = asciiflag_conv(f1);
  SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  AFF_FLAGS(mob_proto + i) = asciiflag_conv(f2);
  GET_ALIGNMENT(mob_proto + i) = t[2];

  switch (letter) {
  case 'S':	/* Simple monsters */
  case 'E':	/* Expanded monsters (simple + stats) */
    get_line(mob_f, line);
    if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	  t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
      fprintf(stderr, "Format error in mob #%d, first line after S flag\n"
	      "...expecting line of form '# # # #d#+# #d#+#'\n", nr);
      exit(1);
    }
    GET_LEVEL(mob_proto + i) = t[0];
    mob_proto[i].points.hitroll = 20 - t[1];
    mob_proto[i].points.armor = 10 * t[2];

    /* max hit = 0 is a flag that H, M, V is xdy+z */
    mob_proto[i].points.max_hit = 0;
    mob_proto[i].points.hit = t[3];
    mob_proto[i].points.mana = t[4];
    mob_proto[i].points.move = t[5];

    mob_proto[i].points.max_mana = 10;
    mob_proto[i].points.max_move = 50;

    mob_proto[i].mob_specials.damnodice = t[6];
    mob_proto[i].mob_specials.damsizedice = t[7];
    mob_proto[i].points.damroll = t[8];

    get_line(mob_f, line);
    sscanf(line, " %d %d ", t, t + 1);
    GET_GOLD(mob_proto + i) = t[0];
    GET_EXP(mob_proto + i) = t[1];

    get_line(mob_f, line);
    if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) == 4)
      mob_proto[i].mob_specials.attack_type = t[3];
    else
      mob_proto[i].mob_specials.attack_type = 0;

    mob_proto[i].char_specials.position = t[0];
    mob_proto[i].mob_specials.default_pos = t[1];
    mob_proto[i].player.sex = t[2];

    for (j = 0; j < 3; j++)
      GET_COND(mob_proto + i, j) = -1;

    /*
     * these are now save applies; base save numbers for MOBs are now from
     * the warrior save table.
     */
    for (j = 0; j < 5; j++)
      GET_SAVE(mob_proto + i, j) = 0;

    break;
  default:
    fprintf(stderr, "Unsupported mob type '%c' in mob #%d\n", letter, nr);
    exit(1);
    break;
  }

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  letter = fread_letter(mob_f);
  if (letter == '>') {
    ungetc(letter, mob_f);
    (void) mprog_read_programs(mob_f, &mob_index[i]);
  } else if (letter != '#') {
    ungetc(letter, mob_f);
    parse_mobile_attributes(mob_f, i);
  } else ungetc(letter, mob_f);

  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  top_of_mobt = i++;
}




/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
  static int i = 0;
  static char line[256];
  int t[10], j, k;
  long T[10];
  char *tmpptr;
  char f1[256], f2[256];
  struct extra_descr_data *new_descr;
  int n;


  obj_index[i].virtual = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].in_room = NOWHERE;
  obj_proto[i].item_number = i;

  sprintf(buf2, "object #%d", nr);

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    fprintf(stderr, "Null obj name or format error at or near %s\n", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (*tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    *tmpptr = UPPER(*tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line) || sscanf(line, " %d %s %s", t, f1, f2) != 3) {
    fprintf(stderr, "Format error in first numeric line, %s\n", buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.type_flag = t[0];
  obj_proto[i].obj_flags.extra_flags = asciiflag_conv(f1);
  obj_proto[i].obj_flags.wear_flags = asciiflag_conv(f2);

  if (!get_line(obj_f, line) || sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
    fprintf(stderr, "Format error in second numeric line, %s\n", buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.value[0] = t[0];
  obj_proto[i].obj_flags.value[1] = t[1];
  obj_proto[i].obj_flags.value[2] = t[2];
  obj_proto[i].obj_flags.value[3] = t[3];

  if (!get_line(obj_f, line)) {
    fprintf(stderr, "Format error in third numeric line, %s\n", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) == 4)
    obj_proto[i].obj_flags.timer = t[3];
  else
    obj_proto[i].obj_flags.timer = 0;
  obj_proto[i].obj_flags.weight = t[0];
  obj_proto[i].obj_flags.cost = t[1];
  obj_proto[i].obj_flags.cost_per_day = t[2];

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  for (k = 0; k < MAX_SPELL_AFFECT; k++) {
       obj_proto[i].spell_affect[k].spelltype = APPLY_NONE;
       obj_proto[i].spell_affect[k].level = 0;
       obj_proto[i].spell_affect[k].percentage = 0;
  }
  strcat(buf2, ", after numeric constants (expecting E/A/S/#xxx)");
  j = 0;
  k = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      fprintf(stderr, "Format error in %s\n", buf2);
      exit(1);
    }
    switch (*line) {
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
    case 'A':
      if (j >= MAX_OBJ_AFFECT) {
	fprintf(stderr, "Too many A fields (%d max), %s\n", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      get_line(obj_f, line);
      sscanf(line, " %d %d ", t, t + 1);
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      j++;
      break;
    case 'B':
      get_line(obj_f, line);
      n = sscanf(line, " %ld %ld", T, T + 1);

      if (n > 0)
        obj_proto[i].obj_flags.bitvector = T[0];
      else
        obj_proto[i].obj_flags.bitvector = 0;

      if (n > 1)
        obj_proto[i].obj_flags.bitvector2 = T[1];
      else
        obj_proto[i].obj_flags.bitvector2 = 0;

      break;

    case 'S':
      if (k >= MAX_SPELL_AFFECT) {
        fprintf(stderr,"Too many S fields (%d max), %s\n",
            MAX_SPELL_AFFECT, buf2);
        exit(1);
      }
      get_line(obj_f, line);
      sscanf(line, " %d %d %d ", t, t + 1 , t + 2);
      obj_proto[i].spell_affect[k].spelltype = t[0];
      obj_proto[i].spell_affect[k].level = t[1];
      obj_proto[i].spell_affect[k].percentage = t[2];
      k++;
      break;
    case '$':
    case '#':
      top_of_objt = i++;
      return line;
      break;
    default:
      fprintf(stderr, "Format error in %s\n", buf2);
      exit(1);
      break;
    }
  }
}


#define Z	zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static int zone = 0;
  int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error;
  char *ptr, buf[256], zname[256];

  strcpy(zname, zonename);

  while (get_line(fl, buf))
    num_of_cmds++;		/* this should be correct within 3 or so */
  rewind(fl);

  if (num_of_cmds == 0) {
    fprintf(stderr, "%s is empty!\n", zname);
    exit(0);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%d", &Z.number) != 1) {
    fprintf(stderr, "Format error in %s, line %d\n", zname, line_num);
    exit(0);
  }
  sprintf(buf2, "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %d %d %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode,
        &Z.zone_flags) == 3) {
/*
    fprintf(stderr, "Format error in 3-constant line of %s", zname);
    exit(0);
*/
    Z.zone_flags = 0;
  }
  cmd_no = 0;

/* HACKED to set the reset time to 10 and to always reset (code 2) */
/* (diabled)
  Z.lifespan = 10;
  Z.reset_mode = 2;
*/
/* end of hack */

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0) {
      fprintf(stderr, "Format error in %s - premature end of file\n", zname);
      exit(0);
    }
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPD", ZCMD.command) == NULL) {	/* a 3-arg command */
      if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
	error = 1;
    } else {
      if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
		 &ZCMD.arg3) != 4)
	error = 1;
    }

    ZCMD.if_flag = tmp;

    if (error) {
      fprintf(stderr, "Format error in %s, line %d: '%s'\n", zname, line_num, buf);
      exit(0);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  top_of_zone_table = zone++;
}

#undef Z


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*********************************************************************** */



int vnum_mobile(char *searchname, struct char_data * ch)
{
  int nr, found = 0;
  int numerical = -1;

  if (isdigit(*searchname))
    numerical = atoi(searchname);

  for (nr = 0; nr <= top_of_mobt; nr++) {
    if (((numerical > -1) && (numerical == mob_index[nr].virtual / 100)) ||
        ((numerical == -1) && (mob_proto[nr].player.name != NULL) && (isname(searchname, mob_proto[nr].player.name)))) {

      sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
	      mob_index[nr].virtual,
	      mob_proto[nr].player.short_descr);
      send_to_char(buf, ch);
    }
  }

  return (found);
}



/*
 * a simple utility to show an object on a line
 * the output looks like:
 * 1. [ 2567] a runed chisel (2d3)  +1 dex
 */
void show_vnum_obj_to_char(struct char_data * ch, int nr, int found) {
  char buf[MAX_STRING_LENGTH];
  char buf2[256];
  int i;


  sprintf(buf, "%3d. [%5d] %s", found,
          obj_index[nr].virtual,
          obj_proto[nr].short_description);
  send_to_char(buf, ch);

  switch (obj_proto[nr].obj_flags.type_flag) {
      case ITEM_WEAPON:
      case ITEM_FIREWEAPON:
              sprintf(buf, " (%dd%d)", obj_proto[nr].obj_flags.value[1],
                                       obj_proto[nr].obj_flags.value[2]);
              send_to_char(buf, ch);
              break;
      default:
              break;
  }

  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if ((obj_proto[nr].affected[i].location != APPLY_NONE) &&
        (obj_proto[nr].affected[i].modifier != 0)) {
      if (obj_proto[nr].affected[i].modifier > 0)
        sprintf(buf, "  +%d ", obj_proto[nr].affected[i].modifier);
      else
        sprintf(buf, "  %d ", obj_proto[nr].affected[i].modifier);
      send_to_char(buf, ch);
      sprinttype(obj_proto[nr].affected[i].location, apply_types, buf2);
      send_to_char(buf2, ch);
    }
  }

  if (obj_proto[nr].obj_flags.bitvector) {
    send_to_char("  ", ch);
    sprintbit(obj_proto[nr].obj_flags.bitvector, affected_bits, buf);
    send_to_char(buf, ch);
  }

  if (obj_proto[nr].obj_flags.bitvector2) {
    send_to_char("  ", ch);
    sprintbit(obj_proto[nr].obj_flags.bitvector2, affected2_bits, buf);
    send_to_char(buf, ch);
  }   


  if (obj_proto[nr].spell_affect[0].spelltype > 0) {
    for (i = 0; i < MAX_SPELL_AFFECT; i++) {
      if (obj_proto[nr].spell_affect[i].spelltype > 0) {
        sprintf(buf, "  %s",
            spells[obj_proto[nr].spell_affect[i].spelltype]);
	send_to_char(buf, ch);
      }
    }
  }
  send_to_char("\r\n", ch);
}



int vnum_object(char *searchname, struct char_data * ch)
{
  int nr, i = 0;
  int numerical = -1;
  int found = 0;

  if(!strcmp(searchname, "+dam")) {
    for (nr = 0; nr <= top_of_objt; nr++) {
      for (i = 0; i < MAX_OBJ_AFFECT; i++) {
	if (obj_proto[nr].affected[i].location == APPLY_DAMROLL) {
	  found++;
	  show_vnum_obj_to_char(ch, nr, found);
	}
      }
    }
  } else {

    if (isdigit(*searchname)) 
      numerical = atoi(searchname);

    for (nr = 0; nr <= top_of_objt; nr++) {
      if (((numerical > -1) && (numerical == obj_index[nr].virtual / 100)) ||
          ((numerical == -1) && (obj_proto[nr].name != NULL) &&
			(isname(searchname, obj_proto[nr].name)))) {

        found++;
        show_vnum_obj_to_char(ch, nr, found);
      }
    }
  }
  return (found);
}



/*
 * A persons first instinct if given a zone # of rooms
 * would be to divide a rooms vnum by 100 and if that equals
 * the number given, to just print those rooms.  But some
 * stock zones (ie New Thalos) are very large, spanning over
 * 250 rooms... and to handle those cases, you first have to
 * take the number given, find the real zone #, then check
 * every room in the world and see if it is of that zone..
 * (a little more work)
 */
int vnum_room(char *searchname, struct char_data * ch)
{
  int virtual_zone = -1;
  int real_zone = -1;
  register int nr;
  int found = 0;


  if (isdigit(*searchname)) {
    virtual_zone = atoi(searchname);

    /* find the real zone number */
    for (real_zone = 0; real_zone <= top_of_zone_table; real_zone++)
      if (zone_table[real_zone].number == virtual_zone)
        break;
    if (real_zone > top_of_zone_table)
      return(found);
  }

  /* list any rooms of that zone or if we're looking for a keyword,
     rooms that have that word in their name */
  for (nr = 0; nr < top_of_world; nr++) {
    if (((real_zone > -1) && (real_zone == world[nr].zone)) ||
        ((real_zone == -1) && (world[nr].name != NULL) &&
         (isname(searchname, world[nr].name)))) {
      sprintf(buf, "%3d. [%5d] %s\r\n",
          ++found, world[nr].number, world[nr].name);
      send_to_char(buf, ch);
    }
  }

  return (found);
}



/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;

  return ch;
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(int nr, int type)
{
  extern void load_shopkeeper(struct char_data *);
  int i;
  struct char_data *mob;
  static int mob_defaults[LVL_IMPL + 1][8] = {
   /* hitroll   ac    damage dice sides hps    xps     gold */
    { 100,	100,	0,	1, 3,	10,	25,	1 },	/* 0 */
    { 20, 	100,	0,	1, 4,	20,	100,	1 },	/* 1 */
    { 19, 	97,	1,	1, 5,	30,	200,	1 },
    { 18, 	94,	1,	1, 5,	40,	350,    2 },
    { 17, 	91,	2,	1, 6,	50,	600,	3 },
    { 16, 	88,	2,	3, 3,	60,	900,	4 },	/* 5 */
    { 15, 	84,	3,	3, 3,	70,	1500,	5 },
    { 14, 	80,	3,	3, 3,	80,	3000,   7 },
    { 13, 	76,	4,	3, 3,	90,	4500,	11 },
    { 12, 	73,	4,	3, 3,	100,	6000,	15 },
    { 11, 	70,	5,	4, 4,	150,	9000,	22 },	/* 10 */
    { 10, 	66,	5,	4, 4,	200,	10500,	26 },
    { 9, 	62,	6,	4, 4,	250,	11500,	28 },
    { 8, 	58,	6,	4, 5,	350,	15000,	30 },
    { 7, 	56,	7,	4, 5,	400,	22500,	32 },
    { 6, 	52,	7,	5, 5,	450,	30000,	35 },	/* 15 */
    { 5, 	48,	8,      5, 5,   550,	37000,	40 },
    { 4, 	44,	8,      5, 6,   650,	44000,	45 },
    { 3, 	40,	9,      5, 6,   750,	50000,	50 },
    { 2, 	37,	9,      5, 6,   900,	65000,  60 },
    { 1, 	34,	10,	6, 6,	1000,	80000,	70 },	/* 20 */
    { 0, 	30,	10,	6, 6,   1200,	100000,	85 },
    { -1, 	26,	11,	6, 6,   1500,	120000,	130 },
    { -1, 	22,	11,	6, 6,   1800,	141000,	160 },
    { -2, 	19,	12,	6, 6,   2200,	170000,	170 },
    { -2, 	16,	12,	6, 7,   2500,	200000,	200 },	/* 25 */
    { -3, 	12,	13,	6, 7,   2800,	240000,	250 },
    { -3, 	8,	13,	6, 7,   3000,	280000,	300 },
    { -4, 	4,	14,	6, 7,   3500,	320000,	350 },
    { -4, 	1,	14,	6, 7,   4000,	355000,	400 },
    { -5, 	-2,	15,	6, 7,   4500,	370000,	500 }, /* 30 */
    { -5, 	-6,	15,	7, 7,   5000,	400000,	550 },
    { -6, 	-10,	16,	7, 7,   5500,	450000,	600 },
    { -7, 	-14,	16,	7, 7,   6000,	480000,	650 },
    { -8, 	-17,	17,	7, 7,   7000,	520000,	700 },	
    { -9,	-20,    17,	7, 7,   8000,   560000, 750 }, /* 35 */
    { -10,	-24,	18,	7, 8,	9000,   600000,	800 },
    { -11,	-28,	18,	7, 8,   10000,  700000,	850 },
    { -12,	-32,	19,	8, 8,   11000,  800000,	900 },
    { -13,	-35,	20,	8, 8,   12000,  900000,	950 },
    { -13,	-38,	21,	8, 8,   13000,  1000000, 1000 }, /* 40 */
    { -14,	-42,	22,	8, 8,   14000,  1300000, 1050 },
    { -14,	-46,	23,	8, 9,   15000,  1500000, 1100 },
    { -15,	-50,	24,	8, 9,   16000,  1900000, 1150 },
    { -16,	-53,	25,	8, 10,  17000,  2000000, 1200 },
    { -17,	-56,	25,	8, 10,  18000,  2300000, 1250 }, /* 45 */
    { -18,	-60,	26,	9, 9,   19000,  2500000, 1300 },
    { -19,	-64,	27,	9, 9,   20000,  2900000, 1350 },
    { -20,	-68,	28,	9, 9,   21000,  3300000, 1400 },
    { -21,	-71,	29,	9, 9,   22000,  3600000, 1450 },
    { -22,	-74,	30,	9, 10,  23000,  3800000, 1500 }, /* 50 */
    { -23,	-77,	31,	9, 10,  24000,	4300000, 1550 },
    { -23,	-80,	32,	9, 11,  25000,	4700000, 1600 },
    { -24,	-83,	33,	9, 11,  26000,	5200000, 1650 },
    { -24,	-86,	34,	9, 12,  27000,	5900000, 1700 },
    { -25,	-91,	35,	9, 13,  28000,	6600000, 1750 }, /* 55 */
    { -25,	-94,	36,	9, 14,  29000,	7300000, 1800 },
    { -26,	-97,	37,	9, 16,  30000,	8900000, 1850 },
    { -27,	-100,	38,	9, 18,  31000,	10000000, 1900 },
    { -28,	-100,	39,	9, 20,  32000,	10000000, 1950 },
    { -30,	-100,	40,    10, 20,  32000,	10000000, 2000 } /* 60 */
  };

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) < 0) {
      sprintf(buf, "Mobile (V) %d does not exist in database.", nr);
      return (0);
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

/* HACKED to give mobs hitpoints, experience, and gold from a table */
/* Old Code - still setting initial values */
  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
      mob->points.move;
  } else
    mob->points.max_hit = number(mob->points.hit, mob->points.mana);
/* New Code */
  mob->points.hitroll = 20 - mob_defaults[(int) GET_LEVEL(mob)][0];
  if (mob->points.armor > mob_defaults[(int) GET_LEVEL(mob)][1]) 
    mob->points.armor = mob_defaults[(int) GET_LEVEL(mob)][1];
  if (mob->points.damroll < mob_defaults[(int) GET_LEVEL(mob)][2])
    mob->points.damroll = mob_defaults[(int) GET_LEVEL(mob)][2];
  mob->mob_specials.damnodice = mob_defaults[(int) GET_LEVEL(mob)][3];
  mob->mob_specials.damsizedice = mob_defaults[(int) GET_LEVEL(mob)][4];
  if (mob->points.max_hit < mob_defaults[(int) GET_LEVEL(mob)][5])
    mob->points.max_hit = mob_defaults[(int) GET_LEVEL(mob)][5];
  if ((GET_EXP(mob) < mob_defaults[(int) GET_LEVEL(mob)][6] / 2) ||
      (GET_EXP(mob) > mob_defaults[(int) GET_LEVEL(mob)][6] * 2))
    GET_EXP(mob) = mob_defaults[(int) GET_LEVEL(mob)][6];
/* HACKED to give mobs gold by default if they have more than 0 */

  if ((GET_GOLD(mob) == 0))
/*
  if ((GET_GOLD(mob) > 0) &&
      ((GET_GOLD(mob) > mob_defaults[(int) GET_LEVEL(mob)][7] * 3 / 2) || 
       (GET_GOLD(mob) < mob_defaults[(int) GET_LEVEL(mob)][7] / 2)))
*/
    GET_GOLD(mob) = (mob_defaults[(int) GET_LEVEL(mob)][7] * 3 / 4) +
                    (number(0, mob_defaults[(int) GET_LEVEL(mob)][7]) / 2);
/* end of hack */
/* end of new code */

  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  if (IS_AFFECTED(mob, AFF_MIRROR_IMAGE))
    GET_IMAGES(mob) = (GET_LEVEL(mob) / 10) + number(2, 5);

  if (IS_AFFECTED(mob, AFF_STONESKIN))
  /* Kas's hp-based limit
    GET_LAYERS(mob) = (GET_LEVEL(mob) * 10) + number(20, 100);
  */
    GET_LAYERS(mob) = (GET_LEVEL(mob) / 6) + number(1, 3);
  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);
  
  if (MOB_FLAGGED(mob, MOB_DSHOPKEEPER)) load_shopkeeper(mob);

  mob_index[i].number++;

  return mob;
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  return obj;
}


/* create a new object from a prototype */
struct obj_data *read_object(int nr, int type)
{
  struct obj_data *obj;
  int i;

  if (nr < 0) {
    log("SYSERR: trying to create obj with negative num!");
    return NULL;
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      sprintf(buf, "Object (V) %d does not exist in database.", nr);
      return NULL;
    }
  } else
    i = nr;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;

  return obj;
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;
  char buf[128];

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode)
	(zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

	CREATE(update_u, struct reset_q_element, 1);

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }	/* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
	is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset);
/* HACKED to show zone number in addition to the name */
      sprintf(buf, "Auto zone reset: %3d %s",
              zone_table[update_u->zone_to_reset].number,
	      zone_table[update_u->zone_to_reset].name);
/* end of hack */
      mudlog(buf, CMP, LVL_GOD, FALSE);
      /* dequeue */
      if (update_u == reset_q.head)
	reset_q.head = reset_q.head->next;
      else {
	for (temp = reset_q.head; temp->next != update_u;
	     temp = temp->next);

	if (!update_u->next)
	  reset_q.tail = temp;

	temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

void log_zone_error(int zone, int cmd_no, char *message)
{
  char buf[256];

  sprintf(buf, "SYSERR: error in zone file: %s", message);
  arealog(buf, NRM, LVL_GOD, TRUE, zone);

  sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	  ZCMD.command, zone_table[zone].number, ZCMD.line);
  arealog(buf, NRM, LVL_GOD, TRUE, zone);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
/* HACKED to add in stoichiometric loading - the load limit is reinterpreted
  to mean the % chance of the item loading. A special exception is made for
  the numbers greater than 100 - 101 would mean to load one object in the old
  load-limits style. */
/* HACKED for load_progs */
void reset_zone(int zone)
{
  int cmd_no = 0;
  int last_cmd = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj = NULL;
  struct obj_data *obj_to = NULL;
  
  void mprog_load_trigger(struct char_data *mob);

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

/* HACKED to ignore the if flag */
    if (ZCMD.if_flag && !last_cmd)
      continue;
/* end of hack */

    switch (ZCMD.command) {
    case '*':			/* ignore command */
      last_cmd = 0;
      break;

    case 'M':			/* read a mobile */
      if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
	mob = read_mobile(ZCMD.arg1, REAL);
	char_to_room(mob, ZCMD.arg3);
	GET_MOB_RACE(mob) = get_race_guess(mob);
	GET_CLASS(mob) = get_class_guess(mob);
	mprog_load_trigger(mob);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'O':			/* read an object */
/* HACKED to add in stoichiometric loading */
/*
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
*/
      if (((ZCMD.arg2 > 100) && (obj_index[ZCMD.arg1].number < ZCMD.arg2 - 100))
         || (number(0,99) < ZCMD.arg2))
/* end of hack */
	if (ZCMD.arg3 >= 0) {
	  if (!get_obj_in_list_num(ZCMD.arg1, world[ZCMD.arg3].contents)) {
	    obj = read_object(ZCMD.arg1, REAL);
	    obj_to_room(obj, ZCMD.arg3);
	    last_cmd = 1;
	  } else
	    last_cmd = 0;
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  obj->in_room = NOWHERE;
	  last_cmd = 1;
	}
      else
	last_cmd = 0;
      break;

    case 'P':			/* object to object */
/* HACKED to add in stoichiometric loading */
/*
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
*/
      if (((ZCMD.arg2 > 100) && (obj_index[ZCMD.arg1].number < ZCMD.arg2 - 100))
         || (number(0,99) < ZCMD.arg2)) {
/* end of hack */
	obj = read_object(ZCMD.arg1, REAL);
	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
	  ZONE_ERROR("target obj not found");
	  break;
	}
	obj_to_obj(obj, obj_to);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'G':			/* obj_to_char */
      if (!mob) {
	ZONE_ERROR("attempt to give obj to non-existant mob");
	break;
      }
/* HACKED to add in stoichiometric loading */
/*
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
*/
      if (((ZCMD.arg2 > 100) && (obj_index[ZCMD.arg1].number < ZCMD.arg2 - 100))
         || (number(0,99) < ZCMD.arg2)) {
/* end of hack */
	obj = read_object(ZCMD.arg1, REAL);
	obj_to_char(obj, mob);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'E':			/* object to equipment list */
      if (!mob) {
	ZONE_ERROR("trying to equip non-existant mob");
	break;
      }
/* Item max
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
*/
      if (((ZCMD.arg2 > 100) && (obj_index[ZCMD.arg1].number < ZCMD.arg2 - 100))
         || (number(0,99) < ZCMD.arg2)) {
/* end of hack */
	if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  equip_char(mob, obj, ZCMD.arg3);
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL) {
        obj_from_room(obj);
        extract_obj(obj);
      }
      last_cmd = 1;
      break;


    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
	ZONE_ERROR("door does not exist");
      } else
	switch (ZCMD.arg3) {
	case 0:
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_CLOSED);
	  break;
	case 1:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  break;
	case 2:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_LOCKED);
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  break;
	}
      last_cmd = 1;
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table!");
      break;
    }
  }

  zone_table[zone].age = 0;
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr)
	return 0;

  return 1;
}





/*************************************************************************
*  stuff related to the save/load player system				 *
*********************************************************************** */


long get_id_by_name(char *name)
{
  int i;

  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
    if (!strcmp((player_table + i)->name, arg))
      return ((player_table + i)->id);

  return -1;
}


char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if ((player_table + i)->id == id)
      return ((player_table + i)->name);

  return NULL;
}


/* Load a char, TRUE if loaded, FALSE if not */
int load_char(char *name, struct char_file_u * char_element)
{
  int player_i;

  int find_name(char *name);

  if ((player_i = find_name(name)) >= 0) {
    fseek(player_fl, (long) (player_i * sizeof(struct char_file_u)), SEEK_SET);
    fread(char_element, sizeof(struct char_file_u), 1, player_fl);
    return (player_i);
  } else
    return (-1);
}




/* write the vital data of a player to the player file */
/* and call save_char_text (in textsave.c) to save player's aliases */
/* and call save_pet (in pets.c) to save player's pet, if any */
/* at the end call save_ascii_char to save character to ascii playerfile */
/* CHANGED - save_ascii_char is not called anymore, save_ascii_data is */
void save_char(struct char_data * ch, sh_int load_room)
{
  extern int save_char_text(struct char_data * ch);
  int write_storage_room( int vnum );
  int save_ascii_char(struct char_data * ch, struct char_file_u * st);
  int save_ascii_pfile(struct char_file_u *st, struct char_data *ch);
  int save_ascii_data(struct char_data *ch);
  void save_pet(struct char_data *ch);

  
  struct char_file_u st;

  if (IS_NPC(ch) || !ch->desc || ch->pfilepos < 0)
    return;

  save_char_text(ch);
/* PETS */
  save_pet(ch);
/* END of PETS */  
  
  char_to_store(ch, &st);

  strncpy(st.host, ch->desc->host, HOST_LENGTH);
  st.host[HOST_LENGTH] = '\0';

  if (!PLR_FLAGGED(ch, PLR_LOADROOM))
    st.player_specials_saved.load_room = load_room;

  strcpy(st.pwd, GET_PASSWD(ch));

  fseek(player_fl, ch->pfilepos * sizeof(struct char_file_u), SEEK_SET);
  fwrite(&st, sizeof(struct char_file_u), 1, player_fl);

/*  save_ascii_char(ch, &st); */
/*  save_ascii_pfile( &st, ch ); */
  save_ascii_data(ch);
  
}



int save_ascii_char(struct char_data * ch, struct char_file_u * st)
{
#if(0)
  FILE *fl;
  char fname[MAX_STRING_LENGTH];

/* HACKED to save only implementors this way for now */
  if (GET_LEVEL(ch) != LVL_IMPL)
    return 1; /* end of hack */

  if (!get_filename(GET_NAME(ch), fname, ASCII_PLAYER_FILE))
    return 1;

  if (!(fl = fopen(fname, "w"))) {
    /* couldnt open the file for writing... the disk is probably full */
    return 1;
  }

  fprintf(fl, "* %s player file\n", st->name);
  fprintf(fl, "* Player Info\n");
  fprintf(fl, "%s\n", st->name);
  if (ch->player.description)
    fprintf(fl, "%s", st->description);
  fprintf(fl, "~\n"); 
  fprintf(fl, "%s\n", st->title);
  fprintf(fl, "%d %d %d %d %d %d %d %d\n", st->sex, st->class, st->level,
        st->hometown, (int) st->birth, st->played, st->weight, st->height);
  fprintf(fl, "%s\n", st->pwd);
  fprintf(fl, "* Char Specials Saved\n");
  fprintf(fl, "* Player Specials Saved\n");
  fprintf(fl, "* Abilities\n");
  fprintf(fl, "* Points\n");
  fprintf(fl, "* Affected\n");
  fprintf(fl, "* Login Info\n");
  fprintf(fl, "%d\n%s\n", (int) st->last_logon, st->host);

  fclose(fl);

#endif
  return 0;
}



/* copy data from the file structure to a char struct */
void store_to_char(struct char_file_u * st, struct char_data * ch)
{
  int i;

  /* to save memory, only PC's -- not MOB's -- have player_specials */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  GET_SEX(ch) = st->sex;
  GET_CLASS(ch) = st->class;
  GET_LEVEL(ch) = st->level;

  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.title = strdup(st->title);
  ch->player.description = strdup(st->description);

  ch->player.hometown = st->hometown;
  ch->player.time.birth = st->birth;
  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  ch->player.weight = st->weight;
  ch->player.height = st->height;

  ch->real_abils = st->abilities;
  ch->aff_abils = st->abilities;
  ch->points = st->points;
  ch->char_specials.saved = st->char_specials_saved;
  ch->player_specials->saved = st->player_specials_saved;
  POOFIN(ch) = NULL;
  POOFOUT(ch) = NULL;

  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;

  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->points.armor = 100;
  ch->points.hitroll = 0;
  ch->points.damroll = 0;

  CREATE(ch->player.name, char, strlen(st->name) + 1);
  strcpy(ch->player.name, st->name);
  strcpy(ch->player.passwd, st->pwd);

  /* Add all spell effects */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, &st->affected[i]);
  }

  ch->in_room = GET_LOADROOM(ch);
#if(0)
  if (ch->in_room > top_of_world) { /* yes, it's a hack */
/*    log("SYSERR: Loading char with vnum for in_room instead of rnum!");
    ch->in_room = real_room(ch->in_room);
    if (ch->in_room == -1) {
  */    log("SYSERR: Loaded in_room is not a valid vnum OR rnum!");
      ch->in_room = r_mortal_start_room;
   /* }*/
  }
#endif


/*   affect_total(ch); also - unnecessary?? */

  /*
   * If you're not poisioned and you've been away for more than an hour,
   * we'll set your HMV back to full
   */

  if (!IS_AFFECTED(ch, AFF_POISON) &&
      (((long) (time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR)) {
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
  }
}				/* store_to_char */




/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data * ch, struct char_file_u * st)
{
  int i;
  struct affected_type *af;
  struct obj_data *char_eq[NUM_WEARS];

  /* Unaffect everything a character can be affected by */

  for (i = 0; i < GET_NUM_WEARS(ch); i++) {
    if (ch->equipment[i])
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = NULL;
  }

  for (af = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (af) {
      st->affected[i] = *af;
      st->affected[i].next = 0;
      af = af->next;
    } else {
      st->affected[i].type = 0;	/* Zero signifies not used */
      st->affected[i].duration = 0;
      st->affected[i].modifier = 0;
      st->affected[i].location = 0;
      st->affected[i].bitvector = 0;
      st->affected[i].bitvector2 = 0;
      st->affected[i].next = 0;
    }
  }


  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  if ((i >= MAX_AFFECT) && af && af->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  st->birth = ch->player.time.birth;
  st->played = ch->player.time.played;
  st->played += (long) (time(0) - ch->player.time.logon);
  st->last_logon = time(0);

  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  st->hometown = ch->player.hometown;
  st->weight = GET_WEIGHT(ch);
  st->height = GET_HEIGHT(ch);
  st->sex = GET_SEX(ch);
  st->class = GET_CLASS(ch);
  st->level = GET_LEVEL(ch);
  st->abilities = ch->real_abils;
  st->points = ch->points;
  st->char_specials_saved = ch->char_specials.saved;
  st->player_specials_saved = ch->player_specials->saved;

  st->points.armor = 100;
  st->points.hitroll = 0;
  st->points.damroll = 0;

/* if (GET_TITLE(ch)) */
    strcpy(st->title, GET_REAL_TITLE(ch));
/*
  else
    *st->title = '\0';
*/

  if (ch->player.description)
    strcpy(st->description, ch->player.description);
  else
    *st->description = '\0';

  strcpy(st->name, GET_NAME(ch));

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, &st->affected[i]);
  }

  for (i = 0; i < GET_NUM_WEARS(ch); i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);
  }
/*   affect_total(ch); unnecessary, I think !?! */
}				/* Char to store */



void save_etext(struct char_data * ch)
{
/* this will be really cool soon */

}


/* create a new entry in the in-memory index table for the player file */
int create_entry(char *name)
{
  int i;

  if (top_of_p_table == -1) {
    CREATE(player_table, struct player_index_element, 1);
    top_of_p_table = 0;
  } else if (!(player_table = (struct player_index_element *)
	       realloc(player_table, sizeof(struct player_index_element) *
		       (++top_of_p_table + 1)))) {
    perror("create entry");
    exit(1);
  }
  CREATE(player_table[top_of_p_table].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (*(player_table[top_of_p_table].name + i) = LOWER(*(name + i)));
       i++);

  return (top_of_p_table);
}



/************************************************************************
*  funcs of a (more or less) general utility nature			*
********************************************************************** */


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n",
	      error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return rslt;
}


/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
{
  int i;
  struct alias *a;

  void free_alias(struct alias * a);
  void free_shopkeeper(struct char_data *ch);

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    free(ch->player_specials);
    if (IS_NPC(ch))
      log("SYSERR: Mob had player_specials allocated!");
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (ch->player.title)
      free(ch->player.title);
    if (ch->player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description)
      free(ch->player.description);
  } else if ((i = GET_MOB_RNUM(ch)) > -1) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
      free(ch->player.name);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
  }
  while (ch->affected)
    affect_remove(ch, ch->affected);

  while ((a = GET_ALIASES(ch)) != NULL) {
    GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
    free_alias(a);
  }
  
  free_shopkeeper(ch);

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  int nr;
  struct extra_descr_data *this, *next_one;

  if ((nr = GET_OBJ_RNUM(obj)) == -1) {
    if (obj->name)
      free(obj->name);
    if (obj->description)
      free(obj->description);
    if (obj->short_description)
      free(obj->short_description);
    if (obj->action_description)
      free(obj->action_description);
    if (obj->ex_description)
      for (this = obj->ex_description; this; this = next_one) {
	next_one = this->next;
	if (this->keyword)
	  free(this->keyword);
	if (this->description)
	  free(this->description);
	free(this);
      }
  } else {
    if (obj->name && obj->name != obj_proto[nr].name)
      free(obj->name);
    if (obj->description && obj->description != obj_proto[nr].description)
      free(obj->description);
    if (obj->short_description && obj->short_description != obj_proto[nr].short_description)
      free(obj->short_description);
    if (obj->action_description && obj->action_description != obj_proto[nr].action_description)
      free(obj->action_description);
    if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
      for (this = obj->ex_description; this; this = next_one) {
	next_one = this->next;
	if (this->keyword)
	  free(this->keyword);
	if (this->description)
	  free(this->description);
	free(this);
      }
  }

  free(obj);
}



/* read contets of a text file, alloc space, point buf to it */
int file_to_string_alloc(char *name, char **buf)
{
  char temp[MAX_STRING_LENGTH];

  if (file_to_string(name, temp) < 0)
    return -1;

  if (*buf)
    free(*buf);

  *buf = strdup(temp);

  return 0;
}



/* read contents of a text file, and place in buf */
int file_to_string(char *name, char *buf)
{
  FILE *fl;
  char tmp[128];

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    sprintf(tmp, "Error reading %s", name);
    perror(tmp);
    return (-1);
  }
  do {
    fgets(tmp, 128, fl);
    tmp[strlen(tmp) - 1] = '\0';/* take off the trailing \n */
    strcat(tmp, "\r\n");

    if (!feof(fl)) {
      if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
	log("SYSERR: fl->strng: string too big (db.c, file_to_string)");
	*buf = '\0';
	return (-1);
      }
      strcat(buf, tmp);
    }
  } while (!feof(fl));

  fclose(fl);

  return (0);
}




/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  int i;

  for (i = 0; i < GET_NUM_WEARS(ch); i++)
    ch->equipment[i] = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  /* ch->in_room = NOWHERE; Used for start in room */
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  HUNTING(ch) = NULL;

  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;

  /* MOBProg foo */
  ch->mpscriptnum = 0;
  ch->mpscriptstep = 0;
  ch->mpscriptactor = NULL;
  ch->mpnextscript = NULL;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  ch->in_room = NOWHERE;
  ch->pfilepos = -1;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;

  GET_AC(ch) = 100;		/* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
  
  ch->shopinfo = NULL;

  /* MOBProg foo */
  ch->mpscriptnum = 0;
  ch->mpscriptstep = 0;
  ch->mpscriptactor = NULL;
  ch->mpnextscript = NULL;
}


void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
}




/* initialize a new character only if race & class is set */
void init_char(struct char_data * ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */

  if (top_of_p_table == 0) {
    GET_EXP(ch) = 7000000;
    GET_LEVEL(ch) = LVL_IMPL;

    ch->points.max_hit = 500;
    ch->points.max_mana = 100;
    ch->points.max_move = 82;
  }
  set_title(ch, NULL);

  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.description = NULL;

  ch->player.hometown = 1;

  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  for (i = 0; i < MAX_TONGUE; i++)
    GET_TALK(ch, i) = 0;

  /* make favors for sex */
  if (ch->player.sex == SEX_MALE) {
    ch->player.weight = number(120, 180);
    ch->player.height = number(160, 200);
  } else {
    ch->player.weight = number(100, 160);
    ch->player.height = number(150, 180);
  }

  ch->points.max_mana = 100;
  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.hit = GET_MAX_HIT(ch);
  ch->points.max_move = 82;
  ch->points.move = GET_MAX_MOVE(ch);
  ch->points.armor = 100;

/* HACK from circle list */
/* Old Code */
/*
  player_table[top_of_p_table].id = GET_IDNUM(ch) = ++top_idnum;
*/
/* New Code */
  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp((player_table + i)->name, GET_NAME(ch)))
      (player_table + i)->id = GET_IDNUM(ch) = ++top_idnum;
/* end of hack */

  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0)
    else
      SET_SKILL(ch, i, 100);
  }

  ch->char_specials.saved.affected_by = 0;

  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);
}



/* returns the real number of the room with given virtual number */
int real_room(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) >> 1;

    if ((world + mid)->number == virtual)
      return mid;
    if (bot >= top)
      return -1;
    if ((world + mid)->number > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the monster with given virtual number */
int real_mobile(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the object with given virtual number */
int real_object(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}


int real_zone(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_zone_table;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((zone_table + mid)->number == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((zone_table + mid)->number > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/* the functions */

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

int mprog_name_to_type(char *name)
{
   if ( !str_cmp( name, "in_file_prog"   ) )    return IN_FILE_PROG;
   if ( !str_cmp( name, "act_prog"       ) )    return ACT_PROG;
   if ( !str_cmp( name, "speech_prog"    ) )    return SPEECH_PROG;
   if ( !str_cmp( name, "rand_prog"      ) )    return RAND_PROG;
   if ( !str_cmp( name, "fight_prog"     ) )    return FIGHT_PROG;
   if ( !str_cmp( name, "hitprcnt_prog"  ) )    return HITPRCNT_PROG;
   if ( !str_cmp( name, "death_prog"     ) )    return DEATH_PROG;
   if ( !str_cmp( name, "entry_prog"     ) )    return ENTRY_PROG;
   if ( !str_cmp( name, "greet_prog"     ) )    return GREET_PROG;
   if ( !str_cmp( name, "all_greet_prog" ) )    return ALL_GREET_PROG;
   if ( !str_cmp( name, "give_prog"      ) )    return GIVE_PROG;
   if ( !str_cmp( name, "bribe_prog"     ) )    return BRIBE_PROG;
   if ( !str_cmp( name, "social_prog"    ) )	return SOCIAL_PROG;
   if ( !str_cmp( name, "command_prog"   ) )	return COMMAND_PROG;
   if ( !str_cmp( name, "script_prog"    ) )    return SCRIPT_PROG;
   if ( !str_cmp( name, "time_prog"      ) )    return TIME_PROG;
   if ( !str_cmp( name, "kill_prog"      ) )    return KILL_PROG;
   if ( !str_cmp( name, "greet_every_prog"))    return GREET_EVERY_PROG;
   if ( !str_cmp( name, "all_greet_every_prog")) return ALL_GREET_EVERY_PROG;
   if ( !str_cmp( name, "spell_prog"     ) )    return SPELL_PROG;
   if ( !str_cmp( name, "load_prog"      ) )	return LOAD_PROG;

   return( ERROR_PROG );
}

/*
 * Read a number from a file.
 */
int fread_number(FILE *fp)
{
    int number;
    bool sign;
    char c;

    do {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    sign = FALSE;
    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = TRUE;
        c = getc(fp);
    }

    if (!isdigit(c)) {
        bug("Fread_number: bad format, digit '%c'.", c);
        exit(1);
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign)
        number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if (c != ' ')
        ungetc(c, fp);

    return number;
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE *fp )
{
    char c;

    do {
        c = getc(fp);
    } while (c != '\n' && c != '\r');

    do {
        c = getc(fp);
    } while (c == '\n' || c == '\r');

    ungetc(c, fp);
    return;
}

/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE *fp)
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do {
        cEnd = getc(fp);
    } while (isspace(cEnd));

    if ( cEnd == '\'' || cEnd == '"' )
    {
        pword   = word;
    }
    else
    {
        word[0] = cEnd;
        pword   = word+1;
        cEnd    = ' ';
    }

    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
        *pword = getc( fp );
        if ( cEnd == ' ' ? isspace(*pword) || *pword == '~' : *pword == cEnd )
        {
            if ( cEnd == ' ' || cEnd == '~' )
                ungetc( *pword, fp );
            *pword = '\0';
            return word;
        }
    }

    log("SYSERR: Fread_word: word too long.");
    exit( 1 );
    return NULL;
}


/* This routine reads in scripts of MOBprograms from a file */

MPROG_DATA* mprog_file_read( char *f, MPROG_DATA *mprg,
                            struct index_data *pMobIndex )
{

  char        MOBProgfile[ MAX_INPUT_LENGTH ];
  MPROG_DATA *mprg2;
  FILE       *progfile;
  char        letter;
  bool        done = FALSE;

  sprintf( MOBProgfile, "%s/%s", MOB_DIR, f );

  progfile = fopen( MOBProgfile, "r" );
  if ( !progfile )
  {
     bug( "Mob: %d couldnt open mobprog file", pMobIndex->virtual);
     exit( 1 );
  }

  mprg2 = mprg;
  switch ( letter = fread_letter( progfile ) )
  {
    case '>':
     break;
    case '|':
       bug( "empty mobprog file before '%c'.", letter );
       exit( 1 );
     break;
    default:
       bug( "in mobprog file syntax error at letter '%c'.", letter );
       exit( 1 );
     break;
  }

  while ( !done )
  {
    mprg2->type = mprog_name_to_type( fread_word( progfile ) );
    switch ( mprg2->type )
    {
     case ERROR_PROG:
        bug( "Error %d, mobprog file type error.", ERROR_PROG );
        exit( 1 );
      break;
     case IN_FILE_PROG:
        bug( "Error %d, mprog file contains a call to file.", IN_FILE_PROG );
        exit( 1 );
      break;
     default:
        sprintf(buf2, "Error in file %s", f);
        pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
        mprg2->arglist       = fread_string( progfile,buf2 );
        mprg2->comlist       = fread_string( progfile,buf2 );
        switch ( letter = fread_letter( progfile ) )
        {
          case '>':
             mprg2->next = (MPROG_DATA *)malloc( sizeof( MPROG_DATA ) );
             mprg2       = mprg2->next;
             mprg2->next = NULL;
           break;
          case '|':
             done = TRUE;
           break;
          default:
             bug( "in mobprog file %s syntax error.", f );
             exit( 1 );
           break;
        }
      break;
    }
  }
  fclose( progfile );
  return mprg2;
}

struct index_data *get_obj_index (int vnum)
{
  int nr;
  for(nr = 0; nr <= top_of_objt; nr++) {
    if(obj_index[nr].virtual == vnum) return &obj_index[nr];
  }
  return NULL;
}

struct index_data *get_mob_index (int vnum)
{
  int nr;
  for(nr = 0; nr <= top_of_mobt; nr++) {
    if(mob_index[nr].virtual == vnum) return &mob_index[nr];
  }
  return NULL;
}

/* This procedure is responsible for reading any in_file MOBprograms.
 */

void mprog_read_programs( FILE *fp, struct index_data *pMobIndex)
{
  MPROG_DATA *mprg;
  char        letter;
  bool        done = FALSE;

  if ( ( letter = fread_letter( fp ) ) != '>' )
  {
/*
      bug( "Load_mobiles: vnum %d MOBPROG char", pMobIndex->virtual);
      exit( 1 );
*/
      sprintf(buf, "SYSERR:  Warning: Error in mobprog vnum %d char\n",
          pMobIndex->virtual);
      log(buf);
      return;
  }
  pMobIndex->mobprogs = (MPROG_DATA *)malloc( sizeof( MPROG_DATA ) );
  mprg = pMobIndex->mobprogs;

  while ( !done )
  {
    mprg->type = mprog_name_to_type( fread_word( fp ) );
    switch ( mprg->type )
     {
     case IN_FILE_PROG:
        sprintf(buf2, "Mobprog for mob #%d", pMobIndex->virtual);
        mprg = mprog_file_read( fread_word(fp), mprg,pMobIndex );
        fread_to_eol( fp );   /* need to strip off that silly ~*/
        switch ( letter = fread_letter( fp ) )
        {
          case '>':
             mprg->next = (MPROG_DATA *)malloc( sizeof( MPROG_DATA ) );
             mprg       = mprg->next;
             mprg->next = NULL;
           break;
          case '|':
             mprg->next = NULL;
             fread_to_eol( fp );
             done = TRUE;
           break;
          default:
             sprintf(buf, "SYSERR:  Warning: Error in mobprog vnum %d bad\n",
                 pMobIndex->virtual);
             log(buf);
             return;

           break;
        }
      break;
     /*
      * error progs make a note that there's an error (this is usually 
      * a problem with the name or similar, and then continue to read
      * in the rest of the prog without breaking or returning
      */
     case ERROR_PROG:
        /* dont quit out */
        /*
        bug( "Load_mobiles: vnum %d MOBPROG type.", pMobIndex->virtual);
        exit( 1 );
        */
        sprintf(buf, "SYSERR:  Warning: Error in mobprog vnum %d type\n",
              pMobIndex->virtual);
        log(buf);

        /* no break and no return which is different than you would expect... */
        /*
        return;
        */
      /*
      break;
      */
      /* continue on to the default behavior (read in the prog) */
     default:
        sprintf(buf2, "Mobprog for mob #%d", pMobIndex->virtual);
        pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
        mprg->arglist        = fread_string( fp, buf2 );
        mprg->comlist        = fread_string( fp, buf2 );
        switch ( letter = fread_letter( fp ) )
        {
          case '>':
             mprg->next = (MPROG_DATA *)malloc( sizeof( MPROG_DATA ) );
             mprg       = mprg->next;
             mprg->next = NULL;
           break;
          case '|':
             mprg->next = NULL;
             fread_to_eol( fp );
             done = TRUE;
           break;
          default:
             /*
             bug( "Load_mobiles: vnum %d bad MOBPROG.", pMobIndex->virtual);
             exit( 1 );
             */
             sprintf(buf, "SYSERR:  Warning: Error in mobprog vnum %d bad\n",
                 pMobIndex->virtual);
             log(buf);
             return;
           break;
        }
      break;
    }
  }

  return;

}



/*
 * this reads from an index file a list of mobprogs to load
 * the format of the index file is:
 * <line>
 * <line>
 *   .
 *   .
 *   .
 * <line>
 * #99999
 * $~
 * where each line consists of a mob #, a program name that corresponds
 * to that mob, and then an optional comment
 * ie something like:
 * 3050		baker.prg	the baker of midgaard
 * $~
 *
 * in addition, you can use *'s for comment lines
 * but please no #99999's on the end of the index file
 *
 */
void mprog_boot(void) {
  FILE *index, *mprog_file, *var_file;
  struct index_data *pMobIndex;
  char index_filename[256];
  char line[256], name[256];
  int nr = 0;
  extern struct mobprog_var_data *mobprog_vars;
  struct mobprog_var_data *tmpvar;

  mobprog_vars = NULL;
  var_file = fopen("misc/mobvars", "r");
  if (var_file) {
    for (;;) {
      fscanf(var_file, "%s %d\n", name, &nr);
      if (*name == '~') break;
      if (!mobprog_vars) {
        CREATE(mobprog_vars, struct mobprog_var_data, 1);
        mobprog_vars->name = strdup(name);
        mobprog_vars->val = nr;
        mobprog_vars->next = 0;
        tmpvar = mobprog_vars;
      } else {
        CREATE(tmpvar->next, struct mobprog_var_data, 1);
        tmpvar = tmpvar->next;
        tmpvar->name = strdup(name);
        tmpvar->val = nr;
        tmpvar->next = 0;
      }
    }
    fclose(var_file);
  }

  /* open the index file */
  sprintf(buf2, "%s/%s", MOB_DIR, INDEX_FILE);

  if (!(index = fopen(buf2, "r"))) {
    log("SYSERR:  Warning: No mobprog index file found, game loading without mobprogs.");
    return;
  }

  for (;;) {
    nr++;

    /* read in mob virtual number, mobprog name, and comments */
    if (!get_line(index, line)) {
      sprintf(buf, "SYSERR:  Warning: Format error in %s line %d\n",
          index_filename, nr);
      log(buf);
      log("SYSERR:  Warning: aborting mprog_boot()\n");
      return;
    }

    if (*line == '$')
      return;

    if ((*line == '*') || (*line == '#'))
      continue;

    if (sscanf(line, " %s %s ", buf1, buf2) != 2) {
      sprintf(buf, "SYSERR:  Warning: Format error in %s line %d\n",
          index_filename, nr);
      log(buf);
      log("SYSERR:  Warning: aborting mprog_boot()\n");
      return;
    }

    sprintf(buf, "%s/%s", MOB_DIR, buf2);

    if (!(mprog_file = fopen(buf, "r"))) {
      sprintf(buf, "SYSERR:  Warning: Cannot open mobprog '%s' for reading\n",
          buf2);
      log(buf);
      continue;
    }

    if (*buf1 == '\0' || !is_number(buf1)) {
      bug("Mpmload - Bad vnum as arg: vnum %s.", buf1);
      fclose(mprog_file);
      continue;
    }

    if ((pMobIndex = get_mob_index(atoi(buf1))) == NULL) {
      bug("Mpmload - Bad mob vnum: vnum %s.", buf1);
      fclose(mprog_file);
      continue;
    }

    mprog_read_programs(mprog_file, pMobIndex);

    fclose(mprog_file);
  }
}



void vwear_object(int wearpos, struct char_data * ch)
{
  int nr, found = 0;
  struct obj_data *obj;
 

  for (nr = 0; nr <= top_of_objt; nr++) {
    obj = read_object(obj_index[nr].virtual, VIRTUAL);
    if (CAN_WEAR(obj, wearpos)) {
      found++;
      show_vnum_obj_to_char(ch, nr, found);
    }
    extract_obj(obj);
  }
}



void vtype_object(int type, struct char_data *ch)
{
  int nr, found = 0;
  struct obj_data *obj;


  for (nr = 0; nr <= top_of_objt; nr++) {
    obj = read_object(obj_index[nr].virtual, VIRTUAL);
    if (GET_OBJ_TYPE(obj) == type) {
      found++;
      show_vnum_obj_to_char(ch, nr, found);
    }
    extract_obj(obj);
  }
}


void free_mprogs() {
  int i;
  MPROG_DATA *mpd, *next;
  struct mobprog_var_data *var, *nextvar;
  extern struct mobprog_var_data *mobprog_vars;
  
  for (i = 0; i < top_of_mobt; i++) {
    for (mpd = mob_index[i].mobprogs; mpd; ) {
      if (mpd) {
        next = mpd->next;
        if (mpd->arglist) free (mpd->arglist);
        if (mpd->comlist) free (mpd->comlist);
        free (mpd);
        mpd = next;
      }
    }
    mob_index[i].mobprogs = NULL;
  }
  for (var = mobprog_vars; var; var = nextvar) {
    nextvar = var->next;
    free (var->name);
    free (var);
  }
}

void reload_mprogs() {
  free_mprogs();
  mprog_boot();
}
