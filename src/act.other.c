/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
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
#include "screen.h"
#include "house.h"

/* extern variables */
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct dex_skill_type dex_app_skill[];
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern char *race_abbrevs[];
extern char *class_abbrevs[];
/* HACKED to add immort abbreviations */
extern char *immort_abbrevs[];
/* end of hack */
extern char *color_codes[];
extern char *affected_bits[];
extern char *affected2_bits[];
extern char *spells[];
extern struct zone_data *zone_table;

/* extern procedures */
SPECIAL(shop_keeper);
void clanlog(char *str, struct char_data * ch);
int can_take_obj(struct char_data * ch, struct obj_data * obj);
void get_check_money(struct char_data * ch, struct obj_data * obj);
struct obj_data *get_object_in_equip_vis(struct char_data * ch,
    char *arg, struct obj_data * equipment[], int *j);
ACMD(do_gen_comm);
ACMD(do_help);
ACMD(do_follow);



ACMD(do_quit)
{
  void die(struct char_data * ch, struct char_data * killer);
  void Crash_rentsave(struct char_data * ch, int cost);
  void extract_pet(struct char_data *pet);
  extern int free_rent;
  struct descriptor_data *d, *next_d;
  bool has_eq = FALSE;
  int eq_pos, num_wears = IS_THRIKREEN(ch)? NUM_THRI_WEARS : NUM_WEARS;

  if (IS_NPC(ch) || !ch->desc)
    return;

  one_argument(argument, arg);


  if (ch->carrying)
    has_eq = TRUE;
  else
    for (eq_pos = 0; eq_pos < num_wears; eq_pos++)
      if (ch->equipment[eq_pos])
        has_eq = TRUE;


  if ((GET_LEVEL(ch) < LVL_IMMORT) && has_eq) {
    send_to_char("Sorry, since you have equipment on you, you have to rent at a receptionist.\r\n", ch);
  } else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch, NULL);
  } else {
    if (!GET_INVIS_LEV(ch))
      act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    clanlog(buf, ch);
    send_to_char("Goodbye, friend.. Come back soon!\r\n", ch);

    /* pets */
    /* unlink all the pet pointers and remove the pet from the game */
    if (HAS_PET(ch)) {
      extract_pet(GET_PET(ch));
    }

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
	continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
	close_socket(d);
    }

    if (free_rent)
      Crash_rentsave(ch, 0);
    extract_char(ch);		/* Char is saved in extract char */
  }
}



ACMD(do_rent)
{
  send_to_char("Rent is free, but you have to rent at a receptionist.\r\n", ch);
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* was temporarily... */
/*  if (subcmd == SCMD_QUIET_SAVE)
    return;*/

/*
  if (cmd) {
    sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }
*/
  if (subcmd != SCMD_QUIET_SAVE) {
    sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }

  save_char(ch, NOWHERE);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
    House_crashsave(world[ch->in_room].number);
}



/*
ACMD(do_not_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  send_to_char("Save now autosaves after each kill.\r\n", ch);

  return;
}
*/



/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}



/* a function to just mark stuff off as disabled. */
ACMD(do_disabled)
{
  send_to_char("Sorry, but that command has been disabled...\r\n", ch);
}



ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);

  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
    return;

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch) / 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  af.bitvector2 = 0;
  affect_to_char(ch, &af);
}



ACMD(do_rage)
{
  byte percent, prob;
  struct affected_type af;

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_RAGE)) {
    send_to_char("You are already full of hate!\r\n", ch);
    return;
  }

  percent = number(1, 101);     /* 101% is a complete failure */
  prob = (GET_SKILL(ch, SKILL_RAGE));

  if (percent > prob) {
    send_to_char("You grit your teeth!\r\n", ch);
    act("$n grits their teeth!", TRUE, ch, 0, 0, TO_ROOM);
  } else {
    /* good */
    af.type = SKILL_RAGE;
    af.location = APPLY_HIT;
    af.bitvector = AFF_RAGE;
    af.bitvector2 = 0;
    af.modifier = GET_LEVEL(ch) * 2;
    af.duration = 2;
    affect_to_char(ch, &af);
    GET_HIT(ch) += GET_LEVEL(ch) * 2;

    send_to_char("You grit your teeth and snarl in rage!\r\n", ch);
    act("$n grits their teeth and snarls in rage!", TRUE, ch, 0, 0, TO_ROOM);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
}



/*
 * valour is a knight skill that is a lot like rage, uses the same affects
 * just has more extreme bonuses to damage and hit points
 */
ACMD(do_valour)
{
  byte percent, prob;
  struct affected_type af;

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the noble deeds to knights.\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_RAGE)) {
    send_to_char("You are already full of glory!\r\n", ch);
    return;
  }

  percent = number(1, 101);     /* 101% is a complete failure */
  prob = (GET_SKILL(ch, SKILL_VALOUR));

  if (percent > prob) {
    /* not so good */
    send_to_char("You hold your head!\r\n", ch);
    act("$n holds $s head!\r\n", TRUE, ch, 0, 0, TO_ROOM);
  } else {
    /* good */
    af.type = SKILL_VALOUR;
    af.location = APPLY_HIT;
    af.bitvector = AFF_RAGE;
    af.bitvector2 = 0;
    af.modifier = GET_LEVEL(ch) * 4;
    af.duration = 2;
    affect_to_char(ch, &af);
    GET_HIT(ch) += GET_LEVEL(ch) * 4;

    send_to_char("You hold your head up high "
                 "and fight for what's right!\r\n",ch);
    act("$n holds $s head up high and fights for what's right!",
        TRUE, ch, 0, 0, TO_ROOM);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
}



ACMD(do_judge)
{
  int i;
  int found;
  struct obj_data *obj;

  struct time_info_data age(struct char_data * ch);

  extern char *item_types[];
  extern char *apply_types[];


  argument = one_argument(argument, buf);

  if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
    if (obj == NULL) {
      send_to_char("You do not have that item.\r\n", ch);
      return;
    }
  }
   
  send_to_char("You feel informed:\r\n", ch);
  sprintf(buf, "Object '%s', Item type: ", obj->short_description);
  sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
  strcat(buf, buf2);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WEAPON:
    case ITEM_FIREWEAPON:
        sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
                GET_OBJ_VAL(obj, 2));
        sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
                (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
        send_to_char(buf, ch);
        break;
    case ITEM_ARMOR:
        sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
        send_to_char(buf, ch);
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_WAND:
    case ITEM_STAFF:
    default:
        send_to_char("You have no idea how to judge that item's "
                "ability.\r\n", ch);
        return;
  }      

  found = FALSE;
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if ((obj->affected[i].location != APPLY_NONE) &&
        (obj->affected[i].modifier != 0)) {
      if (!found) {
        send_to_char("Can affect you as :\r\n", ch);
        found = TRUE;
      }
      sprinttype(obj->affected[i].location, apply_types, buf2);
      if (obj->affected[i].modifier > 0) {
        sprintf(buf, "   Affects: +%d %s\r\n",
            obj->affected[i].modifier, buf2);
      } else {
        sprintf(buf, "   Affects: %d %s\r\n",
            obj->affected[i].modifier, buf2);
      }
      send_to_char(buf, ch);
    }
  }

  if ((obj->obj_flags.bitvector) ||
      (obj->obj_flags.bitvector2))
  {
    send_to_char("Item will give you following abilities:  ", ch);

    if (obj->obj_flags.bitvector) {
      sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }

    if (obj->obj_flags.bitvector2) {
      sprintbit(obj->obj_flags.bitvector2, affected2_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
  }

  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WEAPON:
    case ITEM_FIREWEAPON:
        if (obj->spell_affect[0].spelltype > 0) {
          send_to_char("Weapon spells:\r\n", ch);
          for (i = 0; i < MAX_SPELL_AFFECT; i++) {
            if (obj->spell_affect[i].spelltype > 0) {
              sprintf(buf, "   %-20s  Level %d  %d%%%%\r\n",
                  spells[obj->spell_affect[i].spelltype],
                  obj->spell_affect[i].level,
                  obj->spell_affect[i].percentage);
              send_to_char(buf, ch);
            }
          }
          send_to_char("\r\n", ch);
        }
        break;
    default:
        break;
  }
}



