
/*************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
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

/* extern variables */
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern char *pc_class_types[];
extern char *pc_race_types[];
extern struct title_type titles[NUM_CLASSES][LVL_IMPL + 1];
extern struct command_info cmd_info[];

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *dirs[];
extern char *dir_abbrevs[];
extern int dir_order[];
extern int wear_display_order[];
extern int thri_wear_display_order[];
extern char *where[];
extern char *thri_where[];
extern char *color_liquid[];
extern char *fullness[];
extern char *connected_types[];
extern char *race_abbrevs[];
extern char *clan_names[];
extern char *clan_levels[];
extern char *custom_clan_levels[NUM_CLANS][4];
extern char *class_abbrevs[];
/* HACKED to add immort abbreviations */
extern char *immort_abbrevs[];
/* end of hack */
extern char *room_bits[];
extern char *sector_types[];
extern char *spells[];
extern char *color_codes[];
extern struct spell_info_type spell_info[];
extern struct player_index_element *player_table;
extern int top_of_p_table;

struct time_info_data playing_time;
struct time_info_data real_time_passed(time_t t2, time_t t1);
long find_race_bitvector(char *arg);
long find_class_bitvector(char *arg);
int percent_number_of_attacks(struct char_data * ch);
int parse_clan(char *arg);
ACMD(do_scan);
void perform_map_square(struct char_data *ch, room_num start_room, int room_count, int draw_self_at_count, int row, int col, bool graphical);
ACMD(do_map);



/* HACKED to coalesce items */
struct objlist {
  int num, cnt, mark;
  struct objlist *n;
};



void mark_elem(struct objlist *head, int num)
{
  struct objlist *obj;

  for (obj = head; obj; obj = obj->n)
    if (obj->num == num) {
      obj->mark = 1;
      return;
    }
}




int is_marked_elem(struct objlist *head, int num)
{
  struct objlist *obj;

  for (obj = head; obj; obj = obj->n)
    if ((obj->num == num) && (obj->mark))
      return 1;

  return 0;
}



int count_elem(struct objlist *head, int num) 
{
  struct objlist *obj;

  for (obj = head; obj; obj = obj->n)
    if (obj->num == num)
      return (obj->cnt);

  return 0;
}



void add_elem(struct objlist *head, int num) 
{
  struct objlist *obj;
  for (obj = head; obj; obj = obj->n) {
    if ((obj->num) == num) {
      obj->cnt++;
      return;
    }
    if (!obj->n) {
      obj->n = malloc(sizeof(struct objlist));
      obj->n->num = num;
      obj->n->cnt = 1;
      obj->n->mark = 0;
      obj->n->n = NULL;
      return;
    }
  }
}



void destroy_list(struct objlist *head) 
{
  if (head != NULL) {
    destroy_list(head -> n);
    free(head);
  }
}



/*
 * This is a modified show_obj_to_char.  Note the int cnt in the prototype.
 * You will have to modify anything that calls this.  Chances are if it isn't
 * list_obj_to_char calling it, you can just use 1 as the value.
 */
/******* MASSIVE HACK by Darryl. Adds 1 to cnt if the rnum is -1, to see
 ******* if this fixes the gold bug without breaking anything else.
 *******/
void show_obj_to_char(struct obj_data * object, struct char_data * ch,
			int mode, int cnt)
{
  bool found;
  
  if (object->item_number == -1) ++cnt;

  *buf = '\0';
  if ((mode == 0) && object->description)
    strcpy(buf, object->description);
  else if ((mode == 6) && object->description)
    sprintf(buf, "~%s", object->description);
  else if (object->short_description && ((mode == 1) ||
				 (mode == 2) || (mode == 3) || (mode == 4)))
    strcpy(buf, object->short_description);
  else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
      if (object->action_description) {
	strcpy(buf, "There is something written upon it:\r\n\r\n");
	strcat(buf, object->action_description);
	page_string(ch->desc, buf, 1);
      } else
	act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else			/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");
  }
  if (mode != 3) {
    found = FALSE;
    if (IS_OBJ_STAT(object, ITEM_CONCEALED)) {
      strcat(buf, " (concealed)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
      strcat(buf, " (invisible)");
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_BLESS) && IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      strcat(buf, " ^g(blessed)^n");
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_MAGIC) && IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
      strcat(buf, " ^m(magic)^n");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW)) {
      strcat(buf, " ^y(glows)^n");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_HUM)) {
      strcat(buf, " ^c(hums)^n");
      found = TRUE;
    }
/* end of hack */
  }

/* HACKED so that if the player and the object arent in the same room
  then a couple of spaces are put in front to indent */
  if (ch->in_room != object->in_room) {
    strcpy(buf2, buf);
    sprintf(buf, "  %s", buf2);
  }
/* end of hack */

/* HACKED to show the count of the objects */
  strcpy(buf2, buf);
  if (cnt > 1)
    sprintf(buf, "(%2d) %s", cnt, buf2);
  else
    sprintf(buf, "     %s", buf2);
/* end of hack */

  strcat(buf, "\r\n");

  page_string(ch->desc, buf, 1);
}



/* HACKED to show totals of objects */
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
		           bool show)
{
  struct obj_data *i;
  bool found;
  struct objlist *head;
  extern struct obj_data *obj_proto;

  if (!(head = malloc(sizeof(struct objlist))))
    log("NAJERR: list_obj_to_char: cannot create list");
  head->n = NULL;
  head->num = -1;
  head->cnt = -1;
  head->mark = 0;
  for (i = list; i; i = i->next_content)
    if (CAN_SEE_OBJ(ch, i))
      add_elem(head, i->item_number);
  found = FALSE;
  for (i = list; i; i = i->next_content) {
    if (CAN_SEE_OBJ(ch, i)) {	/* list all corpses individually - naj */
      if (((GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3))) &&
           (i->item_number == NOTHING)) {
        show_obj_to_char(i, ch, mode, count_elem(head, 1));
        found = TRUE;
      } else if (GET_OBJ_RNUM(i) >= 0) /* list all renamed objects - Culvan */
        if (i->name != obj_proto[GET_OBJ_RNUM(i)].name) {
          show_obj_to_char(i, ch, mode, count_elem(head, 1));
          found = TRUE;
      } else if (!is_marked_elem(head, i->item_number)) {
        show_obj_to_char(i, ch, mode, count_elem(head, i->item_number));
        mark_elem(head, i->item_number);
        found = TRUE;
      }
    }
  }
  if (!found && show)
    send_to_char("     Nothing.\r\n", ch);

  destroy_list(head);
}



/* HACKED for scan to not see empty rooms */
int count_list_obj_to_char(struct obj_data * list, struct char_data * ch)
{
  struct obj_data *i;
  int count;

  count = 0;
  for (i = list; i; i = i->next_content)
    if (CAN_SEE_OBJ(ch, i))
      count++;

  return count;
}



const char *diag_char_to_char_short(struct char_data *i) {
  if (i == NULL) {
    log("diag_char_to_char_short(): invalid 'ch' char_data.");
  } else {
    /* Calculate the percentage. */
    const int percent =
      (GET_MAX_HIT(i) > 0 ?
      (GET_HIT(i) * 100) / GET_MAX_HIT(i) : 0);

    if (GET_MAX_HIT(i) <= 0) {
      snprintf(buf, sizeof(buf), "BUG: GET_MAX_HIT() for %s <= 0\n\r", GET_NAME(i));
      mudlog(buf, BRF, LVL_IMPL, TRUE);
    }

    if (percent >= 100) {
      return ("excellent");
    } else if (percent >= 90) {
      return ("scratched");
    } else if (percent >= 75) {
      return ("bruised");
    } else if (percent >= 50) {
      return ("wounded");
    } else if (percent >= 25) {
      return ("nasty");
    } else if (percent >= 15) {
      return ("hurt");
    } else if (percent >= 0) {
      return ("awful");
    } else {
      return ("dying");
    }
  }
  return ("unknown");
}



void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " is in excellent condition.\r\n");
  else if (percent >= 90)
    strcat(buf, " has a few scratches.\r\n");
  else if (percent >= 75)
    strcat(buf, " has some small wounds and bruises.\r\n");
  else if (percent >= 50)
    strcat(buf, " has quite a few wounds.\r\n");
  else if (percent >= 30)
    strcat(buf, " has some big nasty wounds and scratches.\r\n");
  else if (percent >= 15)
    strcat(buf, " looks pretty hurt.\r\n");
  else if (percent >= 0)
    strcat(buf, " is in awful condition.\r\n");
  else
    strcat(buf, " is bleeding awfully from big wounds.\r\n");

  send_to_char(buf, ch);
}


void affects_char_to_char(struct char_data * i, struct char_data * ch)
{
  if (IS_AFFECTED(i, AFF_SANCTUARY)) {
    strcpy(buf, PERS(i, ch));
    CAP(buf);
    strcat(buf, " is surrounded by a soft white aura.\r\n");
    send_to_char(buf, ch);
  }
}



void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, k, found, num_wear;
  struct obj_data *tmp_obj;

  num_wear = IS_THRIKREEN(i)? NUM_THRI_WEARS : NUM_WEARS;

  if (i->player.description)
    send_to_char(i->player.description, ch);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);

  affects_char_to_char(i, ch);
   
  found = FALSE;
  for (j = 0; !found && j < num_wear; j++)
    if (i->equipment[j] && CAN_SEE_OBJ(ch, i->equipment[j]))
      found = TRUE;

/* HACKED to make a special exception for wielded weapons, to show them
  as simply <wielded> if you have but one to make things look 'normal'. */
  if (found) {
    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < num_wear; j++) {
      k = wear_display_order[j];
      if (IS_THRIKREEN(i)) k = thri_wear_display_order[j];
      if (i->equipment[k] && CAN_SEE_OBJ(ch, i->equipment[k])) {
        if (!IS_THRIKREEN(i)) {
          if ((k == WEAR_WIELD && !i->equipment[WEAR_WIELD_2]) ||
              (k == WEAR_WIELD_2 && !i->equipment[WEAR_WIELD]))
            send_to_char("<wielded>            ", ch);
          else
            send_to_char(where[k], ch);
        } else {
          send_to_char(thri_where[k], ch);
        }
	show_obj_to_char(i->equipment[k], ch, 1, 1);
      }
    }
  }
/* end of hack */
  if (ch != i && (GET_CLASS(ch) == CLASS_THIEF || GET_LEVEL(ch) >= LVL_IMMORT)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, 1, 1);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char("You can't see anything.\r\n", ch);
  }
}



void list_one_char(struct char_data * i, struct char_data * ch)
{
  int percent;
  char *positions[] = {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here.", 
    " is searching the area."
  };


  *buf = '\0';

  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {

/* AUTOQUEST */
    if (IS_NPC(i) && (ch)->player_specials->saved.questmob > 0 && GET_MOB_VNUM(i) == (ch)->player_specials->saved.questmob)
        strcat( buf, "[TARGET] ");

    if (MOB_FLAGGED(i, MOB_NOTTHERE))
      strcat(buf, "(not here) ");
    if (IS_AFFECTED(i, AFF_INVISIBLE))
      strcat(buf, "*");
    if (IS_AFFECTED(i, AFF_HIDE))
      strcat(buf, "~");

    /*
     * put in a couple of spaces at the beginning
     * if the characters arent in the same room 
     */
    if (ch->in_room != i->in_room) {
      strcpy(buf2, buf);
      sprintf(buf, "  %s", buf2);
    }

    strcat(buf, i->player.long_descr);
    buf[strlen(buf) - 2] = '\0';          /* GET RID OF NEW LINE */

    if (IS_NPC(i)) {
      if (GET_MAX_HIT(i) > 0)   
        percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
      else
        percent = -1;               /* How could MAX_HIT be < 1?? */
   
      if (percent > 100)
        strcat(buf, " (exceptional)");
      else if (percent == 100)
        strcat(buf, "");
      else if (percent >= 95)
        strcat(buf, " (excellent)");
      else if (percent >= 90)
        strcat(buf, " (scratched)");
      else if (percent >= 75)
        strcat(buf, " (bruised)");
      else if (percent >= 50)
        strcat(buf, " (wounded)");
      else if (percent >= 30)
        strcat(buf, " (nasty)");
      else if (percent >= 15)
        strcat(buf, " (hurt)");
      else if (percent >= 0)
        strcat(buf, " (awful)"); 
      else
        strcat(buf, " (dying)");
    } 

    if (IS_AFFECTED(i, AFF_SANCTUARY))
      strcat(buf, " ^w(white aura)^n");
    if ((IS_AFFECTED(i, AFF_STONESKIN)) && (GET_LAYERS(i) > 0))
      strcat(buf, " (stony texture)");
    if (IS_AFFECTED2(i, AFF2_FIRESHIELD))
      strcat(buf, " ^r(flaming glow)^n");
    if (IS_AFFECTED2(i, AFF2_MANASHELL))
      strcat(buf, " ^Y(flickering aura)^n");
    if (IS_AFFECTED2(i, AFF2_ICE_SHIELD))
      strcat(buf, " ^c(ice shield)^n");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }
  
  if (IS_NPC(i)) {
    strcpy(buf, i->player.short_descr);
    CAP(buf);
  } else {
/*
    get_char_pretitle(i, buf2);
    sprintf(buf, "%s%s %s", buf2, i->player.name, GET_TITLE(i));
*/
    sprintf(buf, "%s", i->player.name);
  }

  if (IS_AFFECTED(i, AFF_INVISIBLE))
    strcat(buf, " (invisible)");
  if (IS_AFFECTED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    strcat(buf, " (linkless)");
  if (PLR_FLAGGED(i, PLR_WRITING)) {
    if (STATE(i->desc) == CON_PLAYING)
      strcat(buf, " (writing)");
    else /* they are in OLC */
      strcat(buf, " (editing)");
  }

  if (GET_POS(i) != POS_FIGHTING)
    strcat(buf, positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      strcat(buf, " is here, fighting ");
      if (FIGHTING(i) == ch)
	strcat(buf, "YOU!");
      else {
	if (i->in_room == FIGHTING(i)->in_room)
	  strcat(buf, PERS(FIGHTING(i), ch));
	else
	  strcat(buf, "someone who has already left");
	strcat(buf, "!");
      }
    } else			/* NIL fighting pointer */
      strcat(buf, " is here struggling with thin air.");
  }

  /* no aura code please */
  /*
  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      strcat(buf, " (Red Aura)");
    else if (IS_GOOD(i))
      strcat(buf, " (Blue Aura)");
  }
  */

  if (IS_AFFECTED(i, AFF_SANCTUARY))
    strcat(buf, " ^w(white aura)^n");
  if ((IS_AFFECTED(i, AFF_STONESKIN)) && (GET_LAYERS(i) > 0))
    strcat(buf, " (stony texture)");
  if (IS_AFFECTED2(i, AFF2_FIRESHIELD))
    strcat(buf, " ^r(flaming glow)^n");
  if (IS_AFFECTED2(i, AFF2_MANASHELL))
    strcat(buf, " ^Y(flickering aura)^n");
  if (IS_AFFECTED2(i, AFF2_ICE_SHIELD))
    strcat(buf, " ^c(ice shield)^n");
  
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}



