
/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
/* #include <assert.h> */

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "auction.h"


/* external vars */
bool str_prefix(const char *astr, const char *bstr);
extern int top_of_world;
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern char *MENU;
extern struct char_data *script_list;
extern int auction_on;


/* external functions */
void free_char(struct char_data * ch);
void stop_fighting(struct char_data * ch);
void remove_follower(struct char_data * ch);
void clearMemory(struct char_data * ch);
void stop_script(struct char_data * ch);
void auction_seller_gone(struct char_data * ch);
void auction_bidder_gone(struct char_data * ch);



char *fname(char *namelist)
{
  static char holder[30];
  register char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}



int isname(char *input, char *namelist) {
  if (input == NULL) {
    log("invalid `input` string");
  } else if (namelist && *namelist != '\0') {
    /* Loop while words remain. */
    register char *curtok = NULL;
    for (curtok = (char*) namelist; curtok && *curtok != '\0'; ) {
      char word[MAX_INPUT_LENGTH] = {'\0'};
      curtok = any_one_arg(curtok, word);
      if (*word != '\0' && is_abbrev(input, word))
        return (TRUE);
    }
  }
  return (FALSE);
}

#define ISEXNAME_WHITESPACE " \t"
int isexname(char *str, char *namelist)
{
  char *newlist;
  char *curtok;

  newlist = strdup(namelist); /* make a copy since strtok 'modifies' strings */

  for (curtok = strtok(newlist, ISEXNAME_WHITESPACE); curtok;
       curtok = strtok(NULL, ISEXNAME_WHITESPACE)) {
    if(curtok && (strlen(curtok) == strlen(str)) && is_abbrev(str, curtok)) {
      free(newlist); 
      return 1;
    }
  }

  free(newlist);
  return 0;
}



void affect_modify(struct char_data * ch, byte loc, sbyte mod,
    long bitvector,
    long bitvector2,
    bool add)
{
  int maxabil;
  /* Next line is part of the below hack */
  extern const int race_stat_adjust[NUM_RACES][MAX_RACE_STAT_ADJUST];
   

  if (add) {
    SET_BIT(AFF_FLAGS(ch), bitvector);
    SET_BIT(AFF2_FLAGS(ch), bitvector2);
  } else {
    REMOVE_BIT(AFF_FLAGS(ch), bitvector);
    REMOVE_BIT(AFF2_FLAGS(ch), bitvector2);
    if (!IS_NPC(ch)) {
      SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][6]);
      SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][7]);
      SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][8]);
      SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][9]);

      SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][10]);
      SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][11]);
      SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][12]);
      SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][13]);
    }
    /* End of HACK */
    mod = -mod;
  }


  /* maxabil = (IS_NPC(ch) ? 25 : 18); */
  maxabil = 25;

  switch (loc) {
  case APPLY_NONE:
    break;
  case APPLY_STR:
    GET_STR(ch) += mod;
    break;
  case APPLY_DEX:
    GET_DEX(ch) += mod;
    break;
  case APPLY_INT:
    GET_INT(ch) += mod;
    break;
  case APPLY_WIS:
    GET_WIS(ch) += mod;
    break;
  case APPLY_CON:
    GET_CON(ch) += mod;
    break;
  case APPLY_CHA:
    GET_CHA(ch) += mod;
    break;

  case APPLY_CLASS:
    /* GET_CLASS(ch) += mod;   ??? */
    break;

  case APPLY_LEVEL:
    /* ??? GET_LEVEL(ch) += mod; */
    break;

  case APPLY_AGE:
    ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT(ch) += mod;
    break;

  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT(ch) += mod;
    break;

  case APPLY_MANA:
    GET_MAX_MANA(ch) += mod;
    break;

  case APPLY_HIT:
    GET_MAX_HIT(ch) += mod;
    break;

  case APPLY_DAMROLL:
    GET_DAMROLL(ch) += mod;
    break;

  case APPLY_MOVE:
    GET_MAX_MOVE(ch) += mod;
    break;

  case APPLY_GOLD:
    break;

  case APPLY_EXP:
    break;

  case APPLY_AC:
    GET_AC(ch) += mod;
    break;

  case APPLY_HITROLL:
    GET_HITROLL(ch) += mod;
    break;

  case APPLY_SAVING_PARA:
    GET_SAVE(ch, SAVING_PARA) += mod;
    break;

  case APPLY_SAVING_ROD:
    GET_SAVE(ch, SAVING_ROD) += mod;
    break;

  case APPLY_SAVING_PETRI:
    GET_SAVE(ch, SAVING_PETRI) += mod;
    break;

  case APPLY_SAVING_BREATH:
    GET_SAVE(ch, SAVING_BREATH) += mod;
    break;

  case APPLY_SAVING_SPELL:
    GET_SAVE(ch, SAVING_SPELL) += mod;
    break;

  case APPLY_RACE:
    /* ??? GET_RACE(ch) += mod; */
    break;

  default:
    log("SYSERR: Unknown apply adjust attempt (handler.c, affect_modify).");
    break;

  }				/* switch */
}



