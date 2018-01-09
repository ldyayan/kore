/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "boards.h"
/* for NeXTs */
#include <ctype.h>

/* external variables */
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct index_data *get_mob_index(int vnum);
extern struct index_data *get_obj_index(int vnum);
extern char *color_codes[];
extern char *dirs[];
extern char *dir_abbrevs[];
extern struct mobprog_var_data *mobprog_vars;

/* external functions */
sh_int find_target_room(struct char_data * ch, char *rawroomstr);
ACMD(do_trans);
int number_range(int from, int to);
void mprog_script_trigger(struct char_data * mob, struct char_data * actor,
    char *script_name);
int is_scripting(struct char_data *mob);
int stop_script(struct char_data *mob);
int find_first_step(sh_int src, sh_int target);


/* defines */
#define bug(x, y) { sprintf(buf2, (x), (y)); log(buf2); }


/*
 * Local functions.
 */

char * mprog_type_to_name(int type);



/*
 * This routine transfers between alpha and numeric forms of the
 * mob_prog bitvector types. It allows the words to show up in mpstat to
 * make it just a hair bit easier to see what a mob should be doing.
 */
char *mprog_type_to_name(int type)
{
  switch (type) {
    case IN_FILE_PROG:          return "in_file_prog";
    case ACT_PROG:              return "act_prog";
    case SPEECH_PROG:           return "speech_prog";
    case RAND_PROG:             return "rand_prog";
    case FIGHT_PROG:            return "fight_prog";
    case HITPRCNT_PROG:         return "hitprcnt_prog";
    case DEATH_PROG:            return "death_prog";
    case ENTRY_PROG:            return "entry_prog";
    case GREET_PROG:            return "greet_prog";
    case ALL_GREET_PROG:        return "all_greet_prog";
    case GIVE_PROG:             return "give_prog";
    case BRIBE_PROG:            return "bribe_prog";
    case SOCIAL_PROG:		return "social_prog";
    case COMMAND_PROG:		return "command_prog";
    case SCRIPT_PROG:		return "script_prog";
    case TIME_PROG:		return "time_prog";
    case KILL_PROG:		return "kill_prog";
    case GREET_EVERY_PROG:	return "greet_every_prog";
    case ALL_GREET_EVERY_PROG:	return "all_greet_every_prog";
    case SPELL_PROG:		return "spell_prog";
    case LOAD_PROG:		return "load_prog";
    default:                    return "ERROR_PROG";
  }
}



/* string prefix routine */

bool str_prefix(const char *astr, const char *bstr)
{
  if (!astr) {
    log("Strn_cmp: null astr.");
    return TRUE;
  }
  if (!bstr) {
    log("Strn_cmp: null astr.");
    return TRUE;
  }
  for(; *astr; astr++, bstr++) {
    if(LOWER(*astr) != LOWER(*bstr)) return TRUE;
  }
  return FALSE;
}


/*
 * A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MOBprograms which are set.
 *
 * Supplanted by an expanded do_stat_character, -Aule
 */
void do_mpstat(struct char_data *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  MPROG_DATA *mprg;
  struct char_data  *victim;
  int error;


  one_argument(argument, arg);

  if (arg[0] == '\0') {
    send_to_char("MobProg stat whom?\n\r", ch);
    return;
  }

  if ((victim = get_char_vis(ch, arg)) == NULL) {
    send_to_char("Nothing around by that name.\n\r", ch);
    return;
  }

  if (!IS_NPC(victim)) {
    send_to_char( "Only mobs can have mobprogs.\n\r", ch);
    return;
  }

  sprintf(buf, "Name: '%s%s%s', Alias: '%s%s%s', VNum: [%s%5d%s]\n\r",
        CCINFO(ch), victim->player.short_descr, CCNRM(ch),
        CCINFO(ch), victim->player.name, CCNRM(ch),
        CCINFO(ch), mob_index[victim->nr].virtual, CCNRM(ch));
  send_to_char(buf, ch);

  if (!(mob_index[victim->nr].progtypes)) {
    send_to_char("Mobprogs: None.\r\n", ch);
    return;
  } else {
    send_to_char("Mobprogs: Exists!\r\n", ch);
  }

  error = 0;
  for (mprg = mob_index[victim->nr].mobprogs; mprg != NULL;
	 mprg = mprg->next) {
    if (mprg->type == ERROR_PROG) {
      error = 1;
      sprintf(buf, "%s>%s%s%s~%s\n\r%s~\n\r",
          CCWARNING(ch), mprog_type_to_name(mprg->type),
          CCINFO(ch), mprg->arglist, CCNRM(ch),
          mprg->comlist);
      send_to_char(buf, ch);
    } else {
      sprintf(buf, "%s>%s%s%s~%s\n\r%s~\n\r",
          CCALERT(ch), mprog_type_to_name(mprg->type),
          CCINFO(ch), mprg->arglist, CCNRM(ch),
          mprg->comlist);
      send_to_char(buf, ch);
    }
  }
  sprintf(buf, "%s|%s\r\n", CCALERT(ch), CCNRM(ch));
  send_to_char(buf, ch);
  if (error) {
    sprintf(buf, "%s*** ERROR_PROG warning!! check mobprog above!! ***%s\r\n",
        CCWARNING(ch), CCNRM(ch));
    send_to_char(buf, ch);
  } 
  
  return;
}



