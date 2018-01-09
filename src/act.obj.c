/* ************************************************************************
*   File: act.obj1.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern variables */
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct index_data *obj_index;
extern char *drinks[];
extern int drink_aff[][3];
extern struct spell_info_type spell_info[];


/* extern functions */
void mprog_give_trigger(struct char_data * mob, struct char_data * ch,
    struct obj_data * obj);
void mprog_bribe_trigger(struct char_data * mob, struct char_data * ch,
    int amount);
extern ACMD(do_use);
ACMD(do_save);
bool class_can_wear( struct char_data *ch, struct obj_data *obj );
int write_storage_room(int rnum);

/* function prototypes */
void perform_remove(struct char_data *ch, int where);



void perform_put(struct char_data * ch, struct obj_data * obj,
		      struct obj_data * cont)
{
  if (IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_IMMORT(ch)) {
    act("You couldn't bear to put $p into $P.",
      FALSE, ch, obj, cont, TO_CHAR);
    return;
  }

  if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0))
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  else {
    obj_from_char(obj);
    obj_to_obj(obj, cont);
    act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
    act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
  }
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0;

  two_arguments(argument, arg1, arg2);
  obj_dotmode = find_all_dots(arg1);
  cont_dotmode = find_all_dots(arg2);

  if (!*arg1)
    send_to_char("Put what in what?\r\n", ch);
  else if (cont_dotmode != FIND_INDIV)
    send_to_char("You can only put things into one container at a time.\r\n", ch);
  else if (!*arg2) {
    sprintf(buf, "What do you want to put %s in?\r\n",
	    ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
    send_to_char(buf, ch);
  } else {
    generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
    if (!cont) {
      sprintf(buf, "You don't see %s %s here.\r\n", AN(arg2), arg2);
      send_to_char(buf, ch);
    } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
    else if (IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED))
      send_to_char("You'd better open it first!\r\n", ch);
    else {
      if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
	if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
	  sprintf(buf, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
	  send_to_char(buf, ch);
	} else if (obj == cont)
	  send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
	else
	  perform_put(ch, obj, cont);
      } else {
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
	      (obj_dotmode == FIND_ALL || isname(arg1, obj->name))) {
	    found = 1;
	    perform_put(ch, obj, cont);
	  }
	}
	if (!found) {
	  if (obj_dotmode == FIND_ALL)
	    send_to_char("You don't seem to have anything to put in it.\r\n", ch);
	  else {
	    sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
	    send_to_char(buf, ch);
	  }
	}
      }
    }
  }
}



int can_take_obj(struct char_data * ch, struct obj_data * obj)
{
  if (GET_LEVEL(ch)>=LVL_IMMORT) return 1;
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  } else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
    act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  } else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  }
  return 1;
}


void get_check_money(struct char_data * ch, struct obj_data * obj)
{
  if ((GET_OBJ_TYPE(obj) == ITEM_MONEY) && (GET_OBJ_VAL(obj, 0) > 0)) {
    obj_from_char(obj);
    if (GET_OBJ_VAL(obj, 0) > 1) {
      sprintf(buf, "There were %d coins.\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
    }
    GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
    extract_obj(obj);
  }
}


void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
				     struct obj_data * cont, int mode)
{
  if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
    else {
      obj_from_obj(obj);
      obj_to_char(obj, ch);
      act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      get_check_money(ch, obj);
    }
  }
}



void get_from_container(struct char_data * ch, struct obj_data * cont,
				char *arg, int mode)
{
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);

/*
 * HACKED to check for players corpses - the strategy to check:
 * if the corpse is a special kind of corpse (-2 in value[2])
 * then to see if the player trying to get is the true owner,
 * compare the description of the corpse to what it'd be if
 * a corpse was made out of the player.
 * Uncharmed unjarred mobs can get all from player corpses
 * YIKES
 */
  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) || (ch->desc)) {
    sprintf(buf, "The corpse of %s is lying here.", GET_NAME(ch));
    if (IS_SET(GET_OBJ_VAL(cont, 2), -2) 
        && GET_OBJ_RNUM(cont) == -1
        && strcmp(buf, cont->description)
        && GET_LEVEL(ch) < LVL_IMPL) {
      send_to_char("You low-down grave robber!\r\n", ch);
      send_to_char("Stealing from the dead is shameful!\r\n", ch);
      act("$n shamelessly tries to loot $p!", TRUE, ch, cont, 0, TO_ROOM);
      return;
    }
  }
/* end of hack */

  if (IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED))
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, cont->contains))) {
      sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg), arg);
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
    } else
      perform_get_from_container(ch, obj, cont, mode);
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg) {
      send_to_char("Get all of what?\r\n", ch);
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_container(ch, obj, cont, mode);
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL)
	act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      else {
	sprintf(buf, "You can't seem to find any %s in $p.", arg);
	act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
}


int perform_get_from_room(struct char_data * ch, struct obj_data * obj)
{
  bool obj_get_prog(struct obj_data *obj, struct char_data *ch);
  if (can_take_obj(ch, obj)) {
    if (obj_get_prog(obj, ch))
      return 1;  /* Return OK...is this proper? */
                 /* Yes it's fine...right now, nothing checks the return status */
    obj_from_room(obj);
    obj_to_char(obj, ch);
    act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
    get_check_money(ch, obj);
    return 1;
  }
  return 0;
}


void get_from_room(struct char_data * ch, char *arg)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))) {
      sprintf(buf, "You don't see %s %s here.\r\n", AN(arg), arg);
      send_to_char(buf, ch);
    } else
      perform_get_from_room(ch, obj);
  } else {
    if (dotmode == FIND_ALLDOT && !*arg) {
      send_to_char("Get all of what?\r\n", ch);
      return;
    }
    for (obj = world[ch->in_room].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_room(ch, obj);
      }
    }
    if (!found) {
      if (dotmode == FIND_ALL)
	send_to_char("There doesn't seem to be anything here.\r\n", ch);
      else {
	sprintf(buf, "You don't see any %ss here.\r\n", arg);
	send_to_char(buf, ch);
      }
    }
  }
}