void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i) {
      if (CAN_SEE(ch, i))
	list_one_char(i, ch);
      else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !HAS_LIGHT(ch) &&
               !MOB_FLAGGED(i, MOB_NOTTHERE) &&
	       IS_AFFECTED(i, AFF_INFRAVISION))
	send_to_char("You see a pair of glowing red eyes looking your way.\r\n", ch);
    }
}



/* HACKED for scan to not list empty rooms */
int count_list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;
  int count;

  count = 0;
  for (i = list; i; i = i->next_in_room)
    if (ch != i) {
      if (CAN_SEE(ch, i))
        count++;
      else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !HAS_LIGHT(ch) &&
               IS_AFFECTED(i, AFF_INFRAVISION))
        count++;
    }

  return count;
}


/* HACKED to be renamed from do_auto_exits to do_auto_dirs */
void do_auto_dirs(struct char_data * ch)
{
  int door;

  *buf = '\0';

  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (EXIT(ch, dir_order[door])
        && EXIT(ch, dir_order[door])->to_room != NOWHERE
        && !IS_SET(EXIT(ch, dir_order[door])->exit_info, EX_CLOSED)) {
      sprintf(buf, "%s%s ", buf, dir_abbrevs[dir_order[door]]);
    }
  }

  sprintf(buf2, "%s{ %s}%s\r\n", CCEXITS(ch),
	  *buf ? buf : "- ", CCNRM(ch));

  send_to_char(buf2, ch);
}


ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (EXIT(ch, dir_order[door])
        && EXIT(ch, dir_order[door])->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, dir_order[door])->exit_info, EX_CLOSED)) {
/* HACKED to not show room numbers if the god doesnt like them */
      if ((GET_LEVEL(ch) >= LVL_IMMORT) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
	sprintf(buf2, "%-9s - [%5d] %s", 
                dirs[dir_order[door]],
		world[EXIT(ch, dir_order[door])->to_room].number,
		world[EXIT(ch, dir_order[door])->to_room].name);
        if (IS_SET(EXIT(ch, dir_order[door])->exit_info, EX_ISDOOR) &&
            EXIT(ch, dir_order[door])->keyword) {
          strcat(buf2, " (");
          strcat(buf2, EXIT(ch, dir_order[door])->keyword);
          strcat(buf2, " open)");
        }
        strcat(buf2, "\r\n");
        strcat(buf, CAP(buf2));
      } else {
	sprintf(buf2, "%-9s - ", dirs[dir_order[door]]);
	if (IS_DARK(EXIT(ch, dir_order[door])->to_room)
            && !CAN_SEE_IN_DARK(ch) && !HAS_LIGHT(ch)
            && !IS_SET(world[EXIT(ch, dir_order[door])->to_room].room_flags, 
                    ROOM_DEATH))
	  strcat(buf2, "Too dark to tell");
	else
	  strcat(buf2, world[EXIT(ch, dir_order[door])->to_room].name);
        if (IS_SET(EXIT(ch, dir_order[door])->exit_info, EX_ISDOOR) &&
            EXIT(ch, dir_order[door])->keyword) {
          strcat(buf2, " (");
          strcat(buf2, EXIT(ch, dir_order[door])->keyword);
          strcat(buf2, " open)");
        }
        strcat(buf2, "\r\n");
        strcat(buf, CAP(buf2));
      }
    }
    if (EXIT(ch, dir_order[door])
        && EXIT(ch, dir_order[door])->to_room != NOWHERE
        && IS_SET(EXIT(ch, dir_order[door])->exit_info, EX_CLOSED)
        && EXIT(ch, dir_order[door])->keyword) {
      if ((GET_LEVEL(ch) >= LVL_IMMORT) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
        sprintf(buf2, "%-9s - [%5d] %s (%s closed)\r\n",
                dirs[dir_order[door]],
                world[EXIT(ch, dir_order[door])->to_room].number,
                world[EXIT(ch, dir_order[door])->to_room].name,
                EXIT(ch, dir_order[door])->keyword);
        strcat(buf, CAP(buf2));
      } else {
        if (!IS_DARK(EXIT(ch, dir_order[door])->to_room)) {
          sprintf(buf2, "%-9s - (%s closed)\r\n", dirs[dir_order[door]],
                  EXIT(ch, dir_order[door])->keyword);
          strcat(buf, CAP(buf2));
        }
      }
    }
  }

  send_to_char(CCEXITS(ch), ch);
  send_to_char("Obvious exits:\r\n", ch);

  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
  send_to_char(CCNRM(ch), ch);
}



void look_at_room(struct char_data * ch, int ignore_brief)
{
  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !HAS_LIGHT(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
  }

  send_to_char(CCROOMNAME(ch), ch);
  if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintf(buf2, "[%5d] %s [ ", world[ch->in_room].number,
        world[ch->in_room].name);
    send_to_char(buf2, ch);
    sprinttype(world[ch->in_room].sector_type, sector_types, buf2);
    send_to_char(buf2, ch);
    send_to_char(": ", ch);
    sprintbit((long) ROOM_FLAGS(ch->in_room), room_bits, buf2);
    send_to_char(buf2, ch);
    send_to_char("]", ch);
  } else {
    send_to_char(world[ch->in_room].name, ch);
  }
  send_to_char(CCNRM(ch), ch);
  send_to_char("\r\n", ch);

  /* description */
  if (!PRF_FLAGGED(ch, PRF_BRIEF) || ignore_brief ||
      ROOM_FLAGGED(ch->in_room, ROOM_DEATH)) {
    send_to_char(CCROOMDESC(ch), ch);
    send_to_char(world[ch->in_room].description, ch);
    send_to_char(CCNRM(ch), ch);
  }

  /* now list characters & objects */
  send_to_char(CCOBJECTS(ch), ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
    list_obj_to_char(world[ch->in_room].hidden, ch, 6, FALSE);
  }
  send_to_char(CCPLAYERS(ch), ch);
  list_char_to_char(world[ch->in_room].people, ch);
  send_to_char(CCNRM(ch), ch);

  /* autoexits */
  if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_exits(ch, "", 0, 0);

  /* autodirs */
  if (PRF_FLAGGED(ch, PRF_AUTODIRS))
    do_auto_dirs(ch);

  /* autoscan */
/*
  if (PRF2_FLAGGED(ch, PRF2_AUTOSCAN))
    do_scan(ch, "", 0, 0);
*/

  /* automap */
  if (PRF2_FLAGGED(ch, PRF2_AUTOMAP))
    do_map(ch, "", 0, 0);
}



void look_in_direction(struct char_data * ch, int dir)
{

  if (!(EXIT(ch, dir))) {
    send_to_char("No exit that way...\r\n",ch);
    return;
  }

  if (EXIT(ch, dir)->to_room == NOWHERE) {
    send_to_char("Nothing special there...\r\n", ch);
    return;
  }

  if (EXIT(ch, dir)) { 

    if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) 
        && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) 
               && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    }

    if (!(IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) 
        && EXIT(ch, dir)->keyword)) {
      if (IS_DARK(EXIT(ch, dir)->to_room) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char("It is pitch black that way...\r\n", ch);
        return;
      } else if (IS_AFFECTED(ch, AFF_BLIND)) {
        send_to_char("You see nothing but infinite darkness that way...\r\n", ch);
        return;
      }
      send_to_char(CCROOMNAME(ch), ch);
      if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
        sprintbit((long) ROOM_FLAGS(EXIT(ch, dir)->to_room), room_bits, buf);
        sprintf(buf2, "[%5d] %s [ %s]", world[EXIT(ch, dir)->to_room].number,
                world[EXIT(ch,dir)->to_room].name, buf);
        send_to_char(buf2, ch);
      } else
        send_to_char(world[EXIT(ch, dir)->to_room].name, ch);

      send_to_char(CCNRM(ch), ch);
      send_to_char("\r\n", ch);

      send_to_char(CCOBJECTS(ch), ch);
      list_obj_to_char(world[EXIT(ch, dir)->to_room].contents, ch, 0, FALSE);
      send_to_char(CCPLAYERS(ch), ch);
      list_char_to_char(world[EXIT(ch, dir)->to_room].people, ch);
      send_to_char(CCNRM(ch), ch);
    }
  }
}



void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Look in what?\r\n", ch);
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				 FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    send_to_char("There's nothing inside that!\r\n", ch);
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
	send_to_char("It is closed.\r\n", ch);
      else {
	send_to_char(fname(obj->name), ch);
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(" (carried): \r\n", ch);
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(" (here): \r\n", ch);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(" (used): \r\n", ch);
	  break;
	}

	list_obj_to_char(obj->contains, ch, 2, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
	send_to_char("It is empty.\r\n", ch);
      else {
	amt = ((GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0));
	sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt],
		color_liquid[GET_OBJ_VAL(obj, 2)]);
	send_to_char(buf, ch);
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return NULL;
}