/*
 * prints the argument to all the rooms aroud the mobile
 */
void do_mpasound(struct char_data *ch, char *argument)
{
  sh_int was_in_room;
  int door;
  /* char arg[MAX_INPUT_LENGTH]; */
  char *p;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  if (argument[0] == '\0') {
    bug("Mpasound - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

/* one_argument(argument, arg); */
  p = argument;
  while(isspace(*p)) p++; /* skip over leading space */

  was_in_room = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    struct room_direction_data       *pexit;
      
    if ((pexit = world[was_in_room].dir_option[door]) != NULL
       && pexit->to_room != NOWHERE
       && pexit->to_room != was_in_room) {
      ch->in_room = pexit->to_room;
      MOBTrigger = FALSE;
      act(p, FALSE, ch, NULL, NULL, TO_ROOM);
    }
  }

  ch->in_room = was_in_room;
  return;
}



/*
 * lets the mobile kill any player or mobile without murder
 */
void do_mpkill(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    bug("Mpkill - no argument: vnum %d.",
            mob_index[ch->nr].virtual );
    return;
  }

  if ((victim = get_char_vis(ch, arg)) == NULL) {
    bug("Mpkill - Victim does not exist: vnum %d.",
            mob_index[ch->nr].virtual);
    return;
  }

  if (victim == ch) {
    bug("Mpkill - Bad victim to attack: vnum %d.",
            mob_index[ch->nr].virtual);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
    bug("MpKill - Charmed mob attacking master: vnum %d.",
            mob_index[ch->nr].virtual);
    return;
  }

  if (FIGHTING(ch)) {
    bug( "MpKill - Already fighting: vnum %d",
            mob_index[ch->nr].virtual);
    return;
  }

  hit(ch, victim, -1);
  return;
}



/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy 
 * items using all.xxxxx or just plain all of them
 */
void do_mpjunk(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  int pos;
  struct obj_data *obj;
  struct obj_data *obj_next;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    bug("Mpjunk - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
    if ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos))
                 != NULL) {
      if (ch->equipment[pos])
        unequip_char(ch, pos);
      else
        bug("Mpjunk - No item found: vnum %d.", mob_index[ch->nr].virtual);
      extract_obj(obj);
      return;
    }
    if ((obj = get_obj_in_list_vis(ch, arg, ch->carrying)) != NULL )
      extract_obj( obj );
    return;
  } else {
    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
      obj_next = obj->next_content;
      if (arg[3] == '\0' || isname(arg+4, obj->name)) {
        extract_obj(obj);
      }
    }
    while ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos))
                    !=NULL) {
      if (ch->equipment[pos])
        unequip_char(ch, pos);
      else
        bug("Mpjunk - Not all items found: vnum %d.", mob_index[ch->nr].virtual);
      extract_obj(obj);
    }   
  }
  return;
}



/*
 * prints the message to everyone in the room other than the mob and victim
 */