ACMD(do_gauge)
{
  struct affected_type af;
  struct char_data *victim;

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  one_argument(argument, buf);
 
  if (!IS_NPC(ch) && !IS_THIEF(ch)) {
    send_to_char("You'd better leave the sneaky stuff to thieves.\r\n", ch);
    return;
  }
 
  if (IS_AFFECTED(ch, AFF_GAUGE)) {
    send_to_char("Nothing happens.\r\n", ch);
    return;
  }
  if (!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("Gauge who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char(
        "You dont get any bonuses for finding your own weaknesses!\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("Where you stand is undefilable by combat!\r\n", ch);
    return;
  }

  if (!IS_NPC(victim)) {
    send_to_char("Sorry, no playerkilling here.\r\n", ch);
    return;
  }
  
  if (GET_SKILL(ch, SKILL_GAUGE) < (number(0, 101))) {
    send_to_char("You fail to sense a weakness.\r\n", ch);
    return;
  }
  if (number (0, 9) == 0) {
    send_to_char("It senses your evil intent and attacks!\r\n", ch);
    set_fighting(ch, victim);
    }
  else {
    af.duration = 3;
    af.type = SKILL_GAUGE;
    af.modifier = (number(2, 5));
    af.location = APPLY_DAMROLL;
    af.bitvector = AFF_GAUGE;
    af.bitvector2 = 0;
    affect_to_char(ch, &af);
    send_to_char("You probe for its weaknesses.\r\n", ch);
  }
}



ACMD(do_hide)
{
  byte percent;

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  send_to_char("You attempt to hide yourself.\r\n", ch);

  if (IS_AFFECTED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
    return;

  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}



ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[240];
  char obj_name[240];
  int percent = 0;
  int gold = 0;
  int eq_pos = 0;
  int pcsteal = 0;
  extern int pt_allowed;
  bool ohoh = FALSE;
  int chance_to_steal = 0;

  void set_fighting( struct char_data *ch, struct char_data *vict );

  argument = one_argument(argument, obj_name);
  one_argument(argument, vict_name);

  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("Where you stand is undefilable by theft!\r\n", ch);         
    return;
  }
  if (!pt_allowed) {
    if (!IS_NPC(vict) && !PLR_FLAGGED(vict, PLR_THIEF) && 
        /* Removed this...sorry, I don't think we WANT
           people to steal eq during deathmatches...
           !IS_CHAOS_ROOM(ch->in_room) &&
        */
        /* Added so mobs can steal from players */
        !IS_NPC(ch) &&
        !PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(ch, PLR_THIEF)) {
      SET_BIT(PLR_FLAGS(ch), PLR_THIEF);
      sprintf(buf, "PC Thief bit set on %s for trying to steal from %s at %s.",
              GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      send_to_char("Okay, you bad man... you're now a THIEF!\r\n", ch);
      pcsteal = 1;
    }
    if (PLR_FLAGGED(ch, PLR_THIEF))
      pcsteal = 1;

    /*
     * We'll try something different... instead of having a thief flag, just
     * have PC Steals fail all the time.
     */
  }
  /* 101% is a complete failure */
  percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;
/* HACKED to make the thiefs chance of success equal about 50% +/- 2 1/2% per
  level difference; the old chance to steal is simply
  GET_SKILL(ch, SKILL_STEAL) of course. 

  chance_to_steal = (GET_SKILL(ch, SKILL_STEAL) / 2)
                    - ((GET_LEVEL(vict) - GET_LEVEL(ch)) * 5/2); */

  chance_to_steal = GET_LEVEL(ch) - ((GET_LEVEL(vict) - GET_LEVEL(ch)) * 5/2);

  /* your chances are never less than 0% nor more than 95% */

/* Heh, tom, is this what you intended? :)
  chance_to_steal = MAX(0, chance_to_steal);
  chance_to_steal = MIN(95, chance_to_steal);
*/
/* end of hack */

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS */

  /* NO NO With Imp's and Shopkeepers! */
  if ((GET_LEVEL(vict) >= LVL_IMMORT) || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < GET_NUM_WEARS(vict); eq_pos++)
	if (vict->equipment[eq_pos] &&
	    (isname(obj_name, vict->equipment[eq_pos]->name)) &&
	    CAN_SEE_OBJ(ch, vict->equipment[eq_pos])) {
	  obj = vict->equipment[eq_pos];
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	/* if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
	  return;
	} else {
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
          if (vict->equipment[eq_pos])
            obj_to_char(unequip_char(vict, eq_pos), ch);
          else {
            log("SYSERR: unequip_char, act.other.c do_steal()");
            exit(1);
          }
	}*/
        act("$n comes to $s senses and attacks!", FALSE, vict, NULL, NULL, TO_ROOM);
        GET_POS(vict) = POS_STANDING;
        set_fighting(vict, ch);
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (AWAKE(vict) && (percent > chance_to_steal)) {
	ohoh = TRUE;
	act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
	act("$n tried to steal something from you!",
            FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.",
            TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
	  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char("Got it!\r\n", ch);
	  }
	} else
	  send_to_char("You cannot carry that much.\r\n", ch);
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > chance_to_steal)) {
      ohoh = TRUE;
      act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
      act("You discover that $n has $s hands in your wallet.",
          FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
	sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
	send_to_char(buf, ch);

	if (GET_LEVEL(vict) >= GET_LEVEL(ch) && IS_NPC(vict) && GET_CLASS(ch) == CLASS_THIEF)
	    gain_exp(ch, gold);

      } else {
	send_to_char("You couldn't get any gold...\r\n", ch);
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_practice)
{
  void list_skills(struct char_data * ch);

  one_argument(argument, arg);

  if (*arg)
    send_to_char("You do not need to practice anymore!\r\n", ch);
  else
    list_skills(ch);
}



ACMD(do_visible)
{
  int did_something = 0;
  void appear(struct char_data * ch);

  if (affected_by_spell(ch, SPELL_MIRROR_IMAGE)) {
    affect_from_char(ch, SPELL_MIRROR_IMAGE);
    send_to_char("You dispel your remaining images.\r\n", ch);
    did_something = 1;
  }
  if (affected_by_spell(ch, SPELL_INVISIBLE)) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
    did_something = 1;
  }
  if (did_something == 0)
    send_to_char("You are already visible.\r\n", ch);
}



ACMD(do_title)
{
  char *c;
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char("Your title is fine... go away.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strstr(argument, "^\\"))
    send_to_char("Titles can't contain newlines.\r\n", ch);
  else if (strchr(argument, PRETITLE_SEP_CHAR)) {
    sprintf(buf, "Titles can't contain the %c character.\r\n", PRETITLE_SEP_CHAR);
    send_to_char(buf, ch);
  } else if (strlen(argument) > MAX_TITLE_LENGTH) {
    sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
	    MAX_TITLE_LENGTH);
    send_to_char(buf, ch);
  } else if (!strcmp(argument, "default")) {
    set_title(ch, NULL);
    sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
/*  } else if (strlen(argument) == 0) {
    send_to_char("What would you like to set your title to?\r\n", ch);*/
  } else {
/*      set_title(ch, argument);
      sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
      send_to_char(buf, ch);*/
    strcpy(buf, GET_REAL_TITLE(ch));
    c = strchr(GET_REAL_TITLE(ch), PRETITLE_SEP_CHAR);
    if (c) {
      strncpy(c+1, argument, MAX_TITLE_LENGTH);
      buf[MAX_TITLE_LENGTH] = '\0';
    } else {
      strncpy(buf, argument, MAX_TITLE_LENGTH);
    }
    set_title(ch, buf);
    if (c) {
      get_char_pretitle(ch, buf2);
      sprintf(buf, "Okay, you're now %s%s %s.\r\n", buf2, GET_NAME(ch), GET_TITLE(ch));
    } else {
      sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    }
    send_to_char(buf, ch);
  }
}



int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (IS_AFFECTED(vict, AFF_GROUP))
    return 0;

  SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return 1;
}