/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
/* HACKED to not affect outside the bounds of normal affects		*/
void affect_total(struct char_data * ch)
{
  struct affected_type *af;
  int i, j;
/*
  int str;
*/


  for (i = 0; i < GET_NUM_WEARS(ch); i++) {
    if (ch->equipment[i])
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        if (ch->equipment[i]->affected[j].location < APPLY_NONE ||
            ch->equipment[i]->affected[j].location >= APPLY_MAX) {
          affect_modify(ch, APPLY_NONE,
                      ch->equipment[i]->affected[j].modifier,
                      ch->equipment[i]->obj_flags.bitvector,
                      ch->equipment[i]->obj_flags.bitvector2,
                      FALSE);
          log("SYSERR:  Warning: bad affect #1"
              "(handler.c affect_total())\n");
        } else {
          affect_modify(ch, ch->equipment[i]->affected[j].location,
		      ch->equipment[i]->affected[j].modifier,
		      ch->equipment[i]->obj_flags.bitvector,
		      ch->equipment[i]->obj_flags.bitvector2,
                      FALSE);
        }
      }
  }


  for (af = ch->affected; af; af = af->next) {
    if (af->location < APPLY_NONE || af->location >= APPLY_MAX) {
      affect_modify(ch, APPLY_NONE, af->modifier,
                    af->bitvector,
                    af->bitvector2,
                    FALSE);
      log("SYSERR:  Warning: bad affect #2"
          "(handler.c affect_total())\n");
    } else
      affect_modify(ch, af->location, af->modifier,
                    af->bitvector,
                    af->bitvector2,
                    FALSE);
  }

  ch->aff_abils = ch->real_abils;

  for (i = 0; i < GET_NUM_WEARS(ch); i++) {
    if (ch->equipment[i])
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        if (ch->equipment[i]->affected[j].location < APPLY_NONE ||
            ch->equipment[i]->affected[j].location >= APPLY_MAX) {
          affect_modify(ch, APPLY_NONE,
                      ch->equipment[i]->affected[j].modifier,
                      ch->equipment[i]->obj_flags.bitvector,
                      ch->equipment[i]->obj_flags.bitvector2,
                      TRUE);
          log("SYSERR:  Warning: bad affect #3"
              "(handler.c affect_total())\n");
        } else {
          affect_modify(ch, ch->equipment[i]->affected[j].location,
		      ch->equipment[i]->affected[j].modifier,
		      ch->equipment[i]->obj_flags.bitvector,
		      ch->equipment[i]->obj_flags.bitvector2,
                      TRUE);
        }
      }
  }


  for (af = ch->affected; af; af = af->next) {
    if (af->location < APPLY_NONE || af->location >= APPLY_MAX) {
      affect_modify(ch, APPLY_NONE, af->modifier,
                    af->bitvector,
                    af->bitvector2,
                    TRUE);
      log("SYSERR:  Warning: bad affect #4"
          "(handler.c affect_total())\n");
    } else {
      affect_modify(ch, af->location, af->modifier,
                    af->bitvector,
                    af->bitvector2,
                    TRUE);
    }
  }

  /* Make certain values are between 0..25, not < 0 and not > 25! */

  /* HACKED Modified to give PCs a max of 25 in their attributes */
  /* i = (IS_NPC(ch) ? 25 : 18); */
  i = 25;

  GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), i));
  GET_INT(ch) = MAX(0, MIN(GET_INT(ch), i));
  GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), i));
  GET_CON(ch) = MAX(0, MIN(GET_CON(ch), i));
  GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), i));
/*GET_STR(ch) = MAX(0, GET_STR(ch)); */
  GET_STR(ch) = MAX(0, MIN(GET_STR(ch), i));

  /* HACKED Modified stradd as well */
/*
  if (IS_NPC(ch)) {
    GET_STR(ch) = MIN(GET_STR(ch), i);
  } else {
    if (GET_STR(ch) > 18) {
      i = GET_ADD(ch) + ((GET_STR(ch) - 18) * 10);
      GET_ADD(ch) = MIN(i, 100);
      GET_STR(ch) = 18;
    }
  }
*/
/*
  if (IS_NPC(ch)) {
    GET_STR(ch) = MIN(GET_STR(ch), i);
  } else {
    if (GET_STR(ch) > 18) {
      i = GET_ADD(ch) + ((GET_STR(ch) - 18) * 10);
      GET_ADD(ch) = i % 100;
      GET_STR(ch) = MIN(18 + (i / 100), 25);
    }
  }
*/
/*
  str = (GET_STR(ch) * 100) + (GET_ADD(ch) * 10);
  GET_STR(ch) = str / 100;
  GET_ADD(ch) = str % 100;
*/
}



