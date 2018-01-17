/*************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

#define READ_TITLE(ch) (GET_SEX(ch) == SEX_MALE ?   \
	titles[(int)GET_CLASS(ch)][(int)GET_LEVEL(ch)].title_m :  \
	titles[(int)GET_CLASS(ch)][(int)GET_LEVEL(ch)].title_f)


extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct title_type titles[NUM_CLASSES][LVL_IMPL + 1];
/* HACKED to use the new experience table */
extern int experience_table[LVL_IMPL + 1];
/* end of hack */
extern struct room_data *world;
extern int max_exp_gain;
extern int max_exp_loss;
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
int number_range(int from, int to);


/* NeXTs need getpid defined for some reason */
#ifdef NeXT
int getpid(void);       /* /NextDeveloper/Headers/bsd/libc.h */
#endif



/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
float graf(float age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (age < 15)
    return (p0);		/* < 15   */
  else if (age <= 29)
    return (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (age <= 44)
    return (p2 + (((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (age <= 59)
    return (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (age <= 79)
    return (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);		/* >= 80 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */

int mana_gain(struct char_data * ch)
{
    int gain;

    if ( IS_NPC(ch) )
    {
	gain = GET_LEVEL(ch);
    }
    else
    {
	gain = number_range( 2, UMAX(3, GET_LEVEL(ch) / 8) );

	if ( GET_POS(ch) < POS_SLEEPING )
	  return 0;

	switch ( GET_POS(ch) )
	{
	case POS_SLEEPING: gain += GET_INT(ch) / 1.25;	break;
	case POS_RESTING: gain += GET_INT(ch) / 2;	break;
	}

	if ( GET_COND(ch, FULL) == 0 )
	    gain /= 2;

	if ( GET_COND(ch, THIRST) == 0 )
	    gain /= 2;

    }

    if ( IS_AFFECTED(ch, AFF_POISON) )
	gain /= 4;

    if (ROOM_FLAGGED(ch->in_room, ROOM_HYPERREGEN)) gain *= 1.25;

  return (gain);
}
/*
int mana_gain(struct char_data * ch)
{
  float gain;

  if (ch->in_room == NOWHERE) return 0;

  if (IS_NPC(ch)) {
    if (IS_AFFECTED(ch, AFF_POISON)) {
      gain = (GET_MAX_HIT(ch) / 10);
    } else {
      gain = (GET_MAX_HIT(ch) / 3);
    }
  } else {
    gain = graf(age(ch).year, 4, 8, 12, 15, 17, 18, 19);

    switch (GET_POS(ch)) {
        case POS_SLEEPING:
        case POS_RESTING:
            gain += (gain  / 2);
            break;
        case POS_SITTING:
            gain += (gain / 4);
            break;
    }

    switch (GET_CLASS(ch)) {
        case CLASS_MAGIC_USER:
        case CLASS_CLERIC:
        case CLASS_BARD:
        case CLASS_DRUID:
            gain *= 2;
            break;
        default:
            break;
    }
  }

  if (IS_AFFECTED(ch, AFF_POISON))
    gain /= 4;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain /= 4;

  switch (GET_CLASS(ch)) {
      case CLASS_MAGIC_USER:
      case CLASS_CLERIC:
      case CLASS_VAMPIRE:
          if ((GET_SKILL(ch, SKILL_MEDITATE) > 50)
              && (GET_POS(ch) == POS_RESTING)) {
            gain = (gain * 2);
          }
          break;
      case CLASS_BARD:
          if ((GET_SKILL(ch, SKILL_MEDITATE) > 50)
              && (GET_POS(ch) == POS_RESTING)) {
            gain = (gain * 1.5);
          }
          break;
      case CLASS_DRUID:
          if ((world[ch->in_room].sector_type) == SECT_FOREST) {
            gain = (gain * 2.5);
          break;
          }
          if ((GET_SKILL(ch, SKILL_MEDITATE) > 50)
              && (GET_POS(ch) == POS_RESTING)) {
            gain = (gain * 1.5);
          }
          break;
      default:
          break;
  };

  if (ROOM_FLAGGED(ch->in_room, ROOM_HYPERREGEN)) gain *= 1.25;
}
*/

int hit_gain(struct char_data * ch)
{
    int gain;

    if ( IS_NPC(ch) && !IS_AFFECTED2(ch, AFF2_JARRED)
				&& !IS_AFFECTED(ch, AFF_CHARM) )
    {
	gain = number_range(GET_LEVEL(ch)/2, GET_LEVEL(ch)*3/2);
    }
    else
    {
	gain = number_range( 2, UMAX(3, GET_LEVEL(ch) / 8) );

	if (IS_THRIKREEN(ch)) {
		gain += GET_CON(ch) / 1.25;
	} else {
	   	switch ( GET_POS(ch) )
	   	{
	   	case POS_DEAD:	   return 0;
	   	case POS_MORTALLYW:
	   	    if (IS_NPC(ch))
	   		return 1;
	   	    else
	   		return -1;
	   	case POS_INCAP:    return -1;
	   	case POS_STUNNED:  return 1;
	   	case POS_SLEEPING: gain += GET_CON(ch) / 1.25;	break;
	   	case POS_RESTING: gain += GET_CON(ch) / 2;	break;
   		}
   	}

        if ( IS_VAMPIRE(ch) ) {
	    if ( GET_COND(ch, THIRST) == 0 )
		gain /= 2;

            if ( OUTSIDE( ch ) )
  	    {
    	       switch(weather_info.sunlight)
    	       {
    	          case SUN_RISE:
    		  case SUN_SET:
      	            gain /= 2;
      	            break;
    		  case SUN_LIGHT:
      		    gain /= 4;
      		    break;
      	       }
      	    }
        }

	if ( GET_COND(ch, FULL) == 0 )
	    gain /= 2;

	if ( GET_COND(ch, THIRST) == 0 )
	    gain /= 2;

    }

    if ( IS_AFFECTED(ch, AFF_POISON) )
	gain /= 4;

    if (ROOM_FLAGGED(ch->in_room, ROOM_HYPERREGEN)) gain *= 1.25;

  return (gain);
}

/* Hitpoint gain pr. game hour 
int hit_gain(struct char_data * ch)
{
  int gain;
  int quickhealed = 0;
  int troll = 0;
  int pos;


  if (ch->in_room == NOWHERE) return 0;

  if (IS_NPC(ch)) {
    if (GET_LEVEL(ch) > 5)
      gain = GET_LEVEL(ch) * (GET_LEVEL(ch) / 5);
    else
      gain = GET_LEVEL(ch);
  } else {

    gain = graf(age(ch).year, 8, 12, 20, 30, 34, 36, 38);

    pos = GET_POS(ch);
    if (IS_THRIKREEN(ch) && pos != POS_FIGHTING) pos = POS_SLEEPING;

    switch (pos) {
        case POS_SLEEPING:
            switch (GET_CLASS(ch)) {
                case CLASS_WARRIOR:
                case CLASS_THIEF:
                case CLASS_DEATHKNIGHT:
                    if (GET_SKILL(ch, SKILL_QUICKHEAL) > 50) {
                      gain = MAX(30, ((GET_LEVEL(ch) * 3)+(GET_CON(ch) * 2)));
                      quickhealed = 1;
                    } else {
                      gain = 30;
                    }
                    break;
                case CLASS_MAGIC_USER:
                    gain = MAX(15, (GET_LEVEL(ch) * 2));
                    break;
                case CLASS_BARD:
                    gain = MAX(20, (GET_LEVEL(ch) * 2.2));
                    break;
                case CLASS_CLERIC:
                default:
                    gain = MAX(10, (GET_LEVEL(ch)));
                    break;
                case CLASS_DRUID:
                    if ((world[ch->in_room].sector_type) == SECT_FOREST) {
                      send_to_char("Your forest surroundings instill you with energy.\r\n", ch);
                      gain = (gain * 1.5);
                    } else {
                      gain = MAX(10, (GET_LEVEL(ch)));
                    }
                    break;
            }
            break;
        case POS_RESTING:
            gain = MAX(10, (GET_LEVEL(ch)));
            break;
        case POS_SITTING:
            gain += (gain >> 3);
            break;
        default: 
            break;
    }
  }

  if (GET_RACE(ch) == RACE_TROLL) {
    gain += gain / 10;
    troll = 1;
  }

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;

  if (ROOM_FLAGGED(ch->in_room, ROOM_HYPERREGEN)) gain *= 1.25;

  return (gain);
}
*/



int move_gain(struct char_data * ch)
/* move gain pr. game hour */
{
  int gain, pos;
  if (ch->in_room == NOWHERE) return 0;

  if (IS_NPC(ch)) {
    return (GET_LEVEL(ch));
    /* Neat and fast */
  } else {
/* HACKED to change gain with age */
/* old gain
    gain = graf(age(ch).year, 16, 20, 24, 20, 16, 12, 10);
*/
    gain = graf(age(ch).year, 16, 20, 24, 26, 20, 18, 14);
/* end of hack */

    /* Class/Level calculations */

    /* Skill/Spell calculations */


    /* Position calculations    */
    pos = GET_POS(ch);
    if (IS_THRIKREEN(ch) && pos != POS_FIGHTING) pos = POS_SLEEPING;
    
    switch (pos) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
    }
  }

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;
    
  if (ch->in_room == NOWHERE) return 0;

  if (ROOM_FLAGGED(ch->in_room, ROOM_HYPERREGEN)) gain *= 1.25;

  return (gain) * 3 / 2;
}



void set_title(struct char_data * ch, char *title)
{
  if (title == NULL)
    title = READ_TITLE(ch);

  if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_REAL_TITLE(ch) != NULL)
    free(GET_REAL_TITLE(ch));

  GET_REAL_TITLE(ch) = strdup(title);
}


void check_autowiz(struct char_data * ch)
{
  char buf[100];
  extern int use_autowiz;
  extern int min_wizlist_lev;
/* NeXT hack -tyoud */
#ifndef NeXT
  pid_t getpid(void);
#endif

  if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
    mudlog("Initiating autowiz.", CMP, LVL_IMMORT, FALSE);
    system(buf);
  }
}



void gain_exp(struct char_data * ch, int gain) {
  int is_altered = FALSE;
  int num_levels = 0;

  if (!IS_NPC(ch) && (GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }

  if (gain > 0) {
    gain = MIN(max_exp_gain, gain); /* put a cap on the max gain per kill */
    GET_EXP(ch) += gain;
    while (GET_LEVEL(ch) < LVL_IMMORT - 1 &&
      GET_EXP(ch) >= titles[(int) GET_CLASS(ch)][GET_LEVEL(ch) + 1].exp) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      char messgbuf[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(messgbuf, sizeof(messgbuf),
	"%s advanced %d level%s to level %d.",
	GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
	GET_LEVEL(ch));
      mudlog(messgbuf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

      if (num_levels == 1) {
	send_to_char("You rise a level!\r\n", ch);
      } else {
	snprintf(messgbuf, sizeof(messgbuf), "You rise %d levels.\r\n", num_levels);
	send_to_char(messgbuf, ch);
      }
      set_title(ch, NULL);
      if (GET_LEVEL(ch) >= LVL_IMMORT)
	check_autowiz(ch);
    }
  } else if (gain < 0) {
    gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
}


void gain_exp_regardless(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMPL &&
	GET_EXP(ch) >= titles[(int) GET_CLASS(ch)][GET_LEVEL(ch) + 1].exp) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      char messgbuf[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(messgbuf, sizeof(messgbuf),
	"%s advanced %d level%s to level %d.",
	GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
	GET_LEVEL(ch));
      mudlog(messgbuf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

      if (num_levels == 1) {
	send_to_char("You rise a level!\r\n", ch);
      } else {
	snprintf(messgbuf, sizeof(messgbuf), "You rise %d levels.\r\n", num_levels);
	send_to_char(messgbuf, ch);
      }
      set_title(ch, NULL);
      if (GET_LEVEL(ch) >= LVL_IMMORT)
	check_autowiz(ch);
    }
  }
}


void gain_condition(struct char_data * ch, int condition, int value)
{
  bool intoxicated;

  if (GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case FULL:
    send_to_char("You are hungry.\r\n", ch);
    return;
  case THIRST:
    send_to_char("You are thirsty.\r\n", ch);
    return;
  case DRUNK:
    if (intoxicated)
      send_to_char("You are now sober.\r\n", ch);
    return;
  default:
    break;
  }
}


void check_idling(struct char_data * ch)
{
  extern int free_rent;
  void Crash_rentsave(struct char_data *ch, int cost);

  if (++(ch->char_specials.timer) > 8)
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
      save_char(ch, NOWHERE);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->char_specials.timer > 48) {
      if (ch->in_room != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc)
	close_socket(ch->desc);
      ch->desc = NULL;
/* Let's comment this check, possible source of rent eq-loss */
      if (free_rent)
	Crash_rentsave(ch, 0);
      else {
	Crash_idlesave(ch);
      sprintf(buf, "%s idle-saved, this should not be.", GET_NAME(ch));
      mudlog(buf, CMP, LVL_GOD, TRUE);
	}
      sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
      mudlog(buf, CMP, LVL_GOD, TRUE);
      extract_char(ch);
    }
}


void update_specials(void)
{
  struct char_data *i, *next_char;

  for (i = character_list; i; i = next_char) {
    next_char = i->next;
/*
   if (!(IS_WARRIOR2(i) || IS_CLERIC2(i) || IS_MAGE2(i) || IS_THIEF2(i) || IS_BARD2(i)))
     return;
   if (IS_WARRIOR2(i))
     send_to_char("A surge of power flows through your body!", i);
   if (IS_CLERIC2(i))
     send_to_char("Your deity rewards your faithful service!", i);
   if (IS_MAGE2(i))
     send_to_char("Ascending powers flow through your body!", i);
   if (IS_BARD2(i))
     send_to_char("You burst into song as powerful music flows into you!", i);
   if (IS_THIEF2(i))
     send_to_char("A surge of power flows through your body!", i);
*/

   if (GET_POS(i) >= POS_STUNNED) {
      if (GET_LEVEL(i) < 15) {
        GET_SPECIAL(i) = 1;
        return;         
      }
      if (GET_LEVEL(i) < 35) {
        GET_SPECIAL(i) = 2;
        return;
      }
    }  
  }
}
   
/* Update PCs, NPCs, and objects */
void point_update(void)
{
  struct char_data *i, *next_char;
/*
  void update_char_objects(struct char_data * ch);
  void extract_obj(struct obj_data * obj);
  bool obj_rot_prog(struct obj_data *obj);
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  char tmp_name[MAX_STRING_LENGTH];
  int where = NOWHERE;
  int r_num = -1;
  int number;
  struct obj_data *obj;
  struct char_data *mob;
*/

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    if (IS_NPC(i)) {
      GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
    } else {
	if (GET_POS(i) >= POS_STUNNED) {
	    GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
	    GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
	    GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
	} else if (GET_POS(i) == POS_INCAP)
	    damage(i, i, 1, TYPE_SUFFERING);
	else if (GET_POS(i) == POS_MORTALLYW)
	    damage(i, i, 2, TYPE_SUFFERING);
    }
  }
}

void obj_update(void) {

  void update_char_objects(struct char_data * ch);	/* handler.c */
  void extract_obj(struct obj_data * obj);	/* handler.c */
  bool obj_rot_prog(struct obj_data *obj);
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  char tmp_name[MAX_STRING_LENGTH];
  int where = NOWHERE;
  int r_num = -1;
  int number;
  struct obj_data *obj;
  struct char_data *mob;

/* this sucks here, but it has to be for now. */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    if (!IS_THRIKREEN(i)) gain_condition(i, THIRST, -1);
    if (!IS_NPC(i)) {
      update_char_objects(i);
      if (GET_LEVEL(i) < LVL_GOD)
        check_idling(i);
    }
    if (IS_AFFECTED(i, AFF_POISON)) {
      act("$n looks really sick and shivers uncomfortably.", TRUE, i, 0, 0, TO_ROOM);
      send_to_char("You feel poison burning in your blood, and suffer.\r\n", i);
      damage(i, i, (3 * GET_LEVEL(i)), SPELL_POISON);
    }
    if (GET_POS(i) <= POS_STUNNED)
      update_pos(i);
  }


  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */

    /* If this is a corpse */
    if ((GET_OBJ_TYPE(j) == ITEM_CONTAINER) && GET_OBJ_VAL(j, 3)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

	if (j->carried_by) {
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
        } else if (j->worn_by) {
          act("$p decays on your body.", FALSE, j->worn_by, j, 0, TO_CHAR);

          if ((j->obj_flags.bitvector > 0) ||
              (j->obj_flags.bitvector2 > 0)) {
            send_to_char("You feel magical power drain from you!\r\n",
                j->worn_by);
          }

	} else if ((j->in_room != NOWHERE) && (world[j->in_room].people)) {
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[j->in_room].people, j, 0, TO_CHAR);
	}
	for (jj = j->contains; jj; jj = next_thing2) {
	  next_thing2 = jj->next_content;	/* Next in inventory */
	  obj_from_obj(jj);

	  if (j->in_obj)
	    obj_to_obj(jj, j->in_obj);
	  else if (j->carried_by)
	    obj_to_room(jj, j->carried_by->in_room);
          else if (j->worn_by)
            obj_to_room(jj, j->worn_by->in_room);
	  else if (j->in_room != NOWHERE)
	    obj_to_room(jj, j->in_room);
	  else
	    assert(FALSE);
	}
	extract_obj(j);
      }
    }

    /*
     * if this is an item with an object timer set
     * unfortunately, 0 means do not rot
     * -1 means set this to rot, and any positive number
     * is the turns until it rots
     */
    if (GET_OBJ_TIMER(j) != 0) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) == 1)
        GET_OBJ_TIMER(j) = -1;
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;

      if (GET_OBJ_TIMER(j) == -1) {
      
        if (obj_rot_prog(j)) {
          GET_OBJ_TIMER(j) = 0;
        } else {

          if (GET_OBJ_TYPE(j) == ITEM_SEED) {
            number = GET_OBJ_VAL(j, 1);
            switch (GET_OBJ_VAL(j, 0)) {
              case SEED_LOAD_OBJ:
                  sprintf(tmp_name, "some object");
                  if ((r_num = real_object(number)) < 0)
                    break;
                  obj = read_object(r_num, REAL);
                  sprintf(tmp_name, "%s", obj->short_description);
                  break;
              case SEED_LOAD_MOB:
                  sprintf(tmp_name, "some mob");
                  if ((r_num = real_mobile(number)) < 0)
                    break;
                  mob = read_mobile(r_num, REAL);
                  sprintf(tmp_name, "%s", GET_NAME(mob));
                  break;
              default:
                  sprintf(tmp_name, "something");
            }
          }

          if (j->carried_by) {
            switch (GET_OBJ_TYPE(j)) {
              case ITEM_FOUNTAIN:
                  act("$p dries up in your hands.", FALSE,
                      j->carried_by, j, 0, TO_CHAR);
                  break;
              case ITEM_PORTAL:
                  act("$p fades away from existance in your hands.", FALSE,
                      j->carried_by, j, 0, TO_CHAR);
                  break;
              case ITEM_SEED:
                  sprintf(buf, "$p breaks open in your hands and out comes %s.",
                      tmp_name);
                  act(buf, FALSE, j->carried_by, j, 0, TO_CHAR);
                  where = j->carried_by->in_room;
                  break;
              default:
                  act("$p deteriorates into uselessness, and you toss it aside.",
                      FALSE, j->carried_by, j, 0, TO_CHAR);
                  break;
            }
          } else if (j->worn_by) {
            switch (GET_OBJ_TYPE(j)) {
              case ITEM_FOUNTAIN:
                  act("$p dries up on your body.", FALSE,
                      j->worn_by, j, 0, TO_CHAR);
                  break;
              case ITEM_PORTAL:
                  act("$p fades away from existance on your body.", FALSE,
                      j->worn_by, j, 0, TO_CHAR);
                  break;
              case ITEM_SEED:
                  sprintf(buf, "$p breaks open on your body and out comes %s.",
                      tmp_name);
                  act(buf, FALSE, j->worn_by, j, 0, TO_CHAR);
                  where = j->worn_by->in_room;
                  break;
              default:
                  act("$p rots away on your body.", FALSE, j->worn_by, j, 0,
                      TO_CHAR);
                  break;
            }
            if ((j->obj_flags.bitvector > 0) ||
                (j->obj_flags.bitvector2 > 0)) {
              send_to_char("You feel magical power drain from you!\r\n",
                  j->worn_by);
            }
          } else if ((j->in_room != NOWHERE) && (world[j->in_room].people)) {
            switch (GET_OBJ_TYPE(j)) {
              case ITEM_FOUNTAIN:
                  act("$p dries up.",
                      TRUE, world[j->in_room].people, j, 0, TO_ROOM);
                  act("$p dries up.",
                      TRUE, world[j->in_room].people, j, 0, TO_CHAR);
                  break;
              case ITEM_PORTAL:
                  act("$p fades away from existance.",
                      TRUE, world[j->in_room].people, j, 0, TO_ROOM);
                  act("$p fades away from existance.",
                      TRUE, world[j->in_room].people, j, 0, TO_CHAR);
                  break;
              case ITEM_SEED:
                  sprintf(buf, "$p breaks open and out comes %s.", tmp_name);
                  act(buf, TRUE, world[j->in_room].people, j, 0, TO_ROOM);
                  act(buf, TRUE, world[j->in_room].people, j, 0, TO_CHAR);
                  where = j->in_room;
                  break;
              default:
                  act("$p rots away.",
                      TRUE, world[j->in_room].people, j, 0, TO_ROOM);
                  act("$p rots away.",
                      TRUE, world[j->in_room].people, j, 0, TO_CHAR);
                  break;
            }
          }

          for (jj = j->contains; jj; jj = next_thing2) {
            next_thing2 = jj->next_content;       /* Next in inventory */
            obj_from_obj(jj);

            if (j->in_obj)
              obj_to_obj(jj, j->in_obj);
            else if (j->carried_by)
              obj_to_char(jj, j->carried_by);
            else if (j->worn_by)
              obj_to_char(jj, j->worn_by);
            else if (j->in_room != NOWHERE)
              obj_to_room(jj, j->in_room);
            else
              assert(FALSE);
          }

          if ((GET_OBJ_TYPE(j) == ITEM_SEED) && (r_num > -1)) {
            switch (GET_OBJ_VAL(j, 0)) {
              case SEED_LOAD_OBJ:
                  obj_to_room(obj, where);
                  break;
              case SEED_LOAD_MOB:
                  char_to_room(mob, where);
                  break;
              default:
                  sprintf(buf, "SYSERR: unknown load type on seed obj vnum %d.",
                      GET_OBJ_VNUM(j));
                  log(buf);
                  break;
            }
          }

          extract_obj(j);
        }
      }
    }
  }
}

void perform_pulse_functions(void) {

}