/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = 0, j, num_wear;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  char *desc;
  
  num_wear = NUM_WEARS;
  if (IS_THRIKREEN(ch)) num_wear = NUM_THRI_WEARS;
  
  if (!*arg) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }
  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) {
    page_string(ch->desc, desc, 0);
    return;
  }
  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < num_wear && !found; j++)
    if (ch->equipment[j] && CAN_SEE_OBJ(ch, ch->equipment[j]))
      if ((desc = find_exdesc(arg, ch->equipment[j]->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  if (bits) {			/* If an object was found back in
				 * generic_find */
    if (!found)
      show_obj_to_char(found_obj, ch, 5, 1);	/* Show no-description */
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);
}



ACMD(do_look)
{
  static char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !HAS_LIGHT(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);	/* glowing red eyes */
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Read what?\r\n", ch);
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (!strcmp(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if ((look_type = search_block(arg, dir_abbrevs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (!strcmp(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}



int r_room_by_dir_distance(struct char_data * ch, int dir,
                                        int distance)
{
  int room = ch->in_room;
  int i = 0;

  while (i < distance) {
    if (!world[room].dir_option[dir])
      return NOWHERE;
    room = world[room].dir_option[dir]->to_room;
    /* dont scan directions where it loops back */
    if (room == ch->in_room)
      return NOWHERE;
    i++;
  }

  return room;
}




static char *sense_distance[] = {
    "",
    "",
    "a ways ",
    "a long ways ",
    "far far "
};

#define MAX_SCAN_DEPTH 4 /* maximum number of rooms to look in a direction */
ACMD(do_scan)
{
  int dir;
  int depth, look_depth, first_dir, last_dir;
  bool found;
  int see_objs, see_chars;
  struct obj_data *o;
  struct char_data *p;
  int percent, prob;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("In your dreams you scan the horizon...\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_BLIND))
    send_to_char("You squint about but you're blind!\r\n", ch);

  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !HAS_LIGHT(ch)) {
    send_to_char("It's pitch black and impossible to see anything!\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);
  } else {
    one_argument(argument, arg);
    if (!*arg) {
      first_dir = 0;
      last_dir = NUM_OF_DIRS - 1;
    } else if ((first_dir = search_block(arg, dirs, FALSE)) >= 0) {
      last_dir = first_dir + 1;
    } else {
      send_to_char("Scan in what direction?!?\r\n", ch);
      return;
    }
    look_depth = MIN(GET_LEVEL(ch) / 10 + 1, MAX_SCAN_DEPTH);
    percent = number(1, 101);	/* 101% is complete failure */
    prob = GET_SKILL(ch, SKILL_SCAN);
    if (IS_THRIKREEN(ch)) prob = 100;
    if (percent > prob)
      look_depth = 1;
    found = FALSE;
    for (dir = first_dir; dir < last_dir; dir++) {
      if (!world[ch->in_room].dir_option[dir])
        continue;
      /* depth starts at 1, 1 room away */
      for (depth = 1; depth < look_depth + 1; depth++) {
        if (r_room_by_dir_distance(ch, dir, depth) == NOWHERE)
          break;
        if (world[r_room_by_dir_distance(ch, dir, depth - 1)].dir_option[dir]
            && world[r_room_by_dir_distance(ch, dir, depth - 1)].dir_option[dir]->keyword) {
          if (!IS_SET(world[r_room_by_dir_distance(ch, dir, depth - 1)].dir_option[dir]->exit_info, EX_CLOSED)) {
            sprintf(buf2, "(%s open)", fname(
              world[r_room_by_dir_distance(ch, dir, depth - 1)].dir_option[dir]->keyword));
            sprintf(buf, "  %-48s: %s%s\r\n", buf2,
                    sense_distance[depth], dirs[dir]);
            send_to_char(buf, ch);
          }
          if (IS_DARK(r_room_by_dir_distance(ch, dir, depth)))
            break;
          if (IS_SET(world[r_room_by_dir_distance(ch, dir, depth - 1)].dir_option[dir]->exit_info, EX_CLOSED)) {
            sprintf(buf2, "(%s closed)", fname(
              world[r_room_by_dir_distance(ch, dir, depth - 1)].dir_option[dir]->keyword));
            sprintf(buf, "  %-48s: %s%s\r\n", buf2,
                    sense_distance[depth], dirs[dir]);
            break;
          }
        }
        see_objs = count_list_obj_to_char(
              world[r_room_by_dir_distance(ch, dir, depth)].contents, ch);
        see_chars = count_list_char_to_char(
              world[r_room_by_dir_distance(ch, dir, depth)].people, ch);
        if (see_objs || see_chars) {
          found = TRUE;
          if (see_objs)
            for (o = world[r_room_by_dir_distance(ch, dir, depth)].contents;
                 o; o = o->next_content) {
              if (CAN_SEE_OBJ(ch, o)) {
                sprintf(buf, "  %-48s: %s%s\r\n", (o->short_description) ?
                    o->short_description : "<None>", sense_distance[depth],
                    dirs[dir]);
                send_to_char(buf, ch);
              }
            }
          if (see_chars)
            for (p = world[r_room_by_dir_distance(ch, dir, depth)].people;
                 p; p = p->next_in_room) {
              if (CAN_SEE(ch, p)) {
                sprintf(buf, "  %-48s: %s%s\r\n", GET_NAME(p),
                    sense_distance[depth], dirs[dir]);
                send_to_char(buf, ch);
              }
            }
        }
      }
    }
    if (!found)
      send_to_char("Nothing.\r\n", ch);
  }
}



ACMD(do_examine)
{
  int bits;
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }

  look_at_target(ch, arg);

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char("When you look inside, you see:\r\n", ch);
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char("You're broke!\r\n", ch);
  else if (GET_GOLD(ch) == 1)
    send_to_char("You have one miserable little gold coin.\r\n", ch);
  else {
    sprintf(buf, "You have %d gold coins.\r\n", GET_GOLD(ch));
    send_to_char(buf, ch);
  }
}


ACMD(do_score)
{
  bool mounted = FALSE;

  sprintf(buf, "You are a %d year old %s.", GET_AGE(ch), 
    (GET_RACE(ch) == RACE_UNDEFINED) ?
      "Undefined" : pc_race_types[(int) GET_RACE(ch)]);

  if ((age(ch).month == 0) && (age(ch).day == 0))
    strcat(buf, "  It's your birthday today.\r\n");
  else
    strcat(buf, "\r\n");

  sprintf(buf,
       "%sYou have %d(%d) hit, %d(%d) mana and %d(%d) movement points.\r\n",
	  buf, GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  sprintf(buf, "%sYour armor class is %d/10, and your alignment is %d.\r\n",
	  buf, GET_AC(ch), GET_ALIGNMENT(ch));

  sprintf(buf, "%sYou have scored %d exp, and have %d gold coins.\r\n",
	  buf, GET_EXP(ch), GET_GOLD(ch));

  if (!IS_NPC(ch)) {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      sprintf(buf, "%sYou need %d exp to reach your next level.\r\n", buf,
      (titles[(int) GET_CLASS(ch)][GET_LEVEL(ch) + 1].exp) - GET_EXP(ch));
  }

  if (!IS_NPC(ch) && (GET_LEVEL(ch) == LVL_IMMORT - 1)) {
    if (GET_EXP(ch) >= (titles[(int) GET_CLASS(ch)][GET_LEVEL(ch) + 1].exp)) 
      sprintf(buf, "%s>>> You are now qualified to remort. <<<\r\n", buf);
    else
      sprintf(buf, "%s*** When you reach Level %d, "
                   "you can be remorted! ***\r\n", buf, LVL_IMMORT);
  }

  playing_time = real_time_passed((time(0) - ch->player.time.logon) +
			  ch->player.time.played, 0);
  sprintf(buf, "%sYou have been playing for %d days and %d hours.\r\n",
	  buf, playing_time.day, playing_time.hours);

  sprintf(buf, "%sThis ranks you as %s %s (level %d).\r\n", buf,
	  GET_NAME(ch), GET_TITLE(ch), GET_LEVEL(ch));

  if (HAS_PET(ch)) if (IS_MOUNTED(GET_PET(ch))) mounted = TRUE;

  if (!mounted) {
  switch (GET_POS(ch)) {
  case POS_DEAD:
    strcat(buf, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    strcat(buf, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    strcat(buf, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    strcat(buf, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    strcat(buf, "You are resting.\r\n");
    break;
  case POS_SITTING:
    strcat(buf, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    if (FIGHTING(ch))
      sprintf(buf, "%sYou are fighting %s.\r\n", buf, PERS(FIGHTING(ch), ch));
    else
      strcat(buf, "You are fighting thin air.\r\n");
    break;
  case POS_STANDING:
    strcat(buf, "You are standing.\r\n");
    break;
  default:
    strcat(buf, "You are floating.\r\n");
    break;
  }
  }
  if (mounted) {
    sprintf(buf, "%sYou are riding your pet, %s.\r\n", buf, GET_NAME(GET_PET(ch)));
  } else if (HAS_PET(ch)) {
    if (GET_PET(ch)->in_room == ch->in_room) {
      sprintf(buf, "%sYour pet %s is following you.\r\n", buf, GET_NAME(GET_PET(ch)));
    }
  }
  
  if (GET_COND(ch, DRUNK) > 10)
    strcat(buf, "You are intoxicated.\r\n");

  if (GET_COND(ch, FULL) == 0)
    strcat(buf, "You are hungry.\r\n");

  if (GET_COND(ch, THIRST) == 0)
    strcat(buf, "You are thirsty.\r\n");
  
  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");

  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");

  if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

  if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
    strcat(buf, "You are sensitive to the presence of magical things.\r\n");

  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");

  if (IS_AFFECTED(ch, AFF_MAGIC_RESIST))
    strcat(buf, "You are protected by an Anti-Magic Shell.\r\n");

  if (IS_AFFECTED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");

  if (IS_AFFECTED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");

  if (affected_by_spell(ch, SPELL_FAERIE_FIRE))
    strcat(buf, "You are surrounded by a pink aura.\r\n");

  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");

  if (IS_AFFECTED2(ch, AFF2_ICE_SHIELD))
    strcat(buf, "Your are protected by an Ice Shield.\r\n");

  send_to_char(buf, ch);
}



/*
 New Score2 format:
 __^__                                                                 __^__
( ___ )---------------------------------------------------------------( ___ )
 | / | Aule the Implementor                                            | \ |
 | / | Level [60]   Race [Giant]   Class [  ]                          | \ |
 | / | Clan [          ]                                               | \ |
 | / | Age [ 17y]   Online Time [ 0d, 0h ]                             | \ |
 | / | It's your birthday today.                                       | \ |
 | / | _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_ | \ |
 | / | HP [  ( )]    Mana [    ]     Move [    ]                       | \ |
 | / | Stats                                                           | \ |
 | / | Mods                                                            | \ |
 | / | AC                                                              | \ |
 | / | HitRoll    DamRoll   Alignment                                  | \ |
 | / | Gold    Bank                                                    | \ |
 | / | Experience Points            Experience to Level                | \ |
 | / | _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_ | \ |
 | / | You are standing.                                               | \ |
 |___|                                                                 |___|
(_____)---------------------------------------------------------------(_____)

*/
ACMD(do_score2)
{

  if (IS_NPC(ch)) {
    send_to_char("Only players have scores!\r\n", ch);
    return;
  }

  /* top of the scrolls */
  send_to_char(" __^__                                 ", ch);
  send_to_char("                                __^__\r\n", ch);
  send_to_char("( ___ )--------------------------------", ch);
  send_to_char("-------------------------------( ___ )\r\n", ch);

  sprintf(buf, "%s %s", GET_NAME(ch), GET_TITLE(ch));
  sprintf(buf2, " | / | %-63s | \\ |\r\n", buf);
  send_to_char(buf2, ch);

  sprintf(buf, " | / | Class [%-12s]  Level [%2d]  Race [%-12s]"
               "           | \\ |\r\n",
      pc_class_types[(int) GET_CLASS(ch)],
      GET_LEVEL(ch), 
      (GET_RACE(ch) == RACE_UNDEFINED) ?
      "Undefined" : pc_race_types[(int) GET_RACE(ch)]);
  send_to_char(buf, ch);

  sprintf(buf, " | / | Clan  [%-12s]  Clan Level [%2d]          "
               "                 | \\ |\r\n",
      clan_names[(int) GET_CLAN(ch)], GET_CLAN_LEVEL(ch));
  send_to_char(buf, ch);

  /* bottom of the scrolls */
  send_to_char(" |___|                                 ", ch);
  send_to_char("                                |___|\r\n", ch);
  send_to_char("(_____)--------------------------------", ch);
  send_to_char("-------------------------------(_____)\r\n", ch);
 
  return;
}

ACMD(do_romscore)
{
    char buf[MAX_STRING_LENGTH];

    playing_time = real_time_passed((time(0) - ch->player.time.logon) +
	ch->player.time.played, 0);
    
    get_char_pretitle(ch, buf2);
    sprintf( buf,
        "You are %s%s %s, level %d, %d years old (%d days and %d hours).\n\r",
	buf2,
        GET_NAME(ch),
        IS_NPC(ch) ? "" : GET_TITLE(ch),
        GET_LEVEL(ch), GET_AGE(ch),
	playing_time.day, playing_time.hours);
    send_to_char( buf, ch ); 
    
    sprintf(buf, "Race: %s  Sex: %s  Class: %s\n\r",
	(GET_RACE(ch) == RACE_UNDEFINED) ?
	"Undefined" : pc_race_types[(int) GET_RACE(ch)],
        GET_SEX(ch) == 0 ? "sexless" : GET_SEX(ch) == 1 ? "male" : "female",
        IS_NPC(ch) ? "mobile" : pc_class_types[(int) GET_CLASS(ch)]);
    send_to_char(buf,ch);
    
    sprintf( buf,
        "You have %d/%d hit, %d/%d mana, %d/%d movement.\n\r",
	GET_HIT(ch), GET_MAX_HIT(ch),
	GET_MANA(ch), GET_MAX_MANA(ch),
	GET_MOVE(ch), GET_MAX_MOVE(ch));
    send_to_char( buf, ch );
    
/*
    sprintf( buf,
        "You are carrying %d/%d items with weight %ld/%d pounds.\n\r",
        ch->carry_number, can_carry_n(ch), 
        get_carry_weight(ch) / 10, can_carry_w(ch) /10 );
    send_to_char( buf, ch ); 
*/
    
    if(GET_LEVEL(ch) > LVL_LOWBIE) {
	sprintf( buf,
	    "Str: %d Int: %d Wis: %d Dex: %d Con: %d Cha: %d\n\r",
	    GET_STR(ch),
	    GET_INT(ch),
	    GET_WIS(ch),
            GET_DEX(ch),
            GET_CON(ch),
            GET_CHA(ch));
	send_to_char( buf, ch );
    }
    
    sprintf( buf,
        "You have scored %d exp, and have %d gold coins.\n\r",
        GET_EXP(ch),  GET_GOLD(ch)); 
    send_to_char( buf, ch );
    
    /* RT shows exp to level */
    if (!IS_NPC(ch) && (GET_LEVEL(ch) < LVL_IMMORT))
    {
	sprintf (buf,
	    "You need %d exp to level.\n\r",
	    (titles[(int) GET_CLASS(ch)][GET_LEVEL(ch) + 1].exp) - GET_EXP(ch));
	send_to_char( buf, ch );
     }
     
    if(GET_LEVEL(ch) < LVL_LOWBIE) {
	sprintf( buf, "Wimpy set to %d hit points.\n\r", GET_WIMP_LEV(ch));
	send_to_char( buf, ch );
    }
    
    if ( !IS_NPC(ch) && GET_COND(ch, DRUNK) > 10 )
        send_to_char( "You are drunk.\n\r",   ch );
    if ( !IS_NPC(ch) && GET_COND(ch, THIRST) ==  0 )
        send_to_char( "You are thirsty.\n\r", ch );
    if ( !IS_NPC(ch) && GET_COND(ch, FULL) ==  0 )
        send_to_char( "You are hungry.\n\r",  ch );
        
    switch (GET_POS(ch)) {
	case POS_DEAD:
	    strcat(buf, "You are DEAD!\r\n");
	    break;
	case POS_MORTALLYW:
	    strcat(buf, "You are mortally wounded.\r\n");
	    break;
	case POS_INCAP:
	    strcat(buf, "You are incapacitated, slowly fading away...\r\n");
	    break;
	case POS_STUNNED:
	    strcat(buf, "You are stunned!  You can't move!\r\n");
	    break;
	case POS_SLEEPING:
	    strcat(buf, "You are sleeping.\r\n");
	    break;
	case POS_RESTING:
	    strcat(buf, "You are resting.\r\n");
	    break;
	case POS_SITTING:
	    strcat(buf, "You are sitting.\r\n");
	    break;
	case POS_FIGHTING:
	    strcat(buf, "You are fighting!\r\n");
	    break;
	case POS_STANDING:
	    strcat(buf, "You are standing.\r\n");
	    break;
	default:
	    strcat(buf, "You are floating.\r\n");
	break;
    }
    
    if (GET_LEVEL(ch) >= 25) 
    {
        sprintf( buf,"Armor class: %d\n\r",
                 GET_AC(ch));
        send_to_char(buf,ch);
    }

    send_to_char("You are ", ch);
        
    if      (GET_AC(ch) >=  101 )
        sprintf(buf,"hopelessly vulnerable.\n\r");
    else if (GET_AC(ch) >= 80)
        sprintf(buf,"defenseless.\n\r");
    else if (GET_AC(ch) >= 60) 
        sprintf(buf,"barely protected.\n\r");
    else if (GET_AC(ch) >= 40)
        sprintf(buf,"slightly armored.\n\r");
    else if (GET_AC(ch) >= 20)
        sprintf(buf,"somewhat armored.\n\r");
    else if (GET_AC(ch) >= 0)
        sprintf(buf,"armored.\n\r");
    else if (GET_AC(ch) >= -20)
        sprintf(buf,"well-armored.\n\r");
    else if (GET_AC(ch) >= -40) 
        sprintf(buf,"very well-armored.\n\r");
    else if (GET_AC(ch) >= -60)
        sprintf(buf,"heavily armored.\n\r");
    else if (GET_AC(ch) >= -80)
        sprintf(buf,"superbly armored.\n\r");
    else if (GET_AC(ch) >= -100)
        sprintf(buf,"almost invulnerable.\n\r");
    else
        sprintf(buf,"divinely armored.\n\r");
    send_to_char(buf, ch);

    if (GET_LEVEL(ch) >= 25) {
	sprintf(buf, "Saving throws: Paral:[%d] Rod:[%d] Petri:[%d] Breath:[%d] Spell:[%d]\r\n",
	    25 + GET_SAVE(ch, 0),
	    25 + GET_SAVE(ch, 1),
	    25 + GET_SAVE(ch, 2),
	    25 + GET_SAVE(ch, 3),
	    25 + GET_SAVE(ch, 4));
	send_to_char(buf, ch);
    }

    /* RT wizinvis and holy light */
    if ( IS_IMMORT(ch))
    {
	send_to_char("Holy Light: ",ch);
	if(PRF_FLAGGED(ch, PRF_HOLYLIGHT))
	    send_to_char("on",ch);
	else
	    send_to_char("off",ch);
      
	if (GET_INVIS_LEV(ch))
	{ 
	    sprintf( buf, "  Invisible: level %d",GET_INVIS_LEV(ch));
	    send_to_char(buf,ch);
	} 
	send_to_char("\n\r",ch);
    }

    if ( GET_LEVEL(ch) >= 15 )
    {
        sprintf( buf, "Hitroll: %d  Damroll: %d.\n\r",
            GET_HITROLL(ch), GET_DAMROLL(ch) );
        send_to_char( buf, ch );
    }   
    
    if ( GET_LEVEL(ch) >= 10 )
    {
        sprintf( buf, "Alignment: %d.  ", GET_ALIGNMENT(ch));
        send_to_char( buf, ch );
    }   
    
    send_to_char( "You are ", ch );
         if ( GET_ALIGNMENT(ch) >  900 ) send_to_char( "angelic.\n\r", ch );
    else if ( GET_ALIGNMENT(ch) >  700 ) send_to_char( "saintly.\n\r", ch );
    else if ( GET_ALIGNMENT(ch) >  350 ) send_to_char( "good.\n\r",    ch );
    else if ( GET_ALIGNMENT(ch) >  100 ) send_to_char( "kind.\n\r",    ch );
    else if ( GET_ALIGNMENT(ch) > -100 ) send_to_char( "neutral.\n\r", ch );
    else if ( GET_ALIGNMENT(ch) > -350 ) send_to_char( "mean.\n\r",    ch );
    else if ( GET_ALIGNMENT(ch) > -700 ) send_to_char( "evil.\n\r",    ch );
    else if ( GET_ALIGNMENT(ch) > -900 ) send_to_char( "demonic.\n\r", ch );
    else                             send_to_char( "satanic.\n\r", ch );
    
}


/*
 * Credits for do_affects to Mink @ Kore  (yjlee@mit.edu)
 */
ACMD(do_affects)
{
  int a = 0;          /* total number of things you are affected by */
  int l = 0;          /* the length of a word from string token */
  int linel = 0;      /* the current line length */
  char *s;            /* a word from string token */
  int i;              /* simple counter */


  sprintf(buf, "You are affected by:");

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    strcat(buf, " blindness"); a++; }
  if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
    strcat(buf, " invis"); a++; }
  if (IS_AFFECTED(ch, AFF_DETECT_INVIS)) {
    strcat(buf, " detect-invis"); a++; }
  if (IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
    strcat(buf, " detect-magic"); a++; }
  if (IS_AFFECTED(ch, AFF_SANCTUARY)) {
    strcat(buf, " sanctuary"); a++; }
  if (IS_AFFECTED(ch, AFF_POISON)) {
    strcat(buf, " poison"); a++; }
  if (IS_AFFECTED(ch, AFF_CHARM)) {
    strcat(buf, " charm"); a++; }
  if (affected_by_spell(ch, SPELL_ARMOR)) {
    strcat(buf, " armor"); a++; }
  if (affected_by_spell(ch, SPELL_BARKSKIN)) {
    strcat(buf, " barkskin"); a++; }
  if (affected_by_spell(ch, SPELL_FAERIE_FIRE)) {
    strcat(buf, " faerie-fire"); a++; }
  if (IS_AFFECTED(ch, AFF_INFRAVISION)) {
    strcat(buf, " infra"); a++; }
  if (IS_AFFECTED(ch, AFF_HASTE)) {
    strcat(buf, " haste"); a++; }
  if (affected_by_spell(ch, SPELL_SLOW)) {
    strcat(buf, " slow"); a++; }
  if (IS_AFFECTED(ch, AFF_INSPIRE)) {
    strcat(buf, " inspire"); a++; }
  if (affected_by_spell(ch, SPELL_MIRROR_IMAGE)) {
    strcat(buf, " mirror-image"); a++; }
  if (IS_AFFECTED(ch, AFF_FLY)) {
    strcat(buf, " fly"); a++; }
  if (affected_by_spell(ch, SPELL_AID)) {
    strcat(buf, " aid"); a++; }
  if (IS_AFFECTED(ch, AFF_RAGE)) {
    strcat(buf, " rage"); a++; }
  if (affected_by_spell(ch, SPELL_WATERWALK)) {
    strcat(buf, " waterwalk"); a++; }
  if (affected_by_spell(ch, SPELL_FEEBLEMIND)) {
    strcat(buf, " feeblemind"); a++; }
  if (IS_AFFECTED(ch, AFF_CURSE)) {
    strcat(buf, " curse"); a++; }
  if (affected_by_spell(ch, SPELL_STRENGTH)) {
    strcat(buf, " strength"); a++; }
  if (affected_by_spell(ch, SPELL_BLESS)) {
    strcat(buf, " bless"); a++; }
  if (IS_AFFECTED(ch, AFF_SENSE_LIFE)) {
    strcat(buf, " sense-life"); a++; }
  if (IS_AFFECTED(ch, AFF_GAUGE)) {
    strcat(buf, " gauge"); a++; }
  if (IS_AFFECTED(ch, AFF_PROTECT_GOOD)) {
    strcat(buf, " protection_good"); a++; }
  if (IS_AFFECTED(ch, AFF_PROTECT_EVIL)) {
    strcat(buf, " protection_evil"); a++; }
  if (IS_AFFECTED(ch, AFF_SNEAK)) {
    strcat(buf, " sneak"); a++; }
  if (IS_AFFECTED(ch, AFF_STONESKIN)) {
    strcat(buf, " stoneskin"); a++; }
  if (IS_AFFECTED2(ch, AFF2_JARRED)) {
    strcat(buf, " jarred"); a++; }
  if (IS_AFFECTED2(ch, AFF2_ICE_SHIELD)) {
    strcat(buf, " ice_shield"); a++; }
  if (IS_AFFECTED2(ch, AFF2_WEBBED)) {
    strcat(buf, " web"); a++; }
  if (IS_AFFECTED2(ch, AFF2_FIRESHIELD)) {
    strcat(buf, " fireshield"); a++; }
  if (IS_AFFECTED2(ch, AFF2_MANASHELL)) {
    strcat(buf, " mana_shell"); a++; }

  if (a == 0)
    strcat(buf, " nothing");

  strcat(buf, ".\r\n");

  /*
   * pretty format the output, so it doesnt wrap around to the next lines.
   * kind of complex; a person can comment out this whole section with
   * ifdef 0 / endif if they dont trust it.
   */ 
  strcpy(buf2, buf);       /* copy it to buf2 since strtok will slice it up */
  *buf = '\0';             /* clear out buf and now write the words back in */

  s = strtok(buf2, " ");   /* get the first word */
  l = strlen(s);
  if ((linel + l) > 75) {
    strcat(buf, "\r\n");
    linel = 0;
  }
  linel += l;
  sprintf(buf, "%s%s ", buf, s);

                           /* now get all the rest of the words */
  /* could have used a while loop, whatever */
  for (i = 0; (s = strtok(NULL, "")) != '\0'; i++) {
    l = strlen(s);
    if ((linel + l) > 75) {     /* 75 is the max line length */
      strcat(buf, "\r\n");
      linel = 0;
    }
    linel += l;
    sprintf(buf, "%s%s ", buf, s);
  }
  /* ok now the string is all pretty formatted */


  send_to_char(buf, ch);
}



ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}



ACMD(do_equipment)
{
  int i, k, found = 0, num_wear;
  
  if (IS_THRIKREEN(ch)) num_wear = NUM_THRI_WEARS;
  else num_wear = NUM_WEARS;


/* HACKED to make a special exception for wielded weapons, when one
  wields just one either primary or secondary, to make it look 'normal'. */
  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < num_wear; i++) {
    k = wear_display_order[i];
    if (IS_THRIKREEN(ch)) k = thri_wear_display_order[i];
    if (ch->equipment[k]) {
      if (!IS_THRIKREEN(ch)) {
        if ((k == WEAR_WIELD && !ch->equipment[WEAR_WIELD_2]) ||
            (k == WEAR_WIELD_2 && !ch->equipment[WEAR_WIELD]))
          send_to_char("<wielded>            ", ch);
        else
          send_to_char(where[k], ch);
      } else {
        send_to_char(thri_where[k], ch);
      }
      if (CAN_SEE_OBJ(ch, ch->equipment[k]))
	show_obj_to_char(ch->equipment[k], ch, 1, 1);
      else 
	send_to_char("       Something.\r\n", ch);
      found = TRUE;
    }
  }
  if (!found) {
    send_to_char(" Nothing.\r\n", ch);
  }
}



ACMD(do_time)
{
  char *suf;
  int weekday, day;
  extern struct time_info_data time_info;
  extern const char *weekdays[];
  extern const char *month_name[];

  sprintf(buf, "It is %d o'clock %s, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  day = time_info.day + 1;	/* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);

  send_to_char(buf, ch);
}


ACMD(do_weather)
{
  static char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
  "lit by flashes of lightning"};

  if (OUTSIDE(ch)) {
    sprintf(buf, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    (weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
}



ACMD(do_help)
{
  extern int top_of_helpt;
  extern struct help_index_element *help_index;
  extern FILE *help_fl;
  extern char *help;

  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_index) {
    send_to_char("No help available.\r\n", ch);
    return;
  }
  bot = 0;
  top = top_of_helpt;

  for (;;) {
    mid = (bot + top) >> 1;
    minlen = strlen(argument);

    if (!(chk = strn_cmp(argument, help_index[mid].keyword, minlen))) {
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_index[mid - 1].keyword, minlen))))
	mid--;
      fseek(help_fl, help_index[mid].pos, SEEK_SET);
      *buf2 = '\0';
      for (;;) {
	fgets(buf, 128, help_fl);
	if (*buf == '#')
	  break;
	buf[strlen(buf) - 1] = '\0';
	strcat(buf2, strcat(buf, "\r\n"));
      }
      page_string(ch->desc, buf2, 1);
      return;
    } else if (bot >= top) {
      send_to_char("There is no help on that word.\r\n", ch);
      sprintf(buf, "No help on topic '%s', requested by %s.",
               argument, GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      return;
    } else if (chk > 0)
      bot = ++mid;
    else
      top = --mid;
  }
}



ACMD(do_clanhelp)
{
  extern int top_of_clanhelpt;
  extern struct help_index_element *clanhelp_index;
  extern FILE *clanhelp_fl;
  extern char *clanhelp;

  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, clanhelp, 0);
    return;
  }
  if (!clanhelp_index) {
    send_to_char("No clanhelp available.\r\n", ch);
    return;
  }
  bot = 0;
  top = top_of_clanhelpt;

  for (;;) {
    mid = (bot + top) >> 1;
    minlen = strlen(argument);

    if (!(chk = strn_cmp(argument, clanhelp_index[mid].keyword, minlen))) {
      while ((mid > 0) &&
         (!(chk = strn_cmp(argument, clanhelp_index[mid - 1].keyword, minlen))))
        mid--;
      fseek(clanhelp_fl, clanhelp_index[mid].pos, SEEK_SET);
      *buf2 = '\0';
      for (;;) {
        fgets(buf, 128, clanhelp_fl);
        if (*buf == '#')
          break;
        buf[strlen(buf) - 1] = '\0';
        strcat(buf2, strcat(buf, "\r\n"));
      }
      page_string(ch->desc, buf2, 1);
      return;
    } else if (bot >= top) {
      send_to_char("There is no clanhelp on that word.\r\n", ch);
      sprintf(buf, "No clanhelp on topic '%s', requested by %s.",
               argument, GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      return;
    } else if (chk > 0)
      bot = ++mid;
    else
      top = --mid;
  }
}



ACMD(do_areahelp)
{
  extern int top_of_areahelpt;
  extern struct help_index_element *areahelp_index;
  extern FILE *areahelp_fl;
  extern char *areahelp;

  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, areahelp, 0);
    return;
  }
  if (!areahelp_index) {
    send_to_char("No areahelp available.\r\n", ch);
    return;
  }
  bot = 0;
  top = top_of_areahelpt;

  for (;;) {
    mid = (bot + top) >> 1;
    minlen = strlen(argument);

    if (!(chk = strn_cmp(argument, areahelp_index[mid].keyword, minlen))) {
      while ((mid > 0) &&
         (!(chk = strn_cmp(argument, areahelp_index[mid - 1].keyword, minlen))))
        mid--;
      fseek(areahelp_fl, areahelp_index[mid].pos, SEEK_SET);
      *buf2 = '\0';
      for (;;) {
        fgets(buf, 128, areahelp_fl);
        if (*buf == '#')
          break;
        buf[strlen(buf) - 1] = '\0';
        strcat(buf2, strcat(buf, "\r\n"));
      }
      page_string(ch->desc, buf2, 1);
      return;
    } else if (bot >= top) {
      send_to_char("There is no areahelp on that word.\r\n", ch);
      sprintf(buf, "No areahelp on topic '%s', requested by %s.",
      argument, GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      return;
    } else if (chk > 0)
      bot = ++mid;
    else
      top = --mid;
  }
}



ACMD(do_wizhelp)
{
  extern int top_of_wizhelpt;
  extern struct help_index_element *wizhelp_index;
  extern FILE *wizhelp_fl;
  extern char *wizhelp;

  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, wizhelp, 0);
    return;
  }
  if (!wizhelp_index) {
    send_to_char("No wizhelp available.\r\n", ch);
    return;
  }
  bot = 0;
  top = top_of_wizhelpt;

  for (;;) {
    mid = (bot + top) >> 1;
    minlen = strlen(argument);

    if (!(chk = strn_cmp(argument, wizhelp_index[mid].keyword, minlen))) {
      while ((mid > 0) &&
         (!(chk = strn_cmp(argument, wizhelp_index[mid - 1].keyword, minlen))))
        mid--;
      fseek(wizhelp_fl, wizhelp_index[mid].pos, SEEK_SET);
      *buf2 = '\0';
      for (;;) {
        fgets(buf, 128, wizhelp_fl);
        if (*buf == '#')
          break;
        buf[strlen(buf) - 1] = '\0';
        strcat(buf2, strcat(buf, "\r\n"));
      }
      page_string(ch->desc, buf2, 1);
      return;
    } else if (bot >= top) {
      send_to_char("There is no wizhelp on that word.\r\n", ch);
      sprintf(buf, "No wizhelp on topic '%s', requested by %s.",
               argument, GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      return;
    } else if (chk > 0)
      bot = ++mid;
    else
      top = --mid;
  }
}



#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-r racelist] [-c classlist] [-s] [-o] [-q] [-m] [-z] [-b clanname]\r\n"

ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH];
  char pretitle[MAX_TITLE_LENGTH];
  char mode;
  int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0, awaywho = 0;
  int showrace = 0, showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int showclan = 0; 
  int clan = CLAN_UNDEFINED;
  int who_room = 0;
/*
  char *do_who_sword = {
"                      o\r\n"
"                     /\r\n"
"                    /___________________________________,\r\n"
"          O========<   |-|.|`|>,<>`|.  <>|''  < <> |>,  /\r\n"
"                    \\-----------------------------------`\r\n"
"                     \\\r\n"
"                      o\r\n\r\n"
  };
*/
/*
      _____     _________
      |\./|  ././.v~__,/~ _____   __________    _________________
      | | |././.(W---\| /',---.`\ |\.,-----.`\  |\./,--------<~~
      | |././ /<M.    ./././~\`\ \| |./~~~\`\ \ | | /~~~~~~~~~~
\~b______./__/ ()\_------------\ \ \-------\ \ \| |/`-------/|---------------.
>@)$$$$$$$$($(   = ##>=========) ) )========) ) ) |-------<  |===------------->
/_p~~~~~~~\~~\ ()/~------------/ / /-------/ / /| |\,======`\|---------------'
      | |`\`\ \<M`    `\ \`\_/ / /| | \___/ / / | | |
      | | |`\`\`(B---/| \ `---' / | | |   \`\`\ | | |\________
      |/'\|  `\`\`?_~~`\_`~~~~~`  | | |    `\`\`\/'\`\-------<__
      ~~~~~    `~~~~~~~~~~        |/'\|      )'\,\~~~~~~~~~~~~~~~~
                                  ~~~~~      ~~~~~~
*/

  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';

  while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);
	break;
      case 'a':
	awaywho = 1;
	strcpy(buf, buf1);
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'm':
	who_room = 1;
	strcpy(buf, buf1);
	break;
      case 'r':
        half_chop(buf1, arg, buf);
        showrace |= find_race_bitvector(arg);
        break;
      case 'c':
	half_chop(buf1, arg, buf);
	showclass |= find_class_bitvector(arg);
	break;
      case 'b':
        showclan = 1;
        half_chop(buf1, arg, buf);
        if (!*arg)
          clan = GET_CLAN(ch);
        else
          clan = parse_clan(arg);
        break;
      default:
	send_to_char(WHO_FORMAT, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(WHO_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */

  sprintf(buf,
"      %s_____    _____________\r\n"
"      %s|\\./|  ./'.%s/.v~__%s,/~_____   __________    _________________\r\n"
"      %s| | |./.%s/.(W%s---\\| /',---.`\\ |\\.,-----.`\\  |\\./,--------<~~\r\n"
"      %s| |./.%s/ /<M.    %s./././~\\`\\ \\| |./~~~\\`\\ \\ | | /~~~~~~~~~~\r\n"
"%s\\~b%s______.%s/__/ %s()%s\\_%s-----------%s\\ \\ \\%s--------%s\\ \\ \\| |/`-------/|%s---------------.\r\n"
"%s>@)%s$$$$$$$$%s($(   = ##>%s=========%s) ) )%s========%s) ) ) |-------<  |%s===------------->\r\n"
"%s/_p%s~~~~~~~%s\\~~\\ %s()%s/~%s-----------%s/ / /%s--------%s/ / /| |\\,======`\\|%s---------------'\r\n"
"      %s| |`\\`%s\\ \\<M`    %s`\\ \\`\\_/ / /| | \\___/ / / | | |\r\n"
"      %s| | |`\\`%s\\`(B%s---/| \\ `---' / | | |   \\`\\`\\ | | |\\________\r\n"
"      %s|/'\\|  `\\`%s\\`?_~~%s`\\_`~~~~~`  | | |    `\\`\\`\\/'\\`\\-------<__\r\n"
"      %s~~~~~    `~~~~~~~~~~        |/'\\|      )'\\,\\~~~~~~~~~~~~~~~~\r\n"
"                                  %s~~~~~      ~~~~~~%s^N\r\n",
     CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCPOMMEL(ch), CCHILT(ch), CCQUILLIONS(ch), CCJEWELS(ch), CCQUILLIONS(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch),
     CCPOMMEL(ch), CCHILT(ch), CCQUILLIONS(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch),
     CCPOMMEL(ch), CCHILT(ch), CCQUILLIONS(ch), CCJEWELS(ch), CCQUILLIONS(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch),
     CCRUNES(ch), CCNRM(ch));

  send_to_char(buf, ch);

  for (d = descriptor_list; d; d = d->next) {
    switch (d->connected) {
        case CON_PLAYING:
        case CON_OEDIT:
        case CON_REDIT:
        case CON_ZEDIT:
        case CON_MEDIT:
        case CON_SEDIT:
        case CON_BATTLE_VRFY:
                    break;	/* go ahead and show them, they're normal */
        default:    continue;	/* people being asked for passwords etc, */
                    break;	/* dont show them on the who list */
    }

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (awaywho && !PRF2_FLAGGED(tch, PRF2_AWAY))
      continue;
    if (questwho && GET_QUEST(tch) == QUEST_OFF)
      continue;
    if (localwho && world[ch->in_room].zone != world[tch->in_room].zone)
      continue;
    if (who_room && (tch->in_room != ch->in_room))
      continue;
    if (showrace && !(showrace & (1 << GET_RACE(tch))))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
    if (showclan && (GET_CLAN(tch) != clan))
      continue;
    if (short_list) {
      if (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && GET_LEVEL(ch) < LVL_IMMORT) {
        sprintf(buf, "%s[-- --     -- ] %-12.12s%s%s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            GET_NAME(tch),
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCNRM(ch) : ""),
            ((!(++num_can_see % 2)) ? "\r\n" : ""));
      } else {
        sprintf(buf, "%s[%2d %s %s] %-12.12s%s%s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            GET_LEVEL(tch), RACE_ABBR(tch), CLASS_ABBR(tch), GET_NAME(tch),
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCNRM(ch) : ""),
            ((!(++num_can_see % 2)) ? "\r\n" : ""));
      }
      send_to_char(buf, ch);
    } else {
      num_can_see++;
      get_char_pretitle(tch, pretitle);
      if (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && GET_LEVEL(ch) < LVL_IMMORT) {
        sprintf(buf, "%s[-- --     -- ] %s%s %s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            pretitle,
            GET_NAME(tch),
            GET_TITLE(tch));
      } else {
        sprintf(buf, "%s[%2d %s %s] %s%s %s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            GET_LEVEL(tch), RACE_ABBR(tch), CLASS_ABBR(tch), pretitle,
            GET_NAME(tch),
            GET_TITLE(tch));
      }

      if (GET_INVIS_LEV(tch))
	sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(tch));
      else if (IS_AFFECTED(tch, AFF_INVISIBLE))
	sprintf(buf, "%s %s(invis)%s", buf, CCINFO(ch), CCNRM(ch));

      if (PLR_FLAGGED(tch, PLR_MAILING))
	sprintf(buf, "%s %s(mailing)%s", buf, CCALERT(ch), CCNRM(ch));
      else if (PLR_FLAGGED(tch, PLR_WRITING)) {
        if (STATE(tch->desc) == CON_PLAYING)
          sprintf(buf, "%s %s(writing)%s", buf, CCALERT(ch), CCNRM(ch));
        else
          sprintf(buf, "%s %s(editing)%s", buf, CCALERT(ch), CCNRM(ch));
      }

      if (PLR_FLAGGED(tch, PLR_MASTER))
        sprintf(buf, "%s %s(DQ Master)%s", buf, CCALERT(ch), CCNRM(ch));
/*
      if (PRF_FLAGGED(tch, PRF_DEAF))
	sprintf(buf, "%s %s(deaf)%s", buf, CCALERT(ch), CCNRM(ch));
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	sprintf(buf, "%s %s(notell)%s", buf, CCALERT(ch), CCNRM(ch));
      if (GET_QUEST(tch) > QUEST_OFF)
        switch (GET_QUEST(tch)) {
          case QUEST_OFF:         sprintf(buf, "%s %s(quest OFF)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_NORMAL:      sprintf(buf, "%s %s(quest)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_ENROLLED:    sprintf(buf, "%s %s(quest enrolled)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_SURVIVAL:    sprintf(buf, "%s %s(survival)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_PKQUEST:     sprintf(buf, "%s %s(pkquest)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_DEATHQUEST:  sprintf(buf, "%s %s(deathquest)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          default:                sprintf(buf, "%s %s(quest UNKNOWN)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
        }
*/
      if (PLR_FLAGGED(tch, PLR_THIEF))
	sprintf(buf, "%s %s(THIEF)%s", buf, CCWARNING(ch), CCNRM(ch));
      if (PLR_FLAGGED(tch, PLR_KILLER))
	sprintf(buf, "%s %s(KILLER)%s", buf, CCWARNING(ch), CCNRM(ch));
      if (PRF2_FLAGGED(tch, PRF2_AWAY))
        sprintf(buf, "%s %s(AFK)%s", buf, CCALERT(ch), CCNRM(ch));
      if ((GET_CLAN(tch) > CLAN_NOCLAN) && (GET_LEVEL(tch) < LVL_GOD)) {
        if (GET_CLAN_LEVEL(tch) > CLAN_LVL_MEMBER) {
          sprintf(buf, "%s %s(%s of %s)%s", buf, CCCLANSAY(ch),
              custom_clan_levels[(int) GET_CLAN(tch)][(int) GET_CLAN_LEVEL(tch)],
              clan_names[(int) GET_CLAN(tch)], CCNRM(ch)); 
        } else {
          sprintf(buf, "%s %s(%s)%s", buf, CCCLANSAY(ch),
              clan_names[(int) GET_CLAN(tch)], CCNRM(ch));
        }
      }
      if (GET_LEVEL(tch) >= LVL_IMMORT)
	strcat(buf, CCNRM(ch));
      strcat(buf, "^N\r\n");
      send_to_char(buf, ch);
    }				/* endif shortlist */
  }				/* end of for */
  if (short_list && (num_can_see % 2))
    send_to_char("\r\n", ch);

  if (num_can_see == 0)
    sprintf(buf, "\r\nNo-one prevents the geno!\r\n");
  else if (num_can_see == 1)
    sprintf(buf, "\r\nOne brave hero defends Kore.\r\n");
  else
    sprintf(buf, "\r\n%d stalwart heroes defend Kore.\r\n", num_can_see);

  send_to_char(buf, ch);
}

ACMD(do_newwho)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH];
  char pretitle[MAX_TITLE_LENGTH];
  char mode;
  int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0, awaywho = 0;
  int showrace = 0, showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int showclan = 0; 
  int clan = CLAN_UNDEFINED;
  int who_room = 0;
/*
  char *do_who_sword = {
"                      o\r\n"
"                     /\r\n"
"                    /___________________________________,\r\n"
"          O========<   |-|.|`|>,<>`|.  <>|''  < <> |>,  /\r\n"
"                    \\-----------------------------------`\r\n"
"                     \\\r\n"
"                      o\r\n\r\n"
  };
*/
/*
      _____     _________
      |\./|  ././.v~__,/~ _____   __________    _________________
      | | |././.(W---\| /',---.`\ |\.,-----.`\  |\./,--------<~~
      | |././ /<M.    ./././~\`\ \| |./~~~\`\ \ | | /~~~~~~~~~~
\~b______./__/ ()\_------------\ \ \-------\ \ \| |/`-------/|---------------.
>@)$$$$$$$$($(   = ##>=========) ) )========) ) ) |-------<  |===------------->
/_p~~~~~~~\~~\ ()/~------------/ / /-------/ / /| |\,======`\|---------------'
      | |`\`\ \<M`    `\ \`\_/ / /| | \___/ / / | | |
      | | |`\`\`(B---/| \ `---' / | | |   \`\`\ | | |\________
      |/'\|  `\`\`?_~~`\_`~~~~~`  | | |    `\`\`\/'\`\-------<__
      ~~~~~    `~~~~~~~~~~        |/'\|      )'\,\~~~~~~~~~~~~~~~~
                                  ~~~~~      ~~~~~~
*/

  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';

  while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);
	break;
      case 'a':
	awaywho = 1;
	strcpy(buf, buf1);
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'm':
	who_room = 1;
	strcpy(buf, buf1);
	break;
      case 'r':
        half_chop(buf1, arg, buf);
        showrace |= find_race_bitvector(arg);
        break;
      case 'c':
	half_chop(buf1, arg, buf);
	showclass |= find_class_bitvector(arg);
	break;
      case 'b':
        showclan = 1;
        half_chop(buf1, arg, buf);
        if (!*arg)
          clan = GET_CLAN(ch);
        else
          clan = parse_clan(arg);
        break;
      default:
	send_to_char(WHO_FORMAT, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(WHO_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */

  sprintf(buf,
"      %s_____    _____________\r\n"
"      %s|\\./|  ./'.%s/.v~__%s,/~_____   __________    _________________\r\n"
"      %s| | |./.%s/.(W%s---\\| /',---.`\\ |\\.,-----.`\\  |\\./,--------<~~\r\n"
"      %s| |./.%s/ /<M.    %s./././~\\`\\ \\| |./~~~\\`\\ \\ | | /~~~~~~~~~~\r\n"
"%s\\~b%s______.%s/__/ %s()%s\\_%s-----------%s\\ \\ \\%s--------%s\\ \\ \\| |/`-------/|%s---------------.\r\n"
"%s>@)%s$$$$$$$$%s($(   = ##>%s=========%s) ) )%s========%s) ) ) |-------<  |%s===------------->\r\n"
"%s/_p%s~~~~~~~%s\\~~\\ %s()%s/~%s-----------%s/ / /%s--------%s/ / /| |\\,======`\\|%s---------------'\r\n"
"      %s| |`\\`%s\\ \\<M`    %s`\\ \\`\\_/ / /| | \\___/ / / | | |\r\n"
"      %s| | |`\\`%s\\`(B%s---/| \\ `---' / | | |   \\`\\`\\ | | |\\________\r\n"
"      %s|/'\\|  `\\`%s\\`?_~~%s`\\_`~~~~~`  | | |    `\\`\\`\\/'\\`\\-------<__\r\n"
"      %s~~~~~    `~~~~~~~~~~        |/'\\|      )'\\,\\~~~~~~~~~~~~~~~~\r\n"
"                                  %s~~~~~      ~~~~~~%s^N\r\n",
     CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCPOMMEL(ch), CCHILT(ch), CCQUILLIONS(ch), CCJEWELS(ch), CCQUILLIONS(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch),
     CCPOMMEL(ch), CCHILT(ch), CCQUILLIONS(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch),
     CCPOMMEL(ch), CCHILT(ch), CCQUILLIONS(ch), CCJEWELS(ch), CCQUILLIONS(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch), CCRUNES(ch), CCBLADE(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch), CCQUILLIONS(ch), CCRUNES(ch),
     CCRUNES(ch),
     CCRUNES(ch), CCNRM(ch));

  send_to_char(buf, ch);

  for (d = descriptor_list; d; d = d->next) {
    switch (d->connected) {
        case CON_PLAYING:
        case CON_OEDIT:
        case CON_REDIT:
        case CON_ZEDIT:
        case CON_MEDIT:
        case CON_SEDIT:
        case CON_BATTLE_VRFY:
                    break;	/* go ahead and show them, they're normal */
        default:    continue;	/* people being asked for passwords etc, */
                    break;	/* dont show them on the who list */
    }

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (awaywho && !PRF2_FLAGGED(tch, PRF2_AWAY))
      continue;
    if (questwho && GET_QUEST(tch) == QUEST_OFF)
      continue;
    if (localwho && world[ch->in_room].zone != world[tch->in_room].zone)
      continue;
    if (who_room && (tch->in_room != ch->in_room))
      continue;
    if (showrace && !(showrace & (1 << GET_RACE(tch))))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
    if (showclan && (GET_CLAN(tch) != clan))
      continue;
    if (short_list) {
      if (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && GET_LEVEL(ch) < LVL_IMMORT) {
        sprintf(buf, "%s[-- --     -- ] %-12.12s%s%s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            GET_NAME(tch),
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCNRM(ch) : ""),
            ((!(++num_can_see % 2)) ? "\r\n" : ""));
      } else {
        sprintf(buf, "%s[%2d %s %s] %-12.12s%s%s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            GET_LEVEL(tch), RACE_ABBR(tch), CLASS_ABBR(tch), GET_NAME(tch),
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCNRM(ch) : ""),
            ((!(++num_can_see % 2)) ? "\r\n" : ""));
      }
      send_to_char(buf, ch);
    } else {
      num_can_see++;
      get_char_pretitle(tch, pretitle);
      if (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && GET_LEVEL(ch) < LVL_IMMORT) {
        sprintf(buf, "%s[-- --     -- ] %s%s %s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            pretitle,
            GET_NAME(tch),
            GET_TITLE(tch));
      } else {
        sprintf(buf, "%s[%2d %s %s] %s%s %s",
            (GET_LEVEL(tch) >= LVL_IMMORT ? CCGODS(ch) : ""),
            GET_LEVEL(tch), RACE_ABBR(tch), CLASS_ABBR(tch), pretitle,
            GET_NAME(tch),
            GET_TITLE(tch));
      }

      if (GET_INVIS_LEV(tch))
	sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(tch));
      else if (IS_AFFECTED(tch, AFF_INVISIBLE))
	sprintf(buf, "%s %s(invis)%s", buf, CCINFO(ch), CCNRM(ch));

      if (PLR_FLAGGED(tch, PLR_MAILING))
	sprintf(buf, "%s %s(mailing)%s", buf, CCALERT(ch), CCNRM(ch));
      else if (PLR_FLAGGED(tch, PLR_WRITING)) {
        if (STATE(tch->desc) == CON_PLAYING)
          sprintf(buf, "%s %s(writing)%s", buf, CCALERT(ch), CCNRM(ch));
        else
          sprintf(buf, "%s %s(editing)%s", buf, CCALERT(ch), CCNRM(ch));
      }

      if (PLR_FLAGGED(tch, PLR_MASTER))
        sprintf(buf, "%s %s(DQ Master)%s", buf, CCALERT(ch), CCNRM(ch));
/*
      if (PRF_FLAGGED(tch, PRF_DEAF))
	sprintf(buf, "%s %s(deaf)%s", buf, CCALERT(ch), CCNRM(ch));
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	sprintf(buf, "%s %s(notell)%s", buf, CCALERT(ch), CCNRM(ch));
      if (GET_QUEST(tch) > QUEST_OFF)
        switch (GET_QUEST(tch)) {
          case QUEST_OFF:         sprintf(buf, "%s %s(quest OFF)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_NORMAL:      sprintf(buf, "%s %s(quest)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_ENROLLED:    sprintf(buf, "%s %s(quest enrolled)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_SURVIVAL:    sprintf(buf, "%s %s(survival)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_PKQUEST:     sprintf(buf, "%s %s(pkquest)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          case QUEST_DEATHQUEST:  sprintf(buf, "%s %s(deathquest)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
          default:                sprintf(buf, "%s %s(quest UNKNOWN)%s",
                                      buf, CCQSAY(ch), CCNRM(ch)); break;
        }
*/
      if (PLR_FLAGGED(tch, PLR_THIEF))
	sprintf(buf, "%s %s(THIEF)%s", buf, CCWARNING(ch), CCNRM(ch));
      if (PLR_FLAGGED(tch, PLR_KILLER))
	sprintf(buf, "%s %s(KILLER)%s", buf, CCWARNING(ch), CCNRM(ch));
      if (PRF2_FLAGGED(tch, PRF2_AWAY))
        sprintf(buf, "%s %s(AFK)%s", buf, CCALERT(ch), CCNRM(ch));
      if ((GET_CLAN(tch) > CLAN_NOCLAN) && (GET_LEVEL(tch) < LVL_GOD)) {
        if (GET_CLAN_LEVEL(tch) > CLAN_LVL_MEMBER) {
          sprintf(buf, "%s %s(%s of %s)%s", buf, CCCLANSAY(ch),
              custom_clan_levels[(int) GET_CLAN(tch)][(int) GET_CLAN_LEVEL(tch)],
              clan_names[(int) GET_CLAN(tch)], CCNRM(ch)); 
        } else {
          sprintf(buf, "%s %s(%s)%s", buf, CCCLANSAY(ch),
              clan_names[(int) GET_CLAN(tch)], CCNRM(ch));
        }
      }
      if (GET_LEVEL(tch) >= LVL_IMMORT)
	strcat(buf, CCNRM(ch));
      strcat(buf, "^N\r\n");
      send_to_char(buf, ch);
    }				/* endif shortlist */
  }				/* end of for */
  if (short_list && (num_can_see % 2))
    send_to_char("\r\n", ch);

  if (num_can_see == 0)
    sprintf(buf, "\r\nNo-one prevents the geno!\r\n");
  else if (num_can_see == 1)
    sprintf(buf, "\r\nOne brave hero defends Kore.\r\n");
  else
    sprintf(buf, "\r\n%d stalwart heroes defend Kore.\r\n", num_can_see);

  send_to_char(buf, ch);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-r racelist] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
  extern char *connected_types[];
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, *format, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = LVL_IMPL, num_can_see = 0;
  int showrace = 0, showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      case 'r':
        playing = 1;
        half_chop(buf1, arg, buf);
        showrace |= find_race_bitvector(arg);
        break;
      case 'c':
	playing = 1;
	half_chop(buf1, arg, buf);
	showclass |= find_class_bitvector(arg);
	break;
      default:
	send_to_char(USERS_FORMAT, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */
/* HACKED to take out Class from the list (it was getting to long) */
  strcpy(line,
	 "Num Name         State          Idl Login@   Site\r\n");
  strcat(line,
	 "--- ------------ -------------- --- -------- ------------------------\r\n");
/* end of hack */
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && playing)
      continue;
    if (!d->connected && deadweight)
      continue;
    if (!d->connected) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showrace && !(showrace & (1 << GET_RACE(tch))))
        continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;