/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data * ch, struct affected_type * af)
{
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;


  ch->affected = affected_alloc;

  if (af->location < APPLY_NONE || af->location >= APPLY_MAX) {
    affect_modify(ch, APPLY_NONE, af->modifier,
                  af->bitvector, 
                  af->bitvector2, 
                  TRUE);
    log("SYSERR:  Warning: bad affect (handler.c affect_to_char())\n");
  } else {
    affect_modify(ch, af->location, af->modifier,
                  af->bitvector,
                  af->bitvector2,
                  TRUE);
  }
  affect_total(ch);
}



/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(struct char_data * ch, struct affected_type * af)
{
  struct affected_type *temp;

  assert(ch->affected);

  if (af->location < APPLY_NONE || af->location >= APPLY_MAX) {
    affect_modify(ch, APPLY_NONE, af->modifier,
                  af->bitvector,
                  af->bitvector2,
                  FALSE);
    log("SYSERR:  Warning: bad affect (handler.c affect_remove())\n");
  } else {
    affect_modify(ch, af->location, af->modifier,
                  af->bitvector,
                  af->bitvector2,
                  FALSE);
  }
  REMOVE_FROM_LIST(af, ch->affected, next);
  free(af);
  affect_total(ch);
}



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(struct char_data * ch, sh_int type)
{
  struct affected_type *hjp;
  struct affected_type *next_hjp;

  for (hjp = ch->affected; hjp != NULL; hjp = next_hjp) {
    next_hjp = hjp->next;
    if (hjp->type == type)
      affect_remove(ch, hjp);
  }
}



/*
 * Return if a char is affected by a spell (SPELL_XXX), NULL indicates
 * not affected
 */
bool affected_by_spell(struct char_data * ch, sh_int type)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == type)
      return TRUE;

  return FALSE;
}