/* modified to save after getting to avoid eq duping */
ACMD(do_get)
{
  
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  two_arguments(argument, arg1, arg2);

/* HACKED so that you cannot get from DTs */
  if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && (GET_LEVEL(ch) < LVL_GRGOD))
    send_to_char("This is an accursed place, take nothing from here!\r\n", ch);
/* end of hack */
  else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) && !(GET_LEVEL(ch)>=LVL_IMMORT))
    send_to_char("Your arms are already full!\r\n", ch);
  else if (!*arg1)
    send_to_char("Get what?\r\n", ch);
  else if (!*arg2)
    get_from_room(ch, arg1);
  else {
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
      if (!cont) {
	sprintf(buf, "You don't have %s %s.\r\n", AN(arg2), arg2);
	send_to_char(buf, ch);
      } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
	act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else
	get_from_container(ch, cont, arg1, mode);
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
	send_to_char("Get from all of what?\r\n", ch);
	return;
      }
      for (cont = ch->carrying; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    found = 1;
	    get_from_container(ch, cont, arg1, FIND_OBJ_INV);
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    found = 1;
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	  }
      for (cont = world[ch->in_room].contents; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    get_from_container(ch, cont, arg1, FIND_OBJ_ROOM);
	    found = 1;
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	    found = 1;
	  }
      if (!found) {
	if (cont_dotmode == FIND_ALL)
	  send_to_char("You can't seem to find any containers.\r\n", ch);
	else {
	  sprintf(buf, "You can't seem to find any %ss here.\r\n", arg2);
	  send_to_char(buf, ch);
	}
      }
    }
  }

  do_save(ch, "", 0, SCMD_QUIET_SAVE);
  if (ROOM_FLAGGED(ch->in_room, ROOM_STORAGE)) write_storage_room(ch->in_room);
}



void perform_drop_gold(struct char_data * ch, int amount,
		            byte mode, sh_int RDR)
{
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
  else if (GET_GOLD(ch) < amount)
    send_to_char("You don't have that many coins!\r\n", ch);
  else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
      obj = create_money(amount);
      if (mode == SCMD_DONATE) {
	send_to_char("You throw some gold into the air where it disappears in a puff of smoke!\r\n", ch);
	act("$n throws some gold into the air where it disappears in a puff of smoke!",
	    FALSE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, RDR);
	act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
	send_to_char("You drop some gold.\r\n", ch);
	sprintf(buf, "$n drops %s.", money_desc(amount));
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, ch->in_room);
      }
    } else {
      sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
	      money_desc(amount));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You drop some gold which disappears in a puff of smoke!\r\n", ch);
    }
    GET_GOLD(ch) -= amount;
  }
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")

int perform_drop(struct char_data * ch, struct obj_data * obj,
		     byte mode, char *sname, sh_int RDR)
{
  bool obj_drop_prog(struct obj_data *obj, struct char_data *ch);
  int value;

  if (IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_IMMORT(ch)) {
    sprintf(buf, "You couldn't bear to %s $p.", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  }
  
  if ((mode == SCMD_DONATE) && IS_OBJ_STAT(obj, ITEM_NODONATE)) {
    act("$p flickers, then reappears in your hand.", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  }
  switch (mode) {
    case SCMD_DROP:
      if (obj_drop_prog(obj, ch)) return 0;
  }
  sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);
  sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
  act(buf, TRUE, ch, obj, 0, TO_ROOM);
  obj_from_char(obj);

/* HACKED so that !DONATE items don't WORK 
  if ((mode == SCMD_DONATE) && IS_OBJ_STAT(obj, ITEM_NODONATE))
    mode = SCMD_JUNK;
END of hack */

  switch (mode) {
  case SCMD_DROP:
    obj_to_room(obj, ch->in_room);
    return 0;
    break;
  case SCMD_DONATE:
/* HACKED to make donated objects !sell */
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
/* end of hack */
    obj_to_room(obj, RDR);
    act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
    return 0;
    break;
  case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) >> 4));
    extract_obj(obj);
    return value;
    break;
  default:
    log("SYSERR: Incorrect argument passed to perform_drop");
    break;
  }

  return 0;
}



/* modified to save after dropping to avoid eq duping */
ACMD(do_drop)
{
  extern sh_int donation_room_1;
#if 0
  extern sh_int donation_room_2;  /* uncomment if needed! */
  extern sh_int donation_room_3;  /* uncomment if needed! */
#endif
  struct obj_data *obj, *next_obj;
  sh_int RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0;
  char *sname;

  switch (subcmd) {
  case SCMD_JUNK:
    sname = "junk";
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = "donate";
    mode = SCMD_DONATE;
    switch (number(0, 2)) {
    case 0:
    case 1:
    case 2:
      RDR = real_room(donation_room_1);
      break;
/*    case 3: RDR = real_room(donation_room_2); break;
      case 4: RDR = real_room(donation_room_3); break;
*/
    }
    if (RDR == NOWHERE) {
      send_to_char("Sorry, you can't donate anything right now.\r\n", ch);
      return;
    }
    break;
  default:
    sname = "drop";
    break;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "What do you want to %s?\r\n", sname);
    send_to_char(buf, ch);
    return;
  } else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
      perform_drop_gold(ch, amount, mode, RDR);
    else {
      /* code to drop multiple items.  anyone want to write it? -je */
      send_to_char("Sorry, you can't do that to more than one item at a time.\r\n", ch);
    }
    do_save(ch, "", 0, SCMD_QUIET_SAVE);
    return;
  } else {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK)
	send_to_char("Go to the dump if you want to junk EVERYTHING!\r\n", ch);
      else
	send_to_char("Go to the donation room if you want to donate EVERYTHING!\r\n", ch);
      return;
    }
    if (dotmode == FIND_ALL) {
      if (!ch->carrying)
	send_to_char("You don't seem to be carrying anything.\r\n", ch);
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  amount += perform_drop(ch, obj, mode, sname, RDR);
	}
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
	sprintf(buf, "What do you want to %s all of?\r\n", sname);
	send_to_char(buf, ch);
	return;
      }
      /* HACKED to let people drop all.coins */
      if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
        perform_drop_gold(ch, GET_GOLD(ch), mode, RDR);
        do_save(ch, "", 0, SCMD_QUIET_SAVE);
        return;
      }
      /* end of hack */
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
	send_to_char(buf, ch);
      }
      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
	amount += perform_drop(ch, obj, mode, sname, RDR);
	obj = next_obj;
      }
    } else {
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf, ch);
      } else
	amount += perform_drop(ch, obj, mode, sname, RDR);
    }
  }

  if (amount && (subcmd == SCMD_JUNK)) {
    send_to_char("You have been rewarded by the gods!\r\n", ch);
    act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
    GET_GOLD(ch) += amount;
  }

  do_save(ch, "", 0, SCMD_QUIET_SAVE);
  if (ROOM_FLAGGED(ch->in_room, ROOM_STORAGE)) write_storage_room(ch->in_room);
}