/* HACKED to not show this class related info */
      if (d->original)
	sprintf(classname, "[%2d %s %s]", GET_LEVEL(d->original),
		RACE_ABBR(d->original), CLASS_ABBR(d->original));
      else
	sprintf(classname, "[%2d %s %s]", GET_LEVEL(d->character),
		RACE_ABBR(d->character), CLASS_ABBR(d->character));
/* end of hack */
    } else {
/* HACKED to not show the class */
      strcpy(classname, "   -   -   ");
/* end of hack */
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (!d->connected && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[d->connected]);

    if (d->character && !d->connected && GET_LEVEL(d->character) < LVL_GOD)
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

/* HACKED to take out the class info - use who if you need that stuff */
    format = "%3d %-12s %-14s %-3s %-8s ";
/* end of hack */

/* HACKED in more places to not have classname */
    if (d->character && d->character->player.name) {
      if (d->original)
	sprintf(line, format, d->desc_num,
		d->original->player.name, state, idletime, timeptr);
      else
	sprintf(line, format, d->desc_num,
		d->character->player.name, state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "UNDEFINED",
	      state, idletime, timeptr);
/* end of hack */

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (d->connected) {
      sprintf(line2, "%s%s%s", CCINFO(ch), line, CCNRM(ch));
      strcpy(line, line2);
    }
    if (d->connected || (!d->connected && CAN_SEE(ch, d->character))) {
      send_to_char(line, ch);
      num_can_see++;
    }
  }

  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  extern char circlemud_version[];

  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(circlemud_version, ch);
    break;
  case SCMD_WHOAMI:
    send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
    break;
  default:
    return;
    break;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct descriptor_data *d;

  if (!*arg) {
    send_to_char("Players in your Zone\r\n--------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE) &&
	    (world[ch->in_room].zone == world[i->in_room].zone)) {
	  sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next)
      if (world[i->in_room].zone == world[ch->in_room].zone && CAN_SEE(ch, i) &&
	  (i->in_room != NOWHERE) && isname(arg, i->player.name) && !IS_NPC(i)) {
	sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	send_to_char(buf, ch);
	return;
      }
    send_to_char("No-one around by that name.\r\n", ch);
  }
}