void affect_join(struct char_data * ch, struct affected_type * af,
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = hjp->next) {
    if (hjp->type == af->type) {

      if (add_dur)
	af->duration += hjp->duration;
      if (avg_dur)
	af->duration >>= 1;

      if (add_mod)
	af->modifier += hjp->modifier;
      if (avg_mod)
	af->modifier >>= 1;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}

/* move a player out of a room */
void char_from_room(struct char_data * ch)
{
  struct char_data *temp;

  if (ch == NULL || ch->in_room == NOWHERE) {
    log("SYSERR: NULL or NOWHERE in handler.c, char_from_room");
    exit(1);
  }
  if (ch->equipment[GET_WEAR_LIGHT(ch)] != NULL)
    if (GET_OBJ_TYPE(ch->equipment[GET_WEAR_LIGHT(ch)]) == ITEM_LIGHT)
      if (GET_OBJ_VAL(ch->equipment[GET_WEAR_LIGHT(ch)], 2))	/* Light is ON */
	world[ch->in_room].light--;

  REMOVE_FROM_LIST(ch, world[ch->in_room].people, next_in_room);
  ch->in_room = NOWHERE;
  ch->next_in_room = NULL;
}


/* place a character in a room */
void char_to_room(struct char_data * ch, int room)
{
  if (!ch || room < 0 || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to char_to_room");
  else {
    ch->next_in_room = world[room].people;
    world[room].people = ch;
    ch->in_room = room;

    if (ch->equipment[GET_WEAR_LIGHT(ch)])
      if (GET_OBJ_TYPE(ch->equipment[GET_WEAR_LIGHT(ch)]) == ITEM_LIGHT)
	if (GET_OBJ_VAL(ch->equipment[GET_WEAR_LIGHT(ch)], 2))	/* Light ON */
	  world[room].light++;
  }
}


/* give an object to a char   */
void obj_to_char(struct obj_data * object, struct char_data * ch)
{
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    object->in_room = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    /* set flag for crash-save system */
    SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
  } else
    log("SYSERR: NULL obj or char passed to obj_to_char");
}


/* take an object from a char */
void obj_from_char(struct obj_data * object)
{
  struct obj_data *temp;

  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system */
  SET_BIT(PLR_FLAGS(object->carried_by), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
}



/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(struct char_data * ch, int eq_pos)
{
  int factor;

  assert(ch->equipment[eq_pos]);

  if (!(GET_OBJ_TYPE(ch->equipment[eq_pos]) == ITEM_ARMOR))
    return 0;

  switch (eq_pos) {

  case WEAR_BODY:
    factor = 3;
    break;			/* 30% */
  case WEAR_HEAD:
    factor = 2;
    break;			/* 20% */
  case WEAR_LEGS:
    factor = 2;
    break;			/* 20% */
  default:
    factor = 1;
    break;			/* all others 10% */
  }

  return (factor * GET_OBJ_VAL(ch->equipment[eq_pos], 0));
}



void equip_char(struct char_data * ch, struct obj_data * obj, int pos)
{
  int j;
  int why_invalid;

  int invalid_class(struct char_data *ch, struct obj_data *obj);
  int invalid_level(struct char_data *ch, struct obj_data *obj);
  void obj_worn_prog(struct obj_data *obj, struct char_data *ch);

  extern int top_of_world;

  if (!(obj && ch)) {
    log("SYSERR: NULL obj or char passed to equip_char");
    return;
  }

  if (IS_THRIKREEN(ch)) {
    assert(pos >= 0 && pos < NUM_THRI_WEARS);
  } else {
    assert(pos >= 0 && pos < NUM_WEARS);
  }

  if (ch->equipment[pos]) {
    sprintf(buf, "SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	    obj->short_description);
    log(buf);
    return;
  }
  if (obj->carried_by) {
    log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (obj->in_room != NOWHERE) {
    log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }  
  if (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) {
    act("You are zapped by $p and instantly let go of it.", FALSE, ch,
            obj, 0, TO_CHAR);
    obj_to_char(obj, ch);
    return;
  }
/* NPCs are not checked for restrictions */
  if (!IS_NPC(ch)) {
    /* changed to drop to inventory instead of to the ground */
    if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {

        act("You are zapped by $p and instantly let go of it.", FALSE, ch,
            obj, 0, TO_CHAR);
        act("$n is zapped by $p and instantly lets go of it.", FALSE, ch,
            obj, 0, TO_ROOM);
        obj_to_char(obj, ch);
        return;
    }
    /* changed to drop into inventory instead of to the ground */
    if ((why_invalid = invalid_class(ch, obj))) {
      if (why_invalid == 1)
        act("Your class cannot use $p.", FALSE, ch, obj, 0, TO_CHAR);
      else {
        if (GET_CLASS(ch) == CLASS_MAGIC_USER)
          act("Your class cannot use $p, mages can only use daggers, darts and staves.",
              FALSE, ch, obj, 0, TO_CHAR);
        else if (GET_CLASS(ch) == CLASS_CLERIC)
          act("Your class cannot use $p, clerics cannot use sharp weapons.",
              FALSE, ch, obj, 0, TO_CHAR);
        else
          act("Your class cannot use $p.", FALSE, ch, obj, 0, TO_CHAR);
      } 
      obj_to_char(obj, ch);
      return;
    }
    why_invalid = invalid_level(ch, obj);
    if (why_invalid) {
      switch(why_invalid) {
        case 1:
          act("You are too low level to use $p.", FALSE, ch, obj, 0, TO_CHAR);
          break;
        case 2:
          act("You can't use $p, it's too juvenile!", FALSE, ch, obj, 0, TO_CHAR);
          break;
        default:
          act("You can't use $p, but I don't know why.", FALSE, ch, obj, 0, TO_CHAR);
          send_to_char("Please tell an immortal!\r\n", ch);
          break;
      }
      obj_to_char(obj, ch);
      return;
    }
  }
/* end of hack */
/* end of hack */
  ch->equipment[pos] = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) -= apply_ac(ch, pos);

/*  if (ch->in_room > top_of_world)
    ch->in_room = r_mortal_start_room;
*/
  if (ch->in_room != NOWHERE) {
    if (pos == GET_WEAR_LIGHT(ch) && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
        if (ch->in_room <= top_of_world)
    	  world[ch->in_room].light++;
  } /* else */
/* HACKED to take out this errorlog, for ascii save files 
   sometimes load NOWHERE */
/*
    log("SYSERR: ch->in_room = NOWHERE when equipping char.");
*/

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    if (obj->affected[j].location < APPLY_NONE ||
        obj->affected[j].location >= APPLY_MAX) {
      affect_modify(ch, APPLY_NONE, obj->affected[j].modifier,
                  obj->obj_flags.bitvector,
                  obj->obj_flags.bitvector2,
                  TRUE);
      log("SYSERR:  Warning: bad affect (handler.c equip_char())\n");
    } else {
      affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier,
		  obj->obj_flags.bitvector,
		  obj->obj_flags.bitvector2,
                  TRUE);
    }
  }

  affect_total(ch);
  obj_worn_prog(obj, ch);
  
}



/* put in a caller and some improved assertion info */
struct obj_data *unequip_char(struct char_data * ch, int pos)
{
  int j;
  struct obj_data *obj;

  if (IS_THRIKREEN(ch)) {
    if (!(pos >= 0 && pos < NUM_THRI_WEARS))
      sprintf(buf, "SYSERR: bad pos: thrikreen %s trying to unequip bad position %d",
              GET_NAME(ch), pos);
    assert(pos >= 0 && pos < NUM_THRI_WEARS);
  } else {
    if (!(pos >= 0 && pos < NUM_WEARS))
      sprintf(buf, "SYSERR: bad pos: %s trying to unequip bad position %d",
              GET_NAME(ch), pos);
    assert(pos >= 0 && pos < NUM_WEARS);
  }
  assert(ch->equipment[pos]);

  obj = ch->equipment[pos];
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) += apply_ac(ch, pos);

  if (ch->in_room != NOWHERE) {
    if (pos == GET_WEAR_LIGHT(ch) && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
        if (ch->in_room < top_of_world)
	  world[ch->in_room].light--;
  } /* else */
/* HACKED to take out this errorlog, for ascii save files
   sometimes load NOWHERE */
/*
    log("SYSERR: ch->in_room = NOWHERE when equipping char.");
*/

  ch->equipment[pos] = NULL;

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    if (obj->affected[j].location < APPLY_NONE ||
        obj->affected[j].location >= APPLY_MAX) {
      affect_modify(ch, APPLY_NONE, obj->affected[j].modifier,
                  obj->obj_flags.bitvector,
                  obj->obj_flags.bitvector2,
                  FALSE);
      log("SYSERR:  Warning: bad affect (handler.c unequip_char())\n");
    } else {
      affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier,
		  obj->obj_flags.bitvector,
		  obj->obj_flags.bitvector2,
                  FALSE);
    }
  }

  affect_total(ch);

  if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_CONCEALED))
    REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_CONCEALED);

  return (obj);
}


