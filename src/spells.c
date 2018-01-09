
/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "screen.h"

extern struct room_data *world;
extern char *spell_wear_off_msg[];
extern char *spell_dispel_msg[];
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct cha_app_type cha_app[];
extern struct int_app_type int_app[];
extern struct index_data *obj_index;
extern struct weather_data weather_info;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
extern int mini_mud;
extern int isexname(char *str, char *namelist);
extern int pk_allowed;
extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern int *max_ac_applys;
extern struct apply_mod_defaults *apmd;
extern char *spells[];
extern char *item_types[];
extern char *extra_bits[];
extern char *apply_types[];
extern char *affected_bits[];
extern char *affected2_bits[];
int number_range(int from, int to);
ACMD(do_speak);

void clearMemory(struct char_data * ch);
void act(char *str, int i, struct char_data * c, struct obj_data * o,
	      void *vict_obj, int j);
void damage(struct char_data * ch, struct char_data * victim,
	         int damage, int weapontype);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_savingthrow(struct char_data * ch, int type);
struct time_info_data age(struct char_data * ch);


/*
 * Special spells appear below.
 */
ASPELL(spell_manual_damage)
{
  int dam;

  sprintf(buf, "Char is: %s Victim is: %s\r\n", GET_NAME(ch), GET_NAME(victim));
  send_to_char(buf, ch);

  dam = dice(6, 5) + 250;

  if (mag_savingthrow(victim, SAVING_SPELL))
    dam >>= 1;

  damage(ch, victim, dam, SPELL_FIREBALL);
}
 

ASPELL(spell_area_scare)
{
  struct char_data *vict;

  ACMD(do_flee);

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    /*
     * players, jarred mobs, and charmed pets are ok targets
     * but not regular old uncharmed npcs
     */
    if ((IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM)))
      continue;
    /* dont target immortals */
    if (GET_LEVEL(vict) >= LVL_IMMORT)
      continue;
    /* valid victim, make them flee in terror */
    do_flee(vict, "", 0, SCMD_FLEE_ALWAYS);
  }
}



const int components_and_demons[][2] = {
  {3602,  15031},     /* black pawn's sword summons a skeleton */
  {3608,  15030},     /* staff of the black bishop summons a zombie */
  {5107,  5111},      /* six headed whip summons the yochlol */
  {12031, 15032},     /* executioner's mace summons the pit fiend */
  {-1,    -1}         /* this must end the list, don't delete */
};

/* unfinished spell */
ASPELL(spell_cacodemon)
{
  struct obj_data *component;


  if (ch == NULL)
    return;

  component = ch->equipment[GET_WEAR_WIELD(ch)];

  if (component == NULL) {
    send_to_char("Cacodemon requires a component you nitwit!\r\n", ch);
    return;
  }

     
}


ASPELL(spell_calm)
{
  ACMD(do_peace);

   send_to_char("You attempt to calm the fighting.\r\n", ch);
   do_peace(ch, "", 0, 0);

}

ASPELL(spell_cantrip)
{
  char social_actor[256];
  char social_and_target[256];
  char social[256];
  char social_target[256];
  char unused_end[256];
 
  ACMD(do_action);

  if (victim == NULL)
    return;

  if (argument == NULL) {
    send_to_char("Cast cantrip on who?\r\n", ch);
    return;
  } else {
    half_chop(argument, social_actor, social_and_target);
  }

  if (social_and_target == NULL) {
    send_to_char("Make them do what social?\r\n", ch);
    return;
  } else {
    half_chop(social_and_target, social, social_target);
    half_chop(social_target, social_target, unused_end);
  }

  if (social == NULL) {
    send_to_char("Make them do what social? #2\r\n", ch);
    return;
  }

/*
  do_action(victim, GET_NAME(ch), find_command("kiss"), 0);
*/
/*
  do_action(victim, social_target, find_command(social), 0);
*/
  sprintf(buf, "do >>%s<< to >>%s<<\r\n", social, social_target);
  send_to_char(buf, ch);
}


ASPELL(spell_charm)
{
  struct affected_type af;
  struct follow_type *f;
  int num_followers = 0;

  extern int cha_max_followers[26];

  if (victim == NULL || ch == NULL)
    return;

  for (f = ch->followers; f; f = f->next) {
    if (IS_NPC(f->follower))
      num_followers++;
  }

  if (victim == ch)
    send_to_char("You like yourself even better!\r\n", ch);
  else if (IS_AFFECTED(victim, AFF_MAGIC_RESIST))
    send_to_char("Your victim is protected by an anti-magic shell!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_CHARM))
    send_to_char("You can't have any followers of your own!\r\n", ch);
  else if (!pk_allowed && !IS_NPC(victim))
    send_to_char("You fail - shouldn't be doing it anyway.\r\n", ch);
  else if (circle_follow(victim, ch))
    send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);

  else if (num_followers >= cha_max_followers[GET_CHA(ch)]) {
    send_to_char("Your victim resists!\r\n", ch);
    hit(victim, ch, TYPE_UNDEFINED);
  }
  else if (IS_AFFECTED(victim, AFF_CHARM)) {
    send_to_char("Your victim resists!\r\n", ch);
    hit(victim, ch, TYPE_UNDEFINED);
  }
  else if (MOB_FLAGGED(victim, MOB_NOCHARM)) {
    send_to_char("Your victim resists!\r\n", ch);
    hit(victim, ch, TYPE_UNDEFINED);
  }
  else if (mag_savingthrow(victim, SAVING_SPELL)) {
    send_to_char("Your victim resists!\r\n", ch);
    hit(victim, ch, TYPE_UNDEFINED);
  }
  else if ((GET_RACE(victim) == RACE_ELF) && number(1, 100) < 90) {
    send_to_char("Your victim resists!\r\n", ch);
    hit(victim, ch, TYPE_UNDEFINED);
  }
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    af.type = SPELL_CHARM;

    af.duration = number_range(GET_LEVEL(ch)/2, GET_LEVEL(ch));

    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    af.bitvector2 = 0;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim)) {
      REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
    }
  }
}