void print_object_location(int num, struct obj_data * obj, 
			struct char_data * ch,
		        int recur)
{
  if (num > 0)
    sprintf(buf, "O%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(buf, "%33s", " - ");

  if (obj->in_room > NOWHERE) {
    sprintf(buf + strlen(buf), "[%5d] %s\n\r",
	    world[obj->in_room].number, world[obj->in_room].name);
    send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(buf + strlen(buf), "carried by %s\n\r",
	    PERS(obj->carried_by, ch));
    send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(buf + strlen(buf), "worn by %s\n\r",
	    PERS(obj->worn_by, ch));
    send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(buf + strlen(buf), "inside %s%s\n\r",
	    obj->in_obj->short_description, (recur ? ", which is" : " "));
    send_to_char(buf, ch);
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else {
    sprintf(buf + strlen(buf), "in an unknown location\n\r");
    send_to_char(buf, ch);
  }
}



void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char("Players\r\n-------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
	  if (d->original)
	    sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n",
		    GET_NAME(i), world[d->character->in_room].number,
		 world[d->character->in_room].name, GET_NAME(d->character));
	  else
	    sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i),
		    world[i->in_room].number, world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
		world[i->in_room].number, world[i->in_room].name);
	send_to_char(buf, ch);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char("Couldn't find any such thing.\r\n", ch);
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_SUPR)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  int i;

  if (IS_NPC(ch)) {
    send_to_char("You ain't nothin' but a hound-dog.\r\n", ch);
    return;
  }
  *buf = '\0';

  for (i = 1; i < LVL_IMMORT; i++) {
    sprintf(buf + strlen(buf), "[%2d] %8d-%-8d : ", i,
	titles[(int) GET_CLASS(ch)][i].exp,
	titles[(int) GET_CLASS(ch)][i + 1].exp);

    switch (GET_SEX(ch)) {
    case SEX_MALE:
    case SEX_NEUTRAL:
      strcat(buf, titles[(int) GET_CLASS(ch)][i].title_m);
      break;
    case SEX_FEMALE:
      strcat(buf, titles[(int) GET_CLASS(ch)][i].title_f);
      break;
    default:
      send_to_char("Oh dear.  You seem to be sexless.\r\n", ch);
      break;
    }
    strcat(buf, "\r\n");
  }
  page_string(ch->desc, buf, 1);
}



