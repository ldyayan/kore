
/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern int rev_dir[];
extern char *dirs[];
extern int movement_loss[];
extern struct spell_info_type spell_info[];
extern struct zone_data *zone_table;

/* external functs */
int special(struct char_data * ch, int cmd, char *arg);
void death_cry(struct char_data * ch);
void mprog_entry_trigger( struct char_data *mob );
void mprog_greet_trigger( struct char_data *ch );
void mprog_greet_every_trigger( struct char_data *ch );
int is_empty(int zone_nr);
void perform_unmount(struct char_data *ch);
void perform_remove(struct char_data * ch, int pos);
int number_range(int from, int to);

int do_web_check(struct char_data * ch)
{
  if (GET_STR(ch) >= (number_range(3, 80))) {
    send_to_char("You manage to break the webs surrounding you.\r\n", ch);
    affect_from_char(ch, SPELL_WEB);
    return 1;
  } else {
    send_to_char("You cannot move under all this sticky webbing.\r\n", ch);
    return 0;
  }
}

/* do_simple_move assumes
 *	1. That there is no master and no followers.
 *	2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 *
 *   Note: Pet followers are handled here, not in perform_move!
 */
int do_simple_move(struct char_data * ch, int dir, int need_specials_check)
{
  int was_in, need_movement, has_boat = 0, k, num_wears;
  struct obj_data *obj;
  struct obj_data *i;
  struct obj_data *x;
  bool mounted = FALSE, ismount = FALSE;

  int special(struct char_data * ch, int cmd, char *arg);
  int check_for_access( struct char_data *ch, int rnum);
  int weight_carried, j;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, ""))
    return 0;

  if (HAS_PET(ch)) if (IS_MOUNTED(GET_PET(ch))) mounted = TRUE;
  if (IS_PET(ch)) if (IS_MOUNTED(ch)) ismount = TRUE;

  /* charmed? */
  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master && ch->in_room == ch->master->in_room) {
    send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }
  if (IS_AFFECTED2(ch, AFF2_WEBBED)) {
    if(!do_web_check(ch)) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return 0;
    }
  }

/* GODROOMS */

  if (!ZONE_FLAGGED(ch->in_room, ZONE_ACTIVE) && GET_LEVEL(ch) < LVL_IMMORT)
  {
      send_to_char("You should not be here, please ask an immortal to retrieve you.\r\n", ch);
      return 0;
  }

  /* if this room or the one we're going to needs a boat, check for one 
     but let people who fly, fly right through */
  if ((world[ch->in_room].sector_type == SECT_WATER_NOSWIM) ||
      (world[EXIT(ch, dir)->to_room].sector_type == SECT_WATER_NOSWIM)) {
    for (obj = ch->carrying; obj; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_BOAT)
	has_boat = TRUE;
    if (!(has_boat || IS_AFFECTED(ch, AFF_FLY) ||
        IS_AFFECTED(ch, AFF_WATERWALK) ||
        (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SWIMMER)))) {
      send_to_char("You need a boat to go there.\r\n", ch);
      return 0;
    }
  }

  /* if this room or the one we're going to needs a ship, check for one
     but let mobs who can swim and people who fly go through */
  if (world[EXIT(ch, dir)->to_room].sector_type == SECT_OCEAN) {
    if (!(IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SWIMMER))) {
      send_to_char("You need a sailing ship to go there.\r\n", ch);
      return 0;
    }
  }

  /* if the mob cant walk on land and its not affected fly, then
    also dont let them go */
  if ((world[EXIT(ch, dir)->to_room].sector_type != SECT_WATER_SWIM) &&
      (world[EXIT(ch, dir)->to_room].sector_type != SECT_WATER_NOSWIM) &&
      (world[EXIT(ch, dir)->to_room].sector_type != SECT_OCEAN)) {
    if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOWALK)) &&
        !IS_AFFECTED(ch, AFF_FLY)) {
      send_to_char("You can't go there, you'd be left high and dry!\r\n", ch);
      return 0;
    }
  }
  
  /* Check if the room's a house */
    if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_HOUSE)) {
      if (!check_for_access(ch, EXIT(ch, dir)->to_room)) {
        send_to_char("Hey! That's private property! Keep out!\r\n", ch);
        return 0;
      }
    }

  if (IS_AFFECTED(ch, AFF_FLY)) {
    need_movement = (movement_loss[world[ch->in_room].sector_type] +
	  movement_loss[world[world[ch->in_room].dir_option[dir]->to_room].sector_type]) / 4;
  } else {
    need_movement = (movement_loss[world[ch->in_room].sector_type] +
	  movement_loss[world[world[ch->in_room].dir_option[dir]->to_room].sector_type]) >> 1;
  }

  if (need_movement < 1) 
    need_movement = 1;

  if (HAS_PET(ch)) if (IS_MOUNTED(GET_PET(ch))) mounted = TRUE;
  if (IS_PET(ch)) if (IS_MOUNTED(ch)) ismount = TRUE;
  
  if (mounted) {
    if (GET_MOVE(GET_PET(ch)) < need_movement) {
      send_to_char("Your mount is too exhausted to carry you.\r\n", ch);
      return 0;
    }
  } else {
    if ((GET_MOVE(ch) < need_movement && !IS_NPC(ch)) || (GET_MOVE(ch) == 0)) {
	if (need_specials_check && ch->master)
	    send_to_char("You are too exhausted to follow.\r\n", ch);
	else
	    send_to_char("You are too exhausted.\r\n", ch);
	return 0;
    }
  }