ASPELL(spell_clairvoyance)
{
  int to_room;
  int from_room = ch->in_room;

  if (victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char("You failed.\r\n", ch);
    return;
  }

  if (IS_SET(world[victim->in_room].room_flags, ROOM_PRIVATE | ROOM_DEATH))
    return;

  to_room = victim->in_room;

  act("$n's eyes become clouded as $s eyes glimpse through $N's!",
      FALSE, ch, 0, victim, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, to_room);
  act("$n looks out through your eyes!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(ch, 0);

  char_from_room(ch);
  char_to_room(ch, from_room);
}


ASPELL(spell_create_water)
{
  int water;

  void name_to_drinkcon(struct obj_data * obj, int type);
  void name_from_drinkcon(struct obj_data * obj);

  if (ch == NULL || obj == NULL)
    return;
  level = MAX(MIN(level, LVL_IMPL), 1);

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
/* Um, this is dumb
      name_from_drinkcon(obj);
*/
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
/* Um, this is dumb
      name_to_drinkcon(obj, LIQ_SLIME);
*/
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	weight_change_object(obj, water);
/* Um, this is dumb
	name_from_drinkcon(obj);
	name_to_drinkcon(obj, LIQ_WATER);
*/
	act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}



ASPELL(spell_dimension_door)
{
  extern int top_of_world;
  int found = 0;
  int temp_zone;
  int i;
  int to_room;


  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char("You failed.\r\n", ch);
    return;
  }
  if (world[ch->in_room].zone != world[victim->in_room].zone)  {
    send_to_char("They are not close enough!\r\n", ch);
    return;
  }
  if (ZONE_FLAGGED(victim->in_room, ZONE_NOTELEPORT)) {
    send_to_char("You cannot go there by magical means!\r\n", ch);
    return;
  }
  temp_zone = world[ch->in_room].zone;
  act("$n disappears suddenly.", TRUE, ch, 0, 0, TO_ROOM);
  if (number(1,100) <= 15 + ((GET_INT(ch) / 7) * (GET_LEVEL(ch) - 14))) {
   for (i = 0; (i < 100); i++) {
      do {
    to_room = number(0, top_of_world);
  } while ((IS_SET(world[to_room].room_flags, ROOM_PRIVATE | ROOM_DEATH)));
    char_from_room(ch);
    char_to_room(ch, to_room);
  if (world[ch->in_room].zone == temp_zone)   {
      send_to_char("You become lost between reality and unreality!\r\n", ch);
      look_at_room(ch, 0);
      found = TRUE;
      return;
    }
   }
  if (found != TRUE) {
    send_to_char("You attempt to teleport but the room just spins "
       "drunkenly!\r\n", ch);
       return;
  }
 }
  else {
   char_from_room(ch);
   char_to_room(ch, victim->in_room);
   act("$n steps out from behind an atom and grins at you!", FALSE, ch, 0,
        victim, TO_VICT);
   look_at_room(ch, 0);
  }
}



/* My attempt at a dimension door spell - Aule */
/*
ASPELL(spell_dimension_door)
{
  int to_v_room, to_room;

  if (ch == NULL)
    return;

  do {
    to_v_room = number(zone_table[12 - 1].top + 1, zone_table[12].top);
    to_room = real_room(to_v_room);
  } while (to_room != -1 &&
           (IS_SET(world[to_room].room_flags, ROOM_PRIVATE |
ROOM_GODROOM) || ZONE_FLAGGED(to_room, ZONE_GODZONE)));

  act("$n opens a magical doorway and leaps through!",
      FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, to_room);
  act("$n jumps in through a magical doorway!", FALSE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}
*/



ASPELL(spell_dispel_magic)
{
/*
  char buf[256];
*/
  struct affected_type *paf;
  struct affected_type *paf_next;

  if (ch == NULL)
      return;

    if ( !(victim->affected) ) {
        send_to_char( "Nothing happens.\n\r", ch );
        return; 
    }       

    for ( paf = victim->affected; paf != NULL; paf = paf_next )
    {
        paf_next    = paf->next;
        
        if ( mag_savingthrow( victim, SAVING_SPELL ) ) {
            continue;
        } else {
            if (*spell_wear_off_msg[paf->type]) {
              send_to_char(spell_wear_off_msg[paf->type], victim);
              send_to_char("\r\n", victim);
	    }

            if (*spell_dispel_msg[paf->type]) {
		sprintf(buf, "%s %s", GET_NAME(victim), spell_dispel_msg[paf->type]);
		act(buf, TRUE, victim, 0, 0, TO_ROOM);
            } 
            affect_remove( victim, paf );  
        }   
    }   

/*
  send_to_room("There is a brief flash of light!\r\n", ch->in_room);
  if (victim->affected) {
    while (victim->affected)
      affect_remove(victim, victim->affected);
    send_to_char("You feel strangely different.\r\n", victim);
    sprintf(buf, "You see %s stagger under the spell.\r\n", GET_NAME(victim));
    send_to_char(buf, ch);
  } else {
    send_to_char("But nothing seems to happen.\r\n", victim);
    send_to_char("But nothing seems to happen.\r\n", ch);
  }
*/

/* HACKED to drop hit points that have gone over the max back to normal */
  if (GET_HIT(victim) > GET_MAX_HIT(victim))
    GET_HIT(victim) = GET_MAX_HIT(victim);
/* end of hack */
}


