/**********************************************************************
 ** This file actually serves two related purposes: It handles clans **
 ** and houses. This is where you add the information for clans and  **
 ** their halls. It also contains the routines to handle crash-proof **
 ** and restricted-access rooms.                                     **
 **********************************************************************/

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "handler.h"
#include "screen.h"
#include "boards.h"

extern char *color_codes[];
extern int Crash_save(struct obj_data *obj, FILE *FP);
static struct obj_data *last_loaded_obj;

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new clans to be added.  If you're adding a new clan,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new clan.
 */



/* Clan #defines first
  Clan names, the way it works out, are of two kinds:
  the first kind is the way a person is used to (Clan X etc)
  the second kind is really for clan mechanics (Clan undefined, outcast)
  etc... the clan mechanics ones are set in structs.h ..
  and the names' kind are set here in clan.c.  They used to all be in
  structs.h but it makes compiles be too long and it isnt that important
  to compile in the clan names there, especially not the clan defines.
  The clan defines that are commented out are the special clan mechanics
  defines.  If you need to touch those, be aware that peoples clan things
  might start jumping around.
*/

/* #define CLAN_UNDEFINED	-1	*/
/* #define CLAN_NOCLAN		0	*/
#define CLAN_X			1
#define CLAN_DYNASTY		2
/* #define CLAN_PLEDGE		3	*/
/* #define CLAN_BLACKLISTED	4	*/
#define CLAN_DRAGONLIEGE	5
#define CLAN_AES_SEDAI    	6
#define CLAN_COVEN		7
#define CLAN_TOOT		8
#define CLAN_ASHA_MAN		9
#define CLAN_SEANCHAN      	10
#define CLAN_ELLCRYS_HAVEN     	11
#define CLAN_MADMEN             12
#define CLAN_UNUSED13     	13
#define CLAN_UNUSED14     	14
#define CLAN_UNUSED15		15
#define CLAN_UNUSED16		16
#define CLAN_EMERALD		17
#define CLAN_UNUSED18		18
#define CLAN_UNUSED19		19
/* #define NUM_CLANS		20	 */	/* change in structs.h */
/* Clan Names */

const char *clan_names[] = {
  "No Clan",
  "Clan X",
  "Dynasty",
  "Pledge",
  "Outcast",
  "DragonLiege",
  "Aes Sedai",
  "Coven",
  "TOOT",
  "Asha'Man",
  "Seanchan",
  "Ellcrys Haven",
  "MadMen",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "House Vherin'Ultrin",
  "UNUSED",
  "UNUSED",
  "\n"
};



/* These are the generic levels, for internal use */
const char *clan_levels[] = {
  "Member",
  "Leader",
  "Patron",
  "Champion",
  "\n"
};

/* These are the customized levels. Used for show only (wholist) */
/* WARNING: The 4 is hardcoded into boot_clan_level_text */
char *custom_clan_levels[NUM_CLANS][4];


/*
 * ...And the appropriate rooms for each clanguard; controls
 * which types of people the various clanguards let through.  i.e., the
 * first line shows that from room 3720, only members of Clan X are allowed
 * to go south.
 * Putting them in order makes the load limits on them easier to do.
 */
int clan_info[][3] = {
  {CLAN_COVEN,			4030,	SCMD_DOWN},
  {CLAN_X,			532,	SCMD_EAST},
  {CLAN_TOOT,			4200,	SCMD_WEST},
  {CLAN_MADMEN,                 540,    SCMD_NORTH},
  {CLAN_EMERALD,		13048,  SCMD_DOWN},
  {CLAN_AES_SEDAI,           	538,	SCMD_NORTH},
  {CLAN_DRAGONLIEGE,           	535,	SCMD_SOUTH},
  {CLAN_DYNASTY,		4031,	SCMD_WEST},
/* this must go last -- add new above! */
  {-1, -1, -1}
};

struct clan_storage_data {
  int room_num, max_items, vnum;
  struct clan_storage_data *next;
} *clan_storage_rooms;

#define MAX_ACCESS_CHARS 10

struct restricted_access_data {
  int number, vnum;
  char *names[MAX_ACCESS_CHARS];
  struct restricted_access_data *next;
} *restricted_rooms;

const sh_int clan_start_room[NUM_CLANS] = {
  3001,		/* No Clan */
  3001,		/* Clan X */
    555,		/* Dynasty */
  3001,		/* Pledge */
  3001,		/* Outcast */
    625,		/* DragonLiege */
  3001,		/* Aes Sedai */
    602,		/* Coven */
    543,		/* TOOT */
  3001,		/* Asha'man */
  3001,		/* Seanchan */
  3001,		/* Ellcrys Haven */
    523,		/* MadMen */
  3001,		/* ShadowTribe */
  3001,		/* SilverCircle */
  3001,		/* Guards of Kore */
  3001,		/* Alliance Warriors */
    641,		/* House Vherin'Ultrin */
  3001,		/* Domini */
  3001,		/* Darken */
};