void perform_give(struct char_data * ch, struct char_data * vict,
		       struct obj_data * obj)
{
  if (IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_IMMORT(ch)) {
    act("You couldn't bear to be apart from $p.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if (!IS_IMMORT(vict) && !IS_IMMORT(ch)) {
    if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
      act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
      act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
  }
  obj_from_char(obj);
  obj_to_char(obj, vict);
  MOBTrigger = FALSE;
  act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
  MOBTrigger = FALSE;
  act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
  MOBTrigger = FALSE;
  act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
  mprog_give_trigger(vict, ch, obj);
}



/* utility function for give */
struct char_data *give_find_vict(struct char_data * ch, char *arg)
{
  struct char_data *vict;

  if (!*arg) {
    send_to_char("To who?\r\n", ch);
    return NULL;
  } else if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return NULL;
  } else if (vict == ch) {
    send_to_char("What's the point of that?\r\n", ch);
    return NULL;
  } else
    return vict;
}


void perform_give_gold(struct char_data * ch, struct char_data * vict,
		            int amount)
{
  if (!IS_NPC(ch) && !IS_NPC(vict)) {
    if ((vict != ch) && (!str_cmp(GET_NAME(vict), GET_NAME(ch)))) {
      sprintf(buf, "Possible gold duping attempt by %s blocked.", GET_NAME(ch));
      mudlog(buf, BRF, 58, TRUE);
      send_to_char("That doesn't strike me as a reasonable request...\r\n", ch);
      return;
    }
  }
  if (amount <= 0) {
    send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_IMMORT + 1))) {
    send_to_char("You don't have that many coins!\r\n", ch);
    return;
  }
  send_to_char(OK, ch);
  mprog_bribe_trigger(vict, ch, amount);
  sprintf(buf, "$n gives you %d gold coins.", amount);
  MOBTrigger = FALSE;
  act(buf, FALSE, ch, 0, vict, TO_VICT);
  sprintf(buf, "$n gives %s to $N.", money_desc(amount));
  MOBTrigger = FALSE;
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
    GET_GOLD(ch) -= amount;
  GET_GOLD(vict) += amount;
}



/* modifed to save after giving to avoid eq duping */
ACMD(do_give)
{
  int amount, dotmode;
  struct char_data *vict = NULL;
  struct obj_data *obj, *next_obj;


  argument = one_argument(argument, arg);

  if (!*arg)
    send_to_char("Give what to who?\r\n", ch);
  else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
      argument = one_argument(argument, arg);
      if ((vict = give_find_vict(ch, arg)))
	perform_give_gold(ch, vict, amount);
    } else {
      /* code to give multiple items.  anyone want to write it? -je */
      send_to_char("You can't give more than one item at a time.\r\n", ch);
      return;
    }
  } else {
    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    if (!IS_NPC(ch)) {
      if ((vict != ch) && (!str_cmp(GET_NAME(vict), GET_NAME(ch)))) {
        sprintf(buf, "Possible duping attempt by %s blocked.", GET_NAME(ch));
        mudlog(buf, BRF, 58, TRUE);
        send_to_char("That doesn't strike me as a reasonable request...\r\n", ch);
        return;
      }
    }
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf, ch);
      } else
	perform_give(ch, vict, obj);
    } else {
      if (dotmode == FIND_ALLDOT && !*arg) {
	send_to_char("All of what?\r\n", ch);
	return;
      }
      if (!ch->carrying)
	send_to_char("You don't seem to be holding anything.\r\n", ch);
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (CAN_SEE_OBJ(ch, obj) &&
	      ((dotmode == FIND_ALL || isname(arg, obj->name))))
	    perform_give(ch, vict, obj);
	}
    }
  }

  do_save(ch, "", 0, SCMD_QUIET_SAVE);
  if (vict)
    do_save(vict, "", 0, SCMD_QUIET_SAVE);
}


/* Everything from here down is what was formerly act.obj2.c */


void weight_change_object(struct obj_data * obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj->in_room != NOWHERE) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if ((tmp_ch = obj->carried_by)) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    log("SYSERR: Unknown attempt to subtract weight from an object.");
  }
}



void name_from_drinkcon(struct obj_data * obj)
{
/* HACKED...this screws up too easily and isn't important
  int i;
  char *new_name;
  extern struct obj_data *obj_proto;

  for (i = 0; (*((obj->name) + i) != ' ') && (*((obj->name) + i) != '\0'); i++);

  if (*((obj->name) + i) == ' ') {
    new_name = strdup((obj->name) + i + 1);
    if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
      free(obj->name);
    obj->name = new_name;
  }
*/
}



void name_to_drinkcon(struct obj_data * obj, int type)
{
/* HACKED...this screws up too easy and isn't important
  char *new_name;
  extern struct obj_data *obj_proto;
  extern char *drinknames[];

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", drinknames[type], obj->name);
  if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);
  obj->name = new_name;
*/
}



