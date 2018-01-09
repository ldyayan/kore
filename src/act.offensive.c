
/************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern int pk_allowed;
extern struct spell_info_type spell_info[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
int number_range(int from, int to);
int do_web_check(struct char_data * ch);
extern char *dirs[];
bool is_safe( struct char_data *ch, struct char_data *victim, bool show_messg);

/* extern functions */
void raw_kill(struct char_data * ch, struct char_data * killer);
void perform_remove(struct char_data * ch, int pos);
void add_follower(struct char_data * ch, struct char_data * leader);
void perform_wear(struct char_data * ch, struct obj_data * obj, int where,
    int dotmode);
void mprog_death_trigger(struct char_data * mob, struct char_data * killer);



ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char("You're already fighting!  "
        "How can you assist someone else?\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Whom do you wish to assist?\r\n", ch);
  else if (!(helpee = get_char_room_vis(ch, arg)))
    send_to_char(NOPERSON, ch);
  else if (helpee == ch)
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  else {
    for (opponent = world[ch->in_room].people;
	 opponent && (FIGHTING(opponent) != helpee);
	 opponent = opponent->next_in_room)
		;

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!pk_allowed && !IS_NPC(opponent) && !IS_NPC(ch)) {
      /* prevent accidental pkill */
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    } else {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}



ACMD(do_hit)
{
  struct char_data *vict;
  extern bool is_nearby(struct char_data * ch, struct char_data * i);
  int wear_ready;
  
  wear_ready = IS_THRIKREEN(ch)? THRI_WEAR_READY : WEAR_READY;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Hit who?\r\n", ch);
/*
  else if (!(vict = get_char_room_vis(ch, arg)) && 
           !(vict = get_char_nearby_vis(ch,arg)))
*/
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be around here.\r\n", ch);


  else if ((GET_POS(vict) <= POS_DEAD) && (GET_LEVEL(ch) < LVL_IMPL)) {
    log("SYSERR: Attempt to kill a corpse, victim is already dead.");
    return;                     /* -je, 7/7/92 */
  } 
            
  else if ((vict->in_room == NOWHERE) && (GET_LEVEL(ch) < LVL_IMPL)) {
    log("SYSERR: Attempt to kill a corpse, victim is nowhere.");
    return;
  } 



  else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict)) {
    act("$N is just such a good friend, you simply can't hit $M.",
        FALSE, ch, 0, vict, TO_CHAR);
  } else if (is_safe(ch, vict, TRUE)) {
    return;
  } else {
    if (!pk_allowed) {
      if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER) &&
                        !IS_CHAOS_ROOM(vict->in_room)) {
	send_to_char("Use 'murder' to hit another player.\r\n", ch);
	return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM))
        if (!IS_NPC(ch->master) && !IS_NPC(vict)) 
	  return;	/* you can't order a charmed pet to attack a player */
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
/*
      if (is_nearby(ch, vict) && !(ch->equipment[wear_ready])) {
        send_to_char("Your target is too far away!\r\n", ch);
        return;
      }
*/
      hit(ch, vict, TYPE_UNDEFINED);
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return;
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }
}



ACMD(do_kill)
{
  struct char_data *vict;

  if ((GET_LEVEL(ch) < LVL_IMPL) || IS_NPC(ch) || IS_CHAOS_ROOM(ch->in_room)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_room_vis(ch, arg)) && 
        !(vict = get_char_nearby_vis(ch, arg)))
      send_to_char("They aren't around here.\r\n", ch);
    else if (ch == vict)
      send_to_char("I don't think that's such a good idea.. :(\r\n", ch);
    else {
      act("You reach into $N's chest and remove $S soul!",
          FALSE, ch, 0, vict, TO_CHAR);
      act("$N reaches into your chest and pulls out your soul!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n reaches into $N's chest and pulls out $S soul!", FALSE, ch, 0, vict, TO_NOTVICT);
      /* MOBProg foo */
      if (ch)
        mprog_death_trigger(vict, ch);
      raw_kill(vict, ch);
    }
  }
}



ACMD(do_backstab)
{
  struct char_data *vict;
  byte percent, prob; 
  char buf[MAX_INPUT_LENGTH];
  int wear_wield = IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD;


  *buf = '\0';

  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Your head swims...\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_THIEF(ch) && !IS_BARD(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_MIRROR_IMAGE)) {
    send_to_char("You cant backstab with so many images scaring them!\r\n", ch);
    return;
  }

  if (!*arg) {
    send_to_char("I dont see that person here.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("They aren't around here.\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("How can you sneak up on yourself?\r\n", ch);
    return;
  }

  if (!ch->equipment[wear_wield]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }

  if ((GET_OBJ_VAL(ch->equipment[wear_wield], 3) != TYPE_PIERCE - TYPE_HIT) && 
    (GET_OBJ_VAL(ch->equipment[wear_wield], 3) != TYPE_STAB - TYPE_HIT)) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }

  if (FIGHTING(vict)) {
    send_to_char("You can't backstab a fighting person -- "
        "they're too alert!\r\n", ch);
    return;
  }   

  if (is_safe(ch, vict, TRUE)) {
    return;
  }

  /*
   * they cant leave part-way through the mobs return attack
   * if the mob dies from it though, it'll get set back to normal.
   * and it'll become normal once its ch's turn to fight like normal.
   */
  GET_CANT_WIMPY(ch) = 1;

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BACKSTAB);

  if (!IS_NPC(ch) && AWAKE(vict) && (percent > prob))
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}