/* HACKED in many small ways to clean it up */
/* HACKED to show people's positions (sitting, whatever) */
void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;
  
  const char *positions[] = {
    "DEAD!",
    "Mortally wounded",
    "Incapacitated",
    "Stunned",
    "Sleeping",
    "Resting",
    "Sitting",
    "Fighting!",
    "Standing"
  };

  if (!IS_AFFECTED(ch, AFF_GROUP))
    send_to_char("But you are not the member of a group!\r\n", ch);
  else {
    send_to_char("Your group consists of:\r\n", ch);

    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP)) {
      sprintf(buf2, "[ %s%3d/%-3dhp %s%3d/%-3dma %s%3d/%-3dmv%s ]",
        CCTHERMO(ch, GET_HIT(k), GET_MAX_HIT(k)),
        GET_HIT(k), GET_MAX_HIT(k),
        CCTHERMO(ch, GET_MANA(k), GET_MAX_MANA(k)),
        GET_MANA(k), GET_MAX_MANA(k),
        CCTHERMO(ch, GET_MOVE(k), GET_MAX_MOVE(k)),
        GET_MOVE(k), GET_MAX_MOVE(k),
        CCNRM(ch));
      sprintf(buf, "%s [%2d %s %s] $N (Fearless Leader)", buf2, GET_LEVEL(k), 
          RACE_ABBR(k), CLASS_ABBR(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR | TO_SLEEP);
    }

    for (f = k->followers; f; f = f->next) {
      if (!IS_AFFECTED(f->follower, AFF_GROUP))
        continue;

      sprintf(buf2, "[ %s%3d/%-3dhp %s%3d/%-3dma %s%3d/%-3dmv%s ]",
        CCTHERMO(ch, GET_HIT(f->follower), GET_MAX_HIT(f->follower)),
        GET_HIT(f->follower), GET_MAX_HIT(f->follower),
        CCTHERMO(ch, GET_MANA(f->follower), GET_MAX_MANA(f->follower)),
        GET_MANA(f->follower), GET_MAX_MANA(f->follower),
        CCTHERMO(ch, GET_MOVE(f->follower), GET_MAX_MOVE(f->follower)),
        GET_MOVE(f->follower), GET_MAX_MOVE(f->follower),
        CCNRM(ch));
      sprintf(buf, "%s [%2d %s %s] $N (%s)", buf2, GET_LEVEL(f->follower),
            RACE_ABBR(f->follower), CLASS_ABBR(f->follower),
            positions[(int)GET_POS(f->follower)]);
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR | TO_SLEEP);
    }
  }
}



ACMD(do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char("Everyone following you is already in your group.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!IS_AFFECTED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
        act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;
  void stop_follower(struct char_data * ch);


  one_argument(argument, buf);

  if ((!*buf) || !strcmp(buf, "all")) {
    if (ch->master || !(IS_AFFECTED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (IS_AFFECTED(f->follower, AFF_GROUP)) {
	REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
	send_to_char(buf2, f->follower);
        if (!IS_AFFECTED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char("You disband the group.\r\n", ch);
    return;
  }
  if (!(tch = get_char_room_vis(ch, buf))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }

  if (!IS_AFFECTED(tch, AFF_GROUP)) {
    send_to_char("That person isn't in your group.\r\n", ch);
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
  if (!IS_AFFECTED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  struct char_data *k;
  struct follow_type *f;

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not a member of any group!\r\n", ch);
    return;
  }
  sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char(buf, f->follower);
  if (k != ch)
    send_to_char(buf, k);
  send_to_char("You report to the group.\r\n", ch);
}



ACMD(do_split)
{
  int amount, num, share;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char("You don't seem to have that much gold to split.\r\n", ch);
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && IS_AFFECTED(ch, AFF_GROUP))
      share = amount / num;
    else {
      send_to_char("With whom do you wish to share your gold?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
	      amount, share);
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
		amount, share);
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);
    send_to_char(buf, ch);
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}



ACMD(do_use)
{
  struct obj_data *mag_item;
  int equipped = 1;
  
  bool obj_use_prog(struct obj_data *obj, struct char_data *ch);

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = ch->equipment[GET_WEAR_HOLD(ch)];
  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
    case SCMD_EAT_PILL:
      equipped = 0;
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf2, ch);
	return;
      }
      break;
    case SCMD_USE:
      sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      send_to_char(buf2, ch);
      return;
      break;
    default:
      log("SYSERR: Unknown subcmd passed to do_use");
      return;
      break;
    }
  }
  if (obj_use_prog(mag_item, ch)) return;
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("You can only quaff potions.", ch);
      return;
    }
    break;
  case SCMD_EAT_PILL:
    if (GET_OBJ_TYPE(mag_item) != ITEM_PILL) {
      send_to_char("You can only eat food.", ch);
      return;
    }
    break;
  case SCMD_RECITE:
    if (IS_AFFECTED(ch, AFF_RAGE)) {
      send_to_char("You are too enraged to read anything!\r\n", ch);
      return;
    }
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("You can only recite scrolls.", ch);
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char("You can't seem to figure out how to use it.\r\n", ch);
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  int wimp_lev;
  extern int max_wimpy_lev;      /* see config.c */

  one_argument(argument, arg);

    if (GET_LEVEL(ch) > LVL_LOWBIE) {
	send_to_char("Huh?!?\n\r", ch);
	return;
    }

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Your current wimp level is %d hit points.\r\n",
	      GET_WIMP_LEV(ch));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg))) {
      if (wimp_lev < 0)
	send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      else if (wimp_lev > GET_MAX_HIT(ch))
	send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
	send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
      else if (wimp_lev > max_wimpy_lev) {
        sprintf(buf, "You can't set your wimp level above %d hit points.\r\n", max_wimpy_lev);
        send_to_char(buf, ch); 
      } else {
	sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
		wimp_lev);
	send_to_char(buf, ch);
	GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);

  return;
}