ACMD(do_drink)
{
  struct obj_data *temp;
  struct affected_type af;
  int amount, weight;
  int on_ground = 0;
  int is_blood = 0;


  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Drink from what?\r\n", ch);
    return;
  }

  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    if (!(temp = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))) {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } else
      on_ground = 1;
  }

  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
    send_to_char("You can't drink from that!\r\n", ch);
    return;
  }

  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
    send_to_char("You have to be holding that to drink from it.\r\n", ch);
    return;
  }

  if ((GET_COND(ch, DRUNK) > 20)) {
    /* The pig is drunk */
    send_to_char("You better not! You might vomit!\r\n", ch);
    act("$n almost vomits, what a loser!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
    send_to_char("Your stomach can't contain anymore!\r\n", ch);
    return;
  }

  if (!GET_OBJ_VAL(temp, 1)) {
    send_to_char("It's empty.\r\n", ch);
    return;
  }

  if (GET_OBJ_VAL(temp, 2) == LIQ_BLOOD)
    is_blood = 1;

  if (subcmd == SCMD_DRINK) {
    if (IS_UNDEAD(ch) && !is_blood) {
      send_to_char("You must drink blood!\r\n", ch);
      return;
    }

    sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, 2)]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);

    sprintf(buf, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    send_to_char(buf, ch);

    if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
    else
      amount = number(3, 10);

  } else {
    act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
    sprintf(buf, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    send_to_char(buf, ch);
    amount = 1;
    if (IS_UNDEAD(ch) && !is_blood) {
      send_to_char("Pheh!\r\n", ch);
      return;
    }
  }

  amount = MIN(amount, GET_OBJ_VAL(temp, 1));

  /* You can't subtract more than the object weighs */
  weight = MIN(amount, GET_OBJ_WEIGHT(temp));

  weight_change_object(temp, -weight);	/* Subtract amount */

  gain_condition(ch, DRUNK,
	 (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount) / 4);

  gain_condition(ch, FULL,
	  (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount) / 4);

  if (IS_UNDEAD(ch) && is_blood) {
    gain_condition(ch, THIRST, amount * 2 / 4);
  } else {
    gain_condition(ch, THIRST,
          (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount) / 4);
  }

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char("You feel pleasantly buzzed.\r\n", ch);

  if (GET_COND(ch, THIRST) > 20)
    send_to_char("You don't feel thirsty any more.\r\n", ch);

  if (GET_COND(ch, FULL) > 20)
    send_to_char("You are full.\r\n", ch);

  if (GET_OBJ_VAL(temp, 3)) {	/* The shit was poisoned ! */
    send_to_char("Oops, it tasted rather strange!\r\n", ch);
    act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

    af.type = SPELL_POISON;
    af.duration = amount * 3;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    af.bitvector2 = 0;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }

  /* empty the container, and no longer poison. */
  GET_OBJ_VAL(temp, 1) -= amount;
  if (!GET_OBJ_VAL(temp, 1)) {	/* The last bit */
    GET_OBJ_VAL(temp, 2) = 0;
    GET_OBJ_VAL(temp, 3) = 0;
    name_from_drinkcon(temp);
  }

  return;
}



ACMD(do_eat)
{
  struct obj_data *food;
  struct affected_type af;
  int amount;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Eat what?\r\n", ch);
    return;
  }

  if (!str_cmp(arg, "#1")) {
    if (ch->carrying != NULL) {
      food = ch->carrying;
    } else {
      send_to_char("You don't seem to have a #1.\r\n", ch);
      return;
    }
  } else if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  }

  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			       (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
/* HACKED to let people eat pills */
  if (GET_OBJ_TYPE(food) == ITEM_PILL) {
    do_use(ch, argument, 0, SCMD_EAT_PILL);
    return;
  }
/* end of hack */
/* old cant eat code */
/*
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char("You can't eat THAT!\r\n", ch);
    return;
  }
*/
  /* basically immorts and uncharmed unjarred mobs can eat anything */
  if (GET_OBJ_TYPE(food) != ITEM_FOOD) {
    if (!IS_NPC(ch)) {
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You can't eat THAT!\r\n", ch);
        return;
      }
    } else {
      if (ch->desc || IS_AFFECTED(ch, AFF_CHARM)) {
        send_to_char("You can't eat THAT!\r\n", ch);
        return;
      }
    }
  }

  if (GET_COND(ch, FULL) > 20) {/* Stomach full */
    act("You are too full to eat more!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_EAT) {
    act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
  } else {
    act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
  }

  amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

  gain_condition(ch, FULL, amount);

  if (GET_COND(ch, FULL) > 20)
    act("You are full.", FALSE, ch, 0, 0, TO_CHAR);

  if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    /* The shit was poisoned ! */
    send_to_char("Oops, that tasted rather strange!\r\n", ch);
    act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

    af.type = SPELL_POISON;
    af.duration = amount * 2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    af.bitvector2 = 0;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }

  if (subcmd == SCMD_EAT)
    extract_obj(food);
  else {
    if (!(--GET_OBJ_VAL(food, 0))) {
      send_to_char("There's nothing left now.\r\n", ch);
      extract_obj(food);
    }
  }

  return;
}