ASPELL(spell_enchant_weapon)
{
  int i;
  int j;

  if (ch == NULL || obj == NULL)
    return;

  if (((GET_OBJ_TYPE(obj) == ITEM_WEAPON) ||
       (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON)) &&
      !IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC) &&
      !isname("newbie", obj->name)) {

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
        return;

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

    j = GET_LEVEL(ch) + GET_INT(ch) + dice(2, 6) - 6;
    if (j <= 11) {
      GET_OBJ_VAL(obj, 2) += 0;
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 4;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 4;
    } else if (j <= LVL_IMMORT + 16) {
      GET_OBJ_VAL(obj, 2) += 1;
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 3;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 3;
    } else if (j <= LVL_IMMORT + 21) {
      GET_OBJ_VAL(obj, 2) += 2;
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 2;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 2;
    } else if (j <= LVL_IMMORT + 25) {
      GET_OBJ_VAL(obj, 2) += 3;
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 1;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 1;
    } else {
      GET_OBJ_VAL(obj, 2) += 4;
      obj->affected[0].location = APPLY_HITROLL;
      obj->affected[0].modifier = 0;
      obj->affected[1].location = APPLY_DAMROLL;
      obj->affected[1].modifier = 0;
    }

    if (IS_GOOD(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
    } else if (IS_EVIL(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
    }                                                                          
  }
}


ASPELL(spell_enchant_armor)
{
  int i;
  int j;

  if (ch == NULL || obj == NULL)
    return;

  if (((GET_OBJ_TYPE(obj) == ITEM_ARMOR)) &&
      !IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)) {

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
        return;

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

    j = GET_LEVEL(ch) + GET_INT(ch) + dice(2, 6) - 6;

    if (j <= 11) {
      GET_OBJ_VAL(obj, 0) += 0;
      obj->affected[0].location = APPLY_AC;
      obj->affected[0].modifier = -2;
      obj->affected[1].location = APPLY_SAVING_SPELL;
      obj->affected[1].modifier = -1;
    } else if (j <= LVL_IMMORT + 16) {
      GET_OBJ_VAL(obj, 0) += 2;
      obj->affected[0].location = APPLY_AC;
      obj->affected[0].modifier = -4;
      obj->affected[1].location = APPLY_SAVING_SPELL;
      obj->affected[1].modifier = -1;
    } else if (j <= LVL_IMMORT + 21) {
      GET_OBJ_VAL(obj, 0) += 4;
      obj->affected[0].location = APPLY_AC;
      obj->affected[0].modifier = -6;
      obj->affected[1].location = APPLY_SAVING_SPELL;
      obj->affected[1].modifier = -2;
    } else if (j <= LVL_IMMORT + 25) {
      GET_OBJ_VAL(obj, 0) += 6;
      obj->affected[0].location = APPLY_AC;
      obj->affected[0].modifier = -8;
      obj->affected[1].location = APPLY_SAVING_SPELL;
      obj->affected[1].modifier = -2;
    } else {
      GET_OBJ_VAL(obj, 0) += 8;
      obj->affected[0].location = APPLY_AC;
      obj->affected[0].modifier = -10;
      obj->affected[1].location = APPLY_SAVING_SPELL;
      obj->affected[1].modifier = -3;
    }

      act("$p shimmers with a gold aura.", FALSE, ch, obj, 0, TO_CHAR);

/*
    if (IS_GOOD(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
    } else if (IS_EVIL(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
    }                                                                          
*/

  }
}


ASPELL(spell_far_see)
{
  int to_room;
  int from_room = ch->in_room;

  if (victim == NULL)
    return;

  if (IS_SET(world[victim->in_room].room_flags, ROOM_PRIVATE | ROOM_DEATH)) 
    return;

  to_room = victim->in_room;

  act("$n tears the fabric of the universe and looks through at $N!",
      FALSE, ch, 0, victim, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, to_room);
  act("$n peeps out through a hole in the air!",
    FALSE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);

  if (IS_NPC(victim))
    hit(victim, ch, TYPE_UNDEFINED);
  if (FIGHTING(ch))
    stop_fighting(ch);
  if (FIGHTING(victim) == ch)
    stop_fighting(victim);

  char_from_room(ch);
  char_to_room(ch, from_room);
  act("$n hurredly pulls back from the tear before it closes!",
    FALSE, victim, 0, 0, TO_ROOM);
}