/* HACKED to make the display system much more understandable */
const char *display_options[] = {
  "on", "all", "off", "none", "color", "holo", "merc",
  "hit", "mana", "movement", "diagnose", "minmax",
  "gold", "experience", "\n"
};

const int display_bits[] = {
  -1, -1, -1, -1, -1, -1, -1,	/* these are reserved */
  PRF_DISPHP, PRF_DISPMANA, PRF_DISPMOVE, PRF_DISPDIAG, PRF_DISPMINMAX,
  PRF_DISPGOLD, PRF_DISPEXP
};

ACMD(do_display)
{
  int i, m;


  if (IS_NPC(ch)) {
    send_to_char("Monsters don't need displays.  Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "The prompt can be customized using any of these options:");
    for (i = 0; *(display_options[i]) != '\n'; i++) {
      if ((i % 6) == 0)
        strcat(buf, "\r\n");
      sprintf(buf2, "%-12s", display_options[i]);
      strcat(buf, buf2);
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;

  } else if (!strncmp(argument, "on", strlen(argument)) ||
             !strncmp(argument, "all", strlen(argument))) {
    SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | 
            PRF_DISPDIAG | PRF_DISPMINMAX | PRF_DISPGOLD | PRF_DISPEXP );
    send_to_char("Prompt initialized.\r\n", ch);
    return;

  } else if (!strncmp(argument, "off", strlen(argument)) ||
             !strncmp(argument, "none", strlen(argument))) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE |
            PRF_DISPDIAG | PRF_DISPMINMAX | PRF_DISPGOLD | PRF_DISPEXP );
    send_to_char("Prompt cleared.\r\n", ch);
    return;

  } else if (!strncmp(argument, "color", strlen(argument))) {
    if (PRF_FLAGGED(ch, PRF_COLORPROMPT)) {
      REMOVE_BIT(PRF_FLAGS(ch), PRF_COLORPROMPT);
      send_to_char("Prompt color toggled off.\r\n", ch);
    } else {
      SET_BIT(PRF_FLAGS(ch), PRF_COLORPROMPT);
      send_to_char("Prompt color toggled on.\r\n", ch);
    }
    return;

  } else if (!strncmp(argument, "holo", strlen(argument))) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_MERCPROMPT);
    send_to_char("Holo-MUD style prompt set.\r\n", ch);
    return;

  } else if (!strncmp(argument, "merc", strlen(argument))) {
    SET_BIT(PRF_FLAGS(ch), PRF_MERCPROMPT);
    send_to_char("Merc-style prompt set.\r\n", ch);
    return;
  }

  /* else try to match one of the other options */
  for (m = 0; *(display_options[m]) != '\n'; m++)
    if (!strncmp(argument, display_options[m], strlen(argument)))
      break;

  /* check for an accidental reserved match */
  if (display_bits[m] == -1)
    return;

  if (!strcmp(display_options[m], "\n")) {
    sprintf(buf, "Unknown prompt option, try one of:");
    for (i = 0; *(display_options[i]) != '\n'; i++) {
      if ((i % 6) == 0)
        strcat(buf, "\r\n");
      sprintf(buf2, "%-12s", display_options[i]);
      strcat(buf, buf2);
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  /* they chose an option for which there was a good match */ 
  TOGGLE_BIT(PRF_FLAGS(ch), display_bits[m]);
  sprintf(buf, "Prompt option '%s' toggled ", display_options[m]);
  if (IS_SET(PRF_FLAGS(ch), display_bits[m]))
    strcat(buf, "on.\r\n");
  else
    strcat(buf, "off.\r\n");
  send_to_char(buf, ch);
  return;
}



ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, *filename, buf[MAX_STRING_LENGTH];
  struct stat fbuf;
  extern int max_filesize;
  time_t ct;


  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
    return;
  }

  sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  mudlog(buf, CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("Error statting file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n",
        ch);
    sprintf(buf, "SYSERR: %s file full", filename);
    mudlog(buf, CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), FALSE);
    return;
  }

  if (!(fl = fopen(filename, "a"))) {
    perror("do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  world[ch->in_room].number, argument);
  fclose(fl);
  send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1
ACMD(do_gen_tog)
{
  ACMD(do_qcomm);
  void clear_deathmatch_scores();
  
  long result;
  extern int nameserver_is_slow;
  extern int arena_deathmatch_mode;
  extern int arena_deathmatch_level;
  
  char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
     "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
     "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
     "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
     "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
     "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
     "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
     "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
     "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
     "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
     "You are now deaf to the Wiz-channel.\r\n"},
    {"You will no longer see the room flags.\r\n",
     "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
     "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
     "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
     "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
     "Autoexits enabled.\r\n"},
    {"Autoloot disabled.\r\n",
     "Autoloot enabled.\r\n"},
    {"Autosacrificing removed.\r\n",
     "Auto corpse sacrificing enabled.\r\n"},
    {"Autogold disabled.\r\n",
     "Autogold enabled.\r\n"},
    {"Autosplit disabled.\r\n",
     "Autosplit enabled.\r\n"},
    {"Autodirs disabled.\r\n",
     "Autodirs enabled.\r\n"},
    {"Autoassist disabled.\r\n",
     "Autoassist enabled.\r\n"},
    {"You can now hear the clan channel.\r\n",
     "You are now deaf to the clan channel.\r\n"},
    {"You are no longer set away from keyboard.\r\n",
     "You are now set away from keyboard.\r\n"},
    {"You are no longer anonymous.\r\n",
     "You are now anonymous.\r\n"},
    {"Autoscan disabled.\r\n",
     "Autoscan enabled.\r\n"},
    {"Battlebrief disabled.\r\n",
     "Battlebrief enabled.\r\n"},
    {"You will no longer see group reports.\r\n",
     "You will now get group reports every round.\r\n"},
    {"The arena is back to its normal deadly mode.\r\n",
     "The arena is now in DEATHMATCH mode. May chaos reign supreme.\r\n"},
    {"You can now hear the beautiful tunes across Kore.\r\n",
     "You turn off your radio and decide not to face the music.\r\n"},
    {"Automap disabled.\r\n",
     "Automap enabled.\r\n"}
  };


  if (IS_NPC(ch) && subcmd != SCMD_DEATHMATCH)
    return;

  switch (subcmd) {
    case SCMD_NOSUMMON:
        result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
        break;
    case SCMD_NOHASSLE:
        result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
        break;
    case SCMD_BRIEF:
        result = PRF_TOG_CHK(ch, PRF_BRIEF);
        break;
    case SCMD_COMPACT:
        result = PRF_TOG_CHK(ch, PRF_COMPACT);
        break;
    case SCMD_NOTELL:
        result = PRF_TOG_CHK(ch, PRF_NOTELL);
        break;
    case SCMD_NOAUCTION:
        result = PRF_TOG_CHK(ch, PRF_NOAUCT);
        break;
    case SCMD_DEAF:
        result = PRF_TOG_CHK(ch, PRF_DEAF);
        break;
    case SCMD_NOGOSSIP:
        result = PRF_TOG_CHK(ch, PRF_NOGOSS);
        break;
    case SCMD_NOGRATZ:
        result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
        break;
    case SCMD_NOWIZ:
        result = PRF_TOG_CHK(ch, PRF_NOWIZ);
        break;
    case SCMD_ROOMFLAGS:
        result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
        break;
    case SCMD_NOREPEAT:
        result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
        break;
    case SCMD_HOLYLIGHT:
        result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
        break;
    case SCMD_SLOWNS:
        result = (nameserver_is_slow = !nameserver_is_slow);
        break;
    case SCMD_DEATHMATCH:
/*        if (GET_QUEST(ch) == QUEST_OFF) {
          send_to_char("You must be on the quest channel!\r\n", ch);
          return;
        }*/

        if (IS_NPC(ch))
          if (ch->desc || ch->master) return;    /* jarred or charmed */

        result = (arena_deathmatch_mode = !arena_deathmatch_mode);
        if (arena_deathmatch_mode) {
          clear_deathmatch_scores();
          one_argument(argument, buf);
          if (*buf) arena_deathmatch_level = atoi(buf);
          else arena_deathmatch_level = 50;
          arena_deathmatch_level = MAX(MIN(arena_deathmatch_level, 50), 1);
          if (arena_deathmatch_level < 50)
            sprintf(buf, "[ The deathmatch has begun! (Level %d) ]", arena_deathmatch_level);
          else
            sprintf(buf, "[ The deathmatch has begun! ]");
          do_qcomm(ch, buf, 0, SCMD_QECHO);
        } else {
          do_qcomm(ch, 
          "[ The deathmatch is now over. Type 'scores' to see final scores ]",
          0, SCMD_QECHO);
        }
        break;
    case SCMD_AUTOEXIT:
        result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
        break;
    case SCMD_AUTOLOOT:
        result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
        break;
    case SCMD_AUTOSAC:
        result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
        break;
    case SCMD_AUTOGOLD:
        result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
        break;
    case SCMD_AUTOSPLIT:
        result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
        break;
    case SCMD_AUTODIRS:
        result = PRF_TOG_CHK(ch, PRF_AUTODIRS);
        break;
    case SCMD_AUTOASSIST:
        result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
        break;
    case SCMD_NOCLAN:
        result = PRF2_TOG_CHK(ch, PRF2_NOCLAN);
        break;
    case SCMD_AWAY: 
        result = PRF2_TOG_CHK(ch, PRF2_AWAY);
        break;
    case SCMD_ANONYMOUS:
        result = PRF2_TOG_CHK(ch, PRF2_ANONYMOUS);
        break;
/*
    case SCMD_AUTOSCAN:
	result = PRF2_TOG_CHK(ch, PRF2_AUTOSCAN);
        break;
*/
    case SCMD_BATTLEBRIEF:
        result = PRF2_TOG_CHK(ch, PRF2_BATTLEBRIEF);
        break;
    case SCMD_AUTOGROUP:
        result = PRF2_TOG_CHK(ch, PRF2_AUTOGROUP);
        break;
    case SCMD_NOMUSIC:
    	result = PRF2_TOG_CHK(ch, PRF2_NOMUSIC);
    	break;
    case SCMD_AUTOMAP:
        result = PRF2_TOG_CHK(ch, PRF2_AUTOMAP);
        break;
    default:
        log("SYSERR: Unknown subcmd in do_gen_toggle");
        return;
        break;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}



ACMD(do_newbie)
{
  int i;
  int r_num;
  extern int newbie_gear[NUM_CLASSES][10];
  struct obj_data *obj;
  bool given_anything = FALSE;


  if (GET_LEVEL(ch) > 1 && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char("You are not a newbie anymore, ", ch);
    send_to_char("you must make it on your own.\r\n", ch);
    return;
  } 

  if (GET_LEVEL(ch) == 1 && GET_EXP(ch) == 0) {
    GET_EXP(ch) = 1;
  } else if (GET_LEVEL(ch) == 1 && GET_EXP(ch) > 0) {
    send_to_char("Ouch. Either you have already 'newbied' ", ch);
    send_to_char("or you've begun adventuring...\r\n", ch);
    send_to_char("Best beg for equipment if you need some.\r\n", ch);
    return;
  }

  for (i = 0; i < 10; i++) {
    if (newbie_gear[(int) GET_CLASS(ch)][i] == 0)
      continue;
    if ((r_num = real_object(newbie_gear[(int) GET_CLASS(ch)][i])) < 0) {
      sprintf(buf, "SYSERR: Unknown obj %d in do_newbie",
              newbie_gear[(int) GET_CLASS(ch)][i]);
      log(buf);
      continue;
    }
    obj = read_object(r_num, REAL);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
    obj_to_char(obj, ch);
    sprintf(buf, "The gods give you %s.\r\n", obj->short_description);
    send_to_char(buf, ch);
    given_anything = TRUE;
  }
  if (!given_anything)
    send_to_char("The gods do not see fit to give newbie gear to you.\r\n", ch);
  else
    send_to_char(
        "Good luck!\r\n"
        "Don't forget to 'wear all' and to wield and ready your weapons.\r\n"
        "Type 'read guide' or 'help' to get some hints on how to play.\r\n"
        "The newbie zone is: north 1, west 1.\r\n",
        ch);
  return;
}



ACMD(do_quest)
{
  ACMD(do_qcomm);


  if (IS_NPC(ch)) {
    send_to_char("Only players can join the quests!\r\n", ch);
    return;
  }
  switch (GET_QUEST(ch)) {
    case QUEST_OFF:
      GET_QUEST(ch) = QUEST_NORMAL;
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("Okay you are part of the Quest!\r\n", ch);
        sprintf(buf, "[ %s has joined the Quest! ]", GET_NAME(ch)); 
        do_qcomm(ch, buf, 0, SCMD_QECHO);
      } else {
        send_to_char(CCQSAY(ch), ch);
        send_to_char("You secretly join the Quest!\r\n", ch);
        send_to_char(CCNRM(ch), ch);
      }
      break;
    case QUEST_NORMAL:
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        sprintf(buf, "[ %s is no longer part of the Quest... ]", GET_NAME(ch));
        send_to_char("You are no longer part of the Quest...\r\n", ch);
        do_qcomm(ch, buf, 0, SCMD_QECHO);
      } else {
        send_to_char(CCQSAY(ch), ch);
        send_to_char("You secretly quit the Quest...\r\n", ch);
        send_to_char(CCNRM(ch), ch);
      }
      GET_QUEST(ch) = QUEST_OFF;
      break;
    case QUEST_ENROLLED:
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        sprintf(buf, "[ %s is no longer part of the Quest... ]", GET_NAME(ch));
        send_to_char("You are no longer part of the Quest...\r\n", ch);
        do_qcomm(ch, buf, 0, SCMD_QECHO);
      } else {
        send_to_char(CCQSAY(ch), ch);
        send_to_char("You secretly quit the Quest...\r\n", ch);
        send_to_char(CCNRM(ch), ch);
      }
      GET_QUEST(ch) = QUEST_OFF;
      break;
    case QUEST_SURVIVAL:
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        sprintf(buf, "[ %s is no longer part of the Survival Quest... ]",
            GET_NAME(ch));
        send_to_char("You are no longer part of the Survival Quest...\r\n", ch);
        do_qcomm(ch, buf, 0, SCMD_QECHO);
      } else {
        send_to_char(CCQSAY(ch), ch);
        send_to_char("You secretly quit the Survival Quest...\r\n", ch);
        send_to_char(CCNRM(ch), ch);
      }
      GET_QUEST(ch) = QUEST_OFF;
      break;
    case QUEST_PKQUEST:
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You cannot leave a Player Killer Quest "
                     "voluntarily!\r\n", ch);
      } else {
        send_to_char(CCQSAY(ch), ch);
        send_to_char("You secretly quit the Player Killer Quest...\r\n", ch);
        send_to_char(CCNRM(ch), ch);
        GET_QUEST(ch) = QUEST_OFF;
      }
      break;
    case QUEST_DEATHQUEST:
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You cannot leave a Death Quest voluntarily!\r\n", ch);
      } else {
        send_to_char(CCQSAY(ch), ch);
        send_to_char("You secretly quit the Death Quest...\r\n", ch);
        send_to_char(CCNRM(ch), ch);
        GET_QUEST(ch) = QUEST_OFF;
      }
      break;
    default:
      send_to_char("Your quest status is rather strange, you should talk"
          "to an god.\r\n", ch);
      GET_QUEST(ch) = QUEST_OFF;
      break;
  }
}