/*  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_ATRIUM))
    if (!House_can_enter(ch, world[EXIT(ch, dir)->to_room].number)) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return 0;
    }
  Let's replace this with our own vastly superior code <g> */

/* Check if the room's a house */
  if (!IS_NPC(ch)) {
    if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_HOUSE)) {
      if (!check_for_access(ch, EXIT(ch, dir)->to_room)) {
        send_to_char("Hey! That's private property! Keep out!\r\n", ch);
        return 0;
      }
    }
  }
  
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM)
    && GET_LEVEL(ch) < 58) {
        send_to_char("You are not godly enough to use that room!\r\n", ch);
        return 0;
  }
  
  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch)) {
    if (mounted) {
      GET_MOVE(GET_PET(ch)) -= need_movement;
    } else {
    GET_MOVE(ch) -= need_movement;
    }
  }

  if (!IS_AFFECTED(ch, AFF_SNEAK) && !ismount) {
    if (mounted) {
	sprintf(buf2, "$n rides $N %s.", dirs[dir]);
	act(buf2, TRUE, ch, 0, GET_PET(ch), TO_ROOM);
    } else {
	sprintf(buf2, "$n leaves %s.", dirs[dir]);
	act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  /* moving stops the mover and their target from fighting */
  if (FIGHTING(ch) && (ch->in_room != FIGHTING(ch)->in_room)) {
    if (FIGHTING(FIGHTING(ch)) == ch)
      stop_fighting(FIGHTING(ch));
    stop_fighting(ch);
  }

  if ((!IS_AFFECTED(ch, AFF_SNEAK) || mounted) && !ismount) {
    if (mounted) {
      act("$n arrives, riding $N.", TRUE, ch, 0, GET_PET(ch), TO_ROOM);
    } else {
    act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  look_at_room(ch, 0);

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH) && 
      GET_LEVEL(ch) < LVL_IMMORT) {

	for (i = ch->carrying; i; i = i->next_content)
	    if (GET_OBJ_VNUM(i) == 21999) {
		extract_obj(i);
		break;
	    }

	num_wears = IS_THRIKREEN(ch)? NUM_THRI_WEARS : NUM_WEARS;

	for (k = 0; k < num_wears; k++) {
	    if (ch->equipment[k]) {
		x = ch->equipment[k];
		if (GET_OBJ_VNUM(x) == 21999) {
		    i = ch->equipment[k];
		    perform_remove(ch, k);
		    extract_obj(i);
		    break;
		}
	    }
	}

	if (!i) {
	    GET_GOLD(ch) = 0;
	    log_death_trap(ch);
	    death_cry(ch);
	    extract_char(ch);
	} else {
	    send_to_char("The amulet of Yari flickers violently, glows a bright white, and is gone!\r\n", ch);
	    char_from_room(ch);
	    char_to_room(ch, was_in);
	}
	return 0;
  }

/* PETS */
/* mobprogs should trigger on owners, not pets, and pets should walk
   into the room beside their owner. Note that with this code, pets
   won't DT */

  if (!IS_NPC(ch) && ch->desc)
    if (HAS_PET(ch))
      if (!ch->desc->original)
        if (was_in == GET_PET(ch)->in_room)
          /* Is a character, has a pet here and a descriptor, isn't switched */
          do_simple_move(GET_PET(ch), dir, need_specials_check);
  
/* HACKED for breakable floors */
/* For this to work, the "down" exit from the room must go somewhere,
   be set breakable, and have the key set as the maxweight. Note that pets
   will follow you INTO the room, but they won't jump down after you if
   you fall through.
   
   The way this is written, greet_progs are checked on the room you fall
   into, not the room you WALKED into, and if the room below is a DT, it
   doesn't check. If you NEED the target room to be a DT for some cruel
   reason, do an all_greet_prog with 100% fire chance */
  
  if (!IS_NPC(ch)) {
    if (EXIT(ch, DOWN)) {
      if (EXIT(ch, DOWN)->to_room != NOWHERE) {
        if (IS_SET(EXIT(ch, DOWN)->exit_info, EX_BREAKABLE)) {
          if (EXIT(ch, DOWN)->key > 0) {
            if (!AFF_FLAGGED(ch, AFF_FLY)) {
              /* WHEW...there IS an exit down that IS breakable and we
                 AREN'T flying */
              weight_carried = IS_CARRYING_W(ch);
              for (j = 0; j < NUM_WEARS; j++) {
                if (ch->equipment[j]) {
                  weight_carried += GET_OBJ_WEIGHT(ch->equipment[j]);
                }
              }
              if (weight_carried > EXIT(ch, DOWN)->key) {
                /* and....we're falling! */
                send_to_char("\r\nThe ground collapses under your feet and "
                             "you fall through!\r\n\r\n", ch);
                act("The ground collapses under $n's feet and $e falls through!",
                     TRUE, ch, 0, 0, TO_ROOM);
                was_in = ch->in_room;
                char_from_room(ch);
                char_to_room(ch, world[was_in].dir_option[DOWN]->to_room);
                act("$n comes crashing in through the ceiling!", TRUE, ch, 0,
                    0, TO_ROOM);
                look_at_room(ch, 0);
                GET_POS(ch) = POS_SITTING;
                if (HAS_PET(ch)) IS_MOUNTED(GET_PET(ch)) = FALSE;
              }
            }
          }
        }
      }
    }
  }

/* END of hack */
  
  if (!is_empty(world[ch->in_room].zone) && !IS_PET(ch)) {
    mprog_entry_trigger(ch);
    mprog_greet_trigger(ch);
    mprog_greet_every_trigger(ch);
  }

  return 1;
}



int perform_move(struct char_data * ch, int dir, int need_specials_check)
{
  int was_in;
  struct follow_type *k, *next;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS)
    return 0;
  else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE) {
    send_to_char("Alas, you cannot go that way...\r\n", ch);
  } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
    if (EXIT(ch, dir)->keyword) {
      sprintf(buf2, "The %s seems to be closed.\r\n",
          fname(EXIT(ch, dir)->keyword));
      send_to_char(buf2, ch);
    } else
      send_to_char("It seems to be closed.\r\n", ch);
  } else {
    if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_SOLITARY)) {
      if (get_first_char(EXIT(ch, dir)->to_room)) {
        send_to_char("You can't get in there, it's too full!\r\n", ch);
        return 0;
      }
    }
    if (!ch->followers)
      return (do_simple_move(ch, dir, need_specials_check));

    was_in = ch->in_room;
    if (!do_simple_move(ch, dir, need_specials_check)) {
      if (IS_AFFECTED(ch, AFF_GAUGE)) {
         REMOVE_BIT(AFF_FLAGS(ch), AFF_GAUGE);
      }
      return 0;
    }
    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((was_in == k->follower->in_room) &&
	  (GET_POS(k->follower) >= POS_STANDING)) {
	act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
    }
    return 1;
  }
  return 0;
}