/*
ACMD(do_devour)
{
  void death_cry(struct char_data * ch);
  struct obj_data *k;
  int where;
  char buf[150];
  if (*arg) {
    where = ch->in_room;
    send_to_char("                     * AHEM *\r\n"
        "Did you really think that your cheating would escape the\r\n"
        "notice of the gods forever?!?  FOOLISH MORTAL!!!  You will\r\n"
        "learn the Hard Way the virtues of Honesty and Integrity!!!\r\n"
        "*** OUR WORDS ARE BACKED BY NUCLEAR WEAPONS!!! ***\r\n\r\n", ch);
    send_to_char("       /\\/\\/\\/\\/\\/\\      HEY YOU!!! HEY YOU!!!\r\n"
        "      { ^ ^ ^  ^ ^ }\r\n"
        "      { ^  ^  ^  ^ }     YOU GOT NUKED!!! BOOM!!!\r\n"
        "      (\\ ^  ^ ^  /)\r\n"
        "   \\\\\\   | . :. |   ///\r\n"
        "   (::)..).:.:. (..(::)\r\n", ch);
    act("       /\\/\\/\\/\\/\\/\\      HEY $n!!! HEY $n!!!\r\n"
        "      { ^ ^ ^  ^ ^ }\r\n"
        "      { ^  ^  ^  ^ }     YOU GOT NUKED!!! BOOM!!!\r\n"
        "      (\\ ^  ^ ^  /)\r\n"
        "   \\\\\\   | . :. |   ///\r\n"
        "   (::)..).:.:. (..(::)\r\n", FALSE, ch, 0, 0, TO_ROOM);
    extract_char(ch);
    for (k = world[where].contents; k; k = world[where].contents) {
      extract_obj(k);
    }
    sprintf(buf, "%s NUKED trying to devour '%s'", GET_NAME(ch), arg);
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
  } 
}
*/



ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj;
  struct obj_data *to_obj;
  int amount;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) {		/* No arguments */
      act("What do you want to pour from?", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      act("You can't pour from that!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }

  if (subcmd == SCMD_FILL) {
    if (!*arg1) {		/* no arguments */
      send_to_char("What do you want to fill?  And what are you filling it from?\r\n", ch);
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      send_to_char("You can't find it!", ch);
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2) {		/* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents))) {
      sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, 1) == 0) {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) {	/* pour */
    if (!*arg2) {
      act("Where do you want it?  Out or in what?", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (!str_cmp(arg2, "out")) {
      act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
      act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

      weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1));	/* Empty */

      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
      name_from_drinkcon(from_obj);

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
	(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
      act("You can't pour anything into that.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (to_obj == from_obj) {
    act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
      (GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))) {
    act("There is already another liquid in it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))) {
    act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) {
    sprintf(buf, "You pour the %s into the %s.",
	    drinks[GET_OBJ_VAL(from_obj, 2)], arg2);
    send_to_char(buf, ch);
  }
  if (subcmd == SCMD_FILL) {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, 1) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

  /* Then how much to pour */
  GET_OBJ_VAL(from_obj, 1) -= (amount =
			 (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

  GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

  if (GET_OBJ_VAL(from_obj, 1) < 0) {	/* There was too little */
    GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
    amount += GET_OBJ_VAL(from_obj, 1);
    GET_OBJ_VAL(from_obj, 1) = 0;
    GET_OBJ_VAL(from_obj, 2) = 0;
    GET_OBJ_VAL(from_obj, 3) = 0;
    name_from_drinkcon(from_obj);
  }
  /* Then the poison boogie */
  GET_OBJ_VAL(to_obj, 3) =
    (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

  /* And the weight boogie */
  weight_change_object(from_obj, -amount);
  weight_change_object(to_obj, amount);	/* Add weight */

  return;
}



void wear_message(struct char_data * ch, struct obj_data * obj, int where)
{
  char *wear_messages[][2] = {
    {"$n lights $p and holds it.",
    "You light $p and hold it."},

    {"$n slides $p on to $s right ring finger.",
    "You slide $p on to your right ring finger."},

    {"$n slides $p on to $s left ring finger.",
    "You slide $p on to your left ring finger."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p on $s body.",
    "You wear $p on your body.",},

    {"$n wears $p on $s head.",
    "You wear $p on your head."},

    {"$n puts $p on $s legs.",
    "You put $p on your legs."},

    {"$n wears $p on $s feet.",
    "You wear $p on your feet."},

    {"$n puts $p on $s hands.",
    "You put $p on your hands."},

    {"$n wears $p on $s arms.",
    "You wear $p on your arms."},

    {"$n straps $p around $s arm as a shield.",
    "You start to use $p as a shield."},

    {"$n wears $p about $s body.",
    "You wear $p around your body."},

    {"$n wears $p around $s waist.",
    "You wear $p around your waist."},

    {"$n puts $p on around $s right wrist.",
    "You put $p on around your right wrist."},

    {"$n puts $p on around $s left wrist.",
    "You put $p on around your left wrist."},

    {"$n wields $p.",
    "You wield $p."},

    {"$n grabs $p.",
    "You grab $p."},

    {"$n readies $p.",
    "You ready $p."},

    {"$n wields $p.",
    "You wield $p."},
    
    {"$p begins floating about $n.",
    "$p starts floating around you."},

    {"$n slips $p through an ear.",
    "You slip $p through an ear."},

    {"$n slides $p onto $s face.",
    "You slide $p onto your face."}
  };

  char *thri_wear_messages[][2] = {
    {"$n lights $p and holds it.",
    "You light $p and hold it."},

    {"$n slides $p on to $s upper left finger.",
    "You slide $p on to your upper left finger."},

    {"$n slides $p on to $s upper right finger.",
    "You slide $p on to your upper right finger."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n slides $p on to $s lower left finger.",
    "You slide $p on to your lower left finger."},

    {"$n slides $p on to $s lower right finger.",
    "You slide $p on to your lower right finger."},

    {"$n straps $p around $s left foreleg as a shield.",
    "You start to use $p as a shield."},

    {"$n straps $p around $s right foreleg as a shield.",
    "You start to use $p as a shield."},

    {"$n wears $p around $s waist.",
    "You wear $p around your waist."},

    {"$n holds $p in $s lower left claw.",
    "You hold $p in your lower left claw."},
    
    {"$n wields $p with $s upper right claw.",
     "You wield $p with your upper right claw."},
    
    {"$n holds $p in $s lower right claw.",
    "You hold $p in your lower right claw."},
    
    {"$n readies $p.",
     "You ready $p."},
    
    {"$n wields $p with $s upper left claw.",
     "You wield $p with your upper left claw."},
     
    {"$p starts floating around $n.",
    "$p starts floating around you."},

    {"$n slides $p onto $s face.",
    "You slide $p onto your face."}
    
  };

  if (IS_THRIKREEN(ch)) {
    act(thri_wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
    act(thri_wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
  } else {
    act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
    act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
  }
}

void perform_thri_wear(struct char_data * ch, struct obj_data * obj,
    int where, int dotmode)
{
  int tempwhere;

  int wear_bitvectors[] = {
    ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
    ITEM_WEAR_NECK, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_SHIELD,
    ITEM_WEAR_SHIELD, ITEM_WEAR_WAIST, ITEM_WEAR_TAKE, ITEM_WEAR_WIELD,
    ITEM_WEAR_TAKE, ITEM_WEAR_READY, ITEM_WEAR_WIELD, ITEM_WEAR_PRIZE,
    ITEM_WEAR_FACE};

  char *already_wearing[] = {
    "You're already using a light.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You can't wear anything else around your neck.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You've got rings on all four claws already.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already using a shield.\r\n",
    "You already have something around your waist.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already holding things in both claws.\r\n",
    "You've already got a weapon readied!\r\n",
    "You're already wielding two weapons!\r\n",
    "You can only use one prize at a time!\r\n",
    "You already wear something through your ears.\r\n",
    "You already wear something over your face.\r\n",
  };

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where])) {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
#if(0)
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) ||
      (where == WEAR_WRIST_R))
    if (ch->equipment[where])
      where++;
#endif
  /* Check for a secondary position if the primary is full */
  if ((ch->equipment[where] && where != THRI_WEAR_LIGHT)) {
    for (tempwhere = 0; tempwhere < NUM_THRI_WEARS && ch->equipment[where];
         tempwhere++) {
      if ((wear_bitvectors[tempwhere] == wear_bitvectors[where]) &&
(tempwhere != THRI_WEAR_LIGHT))
        where = tempwhere;
    }
  }
      
/* HACKED to see if the class can wear this equipment */
  if (!class_can_wear( ch, obj )) {
    act("Your can't use $p, it would cramp your style!\r\n", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
/* end of hack */

#if(0)
/* HACKED to check dual wield if the player is a warrior with the skill
  and they attempt to wield another weapon */
  if (where == WEAR_WIELD &&
      ch->equipment[WEAR_WIELD] &&
      GET_SKILL(ch, SKILL_DUALWIELD))
    where = WEAR_WIELD_2;
/* end of hack */

/* HACKED to tell the player to remove a shield if they want dual wield */
  if (where == WEAR_WIELD_2 &&
      ch->equipment[WEAR_SHIELD]) {
    send_to_char("You'll have to remove your shield first.\r\n", ch);
    return;
  };
  if (where == WEAR_WIELD &&
      ch->equipment[WEAR_SHIELD] &&
      ch->equipment[WEAR_WIELD_2]) {
    send_to_char("You'll have to remove your shield first.\r\n", ch);
    return;
  }
  if (where == WEAR_SHIELD &&
      ch->equipment[WEAR_WIELD] && 
      ch->equipment[WEAR_WIELD_2]) {
    send_to_char("You'll have to put away one of your weapons first.\r\n", ch);
    return;
  };
/* end of hack */
#endif

/* HACKED to remove equipment if already worn on that position */
  if (ch->equipment[where] && dotmode == FIND_ALL) {
    send_to_char(already_wearing[where], ch); 
    return;
  } else if (ch->equipment[where]) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
      act("$p: your hands are so full you can't swap the items you're using!",
              FALSE, ch, obj, 0, TO_CHAR);
      return;
    } else {
      perform_remove(ch, where); /* watch out! items flagged CURSE will not be
                              removeable! this would skip that step */
    }
  }
/* end of hack */

  wear_message(ch, obj, where);
  obj_from_char(obj);
  equip_char(ch, obj, where);

  if ((obj->obj_flags.bitvector > 0) ||
      (obj->obj_flags.bitvector2 > 0))
    send_to_char("You feel magical power rush to you!\r\n", ch);
}



void perform_wear(struct char_data * ch, struct obj_data * obj, int where,
    int dotmode)
{

  struct obj_data *curobj;
  bool obj_wear_prog(struct obj_data *obj, struct char_data *ch);

  int wear_bitvectors[] = {
    ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
    ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
    ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
    ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
    ITEM_WEAR_WIELD, ITEM_WEAR_TAKE, ITEM_WEAR_READY, ITEM_WEAR_WIELD,
    ITEM_WEAR_PRIZE, ITEM_WEAR_EARS, ITEM_WEAR_FACE};

  char *already_wearing[] = {
    "You're already using a light.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something on both of your ring fingers.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You can't wear anything else around your neck.\r\n",
    "You're already wearing something on your body.\r\n",
    "You're already wearing something on your head.\r\n",
    "You're already wearing something on your legs.\r\n",
    "You're already wearing something on your feet.\r\n",
    "You're already wearing something on your hands.\r\n",
    "You're already wearing something on your arms.\r\n",
    "You're already using a shield.\r\n",
    "You're already wearing something about your body.\r\n",
    "You already have something around your waist.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something around both of your wrists.\r\n",
    "You're already wielding a weapon.\r\n",
    "You're already holding something.\r\n",
    "You've already got a weapon readied.\r\n",
    "You're already wielding a secondary weapon.\r\n",
    "You can only use one prize at a time!\r\n",
    "You already wear something through your ears.\r\n",
    "You already wear something over your face.\r\n",
  };
  
  if (IS_THRIKREEN(ch)) {
    perform_thri_wear(ch, obj, where, dotmode);
    return;
  }

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where])) {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) ||
      (where == WEAR_WRIST_R))
    if (ch->equipment[where])
      where++;
      
/* HACKED to see if the class can wear this equipment */
  if (!class_can_wear( ch, obj )) {
    act("Your can't use $p, it would cramp your style!\r\n", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
/* end of hack */

/* HACKED to check dual wield if the player is a warrior with the skill
  and they attempt to wield another weapon */
  if (where == WEAR_WIELD &&
      ch->equipment[WEAR_WIELD] &&
      GET_SKILL(ch, SKILL_DUALWIELD))
    where = WEAR_WIELD_2;
/* end of hack */

/* HACKED to tell the player to remove a shield if they want dual wield */
  if (where == WEAR_WIELD_2 &&
      ch->equipment[WEAR_SHIELD]) {
    send_to_char("You'll have to remove your shield first.\r\n", ch);
    return;
  };
  if (where == WEAR_WIELD &&
      ch->equipment[WEAR_SHIELD] &&
      ch->equipment[WEAR_WIELD_2]) {
    send_to_char("You'll have to remove your shield first.\r\n", ch);
    return;
  }
  if (where == WEAR_SHIELD &&
      ch->equipment[WEAR_WIELD] && 
      ch->equipment[WEAR_WIELD_2]) {
    send_to_char("You'll have to put away one of your weapons first.\r\n", ch);
    return;
  };
/* end of hack */

/* HACKED to remove equipment if already worn on that position */
  if (ch->equipment[where] && dotmode == FIND_ALL) {
    send_to_char(already_wearing[where], ch); 
    return;
  } else if (ch->equipment[where]) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
      act("$p: your hands are so full you can't swap the items you're using!",
              FALSE, ch, obj, 0, TO_CHAR);
      return;
    } else {

	curobj = ch->equipment[where];

	if (IS_OBJ_STAT(curobj, ITEM_NOREMOVE)) {
	    send_to_char( "You cannot remove the current item.\n\r", ch );
	    return;
	}   

	perform_remove(ch, where);
    }
  }
/* end of hack */

  wear_message(ch, obj, where);
  obj_from_char(obj);
  equip_char(ch, obj, where);

  if ((obj->obj_flags.bitvector > 0) ||
      (obj->obj_flags.bitvector2 > 0))
    send_to_char("You feel magical power rush to you!\r\n", ch);
}

int find_thri_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);

int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg)
{
  int where = -1;
  
  static char *keywords[] = {
    "!RESERVED!",
    "finger",
    "!RESERVED!",
    "neck",
    "!RESERVED!",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "shield",
    "about",
    "waist",
    "wrist",
    "!RESERVED!",		/* wrist 2 */
    "!RESERVED!",		/* wield */
    "!RESERVED!",		/* hold */
    "!RESERVED!",		/* ready */
    "!RESERVED!",		/* dual wield */
    "!RESERVED!",		/* prize */
    "ears",
    "face",
    "\n"
  };

  if (IS_THRIKREEN(ch)) return find_thri_eq_pos(ch, obj, arg);

  if (!arg || !*arg) {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
      where = WEAR_FINGER_R;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))
      where = WEAR_NECK_1;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))
      where = WEAR_BODY;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
      where = WEAR_HEAD;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
      where = WEAR_LEGS;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))
      where = WEAR_FEET;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
      where = WEAR_HANDS;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
      where = WEAR_ARMS;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
      where = WEAR_SHIELD;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
      where = WEAR_ABOUT;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
      where = WEAR_WAIST;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
      where = WEAR_WRIST_R;
    if (CAN_WEAR(obj, ITEM_WEAR_PRIZE))
      where = WEAR_PRIZE;
    if (CAN_WEAR(obj, ITEM_WEAR_EARS))
      where = WEAR_EARS;
    if (CAN_WEAR(obj, ITEM_WEAR_FACE))
      where = WEAR_FACE;
  } else {
    if ((where = search_block(arg, keywords, FALSE)) < 0) {
      sprintf(buf, "'%s'?  What part of your body is THAT?\r\n", arg);
      send_to_char(buf, ch);
    }
  }

  return where;
}