int get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.'))) {
    *ppos++ = '\0';
    strcpy(number, *name);
    strcpy(*name, ppos);

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
	return 0;

    return (atoi(number));
  }
  return 1;
}



/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data * list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return i;

  return NULL;
}



/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(int nr)
{
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return i;

  return NULL;
}



/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int room)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
/* HACKED to check the value of get_number and take out the dot bug */
  number = get_number(&tmp);
  if (!(number))
    return NULL;
/* end of hack */

  for (i = world[room].people; i && (j <= number); i = i->next_in_room)
    if (isname(tmp, i->player.name))
      if (++j == number)
	return i;

  return NULL;
}



/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(int nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return i;

  return NULL;
}



/* put an object in a room */
void obj_to_room(struct obj_data * object, int room)
{
  if (!object || room < 0 || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to obj_to_room");
  else {
    object->next_content = world[room].contents;
    world[room].contents = object;
    object->in_room = room;
    object->carried_by = NULL;
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
  }
}


/* Take an object from a room */
void obj_from_room(struct obj_data * object)
{
  struct obj_data *temp;

  if (!object || object->in_room == NOWHERE) {
    log("SYSERR: NULL object or obj not in a room passed to obj_from_room");
    return;
  }
  REMOVE_FROM_LIST(object, world[object->in_room].contents, next_content);

  if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
    SET_BIT(ROOM_FLAGS(object->in_room), ROOM_HOUSE_CRASH);
  object->in_room = NOWHERE;
  object->next_content = NULL;
}


/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data * obj, struct obj_data * obj_to)
{
  struct obj_data *tmp_obj;

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;

  for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

  /* top level object.  Subtract weight from inventory if necessary. */
  GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by)
    IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
}


/* remove an object from an object */
void obj_from_obj(struct obj_data * obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    log("error (handler.c): trying to illegally extract obj from obj");
    return;
  }
  obj_from = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container */
  for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

  /* Subtract weight from char that carries the object */
  GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by)
    IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);

  obj->in_obj = NULL;
  obj->next_content = NULL;
}


/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data * list, struct char_data * ch)
{
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}


/* Extract an object from the world */
void extract_obj(struct obj_data * obj)
{
  struct obj_data *temp;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj->in_room != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);

  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
    extract_obj(obj->contains);

  REMOVE_FROM_LIST(obj, object_list, next);

  if (GET_OBJ_RNUM(obj) >= 0)
    (obj_index[GET_OBJ_RNUM(obj)].number)--;
  free_obj(obj);
}



void update_object(struct obj_data * obj, int use)
{
  if (GET_OBJ_TIMER(obj) == 1)
    GET_OBJ_TIMER(obj) = -1;               /* set it to rot */
  if (GET_OBJ_TIMER(obj) > 0)
    GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}



void update_char_objects(struct char_data * ch)
{
  int i;


  if (ch->equipment[GET_WEAR_LIGHT(ch)] != NULL)
    if (GET_OBJ_TYPE(ch->equipment[GET_WEAR_LIGHT(ch)]) == ITEM_LIGHT)
      if (GET_OBJ_VAL(ch->equipment[GET_WEAR_LIGHT(ch)], 2) > 0) {
	i = --GET_OBJ_VAL(ch->equipment[GET_WEAR_LIGHT(ch)], 2);
	if (i == 1) {
	  act("Your light begins to flicker and fade.", FALSE, ch, 0, 0,
              TO_CHAR);
	  act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0,
              TO_ROOM);
	} else if (i == 0) {
	  act("Your light sputters out and dies.", FALSE, ch, 0, 0, TO_CHAR);
	  act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
	  world[ch->in_room].light--;
	}
      }

  for (i = 0; i < GET_NUM_WEARS(ch); i++)
    if (ch->equipment[i])
      update_object(ch->equipment[i], 1);

  if (ch->carrying)
    update_object(ch->carrying, 1);
}