ACMD(do_consider)
{
  struct char_data *vict;
  int diff;

  one_argument(argument, buf);

  if (!(vict = get_char_room_vis(ch, buf)) &&
      !(vict = get_char_nearby_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }

  if ((ch->in_room != vict->in_room) && !IS_WARRIOR(ch)) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Consider yourself?\r\n", ch);
    return;
  }

  diff = (GET_LEVEL(vict) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char("Now where did that chicken go?\r\n", ch);
  else if (diff <= -5)
    send_to_char("You could do it with a needle!\r\n", ch);
  else if (diff <= -2)
    send_to_char("Easy.\r\n", ch);
  else if (diff <= -1)
    send_to_char("Fairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\r\n", ch);
  else if (diff <= 1)
    send_to_char("You would need some luck!\r\n", ch);
  else if (diff <= 2)
    send_to_char("You would need a lot of luck!\r\n", ch);
  else if (diff <= 3)
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 5)
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  else if (diff <= 10)
    send_to_char("Are you mad!?\r\n", ch);
  else if (diff <= 100)
    send_to_char("You ARE mad!\r\n", ch);

}



/* some functions for do_consider2 */
float consider_offense(struct char_data *ch) {
  float offense = 0.0;
  float avg_barehand = 0.0;
  float avg_weapon_dam = 0.0;
  int wear_wield = WEAR_WIELD;
  
  if (IS_THRIKREEN(ch)) wear_wield = THRI_WEAR_WIELD_R;

  /*
   * for NPCs, offense starts as barehand + 1/4th weapon_dam
   * for players, offense starts as weapon_dam
   */
  if (IS_NPC(ch)) {
    avg_barehand=(ch->mob_specials.damnodice*(ch->mob_specials.damsizedice+1))
                                        / 2;
    if (ch->equipment[wear_wield])
      avg_weapon_dam =  (GET_OBJ_VAL(ch->equipment[wear_wield], 0) *
                         (GET_OBJ_VAL(ch->equipment[wear_wield], 1) + 1))
                                          / 2;
    offense = avg_barehand + (avg_weapon_dam / 4);
  } else {                                         
    /* is a player */
    if (ch->equipment[wear_wield]) {
      avg_weapon_dam =  (GET_OBJ_VAL(ch->equipment[wear_wield], 0) *
                         (GET_OBJ_VAL(ch->equipment[wear_wield], 1) + 1))
                                            / 2;
      offense = avg_weapon_dam;
    } else {
      avg_barehand = 1.0;    /* avg barehand damage */
      offense = avg_barehand;
    } 
  }

  offense += GET_DAMROLL(ch);       /* get that bonus to dam */

  /* more attacks == more offense */
  offense *= (percent_number_of_attacks(ch) / 100);


  return offense;
}