int find_thri_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg)
{
  int where = -1;
  
  static char *keywords[] = {
    "!RESERVED!",
    "!RESERVED!", /* finger 1 */
    "finger",
    "neck",
    "!RESERVED!", /* neck 2 */
    "!RESERVED!", /* finger 3 */
    "!RESERVED!", /* finger 4 */
    "shield",
    "!RESERVED!", /* shield 2 */
    "waist",
    "!RESERVED!", /* held 2 */
    "!RESERVED!", /* wield 1 */
    "!RESERVED!", /* held 1 */
    "!RESERVED!", /* ready */
    "!RESERVED!", /* wield 2 */
    "\n"
  };

  if (!arg || !*arg) {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
      where = THRI_WEAR_FINGER_UR;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))
      where = THRI_WEAR_NECK_1;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
      where = THRI_WEAR_SHIELD_R;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
      where = THRI_WEAR_WAIST;
    if (CAN_WEAR(obj, ITEM_WEAR_PRIZE))
      where = THRI_WEAR_PRIZE;
    if (CAN_WEAR(obj, ITEM_WEAR_FACE))
      where = THRI_WEAR_FACE;
  } else {
    if ((where = search_block(arg, keywords, FALSE)) < 0) {
      sprintf(buf, "'%s'?  What part of your body is THAT?\r\n", arg);
      send_to_char(buf, ch);
    }
  }

  return where;
}



ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;


  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char("Wear what?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char("You can't specify the same body location for more than one item!\r\n", ch);
    return;
  }
  if (dotmode == FIND_ALL) {
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
	items_worn++;
	perform_wear(ch, obj, where, dotmode);
      }
    }
    if (!items_worn)
      send_to_char("You don't seem to have anything wearable.\r\n", ch);
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char("Wear all of what?\r\n", ch);
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
      send_to_char(buf, ch);
    } else
      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg1, obj->next_content);
	if ((where = find_eq_pos(ch, obj, 0)) >= 0)
	  perform_wear(ch, obj, where, dotmode);
	else
	  act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
	obj = next_obj;
      }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
      send_to_char(buf, ch);
    } else {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
	perform_wear(ch, obj, where, dotmode);
      else if (!*arg2)
	act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}