#define NUM_SCROUNGEABLES	25
#define LOAD_MOB_HIT		0
#define LOAD_MOB_FLEE		1
#define LOAD_OBJ		2
#define SCROUNGE_MOVE_COST	10
ACMD(do_scrounge)
{
  byte percent;
  int i;
  int r_num;
  struct char_data *mob;
  struct obj_data *obj;
  int scrounge_table[NUM_SCROUNGEABLES][2] = {
    { LOAD_MOB_HIT, 3750 },	/* the sewer rat */
    { LOAD_MOB_HIT, 3752 },	/* the black fly */
    { LOAD_OBJ, 12555 },	/* a rotten piece of meat */
    { LOAD_OBJ, 137 },		/* a piece of green meat */
    { LOAD_OBJ, 4104 },		/* a green slime mould */
    { LOAD_OBJ, 138 },		/* a jar of yellow mayonnaise */
    { LOAD_OBJ, 139 },		/* a meaty bone */
    { LOAD_OBJ, 3015 },		/* a piece of meat */
    { LOAD_OBJ, 141 },		/* the dregs of someones beer */
    { LOAD_OBJ, 3032 },		/* a bag (wanted a single coin) */
    { LOAD_OBJ, 142 },		/* a gold coin */
    { LOAD_OBJ, 5457 },		/* a carrot */
    { LOAD_OBJ, 144 },		/* half an apple */
    { LOAD_OBJ, 134 },		/* a wormy apple */
    { LOAD_OBJ, 145 },		/* a soft tomato */
    { LOAD_OBJ, 146 },		/* a slice of moldy bread */
    { LOAD_OBJ, 5458 },		/* a tomato */
    { LOAD_OBJ, 131 },		/* a hunk of moldy cheese */
    { LOAD_OBJ, 143 },		/* a lump of stinky cheese */
    { LOAD_OBJ, 147 },		/* a half-eaten sandwich */
    { LOAD_OBJ, 25511 },	/* an apple pastry */
    { LOAD_OBJ, 148 },		/* a cherry tart */
    { LOAD_OBJ, 149 },		/* a slice of hot apple pie */
    { LOAD_OBJ, 3001 },		/* a bottle (of beer) */
    { LOAD_OBJ, 3702 } 		/* a chicken leg */
  };

  ACMD(do_flee);

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You should leave the scrounging to the experts.\r\n", ch);
    return;
  }

  if (!IS_THIEF(ch) && !IS_DRUID(ch)) {
    send_to_char("You should leave the scrounging to the experts.\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("But the love you have for master is so strong!\r\n", ch);
    return;
  }

  if (world[ch->in_room].sector_type != SECT_CITY) {
    send_to_char("You scrounge best in the city.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS)) {
    send_to_char("You find the best stuff outdoors in the gutters.\r\n", ch);
    return;
  }
  if (GET_MOVE(ch) < SCROUNGE_MOVE_COST) {
    send_to_char("You are too exhausted!\r\n", ch);
    return;
  } else {
    GET_MOVE(ch) -= SCROUNGE_MOVE_COST;
  };

  percent = number(1, 101);     /* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SCROUNGE)) {
    send_to_char("You rummage around in the gutter "
                 "but dont find anything.\r\n", ch);
    return;
  }

  i = number(0, NUM_SCROUNGEABLES - 1);
  
  switch (scrounge_table[i][0]) {
    case LOAD_MOB_HIT:
        send_to_char("You scrounge through the garbage "
                     "and stir up a critter!\r\n", ch);
        if ((r_num = real_mobile(scrounge_table[i][1])) < 0) {
          sprintf(buf, "do_scrounge: no such mobile #%d", scrounge_table[i][1]);
          log(buf);
          return;
        }
        mob = read_mobile(r_num, REAL);
        char_to_room(mob, ch->in_room);
        IS_CARRYING_W(mob) = 0;
        IS_CARRYING_N(mob) = 0;
        set_fighting(mob, ch);
        break;
    case LOAD_MOB_FLEE:
        send_to_char("You scrounge through the garbage "
                     "and surprise a critter!\r\n", ch);
        if ((r_num = real_mobile(scrounge_table[i][1])) < 0) {
          sprintf(buf, "do_scrounge: no such mobile #%d", scrounge_table[i][1]);
          log(buf);
          return;
        }
        mob = read_mobile(r_num, REAL);
        char_to_room(mob, ch->in_room);
        IS_CARRYING_W(mob) = 0;
        IS_CARRYING_N(mob) = 0;
        do_flee(mob, "", 0, 0);
        break;
    case LOAD_OBJ:
        if ((r_num = real_object(scrounge_table[i][1])) < 0) {
          sprintf(buf, "do_scrounge: no such object #%d", scrounge_table[i][1]);
          log(buf);
          return;
        }
        obj = read_object(r_num, REAL);
        obj_to_room(obj, ch->in_room);
        act("$n has scrounged up $p!", FALSE, ch, obj, 0, TO_ROOM);
        act("You scrounge up $p!", FALSE, ch, obj, 0, TO_CHAR);
        break;
    default:
        sprintf(buf, "do_scrounge: unknown option");
        log(buf);
        break;
  };
}



#define HEAL_MANA_COST 30
ACMD(do_heal)
{
  struct char_data *vict;
  int spell = SPELL_CURE_LIGHT;   /* default 'heal wounds' level */
  int healadd = 0;


  half_chop(argument, arg, buf);

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You'd better leave the healing to knights.\r\n", ch);
    return;
  }

  if (!IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the healing to knights.\r\n", ch);
    return;
  }

  if (!*arg) {
    send_to_char("Cannot find the target of your heal!\r\n", ch);
    return;
  }

  /* find target */
  vict = get_char_room_vis(ch, buf);
  if (vict == NULL)
    vict = ch;

  /* figure out what spell to cast */
  if (!str_cmp(arg, "wounds")) {
    if (GET_LEVEL(ch) > 40) {
      healadd = 40;
      spell = SPELL_HEAL;
    } else if (GET_LEVEL(ch) > 30) {
      healadd = 30;
      spell = SPELL_CURE_CRITIC;
    } else {
      healadd = 20;
      spell = SPELL_CURE_SERIOUS;
    }
 
  } else if (!str_cmp(arg, "poison")) {
      spell = SPELL_REMOVE_POISON;

  } else if (!str_cmp(arg, "blind")) {
      spell = SPELL_CURE_BLIND;

  } else if (!str_cmp(arg, "curse")) {
      spell = SPELL_REMOVE_CURSE;

  } else {
        send_to_char("You only know how to heal "
                "wounds, poison, blind and curse.\r\n", ch);
        return;
  }

  /* check their mana */
  if ((GET_MANA(ch) < HEAL_MANA_COST + healadd) && (GET_LEVEL(ch) <
LVL_IMMORT)) {
    send_to_char("You haven't the energy to heal!\r\n", ch);
    return;
  }

  /* make some noises in the room */
  switch (spell) {
      case SPELL_CURE_CRITIC:
      case SPELL_CURE_SERIOUS:
      case SPELL_CURE_LIGHT:
      case SPELL_HEAL:
          if (vict == ch) {
              act("You lay your hands on yourself to heal your wounds.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n lays $s hands on $mself to heal $s wounds.",
                      FALSE, ch, NULL, vict, TO_ROOM);
          } else {
              act("You lay your hands on $N to heal $S wounds.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n lays $s hands on $N to heal $S wounds.",
                      FALSE, ch, NULL, vict, TO_NOTVICT);
              act("$n lays $s hands on you to heal your wounds.",
                      FALSE, ch, NULL, vict, TO_VICT);
          }
          break;
      case SPELL_REMOVE_POISON:
          if (vict == ch) {
              act("You lay your hands on yourself to stop your shivering.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n lays $s hands on $mself to stop $s shivering.",
                      FALSE, ch, NULL, vict, TO_ROOM);
          } else {
              act("You lay your hands on $N to stop $S shivering.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n lays $s hands on $N and stops $S shivering.",
                      FALSE, ch, NULL, vict, TO_NOTVICT);
              act("$n lays $s hands on you to stop your shivering.",
                      FALSE, ch, NULL, vict, TO_VICT);
          }
          break;
      case SPELL_CURE_BLIND:
          if (vict == ch) {
              act("You lay your hands on yourself to restore your sight.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n lays $s hands on $mself to restore $s sight.",
                      FALSE, ch, NULL, vict, TO_ROOM);
          } else {
              act("You lay your hands on $N's eyes to restore $S sight.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n lays $s hands on $N's eyes to restores $S sight.",
                      FALSE, ch, NULL, vict, TO_NOTVICT);
              act("$n lays $s hands on you to restore your sight.",
                      FALSE, ch, NULL, vict, TO_VICT);
          }
          break;
      case SPELL_REMOVE_CURSE:
          if (vict == ch) {
              act("You attempt to lift the curse that lays heavy on you.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n attempts to lift the curse that lays heavy on $m.",
                      FALSE, ch, NULL, vict, TO_ROOM);
          } else {
              act("You attempt to lift the curse that lays heavy on $N.",
                      FALSE, ch, NULL, vict, TO_CHAR);
              act("$n attempts lifts the curse that lays heavy on $N.",
                      FALSE, ch, NULL, vict, TO_NOTVICT);
              act("$n attempts to lift the curse that lays heavy on you.",
                      FALSE, ch, NULL, vict, TO_VICT);
          }
          break;
      default:
          return;
          break;
  }

  /* charge the mana cost */
  GET_MANA(ch) = MAX(GET_MANA(ch) - HEAL_MANA_COST - healadd, 0);

  /* cast spell */
  call_magic(ch, vict, NULL, NULL, spell, GET_LEVEL(ch), CAST_SPELL);

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);

}