ACMD(do_move)
{
  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  perform_move(ch, subcmd - 1, 0);
}



int find_door(struct char_data * ch, char *type, char *dir)
{
  int door;

  if (*dir) {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1) {	/* Partial Match */
      send_to_char("That's not a direction.\r\n", ch);
      return -1;
    }
    if (EXIT(ch, door))
      if (EXIT(ch, door)->keyword)
	if (isname(type, EXIT(ch, door)->keyword))
	  return door;
	else {
	  sprintf(buf2, "I see no %s there.\r\n", type);
	  send_to_char(buf2, ch);
	  return -1;
	}
      else
	return door;
    else {
      send_to_char("I really don't see how you can close anything there.\r\n", ch);
      return -1;
    }
  } else {			/* try to locate the keyword */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (isname(type, EXIT(ch, door)->keyword))
	    return door;

    sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    send_to_char(buf2, ch);
    return -1;
  }
}



int has_key(struct char_data * ch, int key)
{
  struct obj_data *o;
  int wear_hold = IS_THRIKREEN(ch)? THRI_WEAR_HOLD_R : WEAR_HOLD;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return 1;

  if (ch->equipment[wear_hold])
    if (GET_OBJ_VNUM(ch->equipment[wear_hold]) == key)
      return 1;

  return 0;
}



/*
 * finds the key, uses it, and if its burnt out, junks it
 * if the key has charges, it subtracts 1 from the total, if the
 * total fell to 0 from a bigger number, the key is junked.
 * if the key started at 0, it must be ok (a normal key)
 */