ASPELL(spell_forget)
{
  if (!IS_NPC(victim))
    return;

  if (mag_savingthrow(victim, SAVING_SPELL)) {
    do_speak(victim, "I do not forget so easily.", 0, 0);
    hit(victim, ch, TYPE_UNDEFINED);
    return;
  }

  if (IS_AFFECTED2(victim, AFF2_WASCHARMED)) {
    do_speak(victim, "I do not forget so easily.", 0, 0);
    hit(victim, ch, TYPE_UNDEFINED);
    return;
  }

  clearMemory(victim);
  send_to_char("I think that your victim no longer remembers you.\r\n", ch);
}



ASPELL(spell_gypsy_dance)
{
  send_to_char("Gypsy dance!! woo!!\r\n", ch);
}



ASPELL(spell_identify)
{
  int i;
  int found;
  
  void obj_ident_prog(struct obj_data *obj, struct char_data *ch);

  if (obj) {
    send_to_char("You feel informed:\r\n", ch);
    sprintf(buf, "Object '%s', Item type: ", obj->short_description);
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    send_to_char("Item is: ", ch);
    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
            GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
    send_to_char(buf, ch);
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

      if (GET_OBJ_VAL(obj, 1) >= 1)
        sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(obj, 1)]);
      if (GET_OBJ_VAL(obj, 2) >= 1)
        sprintf(buf, "%s, %s", buf, spells[GET_OBJ_VAL(obj, 2)]);
      if (GET_OBJ_VAL(obj, 3) >= 1)
        sprintf(buf, "%s, %s", buf, spells[GET_OBJ_VAL(obj, 3)]);
      sprintf(buf, "%s\r\n", buf);
      send_to_char(buf, ch);
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
      sprintf(buf, "%s %s\r\n", buf, spells[GET_OBJ_VAL(obj, 3)]);
      sprintf(buf, "%sIt has %d maximum charge%s and %d remaining.\r\n", buf,
              GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
              GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_WEAPON:
    case ITEM_FIREWEAPON:
      sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
              GET_OBJ_VAL(obj, 2));
      sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
              (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
      send_to_char(buf, ch);
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_MISSILE:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_TREASURE:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_ARMOR:
      sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_TRASH:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      break;
    case ITEM_TRAP:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_CONTAINER:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_PEN:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_INSTRUMENT:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_BOAT:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_MONEY:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_FOOD:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_DRINKCON:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_NOTE:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_PORTAL:
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch); 
      break;
    case ITEM_KEY:
      if (GET_OBJ_VAL(obj, 1) > 0) {
        sprintf(buf, "It has %d charge%s remaining.\r\n", GET_OBJ_VAL(obj, 1),
                GET_OBJ_VAL(obj, 1) == 1 ? "" : "s");
        send_to_char(buf, ch);
      sprintf(buf, "This item rots in %d tics\r\n", GET_OBJ_TIMER(obj));
      send_to_char(buf, ch);
      }
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
        (obj->obj_flags.bitvector2)) {
      send_to_char("Item will give you following abilities:  ", ch);

      sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);

      sprintbit(obj->obj_flags.bitvector2, affected2_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
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
    obj_ident_prog(obj, ch);
  }
}                                                                              



ASPELL(spell_knock)
{
  int door;

  extern char *dirs[];

  extern void do_doorcmd(struct char_data *ch, struct obj_data *obj, 
    int door, int scmd);

  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (EXIT(ch, door)) {
      if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
        if IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF) {
          sprintf(buf, "You fail to knock the %s %s..\r\n",
                  fname(EXIT(ch, door)->keyword), dirs[door]);
                  return;
	}
        if (EXIT(ch, door)->keyword) {
          sprintf(buf, "You unlock the %s %s...\r\n", 
                  fname(EXIT(ch, door)->keyword), dirs[door]);
          send_to_char(buf, ch);
          do_doorcmd(ch, obj, door, SCMD_UNLOCK);
        }
      }
/* HACKED to not open doors, just unlock them

      if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
        if (EXIT(ch, door)->keyword) {
          sprintf(buf, "You open the %s %s...\r\n",
                  fname(EXIT(ch, door)->keyword), dirs[door]);
          send_to_char(buf, ch);
          do_doorcmd(ch, obj, door, SCMD_OPEN);
        }
      }
END of hack */
    }
  }
}



/*
 * uses the argument passed instead of the keywords on the first object
 * found as the thing to be searched for...
 * should work a lot better than stock locate object 
 */