/*
 * This function picks a random room from 1300-1399
 * sort of hokey but kind of works
 * I give it 10,000 tries too.  This should be plenty.
 */
/* #define ARENA_ROOM 1300 */
ACMD(do_battle)
{
  extern int arena_base, arena_deathmatch_mode, arena_deathmatch_level;
  int room = -1;
  int tries = 0;


  if (IS_NPC(ch)) {
    send_to_char("Monsters don't go and do 'battle', that's lame.\r\n", ch);
    return;
  }

  if (FIGHTING(ch)) {
    send_to_char("You're busy fighting already!\r\n", ch);
    return;
  }

  if ((world[ch->in_room].number / 100) == (arena_base / 100)) {
    send_to_char("You are already in the arena!\r\n", ch);
    return;
  }
  
  if (!ZONE_FLAGGED(ch->in_room, ZONE_BATTLEOK)) {
    send_to_char("You can't go to the arena from here!\r\n", ch);
    return;
  }
  
  if ((arena_deathmatch_mode) &&
      (GET_LEVEL(ch) > arena_deathmatch_level)) {
/*      (!IS_MORT(ch)) { */
      send_to_char("You're too powerful to participate!\r\n", ch);
      return;
  }

  switch (subcmd) {
    case SCMD_BATTLE:
        send_to_char("WARNING: the arena is a player killing (PK) arena!\r\n"
                     "Do you really want to fight in the PK arena (Y/N)? ", ch);
        STATE(ch->desc) = CON_BATTLE_VRFY;
        break;
    case SCMD_BATTLE_YES:
        while (room < 1) {
          room = real_room(number(arena_base, arena_base+99));
          if (room >= 1) if ROOM_FLAGGED(room, ROOM_NOTELEPORT) room = 0;
          if (++tries == 10000)
            return;
        }
/*
        if (arena_deathmatch_mode) {
          if (ch->affected) {
    	    while (ch->affected)
    	      affect_remove(ch, ch->affected);
          }
        }
*/
	do_follow(ch, "self", 0, 0);
        send_to_char("You leave to join the battle!\r\n", ch);
        act("$n leaves to join the battle!", FALSE, ch, 0, 0, TO_ROOM);
        char_from_room(ch);
        char_to_room(ch, room); 
        look_at_room(ch, 0);        
        act("$n joins the battle!", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case SCMD_BATTLE_NO:
        /* send_to_char("Another time.\r\n", ch); */
        break; 
    default:
        break;
  }
}



ACMD(do_palm)
{
  struct obj_data *obj;
  byte percent, prob;
  int cont_dotmode;

  bool obj_get_prog(struct obj_data *obj, struct char_data *ch);

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You'd better leave the sneaky stuff to thieves.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_THIEF(ch)) {
    send_to_char("You'd better leave the sneaky stuff to thieves.\r\n", ch);
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char("This is an accursed place, take nothing from here!\r\n", ch);
    return;
  } else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char("Your arms are already full!\r\n", ch);
    return;
  } else if (!*argument) {
    send_to_char("Palm what?\r\n", ch);
    return;
  }
    
  skip_spaces(&argument);

  argument = one_argument(argument, arg);

  cont_dotmode = find_all_dots(arg);
  if (cont_dotmode == FIND_ALLDOT) {
    send_to_char("You can only palm one thing at a time.\r\n", ch);
    return;
  }

  if ((obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)) == NULL) {
    send_to_char("You do not see that here.\r\n", ch);
    return;
  }

  percent = number(1, 101);     /* 101% is a complete failure */
  prob = ((GET_DEX(ch) * 4) - 5);

  if (percent > prob) {
    act("You try to palm $p, but fail miserably.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n tries to palm $p, but fails miserably.", TRUE, ch, obj, 0, TO_ROOM);
  } else {
    if (can_take_obj(ch, obj)) {
      if (obj_get_prog(obj, ch)) return; /* Exec objprog, check for stop_action */
      obj_from_room(obj);
      obj_to_char(obj, ch);
      act("You quietly palm $p.", FALSE, ch, obj, 0, TO_CHAR);
      get_check_money(ch, obj);
    }
  }
}