float calculate_rating(struct char_data *ch) {
  float rating = 0.0;

  rating += GET_DAMROLL(ch) / 2;

  if (!IS_NPC(ch) && (GET_LEVEL(ch) > LVL_GOD))
    rating += 50;
  
  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    rating *= 2;
  if (IS_AFFECTED(ch, AFF_HASTE))
    rating *= 1.5;
  if (IS_AFFECTED2(ch, AFF2_ICE_SHIELD))
    rating *= 1.25;
 
  return rating;
}
 
float consider_defense(struct char_data *ch) {
  float defense = 0.0;

  defense = GET_HIT(ch);

  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    defense *= 2;
  if (IS_AFFECTED2(ch, AFF2_ICE_SHIELD))
    defense *= 1.25;


  return defense;
}



const struct power_type {
    float level;
    char *rating;
} power[] = {
	{	1024.0000,	"Simple.\r\n" },
	{	512.0000,	"Extremely easy.\r\n" },
	{	256.0000,	"Really easy.\r\n" },
	{	128.0000,	"Easy.\r\n" },
	{	64.0000,	"Pretty easy.\r\n" },
	{	32.0000,	"Reasonably easy.\r\n" },
	{	16.0000,	"Sort of easy.\r\n" },
	{	8.0000,		"Fairly easy.\r\n" },
	{	4.0000,		"Somewhat easy.\r\n" },
	{	2.0000,		"Even.\r\n" },
	{	1.0000,		"Somewhat hard.\r\n" },
	{	0.5000,		"Fairly hard.\r\n" },
	{	0.2500,		"Sort of hard.\r\n" },
	{	0.1250,		"Reasonably hard.\r\n" },
	{	0.0625,		"Pretty hard.\r\n" },
	{	0.0313,		"Hard.\r\n" },
	{	0.0156,		"Really hard.\r\n" },
	{	0.0078,		"Extremely hard.\r\n" },
	{	0.0039,		"Getting impossible.\r\n" },
	{	0.0020,		"Impossible.\r\n" },
	{	0.0010,		"Unthinkable.\r\n" },
        {       0.0000,         "Unthinkable.\r\n" },
        {	-0.0001,	"Unthinkable.\r\n" },
	{	-1.00,		"Unthinkable.\r\n" },
	{	-2.00,		"ERROR #1 in do_consider2\r\n" } 
};

ACMD(do_consider2)
{
  struct char_data *vict;
  float ch_offense;
  float ch_defense;
  float vict_offense;
  float vict_defense;
  float ch_power;
  float vict_power;
  float power_guess;
  int i;


  one_argument(argument, buf);

  if (!(vict = get_char_room_vis(ch, buf)) &&
      !(vict = get_char_nearby_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }

  if ((ch->in_room != vict->in_room) && !IS_WARRIOR(ch)) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Consider yourself?\r\n", ch);
    return;
  }

  /* set up variables about ch */
  ch_offense = consider_offense(ch);
  ch_defense = consider_defense(ch);
  ch_power = ch_offense * ch_defense;

  /* set up variables about vict */ 
  vict_offense = consider_offense(vict);
  vict_defense = consider_defense(vict);
  vict_power = MAX(vict_offense * vict_defense, 1);

  /*
   * power_guess is a guess based on relative rounds to kill vict
   * vs how long it would take vict to kill ch..!
   * (bigger numbers are better for ch)
   *
   * [infinity] = ch rules vict
   * 3 = ch is three times as good as vict
   * 2 = ch is twice as good as vict
   * 1 = even
   * 1/2 = vict is twice as good as ch
   * 1/3 = vict is three times as good as ch
   * 0 = vict rules ch
   */
  power_guess = ch_power / vict_power;

  /* let ch know.. */
/*
  sprintf(buf, "You estimate your chances...\r\n"
               "Your power: %f\r\n"
               "Their power: %f\r\n"
               "Odds: %f to 1.0000\n",
      ch_power, vict_power, power_guess);
  send_to_char(buf, ch);
*/

  /* find the right rating string */
  for (i = 0; power[i].level > -1.00; i++)
    if (power_guess >= power[i].level)
      break;

  send_to_char(power[i].rating, ch);
}



ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    } else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}



/* HACKED to add in the auto stuff */
ACMD(do_toggle)
{
  if (IS_NPC(ch))
    return;
  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");
  else
    sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

  sprintf(buf,
	  "Hit Pnt Display: %-3s    "
	  "     Brief Mode: %-3s    "
	  " Summon Protect: %-3s\r\n"

	  "   Move Display: %-3s    "
	  "   Compact Mode: %-3s    "
          "      Anonymous: %-3s\r\n"

	  "   Mana Display: %-3s    "
	  "         NoTell: %-3s    "
	  "   Repeat Comm.: %-3s\r\n"

          "  Diagn Display: %-3s    "
          "   Gold Display: %-3s    "
          "  Exper Display: %-3s\r\n"

          "Min/Max Display: %-3s    "
          "      Away Flag: %-3s\r\n"

	  " Auto Show Exit: %-3s    "
          "      Auto Loot: %-3s    "
          " Auto Sacrifice: %-3s\r\n"

          "      Auto Gold: %-3s    "
          "     Auto Split: %-3s    "
          "Auto Directions: %-3s\r\n"

          "    Auto Assist: %-3s    "
/*
          "      Auto Scan: %-3s    "
*/
          "   Battle Brief: %-3s\r\n"

	  "           Deaf: %-3s    "
	  "     Wimp Level: %-3s\r\n"

	  " Gossip Channel: %-3s    "
	  "Auction Channel: %-3s    "
	  "  Grats Channel: %-3s\r\n"

          "   Clan Channel: %-3s    "
          "  Music Channel: %-3s\r\n"

	  "          Color: %-3s    "
          "       Auto Map: %-3s\r\n",

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
          ONOFF(PRF2_FLAGGED(ch, PRF2_ANONYMOUS)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
	  ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

          ONOFF(PRF_FLAGGED(ch, PRF_DISPDIAG)),
          ONOFF(PRF_FLAGGED(ch, PRF_DISPGOLD)),
          ONOFF(PRF_FLAGGED(ch, PRF_DISPEXP)),

          ONOFF(PRF_FLAGGED(ch, PRF_DISPMINMAX)),
          YESNO(PRF2_FLAGGED(ch, PRF2_AWAY)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTODIRS)),

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
/*
          ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOSCAN)),
*/
          ONOFF(PRF2_FLAGGED(ch, PRF2_BATTLEBRIEF)),

	  YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

          ONOFF(!PRF2_FLAGGED(ch, PRF2_NOCLAN)),
          ONOFF(!PRF2_FLAGGED(ch, PRF2_NOMUSIC)),

          ONOFF(PRF_FLAGGED(ch, PRF_COLOR)),
          ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOMAP))

          );

  send_to_char(buf, ch);
}
/* end of auto stuff hack */


struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
  int a, b, tmp;

  ACMD(do_action);

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  /* create data array */
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

  /* initialize it */
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }

  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}



ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizcommands = 0, socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char("You can't see the commands of people above your level.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZCOMMANDS)
    wizcommands = 1;

  sprintf(buf, "The following %s%s are available to %s:\r\n",
	  wizcommands ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
/* HACKED to give the level the command is available if wizcommands
  is asked for */
  for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
    i = cmd_sort_info[cmd_num].sort_pos;
    if (cmd_info[i].minimum_level >= LVL_DO_NOT_DISPLAY &&
	GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
	(cmd_info[i].minimum_level >= LVL_IMMORT) == wizcommands &&
	(wizcommands || socials == cmd_sort_info[i].is_social)) {
      if (wizcommands) {
        sprintf(buf + strlen(buf), "%-12s %d    ", cmd_info[i].command,
                cmd_info[i].minimum_level);
        if (!(no % 4))
          strcat(buf, "\r\n");
        no++;
      } else {
        if (cmd_info[i].minimum_level == LVL_DO_NOT_DISPLAY) {
          if (GET_LEVEL(ch) >= LVL_IMMORT) {
            sprintf(buf + strlen(buf), "%s%-12s%s",
		CCALERT(ch), cmd_info[i].command, CCNRM(ch));
            if (!(no % 6))
              strcat(buf, "\r\n");
            no++;
          }
        } else {
          sprintf(buf + strlen(buf), "%-12s", cmd_info[i].command);
          if (!(no % 6))
            strcat(buf, "\r\n");
          no++;
        }
      }
    }
  }
/* end of hack */

  strcat(buf, "\r\n");
/* HACKED to page_string to the character */
/*
  send_to_char(buf, ch);
*/
  page_string(ch->desc, buf, 1);
/* end of hack */
}


ACMD(do_socials)
{
  int no, cmd_num;
  extern struct social_info soc_info[];
  extern int list_top;
  sprintf(buf, "The following socials are available to you:\r\n");
  for (no = 1, cmd_num = 0; cmd_num <= list_top; cmd_num++) { 
    if (soc_info[cmd_num].minimum_level >= 0 &&
        GET_LEVEL(ch) >= soc_info[cmd_num].minimum_level) { 
      sprintf(buf + strlen(buf), "%-11s", soc_info[cmd_num].command);
      if (!(no % 7))
        strcat(buf, "\r\n");
      no++;
    }
  }
  
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}




ACMD(do_attribute)
{
  struct time_info_data age(struct char_data * ch);

  sprintf(buf, "Name: %s\r\n", GET_NAME(ch));
  send_to_char(buf, ch);
  if (!IS_NPC(ch)) {
    sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
            GET_NAME(ch), age(ch).year, age(ch).month,
            age(ch).day, age(ch).hours);
    send_to_char(buf, ch);
  }