#define QUEST_OBJECT_VNUM 2 
ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  /* strcpy(name, fname(obj->name)); */
  strcpy(name, argument);
  j = level >> 1;

  for (i = object_list; i && (j > 0); i = i->next) {

    /* might need a macro if on_ground(obj) shrug */
    if (!isexname(name, i->name) || (!i->carried_by && !i->worn_by 
	&& !i->in_obj && IS_SET(world[i->in_room].room_flags, ROOM_STORAGE)))
      continue;

    if (GET_OBJ_VNUM(i) == QUEST_OBJECT_VNUM)
      continue;
    if (IS_OBJ_STAT(i, ITEM_NOLOCATE))
      continue;

    else if ((i->carried_by && GET_LEVEL(i->carried_by) > GET_LEVEL(ch) + 10)
    	|| (i->worn_by && GET_LEVEL(i->worn_by) > GET_LEVEL(ch) + 10)
    	|| (i->carried_by && GET_INVIS_LEV(i->carried_by) > GET_LEVEL(ch))
    	|| (i->worn_by && GET_INVIS_LEV(i->worn_by) > GET_LEVEL(ch)))
	continue;

    else if (i->carried_by)
      sprintf(buf, "%s is being carried by %s.\n\r",
              i->short_description, PERS(i->carried_by, ch));
    else if (i->in_room != NOWHERE)
      sprintf(buf, "%s is in %s.\n\r", i->short_description,
              world[i->in_room].name);
    else if (i->in_obj)
      sprintf(buf, "%s is in %s.\n\r", i->short_description,
              i->in_obj->short_description);
    else if (i->worn_by)
      sprintf(buf, "%s is being worn by %s.\n\r",
              i->short_description, PERS(i->worn_by, ch));
    else
      sprintf(buf, "%s's location is uncertain.\n\r",
              i->short_description);

    CAP(buf);
    send_to_char(buf, ch);
    j--;
  }

  if (j == level >> 1)
    send_to_char("You sense nothing.\n\r", ch);
}



ASPELL(spell_magic_jar)
{ 
  one_argument(argument, arg);

  if (GET_LEVEL(ch) < (GET_LEVEL(victim) + 10)) 
    send_to_char("You are not mighty enough to overthrow their mind!\r\n", ch);
  else if (GET_MAX_HIT(victim) > 2500)
    send_to_char("They are too strong!\r\n", ch);
  else if (IS_AFFECTED(victim, AFF_SANCTUARY)) 
    send_to_char("Their sanctuary protects them from your evil mind!\r\n", ch);
  else if (!IS_NPC(victim)) 
    send_to_char("You cannot posess a player.. yet.\r\n", ch);
  else if (ch->desc->original)
    send_to_char("You're already switched.\r\n", ch);
  else if (!*arg)
    send_to_char("Switch with who?\r\n", ch);
  else if (mag_savingthrow(victim, SAVING_SPELL)) 
    send_to_char("Your intended victim boldly shrugs your spell!\r\n", ch);
  else if (!(victim = get_char_vis(ch, arg)))
    send_to_char("No such character.\r\n", ch);
  else if (ch == victim)
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
  else if (victim->desc)
    send_to_char("You can't do that, the body is already in use!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
  else {
    send_to_char(OK, ch);

/* PETS */
    if (HAS_PET(ch)) {
      GET_OWNER_DESC(GET_PET(ch)) = NULL;
      if (ch->desc) GET_PET(ch->desc) = NULL;
    }
/* END of PETS */

    SET_BIT(AFF2_FLAGS(victim), AFF2_JARRED);
/* victim->master = ch; */

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }  
}

  

ASPELL(spell_phase_door)
{
  int to_room;
  extern int top_of_world;


  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char("Their strong will prevents you from phasing to them!\r\n",
      ch);
    return;
  }

  if (IS_SET(world[victim->in_room].room_flags, ROOM_PRIVATE | ROOM_GODROOM |
             ROOM_DEATH | ROOM_NOPHASEDOOR) ||
      ZONE_FLAGGED(victim->in_room, ZONE_NOPHASEDOOR) ||
      ZONE_FLAGGED(victim->in_room, ZONE_GODZONE)) {
    send_to_char("You cannot go there by magical means!\r\n", ch);
    return;
  }

  act("$n disappears suddenly.", TRUE, ch, 0, 0, TO_ROOM);

  if (number(0, 1) == 1) {
    do {
      to_room = number(0, top_of_world);
    } while (IS_SET(world[to_room].room_flags, ROOM_PRIVATE | ROOM_GODROOM |
                    ROOM_DEATH | ROOM_NOPHASEDOOR) ||
             ZONE_FLAGGED(to_room, ZONE_NOPHASEDOOR) ||
             ZONE_FLAGGED(to_room, ZONE_GODZONE));
    send_to_char("You become lost between reality and unreality!\r\n", ch);
    char_from_room(ch);
    char_to_room(ch, to_room);
    look_at_room(ch, 0);
    return;
  } else {
    if (ZONE_FLAGGED(victim->in_room, ZONE_NOPHASEDOOR)) {
      send_to_char("You cannot go there by magical means!\r\n", ch);
      return;
    }
    char_from_room(ch);
    char_to_room(ch, victim->in_room);
    act("$n steps out from behind an atom and grins at you!", FALSE, ch, 0,
        victim, TO_VICT);
    look_at_room(ch, 0);
  }
}



#define PORTAL_OBJ  20   /* the vnum of the portal object */