#if(0)
#error --- THIS CODE DISABLED to make room for the new conceal code
#error --- The new code is ACMD(do_conceal) in act.obj.c
ACMD(do_conceal)
{
  struct obj_data *obj;
  int pos;
  int cont_dotmode;


  skip_spaces(&argument);

  argument = one_argument(argument, arg);

  cont_dotmode = find_all_dots(arg);
  if (cont_dotmode == FIND_ALLDOT) {
    send_to_char("You can only conceal one thing at a time.\r\n", ch);
    return;
  }

  if (!(obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos))) {
    send_to_char("Conceal what?\r\n", ch);
    return;
  }

  act("You conceal $p.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n conceals $p.", TRUE, ch, obj, 0, TO_ROOM);
  SET_BIT(GET_OBJ_EXTRA(obj), ITEM_CONCEALED);
}
#endif


ACMD(do_forgive)
{
  struct char_data *vict;

  skip_spaces(&argument);

  if (IS_NPC(ch)) {
    send_to_char("Monsters never forgive.\r\n", ch);
  } else if (GET_KILLER_TO_FORGIVE(ch) == NOBODY) {
    send_to_char("You haven't the capacity to forgive anyone right now.\r\n",
        ch);
  } else if (!*argument) {
    send_to_char("Who do you wish to forgive?\r\n", ch);
  } else if (!(vict = get_char_vis(ch, argument))) {
    send_to_char(NOPERSON, ch);
  } else if (ch == vict) {
    send_to_char("Try to forgive yourself?!?", ch);
  } else if (!PLR_FLAGGED(vict, PLR_KILLER)) {
    send_to_char("The accused doesn't appear to be a killer.\r\n", ch);
  } else if (GET_IDNUM(vict) == GET_KILLER_TO_FORGIVE(ch)) {
    REMOVE_BIT(PLR_FLAGS(vict), PLR_KILLER);
    send_to_char("Forgiven.\r\n", ch);
    sprintf(buf, "You have been forgiven by %s!\r\n", GET_NAME(ch));
    send_to_char(buf, vict);
    sprintf(buf, "PC %s forgiven by %s", GET_NAME(vict), GET_NAME(ch));
    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
  } else {
    send_to_char("No, the person that wronged you was someone else.\r\n", ch);
  }
}