void do_mpechoaround(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;
  char *p;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  p = one_argument(argument, arg);
  while(isspace(*p)) p++; /* skip over leading space */

  if (arg[0] == '\0') {
    bug("Mpechoaround - No argument:  vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!(victim = get_char_vis(ch, arg))) {
    bug( "Mpechoaround - victim does not exist: vnum %d.",
            mob_index[ch->nr].virtual );
    return;
  }

  act(p, FALSE, ch, NULL, victim, TO_NOTVICT);
  return;
}



/*
 * prints the message to only the victim
 */
void do_mpechoat(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;
  char *p;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  p = one_argument(argument, arg);
  while(isspace(*p)) p++; /* skip over leading space */

  if (arg[0] == '\0') {
    bug( "Mpechoat - No argument:  vnum %d.",
             mob_index[ch->nr].virtual );
    return;
  }

  if (!(victim = get_char_vis(ch, arg))) {
    bug( "Mpechoat - victim does not exist: vnum %d.",
            mob_index[ch->nr].virtual );
    return;
  }

  act(p, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);

  return;
}



/*
 * prints the message to the room at large
 */
void do_mpecho(struct char_data *ch, char *argument)
{
  char *p;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  if (argument[0] == '\0') {
    bug( "Mpecho - called w/o argument: vnum %d.",
            mob_index[ch->nr].virtual);
    return;
  }
  p = argument;
  while(isspace(*p)) p++;

  act(p, FALSE, ch, NULL, NULL, TO_ROOM );

  return;
}



/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory.  you can specify a level with
 * the load object portion as well.
 */
void do_mpmload(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct index_data *pMobIndex;
  struct char_data      *victim;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0' || !is_number(arg)) {
    bug("Mpmload - Bad vnum as arg: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((pMobIndex = get_mob_index(atoi(arg))) == NULL) {
    bug("Mpmload - Bad mob vnum: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  victim = read_mobile(atoi(arg), VIRTUAL);
  char_to_room(victim, ch->in_room);

  return;
}



void do_mpoload(struct char_data *ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  struct index_data *pObjIndex;
  struct obj_data *obj;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg1);
 
  if (arg1[0] == '\0' || !is_number(arg1)) {
    bug("Mpoload - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }
 
  if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL) {
    bug("Mpoload - Bad vnum arg: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  obj = read_object(atoi(arg1), VIRTUAL);
  if (CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
    obj_to_char( obj, ch );
  } else {
    obj_to_room( obj, ch->in_room );
  }

  return;
}



/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 * itself, but this had best be the last command in the MOBprogram
 * otherwise ugly stuff will happen
 */
void do_mppurge(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;
  struct obj_data *obj;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if ( arg[0] == '\0' ) {
    /* mppurge with no arguments */
    struct char_data *vnext;
    struct obj_data *obj_next;

    for (victim = world[ch->in_room].people; victim != NULL; victim = vnext) {
      vnext = victim->next_in_room;
      if (IS_NPC(victim) && victim != ch)
	extract_char(victim);
    }

    for (obj = world[ch->in_room].contents; obj != NULL; obj = obj_next) {
      obj_next = obj->next_content;
      extract_obj(obj);
    }

    return;
  }

  if ((victim = get_char_room_vis(ch, arg)) != NULL) {
    if (!IS_NPC(victim)) {
      bug("Mppurge - error attempt to purge a PC: vnum %d.",
          mob_index[ch->nr].virtual);
    } else
      extract_char(victim);
  } else if ((obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)) != NULL) {
    extract_obj(obj);
  } else {
/* this not found warning is not important */
/* 
    bug("Mppurge - Target not found: vnum %d.", 
        mob_index[ch->nr].virtual);
*/
  }
  return;
}



/*
 * lets the mobile goto any location it wishes that is not private
 * a special room is 'cleaner' room -- a place where the mob can be
 * safely purged.
 */
#define MPGOTO_CLEANER	"3097"
void do_mpgoto(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  sh_int location;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    bug("Mpgoto - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!strcasecmp(arg, "cleaner") || !strcasecmp(arg, "cleaners")) {
    if ((location = find_target_room(ch, MPGOTO_CLEANER)) < 0) {
      bug("Mpgoto - No such location: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }
  } else if ((location = find_target_room(ch, arg)) < 0) {
    bug("Mpgoto - No such location: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (FIGHTING(ch))
    stop_fighting(ch);

  char_from_room(ch);
  char_to_room(ch, location);

  return;
}



/*
 * lets the mobile do a command at another location. Very useful.
 */
void do_mpat(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  sh_int location;
  sh_int original;
/* struct char_data       *wch; */

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }
 
  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mpat - Bad argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((location = find_target_room(ch, arg)) < 0) {
    bug("Mpat - No such location: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  original = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, argument);

  /*
   * See if 'ch' still exists before continuing!
   * Handles 'at XXXX quit' case.
   */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original);
  }

  return;
}



/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 * the nearby parameter knocks people into a nearby room
 * works for 'all' too, but 'mptransfer all nearby' moves them all
 * to the same room (you may not want that)
 * to pick one room in a range use 'mptrans <target> <roomstart-roomend>'
 * you can also have the room mptrans them to a direction, ie:
 * 'mptrans <target> <dir>'
 */
void do_mptransfer(struct char_data *ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  sh_int location;
  int first_room, last_room;
/*struct descriptor_data *d; */
  struct char_data *victim = NULL;
  struct char_data *next_victim = NULL;
  int dir = -1;
  int i;
  int count = 0;

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if (arg1[0] == '\0') {
    bug("Mptransfer - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

/* old mptrans all code */
/* note that this doesnt do a good job of finding location */
/*
  if (!str_cmp(arg1, "all")) {
    for (d = descriptor_list; d != NULL; d = d->next) {
      if (STATE(d) && d->character != ch
           && d->character->in_room != NOWHERE && CAN_SEE(ch, d->character)) {
        char buf[MAX_STRING_LENGTH];
        sprintf(buf, "%s %s", d->character->player.name, arg2);
        do_trans(ch, buf, 0, 0);
      }
    }
    return;
  }
*/

  /*
   * Thanks to Grodyn for the optional location parameter.
   */
  if (arg2[0] == '\0') {
    location = ch->in_room;
  } else if (!strcmp(arg2, "nearby")) {
    dir = -1;
    location = ch->in_room;
    for (i = 0; i < NUM_OF_DIRS - 1; i++)
      if (CAN_GO(ch, i)) {
        if (number_range(0, count) == 0) {
          dir = i;
          count++;
        }
      }
    if (dir != -1)
      location = EXIT(ch, dir)->to_room;
  } else if ((dir = search_block(arg2, dirs, FALSE)) >= 0) {
    if (CAN_GO(ch, dir))
      location = EXIT(ch, dir)->to_room;
    else
      location = ch->in_room;
  } else if ((dir = search_block(arg2, dir_abbrevs, FALSE)) >= 0) {
    if (CAN_GO(ch, dir))
      location = EXIT(ch, dir)->to_room;
    else
      location = ch->in_room;
  } else if (strstr(arg2, "-") != NULL) {
    first_room = atoi(strtok(arg2, "-"));
    last_room = atoi(strtok(NULL, "-"));
    if (last_room >= first_room)
      location = number_range(first_room, last_room); 
    else
      location = number_range(first_room, last_room);
    if (real_room(location) < 0) {
      bug("Mptransfer - No such location: vnum %d.",
               mob_index[ch->nr].virtual);
      return;
    }
  } else {
    if ((location = find_target_room(ch, arg2)) < 0) {
       bug("Mptransfer - No such location: vnum %d.",
                mob_index[ch->nr].virtual);
       return;
    }

    if (IS_SET(world[location].room_flags, ROOM_PRIVATE)) {
       bug("Mptransfer - Private room: vnum %d.",
                mob_index[ch->nr].virtual);
       return;
    }
  }

  if (!str_cmp(arg1, "all")) {

    for (victim = world[ch->in_room].people;
         victim;
         victim = next_victim) {

      next_victim = victim->next_in_room;

      /*
       * The skips: 1) me
       *            2) uncharmed mobs that arent fighting me
       *            3) people i cant see
       * from there everyone else gets mptranferred
       */
      if (victim == ch)
        continue;
      if (IS_NPC(victim) &&
          !IS_AFFECTED(victim, AFF_CHARM) &&
          FIGHTING(victim) != ch)
        continue;
      if (!CAN_SEE(ch, victim))
        continue;
 
      if (victim->in_room == NOWHERE) {
        bug("Mptransfer - Victim in Limbo: vnum %d.",
                mob_index[ch->nr].virtual);
        return;
      }

      if (FIGHTING(victim))
        stop_fighting(victim);

      char_from_room(victim);
      char_to_room(victim, location);
    }
    return;
  }

  /* just a lone target */
  if ((victim = get_char_vis(ch, arg1)) == NULL) {
    bug("Mptransfer - No such person: vnum %d.",
                mob_index[ch->nr].virtual);
    return;
  }

  if (victim->in_room == NOWHERE) {
    bug("Mptransfer - Victim in Limbo: vnum %d.",
	    mob_index[ch->nr].virtual);
    return;
  }

  if (FIGHTING(victim))
    stop_fighting(victim);

  char_from_room(victim);
  char_to_room(victim, location);

  return;
}



/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
void do_mpforce(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mpforce - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!str_cmp(arg, "all")) {
    struct descriptor_data *i;
    struct char_data *vch;

    for (i = descriptor_list; i ; i = i->next) {
      if (i->character != ch && !i->connected &&
             i->character->in_room == ch->in_room) {
        vch = i->character;
/*        if(GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch)) {*/
        if(GET_LEVEL(vch) < LVL_IMMORT && CAN_SEE(ch, vch)) {
          command_interpreter( vch, argument );
	}
      }
    }
  } else {
    struct char_data *victim;

    if ((victim = get_char_vis(ch, arg)) == NULL) {
      bug("Mpforce - No such victim: vnum %d.",
              mob_index[ch->nr].virtual);
      return;
    }

    if (victim == ch) {
      bug("Mpforce - Forcing oneself: vnum %d.",
              mob_index[ch->nr].virtual );
      return;
    }

    command_interpreter(victim, argument);
  }

  return;
}



/*
 * this casts the spells 'noiselessly' so you'll have to add some echos
 * to make this make sense..
 * you always have to have a type of magic called and a target, always!
 * Always!
 * some examples:
 *  a sanct spell:     mpcallmagic 'sanctuary' $i
 *  a poison spell:    mpcallmagic 'poison' $n
 *  a magic missile:   mpcallmagic 'magic missile' $n
 * a problem that can come up (but not a huge problem, cause its not
 * a crash bug) is that the random_trigger could have an mpcallmagic
 * in it, but the target ($n) might be invisible... if the target is
 * invisible, then mpcalltarget $n magic missile wont do what you'd 
 * expect it to.
 * Remember: you must *always* have a target; usually you mean $i or $n
 * (self or actor)
 */
void do_mpcallmagic(struct char_data *ch, char *argument)
{
  struct char_data *victim = NULL;
  int spellnum;
  char *s;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  if (argument == '\0') {
    bug("Mpcallmagic - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  s = strtok(argument, "'");
  if (s == NULL) {
    bug("Mpcallmagic - No enclosing marks: vnum %d.",
        mob_index[ch->nr].virtual);
    return;
  }

  s = strtok(NULL, "'");
  if (s == NULL) {
    bug("Mpcallmagic - Blank spellname: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum > MAX_SPELLS)) {
    bug("Mpcallmagic - Spell not found: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  /*
   * the rest of the argument, past the last ' mark is the target;
   * this can be legitimately NULL sometimes so dont check it.
   */
  s = strtok(NULL, " ");

  if (s == NULL) {
    bug("Mpcallmagic - No victim specified: vnum %d.",
        mob_index[ch->nr].virtual);
    return;
  }

  if (!str_cmp(s, "all")) {

    for (victim = world[ch->in_room].people;
         victim;
         victim = victim->next_in_room) {

      /*
       * The skips: 1) me
       *            2) uncharmed mobs that arent fighting me
       *            3) people i cant see
       * from there everyone else is a valid target
       */
      if (victim == ch)
        continue;
      if (IS_NPC(victim) &&
          !IS_AFFECTED(victim, AFF_CHARM) &&
          FIGHTING(victim) != ch)
        continue;
      if (!CAN_SEE(ch, victim))
        continue;

      call_magic(ch, victim, NULL, NULL, spellnum, GET_LEVEL(ch), CAST_SPELL);
    }

  } else /* just one victim */ {

    if ((victim = get_char_vis(ch, s)) == NULL) {
      bug("Mpcallmagic - No such victim: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }

    call_magic(ch, victim, NULL, NULL, spellnum, GET_LEVEL(ch), CAST_SPELL);
  }

  return;
}



/*
 * boy is mppose tricky
 * mppose $n sit makes a player sit, even if they're in combat
 * they get so message so its super tricky
 * mppose $n stun doesnt seem to work in combat, to a target not in combat
 * it freezes them up so that they can't move (they're stunned) a super
 * wierd kind of paralyze. maybe to get out of it the victim could sleep
 * then wake, another way to get 'out' is to quit. :)
 * this command is not *quite* as useful as I'd hoped.
 * should now do mppose all <position> with no problems.
 */
void do_mppose(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim = NULL;
  int i;
  int pos;
  int found;

  extern const char *position_types[];


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mppose - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  found = 0;
  for (i = 0; *(position_types[i]) != '\n'; i++) {
    if (!strncasecmp(argument, position_types[i], strlen(argument))) {
      pos = i;
      found = 1;
      break;
    }
  }

  if (!found) {
    bug("Mppose - No such position: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!str_cmp(arg, "all")) {

    for (victim = world[ch->in_room].people;
         victim;
         victim = victim->next_in_room) {

      /*
       * The skips: 1) me
       *            2) uncharmed mobs that arent fighting me
       *            3) people i cant see
       * from there everyone else is a valid target
       */
      if (victim == ch)
        continue;
      if (IS_NPC(victim) &&
          !IS_AFFECTED(victim, AFF_CHARM) &&
          FIGHTING(victim) != ch)
        continue;
      if (!CAN_SEE(ch, victim))
        continue;

      GET_POS(victim) = pos;
    }

  } else /* just one victim */ {

    if ((victim = get_char_vis(ch, arg)) == NULL) {
      bug("Mppose - No such victim: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }

    GET_POS(victim) = pos;
  }

  return;
}



/*
 * mpdamage is a way to hurt victims
 * you can mpdamage all <damage>
 * damage can be a single number, or it can be a diceroll
 * dice (10d10+3) for example
 * damage will never be less than 0
 */
void do_mpdamage(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim = NULL;
  struct char_data *next_vict = NULL;
  int number = 0;
  int sides = 1;
  int modifier = 0;
  int dam = 0;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mpdamage - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (str_cmp(arg, "all")) {
    if ((victim = get_char_vis(ch, arg)) == NULL) {
      bug("Mpdamage - No such victim: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }
  }
 
  if (sscanf(argument, "%d d %d + %d", &number, &sides, &modifier) != 3) {
    if (sscanf(argument, "%d d %d - %d", &number, &sides, &modifier) != 3) {
      if (sscanf(argument, "%d d %d", &number, &sides) != 2) {
        if (sscanf(argument, "%d", &number) != 1) {
          bug("Mpdamage - Unguessable damage string: vnum %d.",
              mob_index[ch->nr].virtual);
          return;
        }
      }
    } else /* it was of the form 'number d sides - modifier' */ {
      modifier *= -1;
    }
  }

  if (!str_cmp(arg, "all")) {
    for (victim = world[ch->in_room].people;
         victim;
         victim = next_vict) {
      next_vict = victim->next_in_room;

      /*
       * The skips: 1) me
       *            2) uncharmed mobs that arent fighting me
       *            3) people i cant see
       * from there everyone else is a valid target
       */
      if ((victim == ch) || (GET_POS(victim) == POS_DEAD))
        continue;
      if (IS_NPC(victim) && 
          !IS_AFFECTED(victim, AFF_CHARM) &&
          FIGHTING(victim) != ch)
        continue;
      if (!CAN_SEE(ch, victim))
        continue;
 
      dam = dice(number, sides) + modifier;

      if ( (ch) && (victim) ) {
	if (ch->in_room == victim->in_room)
	    damage(ch, victim, dam, SKILL_MPDAMAGE);
	else
	    damage(victim, victim, dam, SKILL_MPDAMAGE);
      }
    }
  } else if (GET_POS(victim) != POS_DEAD) {
    dam = dice(number, sides) + modifier;
    damage(victim, victim, dam, SKILL_MPDAMAGE);
  }

  return;
}



/*
 * mpdrainmana is a way to suck mana from victims
 * you can mpdrainmana all <amount>
 * the amount can be specified as a single number or like a diceroll
 * dice (10d10+3) for example
 * the mana drained will never be less than 0
 */
void do_mpdrainmana(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim = NULL;
  int number = 0;
  int sides = 1;
  int modifier = 0;
  int manadrain = 0;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mpdrainmana - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (str_cmp(arg, "all")) {
    if ((victim = get_char_vis(ch, arg)) == NULL) {
      bug("Mpdrainmana - No such victim: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }
  }

  if (sscanf(argument, "%d d %d + %d", &number, &sides, &modifier) != 3) {
    if (sscanf(argument, "%d d %d - %d", &number, &sides, &modifier) != 3) {
      if (sscanf(argument, "%d d %d", &number, &sides) != 2) {
        if (sscanf(argument, "%d", &number) != 1) {
          bug("Mpdrainmana - Unguessable mana drain string: vnum %d.",
              mob_index[ch->nr].virtual);
          return;
        }
      }
    } else /* it was of the form 'number d sides - modifier' */ {
      modifier *= -1;
    }
  }

  if (!str_cmp(arg, "all")) {
    for (victim = world[ch->in_room].people;
         victim;
         victim = victim->next_in_room) {

      /*
       * The skips: 1) me
       *            2) uncharmed mobs that arent fighting me
       *            3) people i cant see
       * from there everyone else is a valid target
       */
      if (victim == ch)
        continue;
      if (IS_NPC(victim) &&
          !IS_AFFECTED(victim, AFF_CHARM) &&
          FIGHTING(victim) != ch)
        continue;
      if (!CAN_SEE(ch, victim))
        continue;

      manadrain = dice(number, sides) + modifier;
      GET_MANA(victim) -= manadrain;
      if (GET_MANA(victim) < 0)
        GET_MANA(victim) = 0;
    }
  } else {
    manadrain = dice(number, sides) + modifier;
    GET_MANA(victim) -= manadrain;
    if (GET_MANA(victim) < 0)
      GET_MANA(victim) = 0;
  }

  return;
}



/*
 * mpdrainmove is a way to suck movement from victims
 * you can mpdrainmove all <movement>
 * the amount can be specified as a single number or like a diceroll
 * dice (10d10+3) for example
 * the movement drained will never be less than 0
 */
void do_mpdrainmove(struct char_data *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim = NULL;
  int number = 0;
  int sides = 1;
  int modifier = 0;
  int movedrain = 0;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mpdrainmove - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (str_cmp(arg, "all")) {
    if ((victim = get_char_vis(ch, arg)) == NULL) {
      bug("Mpdrainmove - No such victim: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }
  }

  if (sscanf(argument, "%d d %d + %d", &number, &sides, &modifier) != 3) {
    if (sscanf(argument, "%d d %d - %d", &number, &sides, &modifier) != 3) {
      if (sscanf(argument, "%d d %d", &number, &sides) != 2) {
        if (sscanf(argument, "%d", &number) != 1) {
          bug("Mpdrainmove - Unguessable movement drain string: vnum %d.",
              mob_index[ch->nr].virtual);
          return;
        }
      }
    } else /* it was of the form 'number d sides - modifier' */ {
      modifier *= -1;
    }
  }

  if (!str_cmp(arg, "all")) {
    for (victim = world[ch->in_room].people;
         victim;
         victim = victim->next_in_room) {

      /*
       * The skips: 1) me
       *            2) uncharmed mobs that arent fighting me
       *            3) people i cant see
       * from there everyone else is a valid target
       */
      if (victim == ch)
        continue;
      if (IS_NPC(victim) &&
          !IS_AFFECTED(victim, AFF_CHARM) &&
          FIGHTING(victim) != ch)
        continue;
      if (!CAN_SEE(ch, victim))
        continue;

      movedrain = dice(number, sides) + modifier;
      GET_MOVE(victim) -= movedrain;
      if (GET_MOVE(victim) < 0)
        GET_MOVE(victim) = 0;
    }
  } else {
    movedrain = dice(number, sides) + modifier;
    GET_MOVE(victim) -= movedrain;
    if (GET_MOVE(victim) < 0)
      GET_MOVE(victim) = 0;
  }

  return;
}



void do_mpremember(struct char_data *ch, char *argument)
{
  struct char_data *victim;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (arg[0] == '\0') {
    bug("Mpremember - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((victim = get_char_vis(ch, arg)) == NULL) {
    bug("Mpremember - No such victim: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!IS_NPC(victim))
    remember(ch, victim);

  return;
}

void do_mpforget(struct char_data *ch, char *argument)
{
  struct char_data *victim;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (arg[0] == '\0') {
    bug("Mpforget - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((victim = get_char_vis(ch, arg)) == NULL) {
    bug("Mpforget - No such victim: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!IS_NPC(victim))
    forget(ch, victim);

  return;
}



/*
 * Very small function, set the global variable to say that the command
 * is considered 'handled' and that the player can't act on whatever command
 * it is that they were trying to do (yikes!)
 * This only has meaning when called from inside a command_prog
 * and what it does is keeps the player from actually *doing* the command
 * they were trying to do.
 */
void do_mpstopcommand(struct char_data *ch, char *argument)
{
  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  MOBHandled = 1;
}



/*
 * The only way to start a script prog is to call mptrigger
 * with what script you want to run, and who the 'actor' is
 * if you dont say, then the actor is assumed to be the
 * mob themselves
 */
void do_mptrigger(struct char_data *ch, char *argument)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *actor;		/* who is the target of the script */


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (arg[0] == '\0') {
    bug("Mptrigger - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((actor = get_char_vis(ch, arg)) == NULL) {
    bug("Mptrigger - No such actor: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  sprintf(buf, " %s", argument);      /* prepend a space to the script_name */

  stop_script(ch); 
  mprog_script_trigger(ch, actor, buf);    /* trigger the script */ 
}



/*
 * lets the mobile do a command quietly.  Very useful.
 */
void do_mpsilent(struct char_data *ch, char *argument)
{
  struct descriptor_data *d;


  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  skip_spaces(&argument);

  if (argument[0] == '\0') {
    bug("Mpsilent - Bad argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  /* set everyone who is playing deaf in the whole game */
  for (d = descriptor_list; d != NULL; d = d->next)
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DEAF;

  /* do the command */
  command_interpreter(ch, argument);

  /* set everyone who is deaf back to playing */
  for (d = descriptor_list; d != NULL; d = d->next)
    if (STATE(d) == CON_DEAF)
      STATE(d) = CON_PLAYING;

  return;
}



/*
 * A mob can only stop its own script.
 */
void do_mpstopscript(struct char_data *ch, char *argument)
{
  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  if (is_scripting(ch))
    stop_script(ch);
}



void do_mptrackto(struct char_data *ch, char *argument)
{
  int r_num;
  struct char_data *vict;
  int dir;


  if ((!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) &&
      (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  skip_spaces(&argument);

  if (!isalpha(argument[0])) {
    r_num = real_room(atoi(argument));
  } else {
    if ((vict = get_char_vis(ch, argument)) == NULL) {
      bug("Mptrackto - Victim does not exist: vnum %d.",
              mob_index[ch->nr].virtual);
      return;
    } else {
      r_num = vict->in_room;
    }
  }

  dir = find_first_step(ch->in_room, r_num);

  switch (dir) {
    case BFS_ERROR:
    case BFS_ALREADY_THERE:
    case BFS_NO_PATH:
        break;
    default:
        perform_move(ch, dir, 0);
        break;
  }
}



void do_mpsteerto(struct char_data *ch, char *argument)
{
  int r_num;
  struct char_data *vict;
  int dir;



  if ((!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) &&
      (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }

  skip_spaces(&argument);

  if (!isalpha(argument[0])) {
    r_num = real_room(atoi(argument));
  } else {
    if ((vict = get_char_vis(ch, argument)) == NULL) {
      bug("Mpsteerto - Victim does not exist: vnum %d.",
              mob_index[ch->nr].virtual);
      return;
    } else {
      r_num = vict->in_room;
    }
  }

  dir = find_first_step(ch->in_room, r_num);

  switch (dir) {
    case BFS_ERROR:
    case BFS_ALREADY_THERE:
    case BFS_NO_PATH:
        break;
    default:
        /* perform_move(ch, dir, 0); */
        sprintf(buf2, "steer %s", dir_abbrevs[dir]);
        command_interpreter(ch, buf2);
        break;
  }
}

void perform_mpbroadcast(struct char_data *ch, char *arg,
      struct char_data *actor, struct char_data *rndm, struct char_data *vict) {

  int vnum, rnum;
  struct char_data *targ;
  register struct char_data *mob;
  
  skip_spaces(&arg);
  
  if ((!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) &&
      (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }
  
  arg = any_one_arg(arg, buf);
  
  vnum = atoi(buf);
  if ((rnum = real_mobile(vnum)) == -1) {
    sprintf(buf, "Error in mobprog for #%d: invalid vnum %d to mpbroadcast",
            GET_MOB_VNUM(ch), vnum);
    arealog(buf, CMP, LVL_DEITY, TRUE, GET_MOB_VNUM(ch) % 100);
    return;
  }

  arg = any_one_arg(arg, buf);

  if (*buf != '$') {
    sprintf(buf, "Error in mobprog for #%d: mpbroadcast requires a $* target",
            GET_MOB_VNUM(ch));
    arealog(buf, CMP, LVL_DEITY, TRUE, GET_MOB_VNUM(ch) % 100);
    return;
  }
  switch(buf[1]) {
    case 'i': targ = ch; break;
    case 'n': targ = actor; break;
    case 't': targ = vict; break;
    case 'r': targ = rndm; break;
    default:
      sprintf(buf, "Error in mobprog for #%d: invalid target $%c to mpbroadcast",
              GET_MOB_VNUM(ch), buf[1]);
      arealog(buf, CMP, LVL_DEITY, TRUE, GET_MOB_VNUM(ch) % 100);
      return;
  }
  if (targ == NULL) {
    sprintf(buf, "Error in mobprog for #%d: nonexistant target $%c to mpbroadcast",
            GET_MOB_VNUM(ch), buf[1]);
    arealog(buf, CMP, LVL_DEITY, TRUE, GET_MOB_VNUM(ch) % 100);
    return;
  }
  
  arg = any_one_arg(arg, buf);
  
  if (!*buf) {
    sprintf(buf, "Error in mobprog for #%d: Not enough arguments to mpbroadcast",
            GET_MOB_VNUM(ch));
    arealog(buf, CMP, LVL_DEITY, TRUE, GET_MOB_VNUM(ch) % 100);
    return;
  }
  
  sprintf(buf2, " %s", buf);      /* prepend a space to the script_name */

  /* Ok, the target char is in targ, the name of the script is in buf2, and
     the mobs we're looking for are rnum */
  for (mob = character_list; mob; mob = mob->next) {
    if (IS_NPC(mob) && (mob->nr == rnum)) {
      stop_script(mob); 
      mprog_script_trigger(mob, targ, buf2);    /* trigger the script */ 
    }
  }
}

ACMD(do_mpdosocial) {
  char *a;
  int cmdn;
/*  extern struct social_info soc_info[]; */
  struct char_data *actor;
  struct social_messg *action;
  extern struct social_messg *soc_mess_list;

  int find_social(char *command);
  
  if ((!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || ch->desc) &&
      (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char("Huh?!?\n\r", ch);
    return;
  }
  
  argument = one_argument(argument, arg);
  a = arg;
  skip_spaces(&a);
  if (!*a) return;
  cmdn = find_social(a);
  
  argument = one_argument(argument, arg);
  a = arg;
  skip_spaces(&a);
  if (!*a) return;
  actor = get_char_room(a, ch->in_room);
  if (!actor) return;
  
  /* Ok, we need to do the social from actor to ch */
  action = &soc_mess_list[cmdn];
  act(action->char_found, 0, actor, 0, ch, TO_CHAR | TO_SLEEP);
  act(action->others_found, action->hide, actor, 0, ch, TO_NOTVICT);
  
  MOBHandled = 1;
}

#if (0)
ACMD(do_mppost) {
  extern int find_slot();

  extern int num_of_msgs[NUM_OF_BOARDS];
  extern char *msg_storage[INDEX_SIZE];
  extern struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];
  
  time_t ct;
  char *tmstr;
  int len;
  
  /* Normal mobs are allowed to do this, but not jarred or charmed */
  if ((IS_NPC(ch) && (ch->desc || ch->master)) || (!IS_NPC(ch))) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }
  
  if (num_of_msgs[1] >= MAX_BOARD_MESSAGES) {
    send_to_char("Sorry, the board's full!", ch);
    return;
  }
  
  if ((NEW_MSG_INDEX(1).slot_num = find_slot()) == -1) {
    send_to_char("The board is malfunctioning - sorry.\r\n", ch);
    log("SYSERR: Board: failed to find empty slot on write.");
    return;
  }

  ct = time(0);
  tmstr = (char *) asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  sprintf(buf2, "(%s)", GET_NAME(ch));
  sprintf(buf, "%6.10s %-12s :: %s", tmstr, buf2, argument);
  len = strlen(buf) + 1;
  if (!(NEW_MSG_INDEX(1).heading = (char *) malloc(sizeof(char) * len))) {
    send_to_char("The board is malfunctioning - sorry.\r\n", ch);
    return;
  }
  strcpy(NEW_MSG_INDEX(1).heading, buf);
  NEW_MSG_INDEX(1).heading[len - 1] = '\0';
  NEW_MSG_INDEX(1).level = GET_LEVEL(ch);
  NEW_MSG_INDEX(1).class = GET_CLASS(ch);
  NEW_MSG_INDEX(1).clan = GET_CLAN(ch);

  *buf = '\0';

  msg_storage[NEW_MSG_INDEX(1).slot_num] = strdup(buf);
  num_of_msgs[1]++;
  Board_save_board(1);
}
#endif

void set_var(char *varname, char *newval) {
  int abs, mod;
  struct mobprog_var_data *var, *oldvar;
  FILE *var_file;
  
  for (oldvar = NULL, var = mobprog_vars; var; var = var->next) {
    if (!str_cmp(var->name, varname)) break;
    oldvar = var;
  }
  if (!var) {
    CREATE(var, struct mobprog_var_data, 1);
    var->name = strdup(varname);
    var->val = 0;
    var->next = NULL;
    if (!oldvar) {
      mobprog_vars = var;
    } else {
      oldvar->next = var;
    }
  }
  
  abs = var->val;
  mod = 0;
  if (isdigit(*newval)) {
    abs = atoi(newval);
  } else {
    if (*newval == '+') {
      mod = 1;
    } else if (*newval == '-') {
      mod = -1;
    };
    mod *= atoi(newval+1);
  }
  var->val = abs + mod;
  var_file = fopen("misc/mobvars", "w");
  for (var = mobprog_vars; var; var = var->next) {
    fprintf(var_file, "%s %d\n", var->name, var->val);
  }
  fprintf(var_file, "~ 0\n");
  fclose(var_file);
}

ACMD(do_mpset) {
  if (((IS_NPC(ch) && (ch->desc || ch->master)) || (!IS_NPC(ch))) && (GET_LEVEL(ch) < 60)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  argument = any_one_arg(argument, arg);
  argument = any_one_arg(argument, buf2);
  
  if (!*arg || !*buf2) return;
  
  set_var(arg, buf2);
}

ACMD(do_mplog) {
  FILE *fl;
  time_t ct;
  char *tmstr;

  if (((IS_NPC(ch) && (ch->desc || ch->master)) || (!IS_NPC(ch))) && (GET_LEVEL(ch) < 60)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  fl = fopen("misc/moblog", "a");
  ct = time(0);
  tmstr = (char *) asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  skip_spaces(&argument);
  fprintf(fl, "%s : %-5d : %s\n", tmstr, GET_MOB_VNUM(ch), argument);
  fclose(fl);
}

ACMD(do_mpconceal) {
  struct obj_data *obj;
  struct room_data *room;

  argument = one_argument(argument, arg);
  
  if (!*arg)
    return;
  else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    return;
  } else {
    if (ch->in_room == NOWHERE) {
      return;
    }
    obj_from_char(obj);
    room = &world[ch->in_room];
    obj->next_content = room->hidden;
    room->hidden = obj;
  }
}