/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(struct char_data * ch)
{
  struct char_data *k, *temp;
  struct descriptor_data *t_desc;
  struct obj_data *obj;
  int i;

  extern struct char_data *combat_list;

  ACMD(do_return);
  void die_follower(struct char_data * ch);
  extern void extract_pet(struct char_data *pet);
  
  if (IS_PET(ch)) {
    extract_pet(ch);
    return;
  }

  if (!IS_NPC(ch) && !ch->desc) {
    for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
      if (t_desc->original == ch)
	do_return(t_desc->character, "", 0, 0);
  }
  if (ch->in_room == NOWHERE) {
    log("SYSERR: NOWHERE extracting char. (handler.c, extract_char)");
    exit(1);
  }
  if (ch->followers || ch->master)
    die_follower(ch);

  /* Forget snooping, if applicable */
  if (ch->desc) {
    if (ch->desc->snooping) {
      ch->desc->snooping->snoop_by = NULL;
      ch->desc->snooping = NULL;
    }
    if (ch->desc->snoop_by) {
      SEND_TO_Q("Your victim is no longer among us.\r\n",
		ch->desc->snoop_by);
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }
  }

  /* auction foo */
  if (auction_on) {
    if (auction.seller == ch)
      auction_seller_gone(ch);
    if (auction.bidder == ch)
      auction_bidder_gone(ch);
  }

  /* transfer objects to room, if any */
  while (ch->carrying) {
    obj = ch->carrying;
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);
  }

  /* transfer equipment to room, if any */
  for (i = 0; i < GET_NUM_WEARS(ch); i++)
    if (ch->equipment[i])
      obj_to_room(unequip_char(ch, i), ch->in_room);

  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }

  /* MOBProg foo */
  stop_script(ch);
  for (k = script_list; k; k = temp) {
    temp = k->mpnextscript;
    if (k->mpscriptactor == ch)
      stop_script(k);
  }

/* PETS */
/* This is probably a DT or death...we'll leave the pet connected to the desc,
   but not the character */
  if (HAS_PET(ch)) {
    GET_OWNER(GET_PET(ch)) = NULL;
    GET_PET(ch) = NULL;
  }
/* END of PETS */

  char_from_room(ch);

  /* pull the char from the list */
  REMOVE_FROM_LIST(ch, character_list, next);

  if (ch->desc && ch->desc->original)
    do_return(ch, "", 0, 0);

  if (!IS_NPC(ch)) {
    save_char(ch, NOWHERE);
    Crash_delete_crashfile(ch);
  } else {
    if (GET_MOB_RNUM(ch) > -1)		/* if mobile */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch);		/* Only NPC's can have memory */
    free_char(ch);
  }

  if (ch->desc) {
    STATE(ch->desc) = CON_MENU;
    SEND_TO_Q(MENU, ch->desc);
  }
}



/* ***********************************************************************
   Here follows high-level versions of some earlier routines, ie functions
   which incorporate the actual player-data.
   *********************************************************************** */


struct char_data *get_player_vis(struct char_data * ch, char *name)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (!IS_NPC(i) && !str_cmp(i->player.name, name) && CAN_SEE(ch, i))
      return i;

  return NULL;
}



struct char_data *get_char_room_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* JE 7/18/94 :-) :-) */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return ch;

  /* 0.<name> means PC with name */
  strcpy(tmp, name);
/* HACKED to take out the dot bug */
/*
  if (!(number = get_number(&tmp)))
    return get_player_vis(ch, tmp);
*/
  number = get_number(&tmp);
/* end of hack */

  for (i = world[ch->in_room].people; i && j <= number; i = i->next_in_room)
    if (isname(tmp, i->player.name))
      if (CAN_SEE(ch, i))
	if (++j == number)
	  return i;

  return NULL;
}



/*
 * This finds the first likely person in the room who'll make a good
 * target.  Takes no argument because this is called when none was supplied.
 * Helps players just hit the first likely mob in the room.
 */
struct char_data *get_vict_room_vis(struct char_data * ch)
{
  struct char_data *vict;


  for (vict = world[ch->in_room].people; vict ; vict = vict->next_in_room)
    if (CAN_SEE(ch, vict)) {
      if (vict == ch)			/* skip yourself */
        continue;
      if (!IS_NPC(vict))		/* skip players */
        continue;
      if (vict->desc)			/* skip familiars and jarred people */
        continue;
      if (IS_AFFECTED(vict, AFF_CHARM))	/* skip charmed pets */
        continue;
      return vict;
    }

  return NULL;
}



bool is_nearby(struct char_data * ch, struct char_data * vict)
{
  int door;
  struct char_data *i;

  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (!EXIT(ch,door)
        || IS_SET(EXIT(ch,door)->exit_info, EX_CLOSED)
        || (EXIT(ch,door)->to_room == NOWHERE))
      continue;
    for (i = world[EXIT(ch, door)->to_room].people; i; i = i->next_in_room)
/* the concept of can see had to be expanded so that mobs cant be plinked */
      if ((i == vict && CAN_SEE(ch, vict)) || (i == vict && IS_NPC(ch)))
        return TRUE;
  }

  return FALSE;
}



struct char_data *get_char_nearby_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;
  int door;

  /* 0.<name> means PC with name */
  strcpy(tmp, name);