ASPELL(spell_portal)
{
  struct obj_data *portal, *tportal;
  struct extra_descr_data *new_descr, *new_tdescr;
  char buf[80];


  if (ch == NULL || victim == NULL)
    return;

  /*
   * creating the portals was changed to use the vnum instead of real
   * number of the target room and to use object value[1] instead of
   * object timer to store how long it lasts
   */

  /* create the portal */
  portal = read_object(PORTAL_OBJ, VIRTUAL);
  GET_OBJ_VAL(portal, 0) = world[victim->in_room].number;
  GET_OBJ_TIMER(portal) = (int) (GET_LEVEL(ch) / 10);
  CREATE(new_descr, struct extra_descr_data, 1);
  new_descr->keyword = str_dup("portal gate gateway");
  sprintf(buf, "You can barely make out %s.\r\n", world[victim->in_room].name);
  new_descr->description = str_dup(buf);
  new_descr->next = portal->ex_description;
  portal->ex_description = new_descr;
  obj_to_room(portal, ch->in_room);
  act("$n conjures a portal out of thin air.",
       TRUE, ch, 0, 0, TO_ROOM);
  act("You conjure a portal out of thin air.",
       TRUE, ch, 0, 0, TO_CHAR);

  /* create the portal at the other end */
  tportal = read_object(PORTAL_OBJ, VIRTUAL);
  GET_OBJ_VAL(tportal, 0) = world[ch->in_room].number;
  GET_OBJ_TIMER(tportal) = (int) (GET_LEVEL(ch) / 10);
  CREATE(new_tdescr, struct extra_descr_data, 1);
  new_tdescr->keyword = str_dup("portal gate gateway");
  sprintf(buf, "You can barely make out %s.\r\n", world[ch->in_room].name);
  new_tdescr->description = str_dup(buf);
  new_tdescr->next = tportal->ex_description;
  tportal->ex_description = new_tdescr;
  obj_to_room(tportal, victim->in_room);
  act("A glowing portal appears out of thin air.",
       TRUE, victim, 0, 0, TO_ROOM);
  act("A glowing portal opens here for you.",
       TRUE, victim, 0, 0, TO_CHAR);
}



ASPELL(spell_recall)
{
  /* extern sh_int r_mortal_start_room; */
/*
  extern sh_int r_lowbie_start_room;
  extern sh_int r_race_start_room[NUM_RACES];
  extern sh_int r_clan_start_room[NUM_CLANS];
*/
  bool recall_pet = FALSE;


  if (victim == NULL || IS_NPC(victim))
    return;

  if (IS_AFFECTED(victim, AFF_CURSE)) {
    if (victim == ch)
      send_to_char("The curse on you prevents the recall!\r\n", ch);
    else
      send_to_char("Magical forces try to recall you"
                   " but the curse on you prevents it!\r\n", victim);
    act("A flash of green light fills the room, dispelling $n's magic!",
        TRUE, victim, 0, 0, TO_ROOM);
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  if (HAS_PET(victim)) {
    if (GET_PET(victim)->in_room == victim->in_room) {
      act("$n disappears.", TRUE, GET_PET(victim), 0, 0, TO_ROOM);
      char_from_room(GET_PET(victim));
      recall_pet = TRUE;
    }
  }
  char_from_room(victim);
/*
  if ((GET_CLAN(victim) != CLAN_UNDEFINED) &&
      (GET_CLAN(victim) != CLAN_NOCLAN) &&
      (GET_CLAN(victim) != CLAN_PLEDGE) &&
      (GET_CLAN(victim) != CLAN_BLACKLISTED))
    char_to_room(victim, r_clan_start_room[(int) GET_CLAN(victim)]);
  else {
    if (GET_LEVEL(victim) <= LVL_LOWBIE)
      char_to_room(victim, r_lowbie_start_room);
    else
      char_to_room(victim, r_race_start_room[(int) GET_RACE(victim)]);
  }
*/
  char_to_room(victim, GET_RECALL(victim));

  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  if (recall_pet == TRUE) {
    act("$n appears in the middle of the room.", TRUE, GET_PET(victim), 0, 0, TO_ROOM);
    char_to_room(GET_PET(victim), victim->in_room);
  }
}



ASPELL(spell_scare)
{
  ACMD(do_flee);

  if (MOB_FLAGGED(victim, MOB_SHOPKEEPER)
	|| MOB_FLAGGED(victim, MOB_DSHOPKEEPER)
	|| MOB_FLAGGED(victim, MOB_SAFE)) {
    send_to_char("Your intended victim boldly shrugs your spell!\r\n", ch);
    return;
  }

  if (mag_savingthrow(victim, SAVING_SPELL)) {
    send_to_char("Your intended victim boldly shrugs your spell!\r\n", ch);
    return;
  } else
    do_flee(victim, "", 0, SCMD_FLEE_ALWAYS);
}


/* HACKED for windwalk or some name */
ASPELL(spell_windwalk)
{
  bool windwalk_pet = FALSE;
  
  if (ch == NULL || victim == NULL)
    return;

  if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE)) {
    send_to_char("That person has summon protection on!\r\n", ch);
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char("You cannot windwalk to monsters!\r\n", ch);
    return;
  }