int use_key(struct char_data * ch, int key)
{
  struct obj_data *o;
  int found = 0;
  int held = 0;
  int wear_hold = IS_THRIKREEN(ch)? THRI_WEAR_HOLD_R : WEAR_HOLD;

  for (o = ch->carrying; o; o = o->next_content) 
    if (GET_OBJ_VNUM(o) == key) {
      found = 1;
      break;
    }

  if (!found) {
    if (ch->equipment[wear_hold]) {
      if (GET_OBJ_VNUM(ch->equipment[wear_hold]) == key) {
        found = 1;
        held = 1;
        o = ch->equipment[wear_hold];
      }
    }
  }

  if (!found)
    return 0;

  if (GET_OBJ_TYPE(o) != ITEM_KEY)
    return 0;

  if (GET_OBJ_VAL(o, 1) != 0) {
    GET_OBJ_VAL(o, 1)--;
    if (GET_OBJ_VAL(o, 1) <= 0) {
      act("As the usefulness of $p vanishes, it crumbles into dust!",
          FALSE, ch, o, 0, TO_CHAR);
      act("As $n uses $p it crumbles into dust!",
          FALSE, ch, o, 0, TO_ROOM);
      if (held) {
        if (ch->equipment[wear_hold])
          o = unequip_char(ch, wear_hold);
        else {
          log("SYSERR: unequip_char, act.movement.c use_key()");
          exit(1);
        }
        obj_to_char(o, ch);
      }
      obj_from_char(o);
      extract_obj(o);
      return 1;
    }
  }

  return 1;
}



#define NEED_OPEN	1
#define NEED_CLOSED	2
#define NEED_UNLOCKED	4
#define NEED_LOCKED	8

char *cmd_door[] = {
        "open",
        "close",
        "unlock",
        "lock",
        "pick"
} ;

const int flags_door[] = {
	NEED_CLOSED | NEED_UNLOCKED,
	NEED_OPEN,
	NEED_CLOSED | NEED_LOCKED,
	NEED_CLOSED | NEED_UNLOCKED,
	NEED_CLOSED | NEED_LOCKED
} ;


#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  int other_room;
  struct room_direction_data *back = 0;

  sprintf(buf, "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]))
      if (back->to_room != ch->in_room)
	back = 0;

  switch (scmd) {
    case SCMD_OPEN:
    case SCMD_CLOSE:
      OPEN_DOOR(ch->in_room, obj, door);
      if (back)
	OPEN_DOOR(other_room, obj, rev_dir[door]);
      send_to_char(OK, ch);
      break;
    case SCMD_UNLOCK:
    case SCMD_LOCK:
      LOCK_DOOR(ch->in_room, obj, door);
      if (back)
	LOCK_DOOR(other_room, obj, rev_dir[door]);
      send_to_char("*Click*\r\n", ch);
      break;
    case SCMD_PICK:
      LOCK_DOOR(ch->in_room, obj, door);
      if (back)
	LOCK_DOOR(other_room, obj, rev_dir[door]);
      send_to_char("The lock quickly yields to your skills.\r\n", ch);
      strcpy(buf, "$n skillfully picks the lock on ");
      break;
  }
  /* Notify the room */
  sprintf(buf + strlen(buf), "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" :
   (EXIT(ch, door)->keyword ? "$F" : "door"));
  if (!(obj) || (obj->in_room != NOWHERE))
    act(buf, FALSE, ch, obj, EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if (((scmd == SCMD_OPEN) || (scmd == SCMD_CLOSE)) && (back)) {
    sprintf(buf, "The %s is %s%s from the other side.\r\n", 
     (back->keyword ? fname(back->keyword) : "door"), cmd_door[scmd],
     (scmd == SCMD_CLOSE) ? "d" : "ed");
    send_to_room(buf, EXIT(ch, door)->to_room);
  }
}


int ok_pick(struct char_data *ch, int keynum, int pickproof, int scmd)
{
  int percent;

  percent = number(1, 101);

  if (scmd == SCMD_PICK) {
    if (keynum < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if (pickproof)
      send_to_char("It resists your attempts at picking it.\r\n", ch);
    else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
      send_to_char("You failed to pick the lock.\r\n", ch);
    else
      return(1);
    return(0);
  }
  return(1);
}


#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			(IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))) :\
			(IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
			(!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
			(!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF)) : \
			(IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, 2)) : \
					(EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 1)) : \
					(EXIT(ch, door)->exit_info))