/*
  sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
          GET_HEIGHT(ch), GET_WEIGHT(ch));
*/
  sprintf(buf, "Level: %d, Hits: %d, Mana: %d\r\n",
          GET_LEVEL(ch), GET_HIT(ch), GET_MANA(ch));
  if (GET_LEVEL(ch) >= LVL_LOWBIE /*|| ((int) GET_CLASS(ch)) > CLASS_BARD*/) {
    sprintf(buf, "%sAC: %d, Hitroll: %d, Damroll: %d\r\n", buf,
            GET_AC(ch), GET_HITROLL(ch), GET_DAMROLL(ch));
    sprintf(buf,
            "%sStr: %d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
/*
            buf, GET_STR(ch), GET_ADD(ch), GET_INT(ch),
*/
            buf, GET_STR(ch), GET_INT(ch),
        GET_WIS(ch), GET_DEX(ch), GET_CON(ch), GET_CHA(ch));
  } else {
    sprintf(buf, "%sAC: %d, Hitroll: ?, Damroll: ?\r\n", buf, GET_AC(ch));
    sprintf(buf,
            "%sStr: ?/?, Int: ?, Wis: ?, Dex: ?, Con: ?, Cha: ?\r\n", buf);
    sprintf(buf, "%s(Your stats are hidden until level %d.)\r\n",
            buf, LVL_LOWBIE);
  }
  send_to_char(buf, ch);
}



ACMD(do_compare)
{
  static char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj1;
  struct obj_data *obj2;
  int value1, value2;
  int i, num_wears;
  bool found = FALSE;

  half_chop(argument, arg, arg2);

  num_wears = IS_THRIKREEN(ch)? NUM_THRI_WEARS : NUM_WEARS;

  if (!*arg) {
    send_to_char("Compare what to what?\r\n", ch);
    return;
  }

  if (!(obj1 = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    if (obj1 == NULL) {
      send_to_char("You do not have that item.\r\n", ch);
      return;
    }
  }

  if (!*arg2) {
    for (i = 0; i < num_wears; i++) {
      if (ch->equipment[i]) {
        if (CAN_SEE_OBJ(ch, ch->equipment[i])) {
          if (GET_OBJ_WEAR(obj1) & GET_OBJ_WEAR(ch->equipment[i])) {
            obj2 = ch->equipment[i];
            found = TRUE;
          }
        } 
      }
    }
    if (!found) {
      send_to_char("You don't have anything comparable.\r\n", ch);
      return;
    }
  } else if (!(obj2 = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
    if (obj2 == NULL) {
      send_to_char("You do not have that item.\r\n", ch);
      return;
    }
  }

  *buf = '\0';
  value1 = 0;
  value2 = 0;

  if (obj1 == obj2) {
    sprintf(buf, "You compare $p to itself.  It looks about the same.");
  } else if (GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2)) {
    sprintf(buf, "You can't compare $p and $P.");
  } else {
    switch (GET_OBJ_TYPE(obj1)) {
      case ITEM_ARMOR:
        value1 = GET_OBJ_VAL(obj1, 0);
        value2 = GET_OBJ_VAL(obj2, 0);
        break;
      case ITEM_WEAPON:
        value1 = 100 *
            (((GET_OBJ_VAL(obj1, 2) + 1) / 2.0) * GET_OBJ_VAL(obj1, 1));
        value2 = 100 *
            (((GET_OBJ_VAL(obj2, 2) + 1) / 2.0) * GET_OBJ_VAL(obj2, 1));
        break;
      default:
        sprintf(buf, "You can't compare $p and $P.");
        break;
    }
  }

  if (!*buf) {
    if (value1 == value2)
      sprintf(buf, "$p and $P look about the same.");
    else if (value1 > value2)
      sprintf(buf, "$p looks better than $P.");
    else
      sprintf(buf, "$p looks worse than $P.");
  }

  act(buf, FALSE, ch, obj1, obj2, TO_CHAR);
  return;
}



ACMD(do_areas)
{
  command_interpreter(ch, "help areas");
}



ACMD(do_auto)
{
  command_interpreter(ch, "help auto");
}


ACMD(do_dm_scores) {
  extern struct dm_score_data *dm_scores;
  struct dm_score_data *cur;
  extern int arena_deathmatch_mode;
  extern int arena_deathmatch_level;
  
  *buf = '\0';
  cur = dm_scores;
  
  if (!dm_scores) {
    send_to_char("Sorry, there aren't any deathmatch scores to view.\r\n", ch);
    return;
  }
  
  if (arena_deathmatch_level < 50)
    sprintf(buf, "Level %d ", arena_deathmatch_level);
  else
    *buf = '\0';
  if (arena_deathmatch_mode) {
    sprintf(buf, "%sDeathmatch results so far:\r\n\r\n", buf);
  } else {
    sprintf(buf, "%sFinal deathmatch results:\r\n\r\n", buf);
  }
  sprintf(buf, "%s%-35s%-10s%-10s%-10s\r\n", buf, "Name", "Kills", "Deaths", "Score");
  sprintf(buf, "%s%-35s%-10s%-10s%-10s\r\n", buf, "----", "-----", "------", "-----");
  
  while (cur != NULL) {
    sprintf(buf, "%s%-35s%-10d%-10d%-10d\r\n", buf,
          cur->plrname, cur->kills, cur->deaths, cur->killscore);
    cur = cur->next;
  }
          
  page_string(ch->desc, buf, 1);
}

ACMD(do_changes)
{
    do_help(ch, "changes", 0, 0);
}



/*
 * we take ch because we need to know if they like their output in ansi ..
 * we take room because we want to know the sector type to draw the terrain
 * and we take x, y, because we want to go ahead and display a value if
 * we're finally at 0,0
 * if we're not there yet, we want to point ourselves in the right direction
 * or if we cant, just assume the squares empty and return emptiness
 * this function will write something out, but wont go to a next line
 * (sending "\r\n") - that has to happen in do_map()
 */
void perform_map_square(struct char_data *ch, room_num start_room, int room_count, int draw_self_at_start, int row, int col, bool graphical)
{
  room_num function_room, was_in;


  /* save what room they started off in */
  function_room = ch->in_room;


  if ((row == 0) && (col == 0)) {
    /* cool, draw the sector type */
    if (graphical) {
      /* pretty picture of the terrain type */
      switch (world[ch->in_room].sector_type) {
        case SECT_INSIDE:       sprintf(buf, "^wI^n");      break;
        case SECT_CITY:         sprintf(buf, "^wC^n");      break;
        case SECT_FIELD:        sprintf(buf, "^yp^n");      break;
        case SECT_FOREST:       sprintf(buf, "^gf^n");      break;
        case SECT_HILLS:        sprintf(buf, "^rh^n");      break;
        case SECT_MOUNTAIN:     sprintf(buf, "^W^^^N");      break;
        case SECT_WATER_SWIM:   sprintf(buf, "^cw^n");      break;
        case SECT_WATER_NOSWIM: sprintf(buf, "^bw^n");      break;
        case SECT_UNDERWATER:   sprintf(buf, "^bu^n");      break;
        case SECT_FLYING:       sprintf(buf, "^w.^n");      break;
        case SECT_DESERT:       sprintf(buf, "^Yd^N");      break;
        case SECT_ICELAND:      sprintf(buf, "^Wi^N");      break;
        case SECT_OCEAN:        sprintf(buf, "^bo^n");      break;
        case SECT_LADDER:       sprintf(buf, "^yl^n");      break;
        case SECT_TREE:         sprintf(buf, "^rt^n");      break;
        case SECT_ASTRAL:       sprintf(buf, "^Wa^n");      break;
        case SECT_SWAMP:        sprintf(buf, "^gs^n");      break;
        default:                sprintf(buf, "^w?^n");	/* bad unknown sector */
      }
    } else {
      /* no graphics, just print the room number */
      sprintf(buf, "%6d", world[(ch)->in_room].number);
    }

    /* is there a road here? */
       /* use: "-" "|" "/" "\" and "+" to mark it.. */
 
    /* cliff edge? */
       /* use: "-" "|" "/" "\" and "+" to mark it but in a different color */
 
    /* a special portal to enter? */
       /* some other special object here? ships maybe? */
       /* maybe use a number, maybe use something else.. :) */

    /* is this where the player is? */
       /* is the current room == start_room? */
       /* use a red @ sign for them */
#if 0
    if ((function_room == start_room) && (room_count == draw_self_at_start)) {
#else
    if (room_count == draw_self_at_start) {
#endif
      if (graphical)
        sprintf(buf, "^R@^n");
      else
        sprintf(buf, " ME...");
    }

    send_to_char(buf, ch);

  } else {

    /* blech, recursion */

    if ((row < 0) && (col < 0)) {
      /* northwest? */
      if (CAN_GO(ch, NORTHWEST)) {
        /* then go the easy way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[NORTHWEST]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row+1, col+1, graphical);
      } else if (CAN_GO(ch, NORTH)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[NORTH]->to_room);
        if (CAN_GO(ch, WEST)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[WEST]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row+1, col+1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" nw...", ch);
        }
      } else if (CAN_GO(ch, WEST)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[WEST]->to_room);
        if (CAN_GO(ch, NORTH)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[NORTH]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row+1, col+1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" nw...", ch);
        }
      } else {
        /* just cant go northwest, return a blank for that square up there */
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" nw...", ch);
      }

    } else if ((row < 0) && (col == 0)) {
      /* straight north? */
      if (CAN_GO(ch, NORTH)) {
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[NORTH]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row+1, col, graphical);
      } else {
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" n....", ch);
      }

    } else if ((row < 0) && (col > 0)) {
      /* northeast? */
      if (CAN_GO(ch, NORTHEAST)) {
        /* then go the easy way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[NORTHEAST]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row+1, col-1, graphical);
      } else if (CAN_GO(ch, NORTH)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[NORTH]->to_room);
        if (CAN_GO(ch, EAST)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[EAST]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row+1, col-1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" ne...", ch);
        }
      } else if (CAN_GO(ch, EAST)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[EAST]->to_room);
        if (CAN_GO(ch, NORTH)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[NORTH]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row+1, col-1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" ne...", ch);
        }
      } else {
        /* just cant go northeast, return a blank for that square up there */
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" ne...", ch);
      }

    } else if ((row == 0) && (col > 0)) {
      /* straight east? */
      if (CAN_GO(ch, EAST)) {
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[EAST]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row, col-1, graphical);
      } else {
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" e....", ch);
      }


    } else if ((row > 0) && (col > 0)) {
      /* southeast? */
      if (CAN_GO(ch, SOUTHEAST)) {
        /* then go the easy way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[SOUTHEAST]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row-1, col-1, graphical);          
      } else if (CAN_GO(ch, SOUTH)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[SOUTH]->to_room);
        if (CAN_GO(ch, EAST)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[EAST]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row-1, col-1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" se...", ch);
        }
      } else if (CAN_GO(ch, EAST)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[EAST]->to_room);
        if (CAN_GO(ch, SOUTH)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[SOUTH]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row-1, col-1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" se...", ch);
        }
      } else {
        /* just cant go southeast, return a blank for that square up there */
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" se...", ch);
      }

    } else if ((row > 0) && (col == 0)) {
      /* straight south? */
      if (CAN_GO(ch, SOUTH)) {
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[SOUTH]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row-1, col, graphical);
      } else {
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" s....", ch);
      }

    } else if ((row > 0) && (col < 0)) {
      /* southwest? */
      if (CAN_GO(ch, SOUTHWEST)) {
        /* then go the easy way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[SOUTHWEST]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row-1, col+1, graphical);          
      } else if (CAN_GO(ch, SOUTH)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[SOUTH]->to_room);
        if (CAN_GO(ch, WEST)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[WEST]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row-1, col+1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" sw...", ch);
        }
      } else if (CAN_GO(ch, WEST)) {
        /* else try going the hard way */
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[WEST]->to_room);
        if (CAN_GO(ch, SOUTH)) {
          was_in = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, world[was_in].dir_option[SOUTH]->to_room);
          /* recursion */
          perform_map_square(ch, start_room, room_count, draw_self_at_start, row-1, col+1, graphical);
        } else {
          if (graphical)
            send_to_char(" ", ch);
          else
            send_to_char(" sw...", ch);
        }
      } else {
        /* just cant go southwest, return a blank for that square up there */
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" sw...", ch);
      }

    } else /* ((row == 0) && (col < 0)) */ {
      /* _must_ be straight west, only direction left! */
      if (CAN_GO(ch, WEST)) {
        was_in = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, world[was_in].dir_option[WEST]->to_room);
        /* recursion */
        perform_map_square(ch, start_room, room_count, draw_self_at_start, row, col+1, graphical);
      } else {
        if (graphical)
          send_to_char(" ", ch);
        else
          send_to_char(" w....", ch);
      }

    }
  }

  /* set things back before leaving here... */
  char_from_room(ch);
  char_to_room(ch, function_room);

}



#define DEFAULT_MAP_RANGE 6
ACMD(do_map)
{
  int row;			/* these start at negative numbers, and turn  */
  int col;			/* positive.  On the same row or column as the*/
  int rowbegin, rowend;
  int colbegin, colend;
  int range;			/* range is the distance to look */ 
  room_num start_room;		/* save this! */
  bool graphical = TRUE;	/* want graphics? */
  int room_count;
  int draw_self_at_start;



  start_room = ch->in_room;


  one_argument(argument, arg);
  if (*arg) {
    range = atoi(arg);
    if ((range == 0) || (range > 100))
      range = DEFAULT_MAP_RANGE;
  } else {
    range = DEFAULT_MAP_RANGE;
  }


  /* the essentials of how to look: 
   * if the row is negative, then its northwards
   * if the row is positive, its southwards
   * if the col is negative, its westwards 
   * if the col is positive, its eastwards
   *
   * now the trick is, if you can take a shortcut nw, ne, sw, se, then do
   * but if you cant, then first get on the right row, then scoot over
   * to the right column
   *
   * once you've got it, then display it
   */

  rowbegin = -1 * (range / 2);
  rowend = (range / 2);
  colbegin = -1 * (range / 2);
  colend = (range / 2);

  room_count = 0;
  draw_self_at_start = (2 * ((range / 2) * (range / 2))) + (2 * (range / 2));

  for (row = rowbegin; row <= rowend; row++) {
    for (col = colbegin; col <= colend; col++) {
      perform_map_square(ch, start_room, room_count, draw_self_at_start, row, col, graphical); 
      room_count++;
    }
    send_to_char("\r\n", ch);
  } 

  char_from_room(ch);
  char_to_room(ch, start_room);
}