/* Clan boards */
/*
  format: vnum, read lvl, write lvl, remove lvl, filename, 0 at end
  Be sure to also change NUM_OF_BOARDS in boards.h
  And to assign the special procedure to the board object
  in spec_assign.c 
  There is no special end of boards marker.
*/
  /* The deathmatch scores are always posted to board 3 (that's the fourth
     one in the list */

struct board_info_type board_info[NUM_OF_BOARDS] = {
  {3099, 0, 0, LVL_GOD, "etc/board.mort", 0},
  {3098, LVL_IMMORT, LVL_IMMORT, LVL_GRGOD, "etc/board.immort", 0},
  {3097, LVL_IMMORT, LVL_FREEZE, LVL_IMPL, "etc/board.freeze", 0},
  {3096, 0, 0, LVL_IMMORT, "etc/board.social", 0},  /* IMPORTANT */
  {3094, LVL_IMMORT, LVL_FREEZE, LVL_IMPL, "etc/board.freeze-2", 0},
  {3095, LVL_GRGOD, LVL_GRGOD, LVL_GRGOD, "etc/board.imp", 0},
  {3088, LVL_IMMORT, 0, LVL_GRGOD, "etc/board.idea", 0},
  {501, 0, 0, 0, "etc/board.clan", 0},
  {528, 0, 0, 0, "etc/board.clan-1", 0},		/* CITADEL */
  {529, 0, 0, 0, "etc/board.clan-11", 0},		/* Ellcrys */
  {531, 0, 0, 0, "etc/board.clan-10", 0},		/* Seanchan */
  {532, 0, 0, 0, "etc/board.clan-6", 0},		/* AES SEDAI */
  {533, 0, 0, 0, "etc/board.clan-8", 0},		/* TOOT */
  {534, 0, 0, 0, "etc/board.clan-12", 0},		/* MADMEN */
  {535, 0, 0, 0, "etc/board.clan-9", 0},		/* Asha'man */
  {539, 0, 0, 0, "etc/board.clan-13", 0},		/* SHADOWTRIBE */
  {540, 0, 0, 0, "etc/board.clan-14", 0},		/* SILVERCIRCLE */
  {613, 0, 0, 0, "etc/board.clan-5", 0},		/* DRAGONLIEGE */
  {600, 0, 0, 0, "etc/board.clan-7", 0},		/* COVEN */
  {566, 0, 0, 0, "etc/board.clan-17", 0},		/* EMERALD ENCLAVE */
};



int parse_clan(char *arg)
{
  int i;
  char buf[MAX_INPUT_LENGTH];


  sprintf(buf, "%c%s", UPPER(arg[0]), arg + 1);
  for (i = 0; *(clan_names[i]) != '\n'; i++)
    if (!strncasecmp(buf, clan_names[i], strlen(arg)))
      break;
  if (!strcmp(clan_names[i], "\n"))
    return CLAN_UNDEFINED;
  else
    return i;
}




long find_clan_bitvector(char *arg)
{
  int i;
  char buf[MAX_INPUT_LENGTH];

  sprintf(buf, "%c%s", UPPER(arg[0]), arg + 1);
  for (i = 0; *(clan_names[i]) != '\n'; i++)
    if (!strncasecmp(buf, clan_names[i], strlen(arg)))
      break;
  if (!strcmp(clan_names[i], "\n"))
    return 0;	/* CLAN_UNDEFINED */
  else
    return (1 << i);
}