/* HACKED to take out the dot bug */
/*
  if (!(number = get_number(&tmp)))
    return get_player_vis(ch, tmp);
*/
  number = get_number(&tmp);
/* end of hack */

  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (!EXIT(ch,door) 
        || IS_SET(EXIT(ch,door)->exit_info, EX_CLOSED)
        || (EXIT(ch,door)->to_room == NOWHERE))
      continue;
    for (i = world[EXIT(ch, door)->to_room].people; 
         i && j <= number; 
         i = i->next_in_room)
      if (isname(tmp, i->player.name))
        if (CAN_SEE(ch, i))
          if (++j == number) 
            return i;
  }

  return NULL;
}


struct char_data *get_exchar_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  if ((i = get_char_room_vis(ch, name)) != NULL)
    return i;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = character_list; i && (j <= number); i = i->next)
    if (isexname(tmp, i->player.name) && CAN_SEE(ch, i))
      if (++j == number)
	return i;

  return NULL;
}

bool is_name ( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
    	return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return FALSE;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
	str = one_argument(str,part);

	if (part[0] == '\0' )
	    return TRUE;

	/* check to see if this is part of namelist */
	list = namelist;
	for ( ; ; )  /* start parsing namelist */
	{
	    list = one_argument(list,name);
	    if (name[0] == '\0')  /* this name was not found */
		return FALSE;

	    if (!str_prefix(string,name))
		return TRUE; /* full pattern match */

	    if (!str_prefix(part,name))
		break;
	}
    }
}

/*
 * True if char can see victim.
 */
bool can_see( struct char_data *ch, struct char_data *victim )
{
/* RT changed so that WIZ_INVIS has levels */
    if ( ch == victim )
	return TRUE;
    
    if ( IS_AFFECTED(ch, AFF_BLIND) )
	return FALSE;

    if ( IS_AFFECTED(victim, AFF_INVISIBLE)
    &&   !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
	return FALSE;

    return TRUE;
}

int number_argument( char *argument, char *arg )  
{  
    char *pdot;
    int number;
    
    for ( pdot = argument; *pdot != '\0'; pdot++ )
    {
        if ( *pdot == '.' )
        {
            *pdot = '\0';  
            number = atoi( argument );
            *pdot = '.'; 
            strcpy( arg, pdot+1 );
            return number;
        }   
    }       
        
    strcpy( arg, argument );
    return 1;
}


/*          
 * Find a char in the world.
 */     
struct char_data *get_char_world(struct char_data *ch, char *argument)
{   
    char arg[MAX_INPUT_LENGTH];
    struct char_data *wch;
    int number;
    int count;
    
    if ((wch = get_char_room_vis(ch, argument)) != NULL)
        return wch;
    
    number = number_argument( argument, arg );
    count  = 0;
    for (wch = character_list; wch; wch = wch->next) {
      if (IN_ROOM(wch) == NOWHERE)
	continue;
      if (!can_see(ch, wch))
	continue;
      if (!is_name(arg, GET_NAME(wch)))
	continue;
      if (++count == number)
	return (wch);
    }   
    return (NULL);
}


struct char_data *get_char_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* check the room first */
  if ((i = get_char_room_vis(ch, name)) != NULL)
    return i;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = character_list; i && (j <= number); i = i->next)
    if (isname(tmp, i->player.name) && CAN_SEE(ch, i))
      if (++j == number)
	return i;

  return NULL;
}



struct obj_data *get_obj_in_list_vis(struct char_data * ch, char *name,
				              struct obj_data * list)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = list; i && (j <= number); i = i->next_content)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (++j == number)
	  return i;

  return NULL;
}



/* search the nearby rooms for an object, and return a pointer */
struct obj_data *get_obj_nearby_vis(struct char_data * ch, char *name)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;
  int door;

  /* 0.<name> means PC with name */
  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return get_obj_vis(ch, tmp);

  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (!EXIT(ch,door) 
        || IS_SET(EXIT(ch,door)->exit_info, EX_CLOSED)
        || (EXIT(ch,door)->to_room == NOWHERE))
      continue;
    for (i = world[EXIT(ch, door)->to_room].contents; 
         i && (j <= number); 
         i = i->next_content)
      if (isname(tmp, i->name))
        if (CAN_SEE_OBJ(ch, i))
          if (++j == number)
            return i;
  }

  return NULL;
}



/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data * ch, char *name)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, ch->carrying)))
    return i;

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
    return i;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && (j <= number); i = i->next)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (++j == number)
	  return i;

  return NULL;
}



struct obj_data *get_object_in_equip_vis(struct char_data * ch,
		           char *arg, struct obj_data * equipment[], int *j)
{
  for ((*j) = 0; (*j) < GET_NUM_WEARS(ch); (*j)++)
    if (equipment[(*j)])
      if (CAN_SEE_OBJ(ch, equipment[(*j)]))
	if (isname(arg, equipment[(*j)]->name))
	  return (equipment[(*j)]);

  return NULL;
}