ACMD(do_block)
{
  struct char_data *vict;
  byte percent, prob;
  int wear_wield = IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD;


  one_argument(argument, buf);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_BARD(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }
  
  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Block who?\r\n", ch);
      return;
    }
  }
  if (!ch->equipment[wear_wield]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (FIGHTING(vict) != ch) {
    send_to_char("You can't block their attacks, "
                 "they're not fighting you!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n tries to block but swats $mself and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOBLOCK)) {
    damage(ch, vict, 0, SKILL_BLOCK);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BLOCK);

  if (AWAKE(vict) && (percent > prob)) {
    damage(ch, vict, 0, SKILL_BLOCK);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
  } else {
    GET_POS(vict) = POS_SLEEPING;
    damage(ch, vict, 5, SKILL_BLOCK);
    GET_BLOCKED(vict) = 1;
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
    return;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
} 



ACMD(do_riposte)
{
  struct char_data *vict;
  byte percent, prob;
  int wear_wield = IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD;


  one_argument(argument, buf);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Riposte whose attack?\r\n", ch);
      return;
    }
  }
  if (!ch->equipment[wear_wield]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (FIGHTING(vict) != ch) {
    send_to_char("You can't riposte their attacks, "
                 "they're not fighting you!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n tries to riposte but bobbles and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOBLOCK)) {
    damage(ch, vict, 0, SKILL_RIPOSTE);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3); 
    return;
  }
  
  percent = number(1, 101);     /* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_RIPOSTE);

  if (AWAKE(vict) && (percent > prob)) {
    /* failed the riposte */
    damage(ch, vict, 0, SKILL_RIPOSTE);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3); 
  } else {
    act("You deftly bat aside $N's attack and quickly counter.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n deftly parries your attack and quickly counters.", FALSE, ch, 0, vict, TO_VICT);
    act("$n deftly parries $N's attack and quickly counters.", FALSE, ch, 0, vict, TO_NOTVICT);
    if (vict) {
      hit(ch, vict, SKILL_RIPOSTE);
      GET_POS(vict) = POS_SLEEPING; 
    }
    GET_BLOCKED(vict) = 1;
    GET_POS(vict) = POS_STANDING;
    WAIT_STATE(ch, PULSE_VIOLENCE * 3); 
    return;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}



ACMD(do_stun)
{
  struct char_data *vict;
  byte percent, prob;
/*
  int wear_wield = IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD;
*/

  one_argument(argument, buf);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_THIEF(ch)) {
    send_to_char("You'd better leave the sneaky stuff to thieves.\r\n", ch);
    return;
  }
  
  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Stun who?\r\n", ch);
      return;
    }
  }
/* bullshit
  if (!ch->equipment[wear_wield]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
*/
  if (is_safe(ch, vict, TRUE))
    return;

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n lunges out to grip $N in a grip but then trips and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }
  
  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_STUN_TOUCH);

  if (AWAKE(vict) && (percent > prob)) {
    damage(ch, vict, 0, SKILL_STUN_TOUCH);
  } else {
    SET_BIT(AFF_FLAGS(vict), AFF_STUN);
    damage(ch, vict, 5, SKILL_STUN_TOUCH);
    WAIT_STATE(ch, PULSE_VIOLENCE * 4);
    return;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
} 