/* some fun clan commands */
#define CLAN_EXP_COST		2000000
#define CLAN_GOLD_COST		500000
#define CLAN_MIN_LEVEL		10
ACMD(do_pledge)
{
  if (IS_NPC(ch)) {
    send_to_char("Mobiles can't pledge to a clan.", ch);
    return;
  }
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    send_to_char("Mobiles can't pledge to a clan.", ch);
    return;
  }
  if (GET_CLAN(ch) == CLAN_PLEDGE) {
    send_to_char("You're already a pledge you dork!\r\n", ch);
    act("$n drools like a stupid clan pledge.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (GET_CLAN(ch) == CLAN_BLACKLISTED) { 
    send_to_char("You are forbidden to take place in clan politics.\r\n", ch);
    return;
  }

  if ((GET_CLAN(ch) != CLAN_NOCLAN) && (GET_CLAN(ch) != CLAN_BLACKLISTED)) {
    send_to_char("You'll have to be dropped from "
                 "your current clan to pledge.\r\n", ch);
    act("$n obviously wants to be a clan pledge again, sniff.",
            FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (GET_LEVEL(ch) < CLAN_MIN_LEVEL) {
    sprintf(buf, "You have be at least level %d to pledge to a clan.",
           CLAN_MIN_LEVEL);
    send_to_char(buf, ch);
    act("$n is obviously not reliable enough to pledge to a clan.",
            FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (GET_EXP(ch) < CLAN_EXP_COST) {
    sprintf(buf, "Joining a clan costs %d experience, "
                 "which you don't have.\r\n",
           CLAN_EXP_COST);
    send_to_char(buf, ch);
    act("$n obviously hasn't got the right character to pledge to a clan.",
            FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (GET_BANK_GOLD(ch) < CLAN_GOLD_COST) {
    sprintf(buf, "Joining a clan will cost a total of %d gold,\r\n"
                 "All of which must be in the bank.\r\n", CLAN_GOLD_COST);
    send_to_char(buf, ch);
    act("$n just doesn't have the money in the bank to pledge to a clan.",
            FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
  
  GET_EXP(ch) -= CLAN_EXP_COST;
  GET_BANK_GOLD(ch) -= CLAN_GOLD_COST;

  send_to_char("You pledge yourself to joining a clan!\r\n", ch);
  act("$n pledges to join a clan!", FALSE, ch, 0, 0, TO_ROOM);

  GET_CLAN(ch) = CLAN_PLEDGE;
  GET_CLAN_LEVEL(ch) = CLAN_LVL_MEMBER;
}

void clanlog(char *str, struct char_data *ch)
{
  char buf[256];
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
  int clan;

  if (GET_LEVEL(ch) >= LVL_IMMORT)	/* cheesy protect invis immortals */
    return;

  clan = GET_CLAN(ch);

  switch (clan) {
    case CLAN_UNDEFINED:
    case CLAN_NOCLAN:
    case CLAN_PLEDGE:
    case CLAN_BLACKLISTED:
        /* dont give a clanlog for any of those "clans" */
        return;
        break;
    default:
        /* continue */
        break;
  }

  sprintf(buf, "%s: %s\r\n", clan_names[clan], str);

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING)) {
      if (GET_CLAN(i->character) == clan &&
          !PRF2_FLAGGED(ch, PRF2_NOCLAN)) {
        send_to_char(CCCLANSAY(i->character), i->character);
        send_to_char(buf, i->character);
        send_to_char(CCNRM(i->character), i->character);
      }
    }
}


ACMD(do_initiate)
{
  struct char_data *vict;

  if (IS_NPC(ch)) {
    send_to_char("Mobiles can't initiate people.\r\n", ch);
    return;
  }

  if ((GET_CLAN(ch) == CLAN_UNDEFINED) ||
      (GET_CLAN(ch) == CLAN_NOCLAN) ||
      (GET_CLAN(ch) == CLAN_PLEDGE) ||
      (GET_CLAN(ch) == CLAN_BLACKLISTED)) {
    send_to_char("You are not in a clan.\r\n", ch);
    return;
  }

  if ((GET_CLAN_LEVEL(ch) == CLAN_LVL_MEMBER)) {
    send_to_char("Only the leaders or co-leaders of clans can initiate new members.\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Initiate who?\r\n", ch);
    return;
  } else if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("They don't seem to be around here.\r\n", ch);
    return;
  } else if (GET_LEVEL(vict) >= LVL_IMMORT) {
    send_to_char("You can't innitiate immortals!\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("You're the leader of the clan you dolt!\r\n", ch);
    return;
  } else if ((GET_CLAN(vict) != CLAN_PLEDGE)) {
    send_to_char("They will have to pledge themselves "
                 "to joining first!\r\n", ch);
    return;
  }

  if (GET_CLAN(vict) == CLAN_BLACKLISTED)
    send_to_char("It may not be wise to initiate "
                 "an outcast rogue...\r\n", ch);

  GET_CLAN(vict) = GET_CLAN(ch);
  GET_CLAN_LEVEL(vict) = CLAN_LVL_MEMBER;

  sprintf(buf, "You read to %s the ancient and accepted rites of %s...\r\n",
          GET_NAME(vict), clan_names[(int) GET_CLAN(ch)]);
  send_to_char(buf, ch);
  sprintf(buf, "%s agrees to those rites and practices set forth.\r\n",
          GET_NAME(vict));
  send_to_char(buf, ch);
  sprintf(buf, "%s is now a full member of %s!!\r\n",
          GET_NAME(vict), clan_names[(int) GET_CLAN(ch)]);
  send_to_char(buf, ch); 

  sprintf(buf, "$n reads to $N the ancient and accepted rites of %s...",
          clan_names[(int) GET_CLAN(ch)]);
  act(buf, FALSE, ch, 0, vict, TO_ROOM);
  act("$N agrees to those rites and practices set forth.",
          FALSE, ch, 0, vict, TO_ROOM);
  sprintf(buf, "$N is now a full member of %s!!",
          clan_names[(int) GET_CLAN(ch)]);
  act(buf, FALSE, ch, 0, vict, TO_ROOM);

  sprintf(buf, "%s has joined the clan.", GET_NAME(vict));
  clanlog(buf, vict);

  return;
}


ACMD(do_clanleave)
{

  if ((GET_CLAN(ch) == CLAN_UNDEFINED) ||
      (GET_CLAN(ch) == CLAN_NOCLAN) ||
      (GET_CLAN(ch) == CLAN_PLEDGE) ||
      (GET_CLAN(ch) == CLAN_BLACKLISTED)) {
    send_to_char("You are not currently in a clan.\r\n", ch);
    return;
  }

  sprintf(buf, "$N has left clan %s.",
          clan_names[(int) GET_CLAN(ch)]);
  act(buf, FALSE, ch, 0, ch, TO_ROOM);

  sprintf(buf, "%s has left the clan.", GET_NAME(ch));
  clanlog(buf, ch);

  sprintf(buf, "You have left clan %s.\r\n",
          clan_names[(int) GET_CLAN(ch)]);
  send_to_char(buf, ch);

  GET_CLAN(ch) = CLAN_BLACKLISTED;
  GET_CLAN_LEVEL(ch) = CLAN_LVL_MEMBER;

  return;
}

ACMD(do_blacklist)
{
  struct char_data *vict;

  if (IS_NPC(ch)) {
    send_to_char("Mobiles can't outcast people.\r\n", ch);
    return;
  }

  if ((GET_CLAN(ch) == CLAN_UNDEFINED) ||
      (GET_CLAN(ch) == CLAN_NOCLAN) ||
      (GET_CLAN(ch) == CLAN_PLEDGE) ||
      (GET_CLAN(ch) == CLAN_BLACKLISTED)) {
    send_to_char("You are not currently in a clan.\r\n", ch);
    return;
  }

  if (GET_CLAN_LEVEL(ch) != CLAN_LVL_LEADER) {
    send_to_char("Only the leaders of clans can outcast members.\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Outcast whom?\r\n", ch);
    return;
  } else if (!(vict = get_char_vis(ch, arg))) {
    send_to_char("They don't seem to be around here.\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("You can't kick yourself out!\r\n", ch);
    return;
  } else if (GET_CLAN(vict) != GET_CLAN(ch)) {
    send_to_char("You can only kick people out of "
                 "your own clan, duh!\r\n", ch);
    return;
  }

  send_to_char("You ring the bell.\r\n", ch);
  send_to_char("You close the book.\r\n", ch);
  send_to_char("You blow out the candle.\r\n", ch);
  sprintf(buf, "%s has been outcast from %s.\r\n", GET_NAME(vict),
          clan_names[(int) GET_CLAN(ch)]);
  send_to_char(buf, ch);

  act("$n rings the bell.", FALSE, ch, 0, vict, TO_ROOM);
  act("$n closes the book.", FALSE, ch, 0, vict, TO_ROOM);
  act("$n blows out the candle.", FALSE, ch, 0, vict, TO_ROOM);
  sprintf(buf, "$N has been outcast from %s.",
          clan_names[(int) GET_CLAN(ch)]);
  act(buf, FALSE, ch, 0, vict, TO_ROOM);

  sprintf(buf, "%s has been outcast from the clan.", GET_NAME(vict));
  clanlog(buf, vict);

  sprintf(buf, "You have been outcast from %s!!!\r\n",
          clan_names[(int) GET_CLAN(ch)]);
  send_to_char(buf, vict);

  GET_CLAN(vict) = CLAN_BLACKLISTED;
  GET_CLAN_LEVEL(vict) = CLAN_LVL_MEMBER;

  return;
}

ACMD(do_clanlevel)
{
  struct char_data *vict;
  int i, curlev, newlev;
  
  char *actions[] = {"promote", "demote"};
  int directions[] = {+1, -1};
  /* Any clan levels NOT in this list are assumed to be higher than SPECIAL */
  int clanlevs[] = {
    CLAN_LVL_MEMBER,
    CLAN_LVL_CHAMPION,
    CLAN_LVL_PATRON,
    -1
  };

  if (IS_NPC(ch)) {
    send_to_char("Mobiles don't run clans!\r\n", ch);
    return;
  }

  if ((GET_CLAN(ch) == CLAN_UNDEFINED) ||
      (GET_CLAN(ch) == CLAN_NOCLAN) ||
      (GET_CLAN(ch) == CLAN_PLEDGE) ||
      (GET_CLAN(ch) == CLAN_BLACKLISTED)) {
    send_to_char("You've got to be running a clan, fool!\r\n", ch);
    return;
  }

  if (GET_CLAN_LEVEL(ch) != CLAN_LVL_LEADER) {
    send_to_char("Only the leaders of clans can do this!\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Who?\r\n", ch);
    return;
  } else if (!(vict = get_char_vis(ch, arg))) {
    send_to_char("They don't seem to be *anywhere* around here.\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("You're already leading!\r\n", ch);
    return;
  } else if (GET_CLAN(vict) != GET_CLAN(ch)) {
    sprintf(buf, "You can only %s people from your own clan!\r\n",
            actions[subcmd]);
    send_to_char(buf, ch);
    return;
  }

  curlev = -1;
  for (i = 0; clanlevs[i] != -1; i++)
    if (clanlevs[i] == GET_CLAN_LEVEL(vict)) curlev = i;
  
  if (curlev == -1) {
    sprintf(buf, "You can't %s someone that powerful!\r\n", actions[subcmd]);
    send_to_char(buf, ch);
    return;
  }
  
  newlev = curlev + directions[subcmd];
  if (newlev > 0) if (clanlevs[newlev] == -1) newlev = -1;
  if (newlev < 0) {
    sprintf(buf, "You can't %s $M any further!", actions[subcmd]);
    act(buf, TRUE, ch, NULL, vict, TO_CHAR);
    return;
  }
  
  GET_CLAN_LEVEL(vict) = clanlevs[newlev];

  return;
}




int write_storage_room( int rnum ) {
  /* Saves a clanroom to disk. Code hacked from the rent code.
     vnum is the vnum of the room. */
 
  struct obj_data *obj;
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  extern struct room_data *world;
  int vnum;
  
  obj = world[rnum].contents;
  vnum = world[rnum].number;
  
  sprintf (buf, "%d", vnum);
  if (!get_filename(buf, fname, STOREROOM_FILE)) {
    log("Error in get_filename() from write_storage_room()");
    return 1;
  }
    
  if (!(fl = fopen(fname, "w"))) {
    /* Couldn't open! Disk full? Permission error? */
    sprintf(buf, "Error opening storeroom file %s", fname);
    log(buf);
    return 1;
  }
  
  log("Saving room");
  Crash_save(obj, fl);  /* What're the chances of this working? */
  log("Room saved Ok");
  fclose(fl);
  return 0;
}

void save_all_storage_rooms() {
  struct clan_storage_data *room;
  
  for (room = clan_storage_rooms; room; room = room->next)
    write_storage_room(room->room_num);
}

ACMD(do_save_rooms) {
  save_all_storage_rooms();
}

int Crash_load_room(FILE * fl, struct room_data * room, struct obj_data * container, int * numItems, int maxItems ) {
  /* return values:
     0 - All is well, rejoice and be glad
     1 - OH NO, didn't work (sucker)
     2 - EOF
     3 - Object does not exist
  */
  char equip_command;
  char line[256];
  int item_number;
  struct obj_data *obj;
  struct obj_file_elem object;
  int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  long tmp8, tmp21;
  int tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17, tmp18,
        tmp19, tmp20;
  int load_results;
  int n;


while (!feof(fl)) { 

  get_line(fl, line);
  if (feof(fl))
    return 2;
  sscanf(line, "%c", &equip_command);
  switch (equip_command) {
    case 'G':
         sscanf(line, "G %d", &item_number);
         break;
    case 'A': /* each affected item uses two lines */
         sscanf(line, "A %d", &item_number);
         /* get_line(fl, line); is done a few lines later... */
         break;
    case 'C':
         /* Begin a container here! */
         if (!last_loaded_obj)
           log ("Container contents started in rent file with nothing to fill!");
         load_results = Crash_load_room(fl, room, last_loaded_obj, numItems, maxItems);
         if (load_results == 2 || load_results == 1) return load_results;
         if (load_results == 3) continue;
         item_number = -1;
         break;
    case 'c':
         if (!container)
           log ("End of container found OUTSIDE container in rent file!");
         return 0;
         /* End a container */
         break;
    case 'D': case 'N': case 'S':
    	item_number = -1;
    	break;
    default:
         /* some new load type? */
         log ("In Crash_load: unknown load type!!!");
         break;
  }
  if (item_number > -1) {
    object.item_number = item_number;
    if (ferror(fl)) {
      perror("Reading crash file: Crash_load.");
      fclose(fl);
      return 1;
    }
    obj = read_object(real_object(object.item_number), REAL);
    /* HACKED to skip loading eq that does not exist */
    /* and read the next line of an 'A' load */
    if (obj == NULL) {
      sprintf(buf, "SYSERR: obj #%d does not exist", object.item_number);
      log(buf);
      if (equip_command == 'A')
        get_line(fl, line);
      return 3;
    }
    /* end of hack */
    
    last_loaded_obj = (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) ? obj : NULL;

    if (equip_command == 'A') {
      get_line(fl, line);
      /* watch out for MAX_OBJ_AFFECTS!! :( */
      n = sscanf(line, "%d %d %d %d %d %d %d %ld %d %d %d %d %d %d %d %d %d %d %d %d %ld",
          &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7, &tmp8,
          &tmp9, &tmp10, &tmp11, &tmp12, &tmp13, &tmp14, 
          &tmp15, &tmp16, &tmp17, &tmp18, &tmp19, &tmp20, &tmp21);
      obj->obj_flags.value[0] = tmp1;
      obj->obj_flags.value[1] = tmp2;
      obj->obj_flags.value[2] = tmp3;
      obj->obj_flags.value[3] = tmp4;
      obj->obj_flags.extra_flags = tmp5;
/*
      obj->obj_flags.weight = tmp6;
*/
      obj->obj_flags.timer = tmp7;

      if (n > 7)
          obj->obj_flags.bitvector = tmp8;
      else
          obj->obj_flags.bitvector = 0;
      if (n > 20)
          obj->obj_flags.bitvector2 = tmp21;
      else
          obj->obj_flags.bitvector2 = 0;

      obj->affected[0].location = tmp9;
      obj->affected[0].modifier = tmp10;
      obj->affected[1].location = tmp11;
      obj->affected[1].modifier = tmp12;
      obj->affected[2].location = tmp13;
      obj->affected[2].modifier = tmp14;
      obj->affected[3].location = tmp15;
      obj->affected[3].modifier = tmp16;
      obj->affected[4].location = tmp17;
      obj->affected[4].modifier = tmp18;
      obj->affected[5].location = tmp19;
      obj->affected[5].modifier = tmp20;
    }
    if (
      IS_OBJ_STAT(obj, ITEM_NORENT) ||
      (*numItems >= maxItems && maxItems > 0) ||
      GET_OBJ_TYPE(obj) == ITEM_TRASH
    ) {
      /* can't be keeping these in the room! */
      extract_obj(obj);
    } else {
      (*numItems)++;
      if (container)
        obj_to_obj(obj, container);
      else
        obj_to_room(obj, real_room(room->number));
    }
    }
  }
  return 0;
};  


int load_one_room( int rnum, int max_items ) {
  /* Loads a clanroom from disk */
   
  int vnum, temp = 0;
  struct room_data *room;
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  extern struct room_data *world;
  
  vnum = world[rnum].number;
  room = &world[rnum];
  
  sprintf (buf, "%d", vnum);
  if (!get_filename(buf, fname, STOREROOM_FILE)) {
    log("Error in get_filename() from write_storage_room()");
    return 1;
  }
    
  if (!(fl = fopen(fname, "r"))) {
    /* Couldn't open! Disk full? Permission error? */
    /* probably just non-existant! */
    sprintf(buf, "No storeroom file for room %s", fname);
    log(buf);
    return 1;
  }
  
  temp = 0;
  Crash_load_room(fl, room, NULL, &temp, max_items);
  /* USE POINTERS or globals */
  
  fclose(fl);
  return 0;
}


void load_storage_rooms() {
  struct clan_storage_data *room;

  for (room = clan_storage_rooms; room; room = room->next)
    load_one_room( room->room_num, room->max_items );
}

ACMD(do_load_storage_rooms) {
  load_storage_rooms();
}

int boot_storage_rooms() {
  FILE *fl;
  int vnum, max_items;
  struct clan_storage_data *room;
  
  clan_storage_rooms = NULL;
  if (!(fl = fopen(STORAGE_FILE, "r"))) {
    sprintf(buf, "SYSERR: can't open storage room file '%s'", STORAGE_FILE);
    perror(buf);
    return 1;
  }
  
  for (;;) {
    fscanf(fl, "%d %d\n", &vnum, &max_items);
    if (vnum == -1) break;
    CREATE(room, struct clan_storage_data, 1);
    room->next = clan_storage_rooms;
    room->vnum = vnum;
    room->room_num = real_room(vnum);
    room->max_items = max_items;
    clan_storage_rooms = room;
  }
  
  fclose(fl);
  return 0;
}
      
int write_storage_room_list() {
  /* Saves the list of storage rooms */
  FILE *fl;
  struct clan_storage_data *room;
  extern struct room_data *world;
  
  log("Writing storage room list.");

  if (!(fl = fopen(STORAGE_FILE, "w"))) {
    sprintf(buf, "SYSERR: can't open storage room file '%s'", STORAGE_FILE);
    perror(buf);
    return 1;
  }

  for (room = clan_storage_rooms; room; room = room->next)
    fprintf(fl, "%d %d\n", world[room->room_num].number, room->max_items);

  fprintf(fl, "-1 -1\n");
  fclose(fl);
  return 0;
}

ACMD(do_add_storage_room) {
  struct clan_storage_data *room;
  int vnum, rnum, max_items;
  
  two_arguments(argument, buf, buf2);
  if (!*buf || !*buf2) {
    send_to_char("Syntax: addstorage room_vnum max_items\r\n"
                 "Use max_items = 0 for infinite capacity.\r\n", ch);
    return;
  }
  vnum = atoi(buf);
  max_items = atoi(buf2);
  
  if ((rnum = real_room(vnum)) == -1) {
    send_to_char("Invalid room vnum!\r\n", ch);
    return;
  }
  
  for (room = clan_storage_rooms; room; room = room->next)
    if (rnum == room->room_num) break;
  if (room) {
    room->max_items = max_items;
    send_to_char("Existing storage room modified.\r\n", ch);
  } else {
    CREATE(room, struct clan_storage_data, 1);
    room->next = clan_storage_rooms;
    clan_storage_rooms = room;
    room->room_num = rnum;
    room->vnum = vnum;
    room->max_items = max_items;
    send_to_char("New storage room added.\r\n", ch);
  }
  
  write_storage_room_list();
}

void list_storage_rooms( struct char_data *ch ) {
  extern struct room_data *world;
  struct clan_storage_data *room;
  
  sprintf(buf, " Vnum         Max. Items     Room\r\n"
               "------------------------------------------------------------\r\n");

  for (room = clan_storage_rooms; room; room = room->next)
    sprintf(buf, "%s%5d%16d        %s\r\n", buf, world[room->room_num].number,
            room->max_items, world[room->room_num].name);
  
  send_to_char(buf, ch);
}

void renum_storage_rooms() {
  /* Reassigns the rnums for the rooms after an OLC edit */
  struct clan_storage_data *room;
  
  for (room = clan_storage_rooms; room; room = room->next)
    room->room_num = real_room(room->vnum);
}

int boot_restricted_rooms() {
  FILE *fl;
  int vnum, cnum;
  struct restricted_access_data *room;
  
  restricted_rooms = NULL;
  if (!(fl = fopen(RESTRICTED_FILE, "r"))) {
    sprintf(buf, "SYSERR: can't open restricted room file '%s'", STORAGE_FILE);
    perror(buf);
    return 1;
  }
  
  for (;;) {
    fscanf(fl, "%d\n", &vnum);
    if (vnum == -1) break;
    CREATE(room, struct restricted_access_data, 1);
    room->next = restricted_rooms;
    room->number = real_room(vnum);
    room->vnum = vnum;
    for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++)
      room->names[cnum] = NULL;
    for (cnum = 0;; cnum++) {
      fscanf(fl, "%s\n", buf);
      if (!strcmp(buf, "_END")) break;
      room->names[cnum] = strdup(buf);
    }
    restricted_rooms = room;
  }
  
  fclose(fl);
  return 0;
}

int check_for_access( struct char_data *ch, int rnum ) {
  struct restricted_access_data *room;
  int cnum;
  
  if (IS_NPC(ch)) return 0; /* Mobs are allowed in by default. Set the room
                               to !MOB if you want to keep them out. */
  /* That was a mistake - charmies & jarred! changed to 0 */
  for (room = restricted_rooms; room; room = room->next) {
    if (room->number == rnum) {
      for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++)
        if (room->names[cnum])
          if (!str_cmp(room->names[cnum], GET_NAME(ch))) return 1;
      return 0; /* Found the room, but he wasn't listed! */
    }
  }
  
  return 1; /* We don't have that room! Everyone's allowed in! */
}

void list_restricted_rooms( struct char_data *ch ) {
  struct restricted_access_data *room;
  int cnum;
  extern struct room_data *world;
  
  *buf = '\0';
  
  for (room = restricted_rooms; room; room = room->next) {
    sprintf(buf, "%s%s (%d) :\r\n", buf, world[room->number].name,
                                         world[room->number].number);
    for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++)
      if (room->names[cnum])
        sprintf(buf, "%s     %s\r\n", buf, room->names[cnum]);
  }
  
  send_to_char(buf, ch);
}

void write_restricted_rooms() {
  struct restricted_access_data *room;
  int cnum;
  FILE *fl;
  extern struct room_data *world;

  log("Writing restricted room list.");

  if (!(fl = fopen(RESTRICTED_FILE, "w"))) {
    sprintf(buf, "SYSERR: can't open restricted room file '%s'", STORAGE_FILE);
    perror(buf);
    return;
  }
  
  for (room = restricted_rooms; room; room = room->next) {
    fprintf(fl, "%d\n", world[room->number].number);
    for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++)
      if (room->names[cnum])
        fprintf(fl, "%s\n", room->names[cnum]);
    fprintf(fl, "_END\n");
  }
  
  fprintf(fl, "-1\n");
  
  fclose(fl);
  return;
}

ACMD(do_addrestrict) {
  struct restricted_access_data *room;
  int cnum, vnum, rnum;
  extern struct room_data *world;
  
  two_arguments(argument, buf, buf2);
  
  if (!*buf || !*buf2) {
    send_to_char("Syntax: addrestrict <room> <character>\r\n", ch);
    return;
  }
  
  if ((rnum = real_room(vnum = atoi(buf))) == -1) {
    sprintf(buf2, "Room %s does not exist!\r\n", buf);
    send_to_char(buf2, ch);
    return;
  }
  
  if (vnum == 0) {
    /* The fool probably has his arguments in the wrong order! */
    send_to_char("Syntax: addrestrict <room> <character>\r\n", ch);
    return;
  }
  
  for (room = restricted_rooms; room; room = room->next)
    if (room->number == rnum) break;
  if (!room) {
    sprintf(buf, "Adding restricted room %s (%d)\r\n"
                 "Remember to set the room's HOUSE flag!\r\n",
                  world[rnum].name, vnum);
    send_to_char(buf, ch);
    sprintf(buf, "Creating restricted room #%d", vnum);
    log(buf);
    CREATE(room, struct restricted_access_data, 1);
    room->next = restricted_rooms;
    restricted_rooms = room;
    room->number = rnum;
    room->vnum = vnum;
    for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++)
      room->names[cnum] = NULL;
  }
  for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++)
    if (room->names[cnum])
      if (!str_cmp(room->names[cnum], buf2)) {
        sprintf(buf, "%s already has access to %s!\r\n", buf2, world[rnum].name);
        send_to_char(buf, ch);
        return;
      }
  for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++)
    if (!room->names[cnum]) break;
  if (cnum >= MAX_ACCESS_CHARS) {
    send_to_char("Sorry, the access list for that room is full!\r\n", ch);
    return;
  }
  sprintf(buf, "Adding %s to %s (%d)\r\n", buf2, world[rnum].name, vnum);
  send_to_char(buf, ch);
  sprintf(buf, "%s has granted %s access to room #%d.", GET_NAME(ch), buf2, vnum);
  log(buf);
  room->names[cnum] = strdup(buf2);
  
  write_restricted_rooms();
}

ACMD(do_removerestrict) {
  int vnum, rnum, cnum;
  struct restricted_access_data *room, *room2;
  extern struct room_data *world;
  
  two_arguments(argument, buf, buf2);
  
  if (!*buf) {
    send_to_char("Syntax: removerestrict <vnum> <char>\r\n"
         "        If char is omitted, all restrictions will be removed.\r\n"
         , ch);
    return;
  }
  
  if ((rnum = real_room(vnum = atoi(buf))) == -1) {
    sprintf(buf2, "Room number %d does not exist!\r\n", vnum);
    send_to_char(buf2, ch);
  }
  
  if (vnum == 0) {
    send_to_char("Syntax: removerestrict <vnum> <char>\r\n"
         "        If char is omitted, all restrictions will be removed.\r\n"
         , ch);
    return;
  }
  
  for (room = restricted_rooms; room; room = room->next) {
    if (room->number == rnum) {
      if (*buf2) {
        for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++) {
          if (room->names[cnum]) {
            if (!str_cmp(room->names[cnum], buf2)) {
              free(room->names[cnum]);
              room->names[cnum] = NULL;
              sprintf(buf, "Removing %s's access to %s (%d).\r\n", buf2,
                      world[rnum].name, vnum);
              send_to_char(buf, ch);
              sprintf(buf, "%s has removed %s's access to restricted room #%d",
                      GET_NAME(ch), buf2, vnum);
              log(buf);
            }
          }
        }
      } else {
        for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++) {
          if (room->names[cnum]) {
            free(room->names[cnum]);
            room->names[cnum] = NULL;
          }
        }
        sprintf(buf, "Removing all characters' access to %s (%d).\r\n",
                world[rnum].name, vnum);
        send_to_char(buf, ch);
        sprintf(buf, "%s has removed all characters from restricted room #%d",
                GET_NAME(ch), vnum);
        log(buf);
      }
      for (cnum = 0; cnum < MAX_ACCESS_CHARS; cnum++) {
        if (room->names[cnum]) break;
      }
      if (cnum >= MAX_ACCESS_CHARS) {
        /* No one's left! */
        if (restricted_rooms == room) {
          restricted_rooms = room->next;
        } else {
          for (room2 = restricted_rooms; room2; room2 = room2->next) {
            if (room2->next == room) {
              room2->next = room->next;
              break;
            }
          }
        }
        free(room);
        sprintf(buf, "No one has access to %s! Deleting record.\r\n", world[rnum].name);
        send_to_char(buf, ch);
        sprintf(buf, "Deleting restricted room #%d (empty).", vnum);
        log(buf);
      }
      write_restricted_rooms();
      break;
    }
  }
}