char *money_desc(int amount)
{
  static char buf[128];

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money.");
    return NULL;
  }
  if (amount == 1)
    strcpy(buf, "a gold coin");
  else if (amount <= 10)
    strcpy(buf, "a tiny pile of gold coins");
  else if (amount <= 20)
    strcpy(buf, "a handful of gold coins");
  else if (amount <= 75)
    strcpy(buf, "a little pile of gold coins");
  else if (amount <= 200)
    strcpy(buf, "a small pile of gold coins");
  else if (amount <= 1000)
    strcpy(buf, "a pile of gold coins");
  else if (amount <= 5000)
    strcpy(buf, "a big pile of gold coins");
  else if (amount <= 10000)
    strcpy(buf, "a large heap of gold coins");
  else if (amount <= 20000)
    strcpy(buf, "a huge mound of gold coins");
  else if (amount <= 75000)
    strcpy(buf, "an enormous mound of gold coins");
  else if (amount <= 150000)
    strcpy(buf, "a small mountain of gold coins");
  else if (amount <= 250000)
    strcpy(buf, "a mountain of gold coins");
  else if (amount <= 500000)
    strcpy(buf, "a huge mountain of gold coins");
  else if (amount <= 1000000)
    strcpy(buf, "an enormous mountain of gold coins");
  else
    strcpy(buf, "an absolutely colossal mountain of gold coins");

  return buf;
}


struct obj_data *create_money(int amount)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200];

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money.");
    return NULL;
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = strdup("coin gold");
    obj->short_description = strdup("a gold coin");
    obj->description = strdup("One miserable gold coin is lying here.");
    new_descr->keyword = strdup("coin gold");
    new_descr->description = strdup("It's just one miserable little gold coin.");
  } else {
    obj->name = strdup("coins gold");
    obj->short_description = strdup(money_desc(amount));
    sprintf(buf, "%s is lying here.", money_desc(amount));
    obj->description = strdup(CAP(buf));

    new_descr->keyword = strdup("coins gold");
    if (amount < 10) {
      sprintf(buf, "There are %d coins.", amount);
      new_descr->description = strdup(buf);
    } else if (amount < 100) {
      sprintf(buf, "There are about %d coins.", 10 * (amount / 10));
      new_descr->description = strdup(buf);
    } else if (amount < 1000) {
      sprintf(buf, "It looks to be about %d coins.", 100 * (amount / 100));
      new_descr->description = strdup(buf);
    } else if (amount < 100000) {
      sprintf(buf, "You guess there are, maybe, %d coins.",
	      1000 * ((amount / 1000) + number(0, (amount / 1000))));
      new_descr->description = strdup(buf);
    } else
      new_descr->description = strdup("There are a LOT of coins.");
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
  GET_OBJ_VAL(obj, 0) = amount;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return obj;
}


/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

int generic_find(char *arg, int bitvector, struct char_data * ch,
		     struct char_data ** tar_ch, struct obj_data ** tar_obj)
{
  int i, found;
  char name[256];

  one_argument(arg, name);

  if (!*name)
    return (0);

  *tar_ch = NULL;
  *tar_obj = NULL;

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	/* Find person in room */
    if ((*tar_ch = get_char_room_vis(ch, name))) {
      return (FIND_CHAR_ROOM);
    }
  }
  if (IS_SET(bitvector, FIND_CHAR_NEARBY)) {
    if ((*tar_ch = get_char_nearby_vis(ch, name))) {
      return (FIND_CHAR_NEARBY);
    }
  }
  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_vis(ch, name))) {
      return (FIND_CHAR_WORLD);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < GET_NUM_WEARS(ch) && !found; i++)
      if (ch->equipment[i] && str_cmp(name, ch->equipment[i]->name) == 0) {
	*tar_obj = ch->equipment[i];
	found = TRUE;
      }
    if (found) {
      return (FIND_OBJ_EQUIP);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
      return (FIND_OBJ_INV);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents))) {
      return (FIND_OBJ_ROOM);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name))) {
      return (FIND_OBJ_WORLD);
    }
  }
  return (0);
}


/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
  if (!strcmp(arg, "all"))
    return FIND_ALL;
  else if (!strncmp(arg, "all.", 4)) {
    strcpy(arg, arg + 4);
    return FIND_ALLDOT;
  } else
    return FIND_INDIV;
}

/* Careful: *pretitle must be at least MAX_TITLE_LENGTH! */
void get_char_pretitle(struct char_data *ch, char *pretitle) {
  if (!HAS_PRETITLE(ch)) {
    strcpy(pretitle, "");
    return;
  }
  strcpy(pretitle, GET_REAL_TITLE(ch));
  strcpy(strchr(pretitle, PRETITLE_SEP_CHAR), " "); /* Yeah, so it's cheesy. */
  return;
}

struct char_data *get_first_char(int room) {
  struct char_data *tch;
  
  if (room < 0 || room >= top_of_world) {
    log("SYSERR: Out-of-range room number (handler.c, get_first_char).");
    return NULL;
  }
  for (tch = world[room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch)) break;
  }
  return tch;
}