ACMD(do_circle)
{
  struct char_data *vict;
  byte percent, prob;
  int wear_wield = IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD;


  one_argument(argument, buf);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_THIEF(ch)) {
    send_to_char("You'd better leave the sneaky stuff to thieves.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Circle who?\r\n", ch);
      return;
    }
  }

  if (vict == ch) {
    send_to_char("How can you circle yourself?\r\n", ch);
    return;
  }

  if (is_safe(ch, vict, TRUE))
    return;

  if (!ch->equipment[wear_wield]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if ((GET_OBJ_VAL(ch->equipment[wear_wield], 3) != TYPE_PIERCE - TYPE_HIT) &&
    (GET_OBJ_VAL(ch->equipment[wear_wield], 3) != TYPE_STAB - TYPE_HIT)) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }
  if (ch == FIGHTING(vict)) {
    send_to_char("You can't circle them, they're fighting you!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n tries to circle around $N but goes wide, trips, and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_CIRCLE);

  if (AWAKE(vict) && (percent > prob))
    damage(ch, vict, 0, SKILL_CIRCLE);
  else
    hit(ch, vict, SKILL_CIRCLE);

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}



ACMD(do_order)
{
  char name[100], message[256];
  char buf[256];
  bool found = FALSE;
  int org_room;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Order who to do what?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, name)) && !is_abbrev(name, "followers"))
    send_to_char("That person isn't here.\r\n", ch);
  else if (ch == vict)
    send_to_char("You obviously suffer from schizophrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);

      if ((vict->master != ch) || !IS_AFFECTED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(OK, ch);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
/*
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, vict, TO_ROOM);
*/

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
	if (org_room == k->follower->in_room)
	  if (IS_AFFECTED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(OK, ch);
      else
	send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}



ACMD(do_bite)
{
  int diff;
  struct char_data *vict;
  ACMD(do_thri_bite);

  if (IS_THRIKREEN(ch)) {
    do_thri_bite(ch, argument, cmd, subcmd);
    return;
  }

  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_UNDEAD(ch))  {
    send_to_char("You'd better leave ghastly stuff to the undead!\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Bite who?\r\n", ch);
      return;
    }
  }

  if (is_safe(ch, vict, TRUE))
    return;

  if (GET_MANA(ch) < 25) {
    send_to_char("You do not have enough energy to do that!\r\n", ch);
    return;
  } else {
    GET_MANA(ch) -= 25;
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }

  if (vict == ch) {
    send_to_char("You foolishly tear a large hole in your flesh.\r\n", ch);
    send_to_char("Your life-blood rushes out of you into a pool on the ground.\r\n", ch);
    act("$n rips a large hole in $s flesh and drops to the ground bleeding.",
	FALSE, ch, 0, vict, TO_ROOM);

    GET_POS(ch) = POS_STUNNED;
    GET_HIT(ch) = 0;
    GET_MANA(ch) = 0;
    GET_MOVE(ch) = 0;
    GET_COND(ch, THIRST) = 0;
    WAIT_STATE(ch, PULSE_VIOLENCE * 10);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n shows $s teeth and lunges for $N but then trips and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  diff = (GET_LEVEL(ch) - GET_LEVEL(vict));
  act("$n jumps on $N's back and bites $S neck!", FALSE, ch, 0, vict, TO_ROOM);
  send_to_char("You suck their blood!\r\n", ch);

  if ((diff > 14) && (GET_LEVEL(vict) < 10))  {
    GET_HIT(ch) += number(1, 100);
    GET_HIT(vict) -= number_range(1, 100);
    GET_COND(ch, THIRST) += 10;
    send_to_char("Life-giving blood flows over your lips.\r\n", ch);
  } else if (diff > 14) {
    GET_HIT(ch) += number_range(1, 50);
    GET_COND(ch, THIRST) += 10;
    GET_HIT(vict) -= number_range(1, 50);
    send_to_char("Life-giving blood flows over your lips.\r\n", ch);
  } else {
    GET_COND(ch, THIRST) += 10;
    send_to_char("Life-giving blood flows over your lips.\r\n", ch);
  }

  if (IS_AFFECTED(ch, AFF_RAGE)) {
    if (GET_HIT(ch) > (GET_MAX_HIT(ch) + GET_LEVEL(ch) * 2))
	GET_HIT(ch) = (GET_MAX_HIT(ch) + GET_LEVEL(ch) * 2);
  } else {
    if (GET_HIT(ch) > GET_MAX_HIT(ch))
	GET_HIT(ch) = GET_MAX_HIT(ch);
  }

  if (!FIGHTING(ch) || !FIGHTING(vict)) {
    hit(ch, vict, TYPE_UNDEFINED);
  }
 
  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
} 

ACMD(do_thri_bite) {
  struct char_data *vict;
  struct affected_type af;
  void check_killer(struct char_data *ch, struct char_data *vict);

  if (!IS_THRIKREEN(ch)) {
    /* huh? */
    send_to_char("Better leave that to the thrikreens!\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  
  if (!*arg) {
    send_to_char("Who would you like to bite?\r\n", ch);
    return;
  }
  
  vict = get_char_room_vis(ch, arg);
  
  if (!vict) {
    send_to_char("Erm...who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("You can't bite yourself - your neck just doesn't move that way.\r\n", ch);
    return;
  }
  
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
  
  if (number(1, 50) > GET_DEX(ch)) {
    act("$N nimbly steps out of the way as $n tries to bite $M!", FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n tries to bite you, but you dance away in defense!", FALSE, ch, 0, vict, TO_VICT);
    act("Learn how to bite, you buffoon! You missed $M!", FALSE, ch, 0, vict, TO_CHAR);
    if (!FIGHTING(vict)) {
      check_killer(ch, vict);
      hit (vict, ch, TYPE_UNDEFINED);
    } else if (!FIGHTING(ch)) {
      hit (ch, vict, TYPE_UNDEFINED);
    }
    return;
  }
  
  act("$n quickly advances and bites into $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  act("$n makes a quick move and bites you!", FALSE, ch, 0, vict, TO_VICT);
  act("You sink your teeth into $N!", FALSE, ch, 0, vict, TO_CHAR);
  
  if (IS_AFFECTED(vict, AFF_POISON)) {
    act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("Nothing seems to happen.\r\n", ch);
    return;
  }
  
  if (!IS_AFFECTED(vict, AFF_POISON)) { /* we need another check */
    af.type = SPELL_POISON;
    af.duration = GET_LEVEL(ch)/8;
    af.modifier = -1;
    af.location = APPLY_STR;
    af.bitvector = AFF_POISON;
    af.bitvector2 = 0;
    affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);
 
    send_to_char("You feel very sick.\r\n", vict);
    act("$n visibly weakens as the poison enters $s body!", TRUE, vict, 0, 0, TO_ROOM);
  } else {
    act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("Nothing seems to happen.\r\n", ch);
  }
  
  if (!FIGHTING(vict)) {
    check_killer(ch, vict);
    hit (vict, ch, TYPE_UNDEFINED);
  } else if (!FIGHTING(ch)) {
    hit (ch, vict, TYPE_UNDEFINED);
  }
  
}
     
ACMD(do_gaze)
{
  ACMD(do_flee);
  int diff, num_followers = 0;
  struct char_data *vict;
  struct follow_type *f;
  extern int cha_max_followers[26];

  for (f = ch->followers; f; f = f->next) {
    if (IS_NPC(f->follower))
      num_followers++;
  }

  one_argument(argument, arg);

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You cannot gaze anything.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_UNDEAD(ch))  {
    send_to_char("You'd better leave ghastly stuff to the undead!\r\n", ch);
    return;
  }
 
  if (!(FIGHTING(ch))) {
     send_to_char("But you are not fighting them!\r\n", ch);
     return;
  } 

  if (GET_MANA(ch) < 35) {
    send_to_char("You do not have enough energy to do that!\r\n", ch);
    return;
  } else {
    GET_MANA(ch) -= 35;
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Gaze at whom?\r\n", ch);
      return;
    }
  }

  if (IS_AFFECTED(vict, AFF_CURSE)) {
    send_to_char("You have allready gazed at them once and they resist!\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n stares really hard at $N and then falls down from the effort.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  diff = (GET_LEVEL(ch) - GET_LEVEL(vict));
  act("$n gazes at $N and they stare back with glazed eyes!", FALSE, ch, 0, 
vict, TO_ROOM);

  if (diff > 37)  {
     GET_HIT(vict) = -1;
     send_to_char("You gaze for death!\r\n", ch);
  } else if ((diff > 30) && (!IS_AFFECTED(vict, AFF_SANCTUARY))) {
    if (ch->master) {
      send_to_char("You gaze to mesmerize, but your victim is too loyal!\r\n", ch);
      return;
    }
    if (num_followers >= cha_max_followers[GET_CHA(ch)]) {
      send_to_char("Your victim resists!\r\n", ch);
      return;
    }
 
    SET_BIT(AFF_FLAGS(vict), AFF_CHARM);
    stop_fighting(ch);
    stop_fighting(vict);
    add_follower(vict, ch);
    send_to_char("You gaze to mesmerize!\r\n", ch);
  } else if (diff > 10) {
    GET_HITROLL(vict) -= 10;
    GET_HIT(vict) -= 50;
    SET_BIT(AFF_FLAGS(vict), AFF_CURSE);
    send_to_char("You gaze for terror!\r\n", ch);
  } else if (diff > 5) {
    do_flee(vict, "", 0, 0);
    send_to_char("You gaze for fear!\r\n", ch);
  } else {
    GET_HITROLL(vict) -= 3;
    GET_DAMROLL(vict) -= 3;
    SET_BIT(AFF_FLAGS(vict), AFF_CURSE);
    send_to_char("You gaze for weakness!\r\n", ch);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
} 



/*
 * checks if they were fighting before they fled
 * deducts 1/400th the experience they need for their own level
 * checks the retreat skill though
 */
ACMD(do_flee)
{
  char buf[256];
  int i, attempt, loss, random;
  bool was_fighting = FALSE;
  bool retreated;
  bool can_enter_room;
  byte percent, prob;
  ACMD(do_say);

  if (!FIGHTING(ch) && subcmd != SCMD_FLEE_ALWAYS) {
    send_to_char("But you're not fighting anyone!\n\r", ch);
    return;
  }

  if (FIGHTING(ch))
    was_fighting = TRUE;

  /* add in retreat, but make mobs flee-retreat differently */
  percent = number(1, 101);	/* 101% is a complete failure */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch) * 100 / LVL_IMPL;
  else 
    prob = GET_SKILL(ch, SKILL_RETREAT) - 10;
  
  if (percent > prob)
    retreated = FALSE;
  else
    retreated = TRUE;

  if (IS_AFFECTED(ch, AFF_RAGE)) {
    do_say(ch, "No Retreat! No Surrender!", 0, 0);
    return;
  }

  if (IS_AFFECTED2(ch, AFF2_WEBBED)) {
    if (!do_web_check(ch)) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return;
    }
  }

  if (GET_POS(ch) > POS_STANDING) {
     send_to_char("You cannot flee from this position.\r\n", ch);
     return;
  }

  /* Make one attempt for each number of directions, except somewhere */
  for (i = 0; i < NUM_OF_DIRS - 1; i++) {
    attempt = i;
    random = number(1, 35);
    
    can_enter_room = TRUE;
    if (CAN_GO(ch, attempt)) {
      if (ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_SOLITARY)) {
        if (get_first_char(EXIT(ch, attempt)->to_room))
          can_enter_room = FALSE;
      }
    }

    if (CAN_GO(ch, attempt) && can_enter_room &&
	!IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_DEATH) &&
        (!IS_NPC(ch) ||
         !IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_NOMOB)) &&
	(random < GET_DEX(ch))) {

	if (retreated) {
	    act("$n considers $s situation and retreats.",
					TRUE, ch, 0, 0, TO_ROOM);
	    send_to_char("You consider your options and decide to retreat.\r\n", ch);
	} else {
	    act("$n panics, and attempts to flee!",
					TRUE, ch, 0, 0, TO_ROOM);
	    send_to_char("You flee head over heels.\r\n", ch);
	}

	if (do_simple_move(ch, attempt, TRUE)) {

	    if(retreated) {
		sprintf(buf, "You retreat %s!\r\n", dirs[attempt]);
		send_to_char(buf, ch);
	    }

	    if (was_fighting) {
		if (!IS_NPC(ch) && !retreated) {
#if (0)
		    THIS SHOULD MAKE THE COMPILER DIE!!
		    /* stock loss */
		    loss = GET_MAX_HIT(FIGHTING(ch)) - GET_HIT(FIGHTING(ch));
		    loss *= GET_LEVEL(FIGHTING(ch));
#endif
		    /* heroes of kore loss */
		    loss = (GET_LEVEL(ch) * GET_LEVEL(ch)) / 3;

		    /* newbies dont lose exp for fleeing */
		    if (GET_LEVEL(ch) < LVL_LOWBIE)
		    loss = 0;

		    gain_exp(ch, -loss);
		    sprintf(buf, "You lose %d experience for fleeing!\r\n", loss);
		    send_to_char(buf, ch);

#if(0)
		    /* Get rid of them charmies! */
		    fol = ch->followers;
		    while (fol) {
			fol_ch = fol->follower;
			nextfol = fol->next;
			if (fol_ch) {
			    if (IS_NPC(fol_ch)) {
				if (fol_ch->master == ch) {
				    stop_follower(fol_ch);
				    extract_char(fol_ch);
				}
			    }
			}
			fol = nextfol;
		    }
#endif
		}

		if (FIGHTING(ch))
		    if (FIGHTING(FIGHTING(ch)) == ch)
			stop_fighting(FIGHTING(ch));
		if (FIGHTING(ch))
		    stop_fighting(ch);
	    }
	    WAIT_STATE(ch, PULSE_VIOLENCE);
	} else {
	    if (retreated)
		act("$n tries to retreat, but can't.",
			TRUE, ch, 0, 0, TO_ROOM);
	    else
		act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);

	    WAIT_STATE(ch, PULSE_VIOLENCE);
      }
      return;
    }
  }
  if (retreated) {
    send_to_char("You panic and attempt to flee.\n\r", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
  } else {
    send_to_char("You panic and attempt to flee.\n\r", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
  }
}



ACMD(do_switch)
{
  struct char_data *vict;

  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!(FIGHTING(ch))) {
    send_to_char("But you are not fighting anything!\r\n", ch);
    return;
  }
  
  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Switch attacks to who?\r\n", ch);
      return;
    }
  }
  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n switches attacks to $N but messes up, trips and falls down.",
        FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n switches attacks to YOU but messes up, trips and falls down.",
        FALSE, ch, 0, vict, TO_VICT);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  stop_fighting(ch);
  set_fighting(ch, vict);
  act("You spin and attack $N!", FALSE, ch, 0, vict, TO_CHAR);
  act("$n spins and attacks $N!", FALSE, ch, NULL, vict, TO_NOTVICT);
  act("$n spins and attacks you!", FALSE, ch, NULL, vict, TO_VICT);

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}