ACMD(do_open)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;

  two_arguments(argument, type, dir);

  if (!*type)
   send_to_char("Open what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
      ch, &victim, &obj))
    /* this is an object */

    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("But it's already open!\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))
      send_to_char("You can't do that.\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("It seems to be locked.\r\n", ch);
    else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
      send_to_char(OK, ch);
      act("$n opens $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* perhaps it is a door */

    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's impossible, I'm afraid.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("It's already open!\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It seems to be locked.\r\n", ch);
    else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if (EXIT(ch, door)->keyword)
        act("$n opens the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      else
        act("$n opens the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char(OK, ch);
      /* now for opening the OTHER side of the door! */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if ((back = world[other_room].dir_option[rev_dir[door]]))
          if (back->to_room == ch->in_room) {
            REMOVE_BIT(back->exit_info, EX_CLOSED);
            if (back->keyword) {
              sprintf(buf, "The %s is opened from the other side.\r\n",
                      fname(back->keyword));
              send_to_room(buf, EXIT(ch, door)->to_room);
            } else
              send_to_room("The door is opened from the other side.\r\n",
                           EXIT(ch, door)->to_room);
          }
    }
}



ACMD(do_close)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;


  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Close what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, 
    &victim, &obj))
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
    send_to_char("But it's already closed!\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))
      send_to_char("That's impossible.\r\n", ch);
    else {
      SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
      send_to_char(OK, ch);
      act("$n closes $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* Or a door */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("It's already closed!\r\n", ch);
    else {
      SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if (EXIT(ch, door)->keyword)
        act("$n closes the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
            TO_ROOM);
      else
        act("$n closes the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char(OK, ch);
      /* now for closing the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if ((back = world[other_room].dir_option[rev_dir[door]]))
          if (back->to_room == ch->in_room) {
            SET_BIT(back->exit_info, EX_CLOSED);
            if (back->keyword) {
              sprintf(buf, "The %s closes quietly.\r\n", back->keyword);
              send_to_room(buf, EXIT(ch, door)->to_room);
            } else
              send_to_room("The door closes quietly.\r\n", EXIT(ch, 
                door)->to_room);
          }
    }
}



ACMD(do_lock)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;


  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Lock what?\r\n", ch);
 else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
                        ch, &victim, &obj))
    /* this is an object */

    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("Maybe you should close it first...\r\n", ch);
    else if (GET_OBJ_VAL(obj, 2) < 0)
      send_to_char("That thing can't be locked.\r\n", ch);
    else if ((!has_key(ch, GET_OBJ_VAL(obj, 2))) && GET_LEVEL(ch) < LVL_GOD)
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("It is locked already.\r\n", ch);
    else {
      SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*Click*\r\n", ch);
      act("$n locks $p.", FALSE, ch, obj, 0, TO_ROOM);
      use_key(ch, GET_OBJ_VAL(obj, 2));
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* a door, perhaps */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You have to close it first, I'm afraid.\r\n", ch);
    else if (EXIT(ch, door)->key < 0)
      send_to_char("There does not seem to be a keyhole.\r\n", ch);
    else if (!has_key(ch, EXIT(ch, door)->key) && GET_LEVEL(ch) < LVL_GOD)
      send_to_char("You don't have the proper key.\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It's already locked!\r\n", ch);
    else {
      SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
        act("$n locks the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
            TO_ROOM);
      else
        act("$n locks the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("*Click*\r\n", ch);
      use_key(ch, EXIT(ch, door)->key);
      /* now for locking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if ((back = world[other_room].dir_option[rev_dir[door]]))
          if (back->to_room == ch->in_room)
            SET_BIT(back->exit_info, EX_LOCKED);
    }
}



ACMD(do_unlock)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;


  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Unlock what?\r\n", ch);
else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
                        ch, &victim, &obj))
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("Silly - it ain't even closed!\r\n", ch);
    else if (GET_OBJ_VAL(obj, 2) < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if ((!has_key(ch, GET_OBJ_VAL(obj, 2))) && GET_LEVEL(ch) < LVL_GOD)
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("Oh.. it wasn't locked, after all.\r\n", ch);
    else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*Click*\r\n", ch);
      act("$n unlocks $p.", FALSE, ch, obj, 0, TO_ROOM);
      use_key(ch, GET_OBJ_VAL(obj, 2));
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* it is a door */

    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("Heck.. it ain't even closed!\r\n", ch);
    else if (EXIT(ch, door)->key < 0)
      send_to_char("You can't seem to spot any keyholes.\r\n", ch);
    else if (!has_key(ch, EXIT(ch, door)->key) && GET_LEVEL(ch) < LVL_GOD)
      send_to_char("You do not have the proper key for that.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It's already unlocked, it seems.\r\n", ch);
    else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
        act("$n unlocks the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
            TO_ROOM);
      else
        act("$n unlocks the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("*Click*\r\n", ch);
      use_key(ch, EXIT(ch, door)->key);
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if ((back = world[other_room].dir_option[rev_dir[door]]))
          if (back->to_room == ch->in_room)
            REMOVE_BIT(back->exit_info, EX_LOCKED);
    }
}



ACMD(do_pick)
{
  byte percent;
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *v;

  two_arguments(argument, type, dir);

  percent = number(1, 101);     /* 101% is a complete failure */

  if (!*type)
    send_to_char("Pick what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &v, 
     &obj)) {
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("Silly - it isn't even closed!\r\n", ch);
    else if (GET_OBJ_VAL(obj, 2) < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("Oho! This thing is NOT locked!\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF))
      send_to_char("It resists your attempts at picking it.\r\n", ch);
    else if (number(1, 101) < 40 || !(IS_THIEF(ch)))
      send_to_char("You failed to pick the lock.\r\n", ch); 
    else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*Click*\r\n", ch);
      act("$n fiddles with $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
    } else if ((door = find_door(ch, type, dir)) >= 0)
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You realize that the door is already open.\r\n", ch);
    else if (EXIT(ch, door)->key < 0)
      send_to_char("You can't seem to spot any lock to pick.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("Oh.. it wasn't locked at all.\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF))
      send_to_char("You seem to be unable to pick this lock.\r\n", ch);
    else if (number(1, 101) < 40 || !(IS_THIEF(ch)))
      send_to_char("You failed to pick the lock.\r\n", ch); 
    else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
        act("$n skillfully picks the lock of the $F.", 0, ch, 0,
          EXIT(ch, door)->keyword, TO_ROOM);
      else
        act("$n picks the lock of the door.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("The lock quickly yields to your skills.\r\n", ch);
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if ((back = world[other_room].dir_option[rev_dir[door]]))
          if (back->to_room == ch->in_room)
            REMOVE_BIT(back->exit_info, EX_LOCKED);
    }
}



ACMD(do_gen_door)
{
  int door, keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *victim;

  if (*arg) {
    two_arguments(argument, type, dir);
    if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
      door = find_door(ch, type, dir);

    if ((obj) || (door >= 0)) {
      keynum = DOOR_KEY(ch, obj, door);
      if (!(DOOR_IS_OPENABLE(ch, obj, door)))
	act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
      else if (!DOOR_IS_OPEN(ch, obj, door) &&
       IS_SET(flags_door[subcmd], NEED_OPEN))
	send_to_char("But it's already closed!\r\n", ch);
      else if (!DOOR_IS_CLOSED(ch, obj, door) && 
       IS_SET(flags_door[subcmd], NEED_CLOSED))
	send_to_char("But it's currently open!\r\n", ch);
      else if (!(DOOR_IS_LOCKED(ch, obj, door)) && 
       IS_SET(flags_door[subcmd], NEED_LOCKED))
	send_to_char("Oh.. it wasn't locked, after all..\r\n", ch);
      else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && 
       IS_SET(flags_door[subcmd], NEED_UNLOCKED))
	send_to_char("It seems to be locked.\r\n", ch);
      else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
       ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
	send_to_char("You don't seem to have the proper key.\r\n", ch);
      else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
	do_doorcmd(ch, obj, door, subcmd); 
    }
    return;
  } 

  sprintf(buf, "%s what?\r\n", cmd_door[subcmd]);
  send_to_char(CAP(buf), ch);
}



/* modified to work with portals */
ACMD(do_enter)
{
  bool obj_use_prog(struct obj_data *obj, struct char_data *ch);

  struct obj_data *obj = NULL;
  int door;
  
  room_num rnum;


  one_argument(argument, buf);

  if (*buf) {

    /* check for portals */
    if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      if (CAN_SEE_OBJ(ch, obj)) {
        if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&
            ((GET_OBJ_VAL(obj, 1) == PORTAL_ENTER) ||
             (GET_OBJ_VAL(obj, 1) == PORTAL_BOARD))) {
          rnum = real_room(GET_OBJ_VAL(obj, 0));
          if (rnum != NOWHERE) {
            if (ROOM_FLAGGED(rnum, ROOM_SOLITARY)) {
              if (get_first_char(rnum)) {
                send_to_char("The passage is blocked!\r\n", ch);
                return;
              }
            }
            if (GET_OBJ_VAL(obj, 1) == PORTAL_ENTER) {
              if (obj_use_prog(obj, ch)) return;
              act("You enter $p.", FALSE, ch, obj, 0, TO_CHAR);
              act("$n enters $p.", TRUE, ch, obj, 0, TO_ROOM);
              char_from_room(ch);
              char_to_room(ch, rnum);
              act("$n comes through $p.", TRUE, ch, obj, 0, TO_ROOM);
            } else {
              act("You board $p.", FALSE, ch, obj, 0, TO_CHAR);
              act("$n boards $p.", TRUE, ch, obj, 0, TO_ROOM);
              char_from_room(ch);
              char_to_room(ch, rnum);
              act("$n comes aboard.", TRUE, ch, obj, 0, TO_ROOM);
            }
          }
          look_at_room(ch, 1);
          return;
        }
      }
    }

    /* an argument was supplied, search for door keyword */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (!str_cmp(EXIT(ch, door)->keyword, buf)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
    send_to_char("You are already indoors.\r\n", ch);
  else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
	      IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("You can't seem to find anything to enter.\r\n", ch);
  }
}



ACMD(do_push)
{
  struct obj_data *obj = NULL;
  room_num rnum;

  one_argument(argument, buf);

  if (*buf) {

    /* check for portals */
    if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      if (CAN_SEE_OBJ(ch, obj)) {
        if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) && 
            (GET_OBJ_VAL(obj, 1) == PORTAL_PUSH)) {
          rnum = real_room(GET_OBJ_VAL(obj, 0));
          if (rnum != NOWHERE) {
            if (ROOM_FLAGGED(rnum, ROOM_SOLITARY)) {
              if (get_first_char(rnum)) {
                send_to_char("Your travel is blocked!\r\n", ch);
                return;
              }
            }
            act("You push $p.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n pushes $p.", TRUE, ch, obj, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, rnum);
            act("$n has arrived.", TRUE, ch, obj, 0, TO_ROOM);
          }
          look_at_room(ch, 1);
          return;
        }
      }
    }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else {
    send_to_char("Push what?\n\r", ch);
    return;
  }
}



ACMD(do_pull)
{
  struct obj_data *obj = NULL;
  room_num rnum;

  one_argument(argument, buf);

  if (*buf) {

    /* check for portals */
    if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      if (CAN_SEE_OBJ(ch, obj)) {
        if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&
            (GET_OBJ_VAL(obj, 1) == PORTAL_PULL)) {
          rnum = real_room(GET_OBJ_VAL(obj, 0));
          if (rnum != NOWHERE) {
            if (ROOM_FLAGGED(rnum, ROOM_SOLITARY)) {
              if (get_first_char(rnum)) {
                send_to_char("Your travel is blocked!\r\n", ch);
                return;
              }
            }
            act("You pull $p.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n pulls $p.", TRUE, ch, obj, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, rnum);
            act("$n has arrived.", TRUE, ch, obj, 0, TO_ROOM);
          }
          look_at_room(ch, 1);
          return;
        }
      }
    }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else {
    send_to_char("Pull what?\r\n", ch);
    return;
  }
}



/* do_enter can also take a 'board' argument */
ACMD(do_board)
{
  struct obj_data *obj = NULL;
  room_num rnum;

  if(!argument) {
    send_to_char("Board what?\r\n", ch);
    return;
  }

  one_argument(argument, buf);

  if (*buf) {

    /* check for portals */
    if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      if (CAN_SEE_OBJ(ch, obj)) {
        if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&
            (GET_OBJ_VAL(obj, 1) == PORTAL_BOARD)) {
          rnum = real_room(GET_OBJ_VAL(obj, 0));
          if (rnum != NOWHERE) {
            if (ROOM_FLAGGED(rnum, ROOM_SOLITARY)) {
              if (get_first_char(rnum)) {
                send_to_char("The gangplank is blocked.\r\n", ch);
                return;
              }
            }
            act("You board $p.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n boards $p.", TRUE, ch, obj, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, rnum);
            act("$n comes aboard.", TRUE, ch, obj, 0, TO_ROOM);
          }
          look_at_room(ch, 1);
          return;
        }
      }
    }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else {
    for (obj = world[ch->in_room].contents; obj; obj = obj->next) {
      if (CAN_SEE_OBJ(ch, obj)) {
        if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&
            (GET_OBJ_VAL(obj, 1) == PORTAL_BOARD)) {
          rnum = real_room(GET_OBJ_VAL(obj, 0));
          if (rnum != NOWHERE && obj->in_room == ch->in_room) {
            if (ROOM_FLAGGED(rnum, ROOM_SOLITARY)) {
              if (get_first_char(rnum)) {
                send_to_char("The gangplank is blocked.\r\n", ch);
                return;
              }
            }
            act("You board $p.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n boards $p.", TRUE, ch, obj, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, rnum);
            act("$n comes aboard.", TRUE, ch, obj, 0, TO_ROOM);
            look_at_room(ch, 1);
          } else {
            send_to_char("You can't seem to find anything to board.\r\n", ch);
          }
          return;
        }
      }
    }
    send_to_char("You can't seem to find anything to board.\r\n", ch);
  }
}



ACMD(do_climb)
{
  struct obj_data *obj = NULL;
  room_num rnum;

  one_argument(argument, buf);

  if (*buf) {

    /* check for portals */
    if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      if (CAN_SEE_OBJ(ch, obj)) {
        if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&
            (GET_OBJ_VAL(obj, 1) == PORTAL_CLIMB)) {
          rnum = real_room(GET_OBJ_VAL(obj, 0));
          if (rnum != NOWHERE) {
            if (ROOM_FLAGGED(rnum, ROOM_SOLITARY)) {
              if (get_first_char(rnum)) {
                send_to_char("There is no room to climb!\r\n", ch);
                return;
              }
            }
            act("You climb $p.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n climbs $p.", TRUE, ch, obj, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, rnum);
            act("$n has arrived.", TRUE, ch, obj, 0, TO_ROOM);
          }
          look_at_room(ch, 1);
          return;
        }
      }
    }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else {
    send_to_char("Climb what?\r\n", ch);
    return;
  }
}



ACMD(do_leave)
{
  int door;

  if (!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
    send_to_char("You are outside.. where do you want to go?\r\n", ch);
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
	      !IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("I see no obvious exits to the outside.\r\n", ch);
  }
}



ACMD(do_stand)
{
  if (HAS_PET(ch)) {
    if (IS_MOUNTED(GET_PET(ch))) {
      perform_unmount(ch);
      return;
    }
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SITTING:
    act("You stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    if (FIGHTING(ch)) {
	GET_POS(ch) = POS_FIGHTING;
    } else {
	GET_POS(ch) = POS_STANDING;
    }
    break;
  case POS_RESTING:
    act("You stop resting, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    if (FIGHTING(ch)) {
	GET_POS(ch) = POS_FIGHTING;
    } else {
	GET_POS(ch) = POS_STANDING;
    }
    break;
  case POS_SLEEPING:
    act("You have to wake up first!", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Do you not consider fighting as standing?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and put your feet on the ground.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}



ACMD(do_sit)
{
  if (HAS_PET(ch)) {
    if (IS_MOUNTED(GET_PET(ch))) {
      send_to_char("You can't sit down, you're mounted!\r\n", ch);
      return;
    }
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You sit down.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char("You're sitting already.\r\n", ch);
    break;
  case POS_RESTING:
    act("You stop resting, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Sit down while fighting? are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and sit down.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}



ACMD(do_rest)
{
  if (HAS_PET(ch)) {
    if (IS_MOUNTED(GET_PET(ch))) {
      send_to_char("You can't rest while mounted!\r\n", ch);
      return;
    }
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You sit down and rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    act("You rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    act("You are already resting.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Rest while fighting?  Are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and stop to rest your tired bones.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}



ACMD(do_sleep)
{
  if (IS_THRIKREEN(ch)) {
    send_to_char("You have no need of slumber!\r\n", ch);
    return;
  }
  if (HAS_PET(ch)) {
    if (IS_MOUNTED(GET_PET(ch))) {
      send_to_char("You can't sleep while mounted!\r\n", ch);
      return;
    }
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
    send_to_char("You go to sleep.\r\n", ch);
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char("You are already sound asleep.\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Sleep while fighting?  Are you MAD?\r\n", ch);
    break;
  default:
    act("You stop floating around, and lie down to sleep.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and lie down to sleep.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
}



ACMD(do_wake)
{
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char("You can't wake people up if you're asleep yourself!\r\n", ch);
    else if ((vict = get_char_room_vis(ch, arg)) == NULL)
      send_to_char(NOPERSON, ch);
    else if (vict == ch)
      self = 1;
    else if (GET_POS(vict) > POS_SLEEPING)
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (IS_AFFECTED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) != POS_SLEEPING)
      act("You can't wake $M up, $E is not sleeping", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }
  if (IS_AFFECTED(ch, AFF_SLEEP))
    send_to_char("You can't wake up!\r\n", ch);
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char("You are already awake...\r\n", ch);
  else {
    send_to_char("You awaken, and sit up.\r\n", ch);
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
  }
}



ACMD(do_follow)
{
  struct char_data *leader;

  void stop_follower(struct char_data * ch);
  void add_follower(struct char_data * ch, struct char_data * leader);

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    }
  } else {
    send_to_char("Whom do you wish to follow?\r\n", ch);
    return;
  }

/*
  if ((GET_LEVEL(ch) - GET_LEVEL(leader) > 11) ||
	(GET_LEVEL(leader) - GET_LEVEL(ch) > 11)) {
	send_to_char("You are not of the right caliber.\n\r", ch);
	return;
  }
*/

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
	send_to_char("You are already following yourself.\r\n", ch);
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
	act("Sorry, but following in loops is not allowed.", FALSE, ch, 0, 0, TO_CHAR);
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}