ACMD(do_wield)
{
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Wield what?\r\n", ch);
  else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else {
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
      send_to_char("You can't wield that.\r\n", ch);
    else if (IS_MONK(ch) && (GET_OBJ_VAL(obj, 3) + TYPE_HIT != TYPE_CLAW))
      send_to_char("You have no idea what do DO with that!\r\n", ch);
    else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
      send_to_char("It's too heavy for you to use.\r\n", ch);
    else
      if (IS_THRIKREEN(ch))
        perform_wear(ch, obj, THRI_WEAR_WIELD_R, FIND_INDIV);
      else
        perform_wear(ch, obj, WEAR_WIELD, FIND_INDIV);
  }
}



ACMD(do_ready)
{
  struct obj_data *obj;

  one_argument(argument, arg);

/* No more ranged combat */

  send_to_char("No more ranged combat, no more ready.\r\n", ch);
  return;

  if (!*arg)                                                                    
    send_to_char("Ready what?\r\n", ch);
  else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else {
    if (!CAN_WEAR(obj, ITEM_WEAR_READY))
      send_to_char("You can't ready that.\r\n", ch);
    else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
      send_to_char("It's too heavy for you to use.\r\n", ch);
    else
      if (IS_THRIKREEN(ch))
        perform_wear(ch, obj, THRI_WEAR_READY, FIND_INDIV);
      else
        perform_wear(ch, obj, WEAR_READY, FIND_INDIV);
  }
}