#define BERSERK_MOVEMENT_COST	10
ACMD(do_berserk)
{
  ACMD(do_stand);
  struct char_data *vict;
  byte percent, prob;


  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_BARD(ch) && !IS_UNDEAD(ch) &&
      !IS_DRAGON(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      /* try try try to find a valid target since they didnt offer one */
      if (!(vict = get_vict_room_vis(ch))) {
        send_to_char("Berserk all over who?\r\n", ch);
        return;
      }
    }
  }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if (is_safe(ch, vict, TRUE))
    return;

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n kind of screams at $N and lunges, but then trips and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  /* check available movement */
  if (!IS_NPC(ch)) {
    if (GET_MOVE(ch) < BERSERK_MOVEMENT_COST) {
      send_to_char("You are so tired, you just wheeze.\r\n", ch);
      act("$n wheezes for breath.", FALSE, ch, 0, vict, TO_ROOM);
      GET_MOVE(ch) = 0;
      return;
    } else {
      GET_MOVE(ch) -= BERSERK_MOVEMENT_COST;
    }
  }

  percent = number(1, 101);     /* 101% is a complete failure */
  prob = (GET_SKILL(ch, SKILL_BERSERK));

  if (percent > prob) {
    send_to_char("You start to go berserk but then you wuss out!\r\n", ch);
    act("$n starts to go berserk but then wusses out!",
        FALSE, ch, 0, vict, TO_ROOM);
  } else {
    send_to_char("You go totally berserk!\r\n", ch);
    act("$n goes totally berserk!", FALSE, ch, 0, vict, TO_ROOM);
    mag_areas(GET_LEVEL(ch), ch, vict, SKILL_BERSERK, 1);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_bash)
{
  ACMD(do_stand);
  struct char_data *vict;
  byte percent, prob;
/*
  int wear_wield = IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD;
*/
  int wear_shield = IS_THRIKREEN(ch)? THRI_WEAR_SHIELD_R : WEAR_SHIELD;
  int dam = 0;
  int stone = 0;
  int mirror = 0;

  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_BARD(ch) && !IS_UNDEAD(ch) &&
      !IS_DRAGON(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if ((!ch->equipment[wear_shield]) && !IS_NPC(ch)) {
    send_to_char("You need to wear a shield to bash.\r\n", ch);
    return;
  }

  if (is_safe(ch, vict, TRUE))
    return;

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n lunges for $N but can't quite seem to connect.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  if ((GET_POS(vict) != POS_STANDING) && (GET_POS(vict) != POS_FIGHTING)) {
    send_to_char("They aren't in a position to be bashed.\r\n", ch);
    return;
  }

  if (MOB_FLAGGED(vict, MOB_NOBASH)) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    return;
  }


  if (IS_AFFECTED(vict, AFF_STONESKIN)) {
      GET_LAYERS(vict) = GET_LAYERS(vict) - 1;
      act("You try to bash $N and deflect off $S stone skin!",
          FALSE, ch, 0, vict, TO_CHAR);
      act("$n tries to bash you and deflects off your stone skin!",
          FALSE, ch, 0, vict, TO_VICT);
      act("$n attempts to bash $N and deflects off $S stone skin!",
          FALSE, ch, 0, vict, TO_NOTVICT);
      if (GET_LAYERS(vict) <= 0) {
        act("Your skin loses its stony texture!",
            FALSE, ch, 0, vict, TO_VICT);
        if (IS_AFFECTED(vict, AFF_STONESKIN))
          affect_from_char(vict, SPELL_STONESKIN);
        GET_LAYERS(vict) = 0;
      }
      WAIT_STATE(ch, PULSE_VIOLENCE * 2);
      stone = 1;
  }

  if (IS_AFFECTED(vict, AFF_MIRROR_IMAGE)) {
    if (number(0, GET_IMAGES(vict)) < GET_IMAGES(vict)) {
      dam = 0;
      GET_IMAGES(vict) = GET_IMAGES(vict) - 1;
      act("You bash an image of $N!", FALSE, ch, 0, vict, TO_CHAR);
      act("$n bashes an image of you!", FALSE, ch, 0, vict, TO_VICT);
      act("$n bashes an image of $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      mirror = 1;

      if (GET_IMAGES(vict) <= 0) {
        if (IS_AFFECTED(vict, AFF_MIRROR_IMAGE))
        affect_from_char(vict, SPELL_MIRROR_IMAGE);
        GET_IMAGES(vict) = 0;
        send_to_char("The last of your images disipates, leaving you unprotected.
\r\n", vict);

      }
    }
  }


  percent = number(1, 101);	/* 101% is a complete failure */
  prob = (GET_SKILL(ch, SKILL_BASH));
  
  if (((percent > prob) && (subcmd != SCMD_BASH_NO_SAVE)) || mirror || stone) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
  } else {
    act("You send $N sprawling with a powerful bash!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n sends you sprawling with a powerful bash!", FALSE, ch, 0, vict, TO_VICT);
    act("$n sends $N to the ground with a bash!", FALSE, ch, 0, vict, TO_NOTVICT);
    dam = number_range(1, GET_LEVEL(ch));
    damage(ch, vict, dam, SKILL_BASH);
    GET_POS(vict) = POS_SITTING;
    WAIT_STATE(vict, PULSE_VIOLENCE * 2);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_trip)
{
  ACMD(do_stand);
  struct char_data *vict;
  byte percent, prob;
  int dam = 0;


  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_THIEF(ch) && !IS_BARD(ch)) {
    send_to_char("You'd better leave the sneaky stuff to the thieves.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Trip whom?\r\n", ch);
      return;
    }
  }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if (is_safe(ch, vict, TRUE))
    return;

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n lunges for $N but then trips and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  if (IS_AFFECTED(vict, AFF_FLY)) {
    send_to_char("Their feet aren't even touching the ground.\r\n", ch);
    return;
  }

  if ((GET_POS(vict) != POS_STANDING) && (GET_POS(vict) != POS_FIGHTING)) {
    send_to_char("They aren't in a position to be tripped.\r\n", ch);
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = (GET_SKILL(ch, SKILL_TRIP));
  
  if ((percent > prob) && (subcmd != SCMD_TRIP_NO_SAVE)) {
    damage(ch, vict, 0, SKILL_TRIP);
  } else {
    act("You trip $N, sending them to the ground.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n trips you to the ground.", FALSE, ch, 0, vict, TO_VICT);
    act("$n trips $N, sending them to the ground.", FALSE, ch, 0, vict, TO_NOTVICT);
    dam = number_range(1, 10);
    damage(ch, vict, dam, SKILL_TRIP);
    GET_POS(vict) = POS_SITTING;
    WAIT_STATE(vict, PULSE_VIOLENCE * 2);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

ACMD(do_disarm)
{
  struct char_data *vict;
  byte percent, prob;
  int wear_wield;

  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_BARD(ch) && !IS_UNDEAD(ch) &&
      !IS_DRAGON(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Disarm who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Why not just remove your weapon?\r\n", ch);
    return;
  }

  if (is_safe(ch, vict, TRUE))
    return;

  wear_wield = IS_THRIKREEN(vict)? THRI_WEAR_WIELD_R : WEAR_WIELD;

  if (!vict->equipment[wear_wield]) {
    send_to_char("They're not wielding anything!\r\n", ch);
    return;
  }

  if (IS_AFFECTED(vict, AFF_SANCTUARY) && !IS_NPC(ch)) {
    act("The gods protect $N.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  percent = number(1, 101);     /* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_DISARM) - 30;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_DISARM);
  } else {
    if ((GET_STR(vict) + number_range(1, 10)) > (GET_STR(ch) + number_range(1, 10))) {
      damage(ch, vict, 0, SKILL_DISARM);
    } else {
      damage(ch, vict, 2, SKILL_DISARM);
      perform_remove(vict, wear_wield);
    }
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}



ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;
  byte percent, prob;

  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_BARD(ch) && !IS_UNDEAD(ch) &&
      !IS_DRAGON(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("Who do you want to rescue?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("What about fleeing instead?\r\n", ch);
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
    return;
  }
  for (tmp_ch = world[ch->in_room].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n tries to rescue $N but then falls over.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_RESCUE);

  if (percent > prob) {
    send_to_char("You fail the rescue!\r\n", ch);
    return;
  }
  send_to_char("Banzai!  To the rescue...\r\n", ch);
  act("You are rescued by $N, you are confused!",
      FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);

  WAIT_STATE(vict, PULSE_VIOLENCE * 1);
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}



ACMD(do_defend)
{
  struct char_data *vict, *tmp_ch;
  byte percent, prob;
  int wear_shield = IS_THRIKREEN(ch)? THRI_WEAR_SHIELD_R : WEAR_SHIELD;

  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_BARD(ch) && !IS_UNDEAD(ch) &&
      !IS_DRAGON(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!ch->equipment[wear_shield]) {
    send_to_char("You'll need a shield to make it a success!\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("Who do you want to defend?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("What about fleeing instead?\r\n", ch);
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char("How can you defend someone you are trying to kill?\r\n", ch);
    return;
  }
  for (tmp_ch = world[ch->in_room].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n throws $mself in front of $N to save $M, but trips and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  percent = number(1, 101);   /* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_DEFEND);

  if (percent > prob) {
    act("You fail to defend $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  act("You defend $M heroically!", FALSE, ch, 0, vict, TO_CHAR);
  act("You are defended by $N!",
      FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically charges in to defend $N!",
      FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);

  WAIT_STATE(vict, PULSE_VIOLENCE * 2);
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}



ACMD(do_kick)
{
  struct char_data *vict;
  byte prob;
  int dam = 0;


  one_argument(argument, arg);

  if ( IS_NPC(ch) && (IS_AFFECTED2(ch, AFF2_JARRED) || IS_AFFECTED(ch, AFF_CHARM)) ) {
    send_to_char("Charmed mobiles may not use this.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_BARD(ch) && !IS_UNDEAD(ch) &&
      !IS_DRAGON(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if (is_safe(ch, vict, TRUE))
    return;

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n does a big kick and falls over.", FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  prob = GET_SKILL(ch, SKILL_KICK);

  if (number(1, 101) > prob) {
    damage(ch, vict, 0, SKILL_KICK);
  } else
    dam = (number_range(1, GET_LEVEL(ch))) + GET_DAMROLL(ch);
    damage(ch, vict, dam, SKILL_KICK);

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}



#define BREATH_MOVE_COST 60
ACMD(do_breathe)
{
  struct char_data *vict;
  int dam;

  one_argument(argument, arg);

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_DRAGON(ch)) {
    send_to_char("Ew your breath is bad!\r\n", ch);
    return;
  }

  if (GET_MOVE(ch) < BREATH_MOVE_COST) {
    send_to_char("You are too tired and out of breath!\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      if (ch->in_room == FIGHTING(ch)->in_room) {
        vict = FIGHTING(ch);
      } else {
        send_to_char("They are not close enough!\r\n", ch);
        return;
      }
    } else {
      send_to_char("Breathe on who?\r\n", ch);
      return;
    }
  }

  if (is_safe(ch, vict, TRUE))
    return;

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("$n breathes a little too hard, hyperventilates and falls down.",
        FALSE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }

  GET_MOVE(ch) -= BREATH_MOVE_COST;

  if (vict == ch) {
    send_to_char("You clean yourself with a blast from your lungs!\r\n", ch);
    return;
  }

  /* the damage gets quadrupled later */
  dam = (GET_LEVEL(ch) / 2) + number(0, 25);

  switch (number(0, 4)) {
    case 0:
        damage(ch, vict, dam, TYPE_BREATHE_LIGHTNING);
        break;
    case 1:
        damage(ch, vict, dam, TYPE_BREATHE_FROST);
        break;
    case 2:
        damage(ch, vict, dam, TYPE_BREATHE_ACID);
        break;
    case 3:
        damage(ch, vict, dam, TYPE_BREATHE_FIRE);
        break;
    case 4:
        damage(ch, vict, dam, TYPE_BREATHE_GAS);
        break;
    default:
        damage(ch, vict, dam, TYPE_BREATHE_FIRE);
        break;
  };

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}



ACMD(do_quickdraw)
{
  int wear_wield = IS_THRIKREEN(ch) ? THRI_WEAR_WIELD_R : WEAR_WIELD;
  struct obj_data *wielded = ch->equipment[wear_wield];
  struct obj_data *obj;
  struct char_data *vict = NULL;

  ACMD(do_wake);
  ACMD(do_stand);
  void perform_unmount(struct char_data *ch);


  one_argument(argument, arg);

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_WARRIOR(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to fighters.\r\n", ch);
    return;
  }

  if (subcmd == SCMD_REAL_QUICKDRAW) {
    if (!(vict = get_char_room_vis(ch, arg))) {
      return;
    }
  }

  if (HAS_PET(ch))
    if (IS_MOUNTED(GET_PET(ch)))
	perform_unmount(ch);

  switch (GET_POS(ch)) {
    case POS_DEAD:
    case POS_MORTALLYW:
    case POS_SLEEPING:
    case POS_STUNNED:
	return;				/* cant save you */
    case POS_RESTING:
    case POS_SITTING:
	do_stand(ch, "", 0, 0);		/* stand up and break */
	break;
    case POS_STANDING:
	break;				/* ok keep going ... */
    case POS_FIGHTING:
        return;				/* already fighting, just bail */
    default:
	return;				/* ack! unknown position! */
  }

  /* wield a handy weapon */
  if (!wielded) {
    for (obj = ch->carrying; obj; obj = obj->next_content) {
      if (CAN_WEAR(obj, ITEM_WEAR_WIELD) &&
         (GET_OBJ_WEIGHT(obj) <= str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)) {
        perform_wear(ch, obj, wear_wield, FIND_INDIV);
	break;
      }
    }  
  }

  /* make them hit the mob that has goaded them */
  if (subcmd == SCMD_REAL_QUICKDRAW) {
    act("You sense $N's evil intent and strike!",
         FALSE, vict, 0, vict, TO_CHAR);
    act("$N senses your evil intent and strikes!",
	 FALSE, vict, 0, ch, TO_CHAR);
    act("$n senses $N's evil intent and strikes!",
         FALSE, ch, 0, vict, TO_NOTVICT);
  } else {	/* just SCMD_TEST_QUICKDRAW */
    send_to_char("You stand ready to fight!\r\n", ch);
    act("$n stands ready to fight!", FALSE, ch, 0, 0, TO_NOTVICT);
  }
}



ACMD(do_avenging_blow)
{
  int wear_wield = IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD;
  struct obj_data *wielded = ch->equipment[wear_wield];
  struct obj_data *obj;
  struct char_data *vict = NULL;

  ACMD(do_wake);
  ACMD(do_stand);
  void perform_unmount(struct char_data *ch);


  one_argument(argument, arg);

  if (IS_NPC(ch) && ch->desc) {
    send_to_char("You'd better leave the martial arts to knights.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_DEATHKNIGHT(ch)) {
    send_to_char("You'd better leave the martial arts to knights.\r\n", ch);
    return;
  }

  if (subcmd == SCMD_REAL_AVENGING_BLOW) {
    if (!(vict = get_char_room_vis(ch, arg))) {
      return;                           /* hrm, not good :( */
    }
  }

  /* move them into the right position */
  if (HAS_PET(ch)) if (IS_MOUNTED(GET_PET(ch))) perform_unmount(ch);
  switch (GET_POS(ch)) {
    case POS_DEAD:
    case POS_MORTALLYW:
    case POS_STUNNED:
        return;                         /* cant save you */
    case POS_SLEEPING:
        do_wake(ch, "", 0, 0);          /* wake up and (next) stand up */
    case POS_RESTING:
    case POS_SITTING:
        do_stand(ch, "", 0, 0);         /* stand up and break */
        break;
    case POS_STANDING:
        break;                          /* ok keep going ... */
    case POS_FIGHTING:
        return;                         /* already fighting, just bail */
    default:
        return;                         /* ack! unknown position! */
  }

  /* wield a handy weapon */
  if (!wielded) {
    for (obj = ch->carrying; obj; obj = obj->next_content) {
      if (CAN_WEAR(obj, ITEM_WEAR_WIELD) &&
         (GET_OBJ_WEIGHT(obj) <= str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)) {
        perform_wear(ch, obj, wear_wield, FIND_INDIV);
      }
    }
  }

  /* make them hit the mob that has goaded them */
  if (subcmd == SCMD_REAL_AVENGING_BLOW) {
    act("You sense $N's evil intent and strike an AVENGING BLOW!",
         FALSE, ch, 0, vict, TO_CHAR);
    act("$N senses your evil intent and strikes an AVENGING BLOW!",
         FALSE, vict, 0, ch, TO_CHAR);
    act("$n senses $N's evil intent and strikes an AVENGING BLOW!",
         FALSE, ch, 0, vict, TO_NOTVICT);
  } else {      /* just SCMD_TEST_AVENGING_BLOW */
    send_to_char("You stand ready to FIGHT!\r\n", ch);
    act("$n stands ready to FIGHT!", FALSE, ch, 0, 0, TO_NOTVICT);
  }
}