/* terrain and room types that does NOT allow windwalk.  If new types
   are added in, code will have to be revised */

    if (((world[ch->in_room].sector_type) == SECT_INSIDE) || 
       ((world[ch->in_room].sector_type) == SECT_CITY) || 
       ((world[ch->in_room].sector_type) == SECT_ASTRAL) || 
       ((world[ch->in_room].sector_type) == SECT_UNDERWATER) || 
       ((world[ch->in_room].room_flags) == ROOM_INDOORS) ||
       ((world[ch->in_room].room_flags) == ROOM_TUNNEL) || 
       ((world[ch->in_room].room_flags) == ROOM_HOUSE) ||
       ((world[ch->in_room].room_flags) == ROOM_HOUSE_CRASH) ||
       ((world[ch->in_room].room_flags) == ROOM_ATRIUM)) {
    send_to_char("You attempt to walk the wind, but no true wind blows here.\r\n", ch);
    return;
  } 
  
  if (ROOM_FLAGGED(victim->in_room, ROOM_SOLITARY)) {
    send_to_char("There is no space there to windwalk too!\r\n", ch);
    return;
  }

  if (POOFOUT(ch))
    sprintf(buf, "$n %s", POOFOUT(ch));
  else
  
  if (HAS_PET(ch)) if (GET_PET(ch)->in_room == ch->in_room) {
    windwalk_pet = TRUE;
  }    

  char_from_room(ch);
  char_to_room(ch, victim->in_room);

  if (POOFIN(ch))
    sprintf(buf, "$n %s", POOFIN(ch));
  else 
    sprintf(buf, "The air begins to swirl around you as $n materializes from a gale.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  act("You become one with the wind and ride to $N.", FALSE, ch, 0, victim, TO_CHAR);

  look_at_room(ch, 0);
  if (windwalk_pet) {
    act("$n slowly fades into mist and is carried away on a warm breeze.",
        TRUE, GET_PET(ch), 0, 0, TO_ROOM);
    char_from_room(GET_PET(ch));
    char_to_room(GET_PET(ch), ch->in_room);
    act("The air begins to swirl around you as $n materializes from a gale.",
        TRUE, GET_PET(ch), 0, 0, TO_ROOM);
  }
}

/* end of hack */

ASPELL(spell_relocate)
{
  bool relocate_pet = FALSE;
  
  if (ch == NULL || victim == NULL)
    return;

  if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE)) {
    send_to_char("That person has summon protection on!\r\n", ch);
    return;
  }

  if (IS_AFFECTED(victim, AFF_CURSE)) {
    act("A flash of green light fills the room, dispelling $n's magic!",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("That person is cursed!\r\n", ch);
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char("You cannot relocate to monsters!\r\n", ch);
    return;
  }

  if (FIGHTING(victim) && (number_range(1, 100) < 70)) {
    send_to_char("You can't get a clear idea of that person's location.\r\n", ch);
    return;
  }

  if (ROOM_FLAGGED(victim->in_room, ROOM_SOLITARY)) {
    send_to_char("Your magic can find no space to enter!\r\n", ch);
    return;
  }

  if (HAS_PET(ch)) if (GET_PET(ch)->in_room == ch->in_room) relocate_pet = TRUE;

  if (POOFOUT(ch))
    sprintf(buf, "$n %s", POOFOUT(ch));
  else
    sprintf(buf, "$n disappears suddenly.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, victim->in_room);

  if (POOFIN(ch))
    sprintf(buf, "$n %s", POOFIN(ch));
  else
    sprintf(buf, "$n arrives suddenly.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  act("You relocate to $N!", FALSE, ch, 0, victim, TO_CHAR);

  look_at_room(ch, 0);
  if (relocate_pet) {
    act("$n disappears suddenly.", TRUE, GET_PET(ch), 0, 0, TO_ROOM);
    char_from_room(GET_PET(ch));
    char_to_room(GET_PET(ch), ch->in_room);
    act("$n arrives suddenly.", TRUE, GET_PET(ch), 0, 0, TO_ROOM);
  }
}



ASPELL(spell_summon)
{
  bool summon_pet = FALSE;
  
  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char("You failed.\r\n", ch);
    return;
  }

  if IS_NPC(victim) {
    send_to_char("You cannot summon monsters!\r\n", ch);
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOLITARY)) {
    send_to_char("There is nowhere to summon anyone to!\r\n", ch);
    return;
  }

  if (FIGHTING(victim) && (number_range(1, 100) < 70)) {
    send_to_char("No one responds to your call.\r\n", ch);
    send_to_char("You feel a distant calling but are locked in combat.\r\n", victim);
    return;
  }

  if (!pk_allowed) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
          "through time and space towards you, you realize that $E is\r\n"
          "aggressive and might harm you, so you wisely send $M back.",
          FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
        !PLR_FLAGGED(victim, PLR_KILLER)) {
      sprintf(buf, "%s just tried to summon you to: %s.\r\n"
              "%s failed because you have summon protection on.\r\n"
              "Type NOSUMMON to allow other players to summon you.\r\n",
              GET_NAME(ch), world[ch->in_room].name,
              (ch->player.sex == SEX_MALE) ? "He" : "She");
      send_to_char(buf, victim);

      sprintf(buf, "You failed because %s has summon protection on.\r\n",
              GET_NAME(victim));
      send_to_char(buf, ch);

      sprintf(buf, "%s failed summoning %s to %s.",
              GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      return;
    }
  }

  if (IS_AFFECTED(victim, AFF_CURSE)) {
    act("A flash of green light fills the room, dispelling $n's magic!",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("That person is cursed!\r\n", ch);
    return;
  }

  if (IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL)) {
    send_to_char("You failed.\r\n", ch);
    return;
  }

  if (HAS_PET(victim)) if (GET_PET(victim)->in_room == victim->in_room) summon_pet = TRUE;

  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, ch->in_room);

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
  
  if (summon_pet) {
    act("$n disappears suddenly.", TRUE, GET_PET(victim), 0, 0, TO_ROOM);
    char_from_room(GET_PET(victim));
    char_to_room(GET_PET(victim), victim->in_room);
    act("$n arrives suddenly.", TRUE, GET_PET(victim), 0, 0, TO_ROOM);
  }
}