ACMD(do_grab)
{
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Hold what?\r\n", ch);
  else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (IS_THRIKREEN(ch)) {
        perform_wear(ch, obj, THRI_WEAR_LIGHT, FIND_INDIV);
      } else {
        perform_wear(ch, obj, WEAR_LIGHT, FIND_INDIV);
      }
    else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD)
          && GET_OBJ_TYPE(obj) != ITEM_WAND
          && GET_OBJ_TYPE(obj) != ITEM_STAFF
          && GET_OBJ_TYPE(obj) != ITEM_SCROLL
	  && GET_OBJ_TYPE(obj) != ITEM_POTION
          && GET_OBJ_TYPE(obj) != ITEM_PILL)
	send_to_char("You can't hold that.\r\n", ch);
      else
        if (IS_THRIKREEN(ch))
          perform_wear(ch, obj, THRI_WEAR_HOLD_R, FIND_INDIV);
        else
	  perform_wear(ch, obj, WEAR_HOLD, FIND_INDIV);
    }
  }
}



void perform_remove(struct char_data * ch, int pos)
{
  struct obj_data *obj;

  if (!(obj = ch->equipment[pos])) {
    sprintf(buf, "Error in perform_remove: bad pos passed;"
                 " %s removing from %d", GET_NAME(ch), pos);
    log(buf);
    return;
  }

  if (IS_OBJ_STAT(obj, ITEM_NOREMOVE) && !IS_IMMORT(ch)) {
    act("You couldn't bear to remove $p.",
      FALSE, ch, obj, NULL, TO_CHAR);
    return;
  }

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
  else {
    if (ch->equipment[pos])
      obj_to_char(unequip_char(ch, pos), ch);
    else {
      log("SYSERR: unequip_char, act.obj.c perform_remove()");
      exit(1);
    }
    act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
  }

  if ((obj->obj_flags.bitvector > 0) ||
      (obj->obj_flags.bitvector2 > 0))
    send_to_char("You feel magical power drain from you!\r\n", ch);
}



ACMD(do_remove)
{
  struct obj_data *obj;
  int i, dotmode, found, num_wears;
 
 
  num_wears = IS_THRIKREEN(ch)? NUM_THRI_WEARS : NUM_WEARS;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Remove what?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_ALL) {
    found = 0;
    for (i = 0; i < num_wears; i++)
      if (ch->equipment[i]) {
	perform_remove(ch, i);
	found = 1;
      }
    if (!found)
      send_to_char("You're not using anything.\r\n", ch);
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg)
      send_to_char("Remove all of what?\r\n", ch);
    else {
      found = 0;
      for (i = 0; i < num_wears; i++)
	if (ch->equipment[i] && CAN_SEE_OBJ(ch, ch->equipment[i]) &&
	    isname(arg, ch->equipment[i]->name)) {
	  perform_remove(ch, i);
	  found = 1;
	}
      if (!found) {
	sprintf(buf, "You don't seem to be using any %ss.\r\n", arg);
	send_to_char(buf, ch);
      }
    }
  } else {
    if (!(obj = get_object_in_equip_vis(ch, arg, ch->equipment, &i))) {
      sprintf(buf, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
      send_to_char(buf, ch);
    } else
      perform_remove(ch, i);
  }
}



ACMD(do_make)
{
  send_to_char("You try your best to fashion something useful.\r\n", ch);
}


ACMD(do_conceal) {
  struct obj_data *obj;
  struct room_data *room;

  argument = one_argument(argument, arg);
  
  if (!*arg) {
    send_to_char("You hide nothing. No one will ever find it!\r\n", ch);
    return;
  } else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  } else {
    if (ch->in_room == NOWHERE) {
      send_to_char("You can't hide anything here!\r\n", ch);
      return;
    }
    obj_from_char(obj);
    room = &world[ch->in_room];
    obj->next_content = room->hidden;
    room->hidden = obj;
    act("You conceal $p!", TRUE, ch, obj, NULL, TO_CHAR);
  }
}
    
ACMD(do_search) {
  if (GET_POS(ch) == POS_SEARCHING) {
    send_to_char("You stop searching.\r\n", ch);
    act("$n stops searching.", TRUE, ch, NULL, NULL, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    return;
  }
  send_to_char("You begin searching the room.\r\n", ch);
  act("$n starts searching the room.", TRUE, ch, NULL, NULL, TO_ROOM);
  GET_POS(ch) = POS_SEARCHING;
}

void perform_search(struct char_data *ch) {
  int number_percent(void);

  struct obj_data *obj, *tempobj, *lastobj;
  bool found_something = FALSE;
  
  if (ch->in_room == NOWHERE) return;
  
  lastobj = NULL;
  for (obj = world[ch->in_room].hidden; obj; obj = tempobj) {
    tempobj = obj->next_content;
    if (number_percent() < 35 || GET_LEVEL(ch) > 50) {
      if (lastobj) lastobj->next_content = tempobj;
      if (world[ch->in_room].hidden == obj) {
        world[ch->in_room].hidden = tempobj;
      }
      obj->next_content = NULL;
      obj_to_room(obj, ch->in_room);
      act("You find $p!", TRUE, ch, obj, NULL, TO_CHAR);
      act("$n finds $p.", FALSE, ch, obj, NULL, TO_ROOM);
      if (CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
        perform_get_from_room(ch, obj);
      }
      found_something = TRUE;
    } else {
      lastobj = obj;
    }
  }
  if (!found_something) {
    send_to_char("You search, but do not find anything.\r\n", ch);
  }
}

void perform_all_search() {
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *desc;
  struct char_data *ch;
  
  void perform_search(struct char_data *ch);
  
  /* Functions for each PC */
  for (desc = descriptor_list; desc; desc = desc->next) {
    if ((ch = desc->character)) {
      if (GET_POS(ch) == POS_SEARCHING) perform_search(ch);
    }
  }
}