void renum_restricted_rooms() {
  /* Reassigns vnums for restricted rooms after an OLC edit */
  struct restricted_access_data *room;
  
  for (room = restricted_rooms; room; room = room->next)
    room->number = real_room(room->vnum);
}

void boot_clan_level_text() {
  int i, j, num;
  FILE *fl;
  
  for (i = 0; i < NUM_CLANS; i++) {
    custom_clan_levels[i][0] = strdup("Member");
    custom_clan_levels[i][1] = strdup("Leader");
    custom_clan_levels[i][2] = strdup("Patron");
    custom_clan_levels[i][3] = strdup("Champion");
  }
  
  fl = fopen(CLANLEV_FILE, "r");
  if (fl) {
    for (;;) {
      fscanf(fl, "%d\n", &num);
      if (feof(fl)) break;   /* just in case */
      if (num == -1) break;
      for (j = 0; j < 4; j++) {
	free(custom_clan_levels[num][j]);
	get_line(fl, buf);
	custom_clan_levels[num][j] = strdup(buf);
      }
    }
    fclose(fl);
  }
}

void reload_clan_text() {
  int i, j;
  for (i = 0; i < NUM_CLANS; i++) {
    for (j = 0; j < 4; j++) {
      free (custom_clan_levels[i][j]);
    }
  }
  boot_clan_level_text();
}

ACMD(do_clanleav)
{
    send_to_char("If you want to leave your clan, you must spell it out entirely.\n\r", ch);
    return;
}