ASPELL(spell_teleport)
{
  int to_room;
  extern int top_of_world;


  if (ch == NULL)
    return;

  if (IS_AFFECTED(ch, AFF_CURSE)) {
    send_to_char("The curse on you prevents the teleport!\r\n", ch);
    act("A flash of green light fills the room, dispelling $n's magic!",
        TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  do {
    to_room = number(0, top_of_world);
  } while (IS_SET(world[to_room].room_flags, ROOM_PRIVATE | ROOM_GODROOM |
                  ROOM_DEATH | ROOM_NOTELEPORT) ||
           ZONE_FLAGGED(to_room, ZONE_NOTELEPORT) ||
           ZONE_FLAGGED(to_room, ZONE_GODZONE) ||
           ROOM_FLAGGED(to_room, ROOM_SOLITARY));

  act("$n slowly fades out of existence and is gone.",
      FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, to_room);
  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}



ASPELL(spell_wizard_lock)
{
  int door;

  extern char *dirs[];

  extern void do_doorcmd(struct char_data *ch, struct obj_data *obj,
    int door, int scmd);

  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (EXIT(ch, door)) {
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
        if (EXIT(ch, door)->keyword) {
          sprintf(buf, "You close the %s %s...\r\n",
                  fname(EXIT(ch, door)->keyword), dirs[door]);
          send_to_char(buf, ch);
          do_doorcmd(ch, obj, door, SCMD_CLOSE);
        }
      }
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
        if (EXIT(ch, door)->keyword) {
          sprintf(buf, "You lock the %s %s...\r\n",
                  fname(EXIT(ch, door)->keyword), dirs[door]);
          send_to_char(buf, ch);
          do_doorcmd(ch, obj, door, SCMD_LOCK);
        }
      }
    }
  }
}

ASPELL(spell_create_weapon) {
  int rnum;
  struct obj_data *tobj;
  
  struct obj_data *read_object(int nr, int type);
  
#ifndef DRAGONSLAVE
  rnum = 0;
  return;
#else
  
  if (ch->in_room == NOWHERE) return;
  
  if ((rnum = real_object(16101)) > 0) {
    tobj = read_object(rnum, REAL);
    obj_to_room(tobj, ch->in_room);
    act("As $n reads the scroll in $s hand, $e is lifted into the air and\r\n"
        "surrounded by a swirling vortex. As the last word is incanted, $e falls\r\n"
        "back to the ground, and the air in front of $m shivers through a vast array\r\n"
        "of blues and whites defying description. This light show solidifies into a\r\n"
        "strange, transparent sword, which falls noiselessly to the ground.", TRUE,
        ch, 0, 0, TO_ROOM);
    act("As you read the scroll in your hand, you are lifted into the air and\r\n"
        "surrounded by a swirling vortex. As the last word is incanted, you fall\r\n"
        "back to the ground, and the air in front of you shivers through a vast array\r\n"
        "of blues and whites defying description. This light show solidifies into a\r\n"
        "strange, transparent sword, which falls noiselessly to the ground.", TRUE,
        ch, 0, 0, TO_CHAR);
  }
  sprintf(buf, "%s has created the Sword of Dismissal!", GET_NAME(ch));
  log(buf);
#endif
}

ASPELL(spell_remove_curse) {

    int i, num_wears;
    struct obj_data *next_obj;
    
    if (!victim) {
      if (!obj) {
        /* Erk! */
        return;
      }
      if (CAN_SEE_OBJ(ch, obj)) {
	if ((IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
			&& !IS_OBJ_STAT(obj, ITEM_SUPERCURSED)) {
	    if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
		send_to_char( "You feel a burden released.\n\r", victim );
		return;
	    }
	    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NOREMOVE);
		send_to_char( "You feel a burden released.\n\r", victim );
		return;
	    }
	}
      } 
      return;
    }

    if (affected_by_spell(victim, SPELL_CURSE))
    {
	affect_from_char(victim, SPELL_CURSE);

	send_to_char( "The weight of your curse is lifted.\n\r", victim );
	if ( ch != victim )
	    act( "You dispel the curses afflicting $N.", TRUE, ch, 0, victim, TO_CHAR );
	return;
    }
    else
    for (obj = victim->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj)) {
	if ((IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
			&& !IS_OBJ_STAT(obj, ITEM_SUPERCURSED)) {
	    if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
		send_to_char( "You feel a burden released.\n\r", victim );
		return;
	    }
	    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NOREMOVE);
		send_to_char( "You feel a burden released.\n\r", victim );
		return;
	    }
	}
      } 
    }   

    num_wears = IS_THRIKREEN(victim)? NUM_THRI_WEARS : NUM_WEARS;

    for (i = 0; i < num_wears; i++) {
	if (victim->equipment[i]) {
	    obj = victim->equipment[i];
	    if (CAN_SEE_OBJ(ch, obj)) {
		if ((IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
			&& !IS_OBJ_STAT(obj, ITEM_SUPERCURSED)) {
		    if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
			REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
			send_to_char( "You feel a burden released.\n\r", victim );
			return;
		    }
		    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
			REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NOREMOVE);
			send_to_char( "You feel a burden released.\n\r", victim );
			return;
		    }
		}
	    } 
	}
    }
    return;
}