ACMD(do_bandage) {
  struct char_data *vict;
  const double bandage_percent = 0.2; /* How healthy you can make a guy */

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You're a MONSTER, you don't HELP people!\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_THIEF(ch)) {
    send_to_char("You aren't trained; you might make it worse!\r\n", ch);
    return;
  }
  
  one_argument(argument, arg);
  vict = get_char_room_vis(ch, arg);
  
  if (!vict) {
    send_to_char("Nice of you, but your patient isn't around!!\r\n", ch);
    return;
  }
  
  if ((double)GET_HIT(vict) / (double)GET_MAX_HIT(vict) > bandage_percent) {
    act("%N's pretty healthy; you can't help %M!", FALSE, ch, NULL, vict, TO_CHAR);
    return;
  }
  
  act("You bandage %N's wounds.", FALSE, ch, NULL, vict, TO_CHAR);
  return;
}

ACMD(do_yank) {
  struct char_data *vict;

  one_argument(argument, arg);
  vict = get_char_room_vis(ch, arg);
  
  if (!vict) {
    send_to_char("You haul at the air for a while, but it just won't stand up.\r\n", ch);
    return;
  }

  if (FIGHTING(vict) || FIGHTING(ch)) {
    send_to_char("You can't do that during combat.\n\r", ch);
    return;
  }

  if (GET_POS(vict) < POS_RESTING) {
	act ("$E is in no condition to be standing.", FALSE, ch, NULL, vict, TO_CHAR);
	return;
  }

  if (AFF_FLAGGED(vict, AFF_GROUP) && vict->master == ch) {
    if (GET_POS(vict) == POS_STANDING) {
      act ("$E's already standing up!", FALSE, ch, NULL, vict, TO_CHAR);
    } else {
      act("You yank $M to $S feet.", FALSE, ch, NULL, vict, TO_CHAR);
      act("$n yanks you to your feet.", FALSE, ch, NULL, vict, TO_VICT);
      act("$n yanks $N to $S feet.", FALSE, ch, NULL, vict, TO_NOTVICT);
      GET_POS(vict) = POS_STANDING;
    }
  } else {
    act("$E's not under your command! Leave $M alone!", FALSE, ch, NULL, vict, TO_CHAR);
  }
  
  return;
}

ACMD(do_custprompt) {

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (ch->player_specials->prompt) free(ch->player_specials->prompt);
  if (!*argument) {
    ch->player_specials->prompt = NULL;
    send_to_char("Custom prompt removed.\r\n", ch);
  } else {
    ch->player_specials->prompt = strdup(argument);
    send_to_char("Ok.\r\n", ch);
  }
}
