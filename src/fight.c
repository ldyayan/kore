
/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "boards.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External variables */
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern sh_int r_lowbie_start_room;
extern sh_int r_race_start_room[NUM_RACES];
extern sh_int r_clan_start_room[NUM_CLANS];
extern int pk_allowed;		/* see config.c */
extern int auto_save;		/* see config.c */
extern int max_exp_gain;	/* see config.c */
extern char *color_codes[];
extern struct spell_info_type spell_info[];
extern struct zone_data *zone_table;
extern struct index_data *mob_index;
extern struct title_type titles[NUM_CLASSES][LVL_IMPL + 1];
extern const int race_stat_adjust[NUM_RACES][MAX_RACE_STAT_ADJUST];
int number_range(int from, int to);
extern char *spells[];


/* External procedures */
char *fread_action(FILE * fl, int nr);
char *fread_string(FILE * fl, char *error);
void stop_follower(struct char_data * ch);
ACMD(do_flee);
ACMD(do_save);
void hit(struct char_data * ch, struct char_data * victim, int type);
void forget(struct char_data * ch, struct char_data * victim);
void remember(struct char_data * ch, struct char_data * victim);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
void mprog_kill_trigger(struct char_data * mob, struct char_data * victim);
void mprog_death_trigger(struct char_data * mob, struct char_data * killer);
void mprog_hitprcnt_trigger(struct char_data * mob, struct char_data * ch);
void mprog_fight_trigger(struct char_data * mob, struct char_data * ch);
void hunt_victim(struct char_data * ch);
void clanlog(char *str, struct char_data * ch);
void stop_script(struct char_data * ch);
int corpse_cost(struct char_data * ch);
float calculate_rating(struct char_data *ch);
ACMD(do_get);
ACMD(do_split);
ACMD(do_sacrifice);
void print_group(struct char_data *ch);

/* The deathquest scores! */
struct dm_score_data *dm_scores = NULL;
void log_deathmatch_kill( struct char_data *ch, struct char_data *vict);
int arena_base = 1300;
extern int arena_deathmatch_mode, arena_deathmatch_level;

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},				/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},			/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},			/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"},
  {"chop", "chops"},				/* 15 */
  {"breathe lightning", "breathes lightning"},
  {"breathe frost", "breathes frost"},
  {"breathe acid", "breathes acid"},
  {"breathe fire", "breathes fire"},
  {"breathe gas", "breathes gas"}		/* 20 */
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))


/* The Fight related routines */

void appear(struct char_data * ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
}



void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    sprintf(buf2, "Error reading combat message file %s", MESS_FILE);
    perror(buf2);
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      fprintf(stderr, "Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}



void update_pos(struct char_data * victim)
{
  ACMD(do_stand);


/* HACKED to try to make mobs stand */
  if (IS_NPC(victim) && FIGHTING(victim) &&
      (GET_POS(victim) == POS_SITTING) && (number(0,2) == 2)) {
    do_stand(victim, 0, 0, 0);
    GET_POS(victim) = POS_FIGHTING;
  }
/*
  if (IS_NPC(victim) && FIGHTING(victim) &&
      (GET_POS(victim) == POS_RESTING)) {
    do_stand(victim, 0, 0, 0);
  }
*/
/* end of hack */

  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
/* PETS */
  if (HAS_PET(victim)) {
    if (IS_MOUNTED(GET_PET(victim))) {
      if (GET_POS(victim) != POS_STANDING) { /* You stand on your pet...silly,
                                                I know */
        act("$n falls off $N!", TRUE, victim, NULL, GET_PET(victim), TO_ROOM);
        act("You fall off $N!", TRUE, victim, NULL, GET_PET(victim), TO_CHAR);
        IS_MOUNTED(GET_PET(victim)) = FALSE;
      }
    }
  }
}



void check_killer(struct char_data * ch, struct char_data * vict)
{
  if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF)
      && !PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(ch) && !IS_NPC(vict) &&
      !IS_CHAOS_ROOM(ch->in_room) && (ch != vict)) {
    char buf[256];

    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
    sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
    GET_KILLER_TO_FORGIVE(vict) = GET_IDNUM(ch);
  }
}



/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;

  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) ||
      ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL))
    return;

  if (FIGHTING(ch)) {
    log("ERROR: set_fighting called, ch fighting!");
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_AFFECTED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;
  
  if (HAS_PET(ch)) if (IS_MOUNTED(GET_PET(ch))) IS_MOUNTED(GET_PET(ch)) = FALSE;

  /* be sure their attacks arent blocked by clearing that out */
  GET_BLOCKED(ch) = 0;

  if (!pk_allowed)
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *tmp;

  /* HACKED to make it (hopefully) recover from this */
  if (!FIGHTING(ch)) {
    log("ERROR: stop_fighting called, ch not fighting!");
    return;
  }

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  if (combat_list == ch)
    combat_list = ch->next_fighting;
  else {
    for (tmp = combat_list; tmp && (tmp->next_fighting != ch);
	 tmp = tmp->next_fighting);
    if (!tmp) {
      log("SYSERR: Char fighting not found Error (fight.c, stop_fighting)");
      abort();
    }
    tmp->next_fighting = ch->next_fighting;
  }

  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  GET_CANT_WIMPY(ch) = 0;
  update_pos(ch);
}



void make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o, *nextobj;
  struct obj_data *money;
  int i;
  extern int max_npc_corpse_time, max_pc_corpse_time;
  extern sh_int r_mortal_start_room;
  extern sh_int r_lowbie_start_room;

  struct obj_data *create_money(int amount);

  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;
  corpse->name = str_dup("corpse");

  sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = str_dup(buf2);

  sprintf(buf2, "the corpse of %s", GET_NAME(ch));
  corpse->short_description = str_dup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */

  if (!IS_NPC(ch)) {
    GET_OBJ_VAL(corpse, 2) = -2;/* -2 means player corpse */
    GET_OBJ_WEAR(corpse) = 0;
  }

  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch); 
  GET_OBJ_COST(corpse) = corpse_cost(ch);
  GET_OBJ_RENT(corpse) = GET_MOB_VNUM(ch);
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

  corpse->contains = ch->carrying;

  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  for (i = 0; i < NUM_WEARS; i++)
    if (ch->equipment[i])
	    obj_to_obj(unequip_char(ch, i), corpse);

  GET_OBJ_WEIGHT(corpse) += IS_CARRYING_W(ch);
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;
  
  for  (o = corpse->contains; o != NULL; o = nextobj) {
    nextobj = o->next_content;
    if (IS_OBJ_STAT(o, ITEM_QUEST)) {
      act("$p disintegrates and fades away!", FALSE, ch, o, 0, TO_ROOM);
      obj_from_obj(o);
      extract_obj(o);
    } else {
      if (IS_OBJ_STAT(o, ITEM_AUTOQUEST)) {
	obj_from_obj(o);
	obj_to_char(o, ch);
      }
    }
  }

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole */
    /*
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
    */
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    /*
    }
    */
    GET_GOLD(ch) = 0;
  }

  if (IS_NPC(ch))
    obj_to_room(corpse, ch->in_room);
  else if (GET_LEVEL(ch) <= LVL_LOWBIE)
    obj_to_room(corpse, r_lowbie_start_room);
  else if (IS_CHAOS_ROOM(ch->in_room))
    obj_to_room(corpse, r_mortal_start_room);
  else
    obj_to_room(corpse, ch->in_room);
}



/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  if (!IS_NPC(ch) && !IS_NPC(victim) && IS_CHAOS_ROOM(ch->in_room)) return;
  
  /* new way is more level based */
/* put undead on same scales as others 
  if (GET_RACE(ch) == RACE_UNDEAD)
    GET_ALIGNMENT(ch) += (((-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) >> 4) *
                        GET_LEVEL(victim) / GET_LEVEL(ch)) / 5; 
*/
    GET_ALIGNMENT(ch) += ((-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) >> 8) *
	GET_LEVEL(victim) / GET_LEVEL(ch);

    if ((GET_RACE(ch) == RACE_DROW) && (GET_ALIGNMENT(ch) > -300))
	GET_ALIGNMENT(ch) = -300;
    else if ((GET_RACE(ch) == RACE_DUERGAR) && (GET_ALIGNMENT(ch) > -300))
	GET_ALIGNMENT(ch) = -300;
    else if ((GET_RACE(ch) == RACE_TROLL) && (GET_ALIGNMENT(ch) > -300))
	GET_ALIGNMENT(ch) = -300;
    else if ((GET_RACE(ch) == RACE_ORC) && (GET_ALIGNMENT(ch) > -300))
	GET_ALIGNMENT(ch) = -300;
}

/* old way */
/* I moved this to the bottom to avoid confusion -Brian */
  /*
   * new change alignment algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
/*
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) >> 4;
*/



/* HACKED to not send death cries through exits that lead back to the 
  original room */
void death_cry(struct char_data * ch)
{
  int door, was_in;

/* this is new btw */
   if (ch->in_room == NOWHERE) {
        sprintf(buf, "SYSERR: %s trying to deathcry in NOWHERE.",
            GET_NAME(ch));
        log(buf);
	return;
   }

  act("Your blood freezes as you hear $n's death cry.",
      FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;
  for (door = 0; door < NUM_OF_DIRS - 1; door++) {
    if (CAN_GO(ch, door) &&
        (was_in != world[was_in].dir_option[door]->to_room)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("Your blood freezes as you hear someone's death cry.",
          FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}



void raw_kill(struct char_data * ch, struct char_data * killer)
{
  extern sh_int r_mortal_start_room;
  
  if (FIGHTING(ch))
    stop_fighting(ch);

  GET_CANT_WIMPY(killer) = 0;

  /* MOBProg foo */
  if (ch->mpscriptnum > 0)
    stop_script(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  death_cry(ch);

  if (ch->in_room == NOWHERE) {
    sprintf(buf, "SYSERR: %s is NOWHERE in raw_kill() fight.c", GET_NAME(ch));
    log(buf);
  } else {
    make_corpse(ch);

    /* PETS */
    if (IS_PET(ch)) {
	if (GET_OWNER(ch) != NULL) {
	    if (GET_OWNER(ch)->in_room != ch->in_room) {
		sprintf(buf, "You hear %s's death cry in the distance and feel a deep sorrow.\r\n",
		GET_NAME(ch));
		send_to_char(buf, GET_OWNER(ch));
	    }
	}
	char_from_room(ch);
	char_to_room(ch, r_mortal_start_room);
	act("$n staggers in, bruised and bleeding.", TRUE, ch, NULL, NULL, TO_ROOM);
	GET_HIT(ch) = MAX(GET_HIT(ch), 1);
	GET_MOVE(ch) = MAX(GET_MOVE(ch), 1);
	GET_MANA(ch) = MAX(GET_MANA(ch), 1);
	GET_POS(ch) = POS_STANDING;
    } else if (!IS_NPC(ch)) {

    char_from_room(ch);

    if (GET_LEVEL(ch) <= LVL_LOWBIE)
	char_to_room(ch, r_lowbie_start_room);
    else
	char_to_room(ch, r_race_start_room[(int) GET_RACE(ch)]);

    act("$n appears in an swirling mist of colors.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    send_to_char("You have been killed!\r\n", ch);
    gain_exp(ch, 1); /* Trigger any delevel effects */
/*
    do_save(ch, "", 0, SCMD_QUIET_SAVE);
*/

    
    GET_HIT(ch) = 1;
    GET_MANA(ch) = 1;
    
    while (ch->affected) affect_remove(ch, ch->affected);

    /* Leave them grouped if possible */
    if (IS_AFFECTED(ch, AFF_GROUP))
	AFF_FLAGS(ch) = AFF_GROUP;
    else
	AFF_FLAGS(ch) = 0;
    affect_total(ch); 

    SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][6]);
    SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][7]);
    SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][8]);
    SET_BIT(AFF_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][9]);
   
    SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][10]);
    SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][11]);
    SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][12]);
    SET_BIT(AFF2_FLAGS(ch), race_stat_adjust[GET_RACE(ch)][13]);
 
    GET_POS(ch) = POS_RESTING;

    } else {
	extract_char(ch);
    }
    /* END of PETS */
  }
}


void die(struct char_data * vict, struct char_data * ch)
{
  int gold = GET_GOLD(vict);
  int gain;
  bool in_same_room;
  char buf[256];
  extern struct char_data *mob_proto;

  if (IN_ROOM(vict) == IN_ROOM(ch))
    in_same_room = TRUE;
  else
    in_same_room = FALSE;

  /* HACKED to track kills of PCs by NPCs */
  if (!IS_NPC(vict) && IS_NPC(ch)) {
    ch->mob_specials.kills++;
    mob_proto[ch->nr].mob_specials.kills++;
  }
  if (IS_NPC(vict) && !IS_NPC(ch)) {
    mob_proto[vict->nr].mob_specials.deaths++;
  }
  /* END of hack */
  
  if (IS_CHAOS_ROOM(vict->in_room) && arena_deathmatch_mode) {
    log_deathmatch_kill(ch, vict);
  }
/*  if (IS_CHAOS_ROOM(vict->in_room) && arena_deathmatch_mode && !IS_NPC(ch)) { */
  if (IS_CHAOS_ROOM(vict->in_room) && arena_deathmatch_mode) {
    char_from_room(vict);
    death_cry(ch);
    /* the following code up to the act() is cloned from spell_recall */
    if ((GET_CLAN(vict) != CLAN_UNDEFINED) &&
        (GET_CLAN(vict) != CLAN_NOCLAN) &&
        (GET_CLAN(vict) != CLAN_PLEDGE) &&
        (GET_CLAN(vict) != CLAN_BLACKLISTED))
      char_to_room(vict, r_clan_start_room[(int) GET_CLAN(vict)]);
    else {
      if (GET_LEVEL(vict) <= LVL_LOWBIE)
        char_to_room(vict, r_lowbie_start_room);
      else
       char_to_room(vict, r_race_start_room[(int) GET_RACE(vict)]);
    }
    act("$n appears in an explosion of blood.", TRUE, vict, 0, 0, TO_ROOM);
    look_at_room(vict, 0);
      
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

    while (vict->affected) affect_remove(vict, vict->affected);    
    AFF_FLAGS(vict) = 0;
    affect_total(vict);

    send_to_char("You get up off the floor and dust yourself off.\r\n", vict);
    GET_POS(vict) = POS_STANDING;
    
    return;
  }
   
/* end of deathmatch hack */


/* This sets gain to the exp they need from prev level to this level, and then
	that value is modified below */

  gain = (titles[(int) GET_CLASS(vict)][(int) GET_LEVEL(vict)].exp -
	titles[(int) GET_CLASS(vict)][GET_LEVEL(vict) - 1].exp);

/* Debug code
  sprintf(buf, "Level is %d, Exp for this level is %d, Exp for previous level is %d.  Gain is %d, should lose half that.",
	GET_LEVEL(vict),
	titles[(int) GET_CLASS(vict)][(int) GET_LEVEL(vict)].exp,
	titles[(int) GET_CLASS(vict)][GET_LEVEL(vict) - 1].exp,
	gain);
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
*/

  gain_exp(vict, -(gain / 3));

  if (!IS_NPC(vict))
    REMOVE_BIT(PLR_FLAGS(vict), PLR_KILLER | PLR_THIEF);

  raw_kill(vict, ch);

  if (IS_NPC(vict) && PRF_FLAGGED(ch, PRF_AUTOSPLIT) && in_same_room) {
    do_get(ch, "all.coins corpse", 0, 0);
    if (IS_AFFECTED(ch, AFF_GROUP)) {
      sprintf(buf, "%d", gold);
      do_split(ch, buf, 0, 0);
    }
  }

  if (IS_NPC(vict) && PRF_FLAGGED(ch, PRF_AUTOGOLD)
      && !PRF_FLAGGED(ch, PRF_AUTOSPLIT) && in_same_room)
    do_get(ch, "all.coins corpse", 0, 0);
  if (IS_NPC(vict) && PRF_FLAGGED(ch, PRF_AUTOLOOT) && in_same_room)
    do_get(ch, "all corpse", 0, 0);
  if (IS_NPC(vict) && PRF_FLAGGED(ch, PRF_AUTOSAC) && in_same_room)
    do_sacrifice(ch, "corpse", 0, 0);

}


void perform_group_gain(struct char_data * ch, int base,
			     struct char_data * victim)
{
  int share;

  if (IS_NPC(ch))
    return;

  if (!IS_NPC(victim)) {
    send_to_char("You didn't get any experience at all!\r\n", ch);
  } else {

    share = MIN(max_exp_gain, MAX(1, base));

    if (IS_AFFECTED2(ch, AFF2_QUESTOR) && IS_NPC(victim))
    {
      if ((ch)->player_specials->saved.questmob == GET_MOB_VNUM(victim))
      {
  	send_to_char("You have almost completed your QUEST!\n\r",ch);
	send_to_char("Return to the questmaster before your time runs out!\n\r",ch);         
	(ch)->player_specials->saved.questmob = -1;
      }   
    }

    if (share > 1) {
      sprintf(buf2, "You receive your share of experience -- %d points.\r\n", share);
      send_to_char(buf2, ch);
    } else
      send_to_char("You receive your share of experience -- one measly little point!\r\n", ch);

    gain_exp(ch, share);
  }


  change_alignment(ch, victim);
}



void group_gain(struct char_data * ch, struct char_data * victim)
{

  int tot_members, tot_levels, base, grp_avg_lvl;
  struct char_data *k;
  struct follow_type *f;

  if ( !IS_NPC(victim) || victim == ch )
    return;

  if (!(k = ch->master))
    k = ch;

  if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)) {
    tot_members = 1;
    tot_levels = GET_LEVEL(k);
  } else {
    tot_members = 0;
    tot_levels = 0;
  }

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && 
        f->follower->in_room == ch->in_room) {
      tot_members++;
      tot_levels += GET_LEVEL(f->follower);
    }

  /* round up to the next highest tot_members */
  base = (GET_EXP(victim) / 3) + tot_members - 1;

  if (tot_members >= 1)
    base = MAX(1, GET_EXP(victim) / (3 * tot_members));
  else
    base = 0;

  if (tot_members >= 1)
    grp_avg_lvl = (tot_levels / tot_members);

  if (IS_AFFECTED(k, AFF_GROUP) && k->in_room == ch->in_room) {
    if (GET_LEVEL(k) <= (grp_avg_lvl - 10)) {
	perform_group_gain(k, 1, victim);
    } else {
    perform_group_gain(k,
            base * GET_LEVEL(k) * tot_members / tot_levels,
            victim);
    }
  }

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
	if (GET_LEVEL(f->follower) <= (grp_avg_lvl - 10)) {
	    perform_group_gain(f->follower, 1, victim);
	} else {
	    perform_group_gain(f->follower,
		base * GET_LEVEL(f->follower) * tot_members / tot_levels,
            victim);
	}

/* end of hack */
}



char *replace_string(char *str, char *weapon_singular, char *weapon_plural)
{
  static char buf[256];
  char *cp;

  cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}



void ranged_message(struct char_data * ch, struct char_data * vict,
        char *from_buf, char *to_buf)
{
  if ((ch == NULL) || (ch->in_room == NOWHERE)) {
    sprintf(from_buf, "From Nowhere, ");
    log("ranged_message: cannot find ch.");
  } else if (ch->in_room == vict->in_room) {
    *from_buf = '\0';
  } else {
    if (OUTSIDE(ch)) {
      sprintf(from_buf, "From %s, ", world[ch->in_room].name);
    } else {
      sprintf(from_buf, "From inside %s, ", world[ch->in_room].name);
    }
  }

  if ((vict == NULL) || (vict->in_room == NOWHERE)) {
    sprintf(to_buf, "Out to Nowhere, ");
    log("ranged_message: cannot find vict.");
  } else if (vict->in_room == ch->in_room) {
    *to_buf = '\0';
  } else {
    if (OUTSIDE(vict)) {
      sprintf(to_buf, "Out to %s, ", world[vict->in_room].name);
    } else {
      sprintf(to_buf, "Into %s, ", world[vict->in_room].name);
    }
  }

  return;
}


void new_dam_message(int dam, struct char_data * ch, struct char_data * victim,
	int dt)
{

    static char * const attack_table[] =
    {
	"hit",
	"sting",  "whip",  "slash", "bite", "bludgeon",
	"crush",  "pound", "claw", "maul", "thrash",
	"pierce", "blast", "punch", "stab", "chop",
	"lightning breath", "frost breath", "acid breath",
	"fire breath", "gas breath"
    };

    char buf1[256], buf2[256], buf3[256];
    const char *vs;
    const char *vp;
    const char *attack;
    char punct;

	 if ( dam ==   0 ) { vs = "miss";	vp = "misses";		}
    else if ( dam <=   4 ) { vs = "scratch";	vp = "scratches";	}
    else if ( dam <=  10 ) { vs = "graze";	vp = "grazes";		}
    else if ( dam <=  18 ) { vs = "hit";	vp = "hits";		}
    else if ( dam <=  32 ) { vs = "injure";	vp = "injures";		}
    else if ( dam <=  40 ) { vs = "wound";	vp = "wounds";		}
    else if ( dam <=  48 ) { vs = "maul";       vp = "mauls";		}
    else if ( dam <=  56 ) { vs = "decimate";	vp = "decimates";	}
    else if ( dam <=  64 ) { vs = "devastate";	vp = "devastates";	}
    else if ( dam <=  72 ) { vs = "maim";	vp = "maims";		}
    else if ( dam <=  80 ) { vs = "MUTILATE";	vp = "MUTILATES";	}
    else if ( dam <=  85 ) { vs = "DISEMBOWEL";	vp = "DISEMBOWELS";	}
    else if ( dam <=  90 ) { vs = "EVISCERATE";	vp = "EVISCERATES";	}
    else if ( dam <= 125 ) { vs = "MASSACRE";	vp = "MASSACRES";	}
    else if ( dam <= 200 ) { vs = "DEMOLISH";	vp = "DEMOLISHES";	}
    else if ( dam <= 250 ) { vs = "ANNIHILATE"; vp = "ANNIHILATES";	}
    else 		   { vs = "LIQUIFIES";	vp = "LIQUIFIES";	}
    punct   = (dam <= 80) ? '.' : '!';

    if ( ( dt == TYPE_SUFFERING ) ||
	 ( dt == 1343 ) ||
	 ( dt == SPELL_POISON) )
	return;

    if ( dt == TYPE_HIT )
    {
	sprintf( buf1, "$n %s $N%c",  vp, punct );
	sprintf( buf2, "You %s $N%c", vs, punct );
	sprintf( buf3, "$n %s you%c", vp, punct );
    }
    else
    {
	if ( dt >= 0 && dt < MAX_SKILLS )
	    attack	= spells[dt];
	else if ( dt >= TYPE_HIT
	&& dt < TYPE_HIT + sizeof(attack_table)/sizeof(attack_table[0]) )
	    attack	= attack_table[dt - TYPE_HIT];
	else
	{
	    sprintf(buf, "Dam_message: bad dt %d.", dt);
	    mudlog(buf, BRF, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE);
	    dt  = TYPE_HIT;
	    attack  = attack_table[0];
	}

	sprintf( buf1, "$n's %s %s $N%c",  attack, vp, punct );
	sprintf( buf2, "Your %s %s $N%c",  attack, vp, punct );
	sprintf( buf3, "$n's %s %s you%c", attack, vp, punct );
    }

    if (PRF2_FLAGGED(ch, PRF2_SHOW_DAMAGE)) {
	char foo[256];
	sprintf(foo, "[%d]", dam);
	strcat(buf2, foo);
    }

    if (PRF2_FLAGGED(victim, PRF2_SHOW_DAMAGE)) {
	char foo[256];
	sprintf(foo, "[%d]", dam);
	strcat(buf3, foo);
    }

    act( buf1, FALSE, ch, NULL, victim, TO_BATTLE );
    send_to_char(CCALERT(ch), ch);
    act( buf2, FALSE, ch, NULL, victim, TO_CHAR );
    send_to_char(CCNRM(ch), ch);
    send_to_char(CCWARNING(victim), victim);
    act( buf3, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP );
    send_to_char(CCNRM(victim), victim);

    return;
}


#define MAX_DAMAGE 30000	/* upper limit would be 32767 */
void damage(struct char_data * ch, struct char_data * victim, int dam,
	    int attacktype)
{
  int exp;
  extern int max_wimpy_lev;    /* see config.c */


  if (GET_POS(victim) <= POS_DEAD) {
    log("SYSERR: Attempt to damage a corpse, victim is already dead.");
    return;			/* -je, 7/7/92 */
  }

  if (victim->in_room == NOWHERE) {
    log("SYSERR: Attempt to damage a corpse, victim is nowhere.");
    return;
  }

  /* Now level_god: You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_GOD))
    dam = 0;

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return;

  /* protection in peaceful rooms */
  if ((ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) ||
	ROOM_FLAGGED(victim->in_room, ROOM_PEACEFUL)) &&
	(attacktype != SPELL_POISON))
  return;

  if (victim != ch) {
    if (GET_POS(ch) > POS_STUNNED) {
      if (!(FIGHTING(ch)))
	set_fighting(ch, victim);

      if (IS_NPC(ch) && IS_NPC(victim) && victim->master &&
	  !number(0, 10) && IS_AFFECTED(victim, AFF_CHARM) &&
	  (victim->master->in_room == ch->in_room)) {
	if (FIGHTING(ch))
	  stop_fighting(ch);
	hit(ch, victim->master, TYPE_UNDEFINED);
	return;
      }
    }
    if (GET_POS(victim) > POS_STUNNED && !FIGHTING(victim)) {
      set_fighting(victim, ch);
      /* don't let blinded mobs remember */
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch) &&
	  (GET_LEVEL(ch) < LVL_IMMORT) && !IS_AFFECTED(ch, AFF_BLIND))
	remember(victim, ch);
    }
  }

  if (victim->master == ch)
    stop_follower(victim);

  if (IS_AFFECTED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  if (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && (IS_GOOD(ch)))
    dam = (dam * 4 / 5);
  
  if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) && (IS_EVIL(ch)))
    dam = (dam * 4 / 5);

  if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) && (IS_EVIL(victim)))
    dam = (dam * 5 / 4);
 
  if (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && (IS_GOOD(victim)))
    dam = (dam * 5 / 4);

  if (IS_AFFECTED(victim, AFF_SANCTUARY))
    dam >>= 1;		/* 1/2 damage when sanctuary */

  if (IS_AFFECTED2(victim, AFF2_MANASHELL)) {
    if (IS_NPC(victim)) {
      dam >>= 1;
    } else {
      if (GET_MANA(victim) > dam) {
        dam >>= 1;
        GET_MANA(victim) = GET_MANA(victim) - dam;
      } else {
        dam >>= 1;

        act("You lack the mana to sustain your manashell.", FALSE, ch, 0, victim, TO_VICT);
        affect_from_char(victim, SPELL_MANASHELL);
      }
    }
  }

  if (IS_AFFECTED(victim, AFF_STUN)) {
    dam += dam;   
  }

  if (IS_AFFECTED(victim, AFF_STUN)) 
    if (number(0, 2) > 1) {
       REMOVE_BIT(AFF_FLAGS(victim), AFF_STUN);
    }

  if (IS_AFFECTED(victim, AFF_MIRROR_IMAGE) && (attacktype != SPELL_POISON)) {
    if (number(0, GET_IMAGES(victim)) < GET_IMAGES(victim)) {
      dam = 0;
      GET_IMAGES(victim) = GET_IMAGES(victim) - 1;
      act("You hit an image of $N!", FALSE, ch, 0, victim, TO_CHAR);
      act("$n hits an image of you!", FALSE, ch, 0, victim, TO_VICT);
      act("$n hits an image of $N!", FALSE, ch, 0, victim, TO_NOTVICT);

      if (GET_IMAGES(victim) <= 0) {
        if (IS_AFFECTED(victim, AFF_MIRROR_IMAGE))
        affect_from_char(victim, SPELL_MIRROR_IMAGE);
        GET_IMAGES(victim) = 0;
	send_to_char("The last of your images disipates, leaving you unprotected.\r\n", victim);

      }
    }
  }    

  if ( (IS_AFFECTED(victim, AFF_STONESKIN)) &&
	(((attacktype >= TYPE_HIT) && (attacktype <= 3015)) ||
	((attacktype >= 1301) && (attacktype <= 1349))) ) {

      dam = 1;

      GET_LAYERS(victim) = GET_LAYERS(victim) - 1;
      act("You hit $N and deflect off $S stone skin!",
          FALSE, ch, 0, victim, TO_CHAR);
      act("$n hits you and deflects off your stone skin!",
          FALSE, ch, 0, victim, TO_VICT);
      act("$n hits $N and deflects off $S stone skin!",
          FALSE, ch, 0, victim, TO_NOTVICT);
      if (GET_LAYERS(victim) <= 0) {
        act("Your skin loses its stony texture!",
            FALSE, ch, 0, victim, TO_VICT);
        if (IS_AFFECTED(victim, AFF_STONESKIN))
          affect_from_char(victim, SPELL_STONESKIN);
        GET_LAYERS(victim) = 0;
      }
  }

  if (!pk_allowed) {
    check_killer(ch, victim);

    if (PLR_FLAGGED(ch, PLR_KILLER)) 
      dam = 0;  
  }

  dam = MAX(MIN(dam, MAX_DAMAGE), 0);
  GET_HIT(victim) -= dam;

  /* dont let mobs gain experience */
  if ((ch != victim) && !IS_NPC(ch))
    gain_exp(ch, GET_LEVEL(victim) * dam);

  update_pos(victim);

  new_dam_message(dam, ch, victim, attacktype);

  if ( (IS_AFFECTED2(victim, AFF2_FIRESHIELD)) &&
	(((attacktype >= TYPE_HIT) && (attacktype <= 3015)) ||
	((attacktype >= 1301) && (attacktype <= 1349))) ) {

    if (IS_AFFECTED2(ch, AFF2_RES_FIRE)) {
      send_to_char(CCWARNING(ch), ch);
      act("Flames pass harmlessly around your body.", FALSE, ch, 0, 0, TO_CHAR);
      send_to_char(CCNRM(ch), ch);
    } else {
      send_to_char(CCWARNING(ch), ch);
      act("You are engulfed in glowing flames!", FALSE, ch, 0, 0, TO_CHAR);
      send_to_char(CCNRM(ch), ch);
      send_to_char(CCWARNING(victim), victim);
      act("$N is surrounded by flames from your fireshield.", FALSE, victim, 0, ch, TO_CHAR);
      send_to_char(CCNRM(victim), victim);
      GET_HIT(ch) -= dam/2;
      update_pos(ch);
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", victim);
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", victim);
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You're stunned, but will probably regain consciousness again.\r\n", victim);
    break;
  case POS_DEAD:
    /* MOBProg foo */
    if (ch)
      mprog_death_trigger(victim, ch);
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    if (ch)
      mprog_kill_trigger(ch, victim);
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) >> 2))
      act("That really did HURT!", FALSE, victim, 0, 0, TO_CHAR);

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2)) {
      sprintf(buf2, "%sYou wish that your wounds would "
              "stop BLEEDING so much!%s\r\n",
	      CCWARNING(victim), CCNRM(victim));
      send_to_char(buf2, victim);
    }

    /* player wimpy */
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	(GET_HIT(victim) < GET_WIMP_LEV(victim)) &&
        (GET_HIT(victim) < max_wimpy_lev) &&
        (GET_CANT_WIMPY(victim) != 1)) {
      send_to_char("You wimp out, and attempt to flee!\r\n", victim);
      do_flee(victim, "", 0, 0);
    }

    /* mob wimpy */
    if (MOB_FLAGGED(victim, MOB_WIMPY))
      if (GET_HIT(victim) < (GET_MAX_HIT(victim)/10))
        do_flee(victim, "", 0, 0);

    break;
  }

  if (!IS_NPC(victim) && !(victim->desc)) {
    do_flee(victim, "", 0, 0);
    if (!FIGHTING(victim)) {
      if (victim->in_room == NOWHERE) {
        sprintf(buf, "SYSERR: %s NOWHERE when rescued by divine forces.",
            GET_NAME(victim));
        log(buf);
      } else {
        act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      }
      GET_WAS_IN(victim) = victim->in_room;
    }
  }
  if (!AWAKE(victim))
    if (FIGHTING(victim))
      stop_fighting(victim);

/* This is duplicate code to check for people dying to fireshield/mpdamage */

  if (GET_POS(ch) == POS_DEAD) {
    if (IS_NPC(ch) || ch->desc)
      if (IS_AFFECTED(victim, AFF_GROUP)) {
	group_gain(victim, ch);
      } else {


	exp = MIN(max_exp_gain, GET_EXP(ch) / 3);

	/* Calculate level-difference bonus */
	if (IS_NPC(victim))
	  exp += MAX(0, (exp * MIN(4, (GET_LEVEL(ch) - GET_LEVEL(victim)))) >> 3);
	else
	  exp += MAX(0, (exp * MIN(8, (GET_LEVEL(ch) - GET_LEVEL(victim)))) >> 3);
	exp = MAX(exp, 1);
        if (!IS_NPC(victim)) {

	  if ( IS_NPC(victim) || !IS_NPC(ch) || victim == ch )
            exp = 0;

	  if (IS_AFFECTED2(victim, AFF2_QUESTOR) && IS_NPC(ch))
	  {
	    if ((victim)->player_specials->saved.questmob == GET_MOB_VNUM(ch))
            {
                send_to_char("You have almost completed your QUEST!\n\r",victim);
                send_to_char("Return to the questmaster before your time runs out!\n\r",victim);         
                (victim)->player_specials->saved.questmob = -1;
            }   
	  }


	  if (exp > 1) {
	    sprintf(buf2, "You receive %d experience points.\r\n", exp);
	    send_to_char(buf2, victim);
  	  } else if (exp == 1) {
	    send_to_char("You receive one lousy experience point.\r\n", victim);
          } else if (exp == 0) {
            send_to_char("You didn't get any experience at all!\r\n", victim);
          } else {
            send_to_char("Wow you LOST experience from that kill!\r\n", victim);
          }
          if (ch != victim)
	    gain_exp(victim, exp);

	  change_alignment(victim, ch);
        }
      }
    if (!IS_NPC(ch)) {
      sprintf(buf2, "%s killed by %s at %s", GET_NAME(ch), GET_NAME(victim),
	      (ch->in_room == NOWHERE) ? "Nowhere" :
              world[ch->in_room].name);
      if (ch->in_room != NOWHERE) {
        if (IS_CHAOS_ROOM(ch->in_room) && arena_deathmatch_mode) {
          mudlog(buf2, CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(victim)), TRUE);
        } else {
      mudlog(buf2, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(victim)), TRUE);
        }
      } else {
        mudlog(buf2, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(victim)), TRUE);
      }
      /* clanlog */
      sprintf(buf2, "%s has died.", GET_NAME(ch));
      clanlog(buf2, ch);

      if (MOB_FLAGGED(victim, MOB_MEMORY))
	forget(victim, ch);
    }

    die(ch, victim);
  }

/* End of duplicate check */

  if (GET_POS(victim) == POS_DEAD) {
    if (IS_NPC(victim) || victim->desc)
      if (IS_AFFECTED(ch, AFF_GROUP)) {
	group_gain(ch, victim);
      } else {


	exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

	/* Calculate level-difference bonus */
	if (IS_NPC(ch))
	  exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	else
	  exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	exp = MAX(exp, 1);
        if (!IS_NPC(ch)) {

	  if ( IS_NPC(ch) || !IS_NPC(victim) || victim == ch )
            exp = 0;

	  if (IS_AFFECTED2(ch, AFF2_QUESTOR) && IS_NPC(victim))
	  {
	    if ((ch)->player_specials->saved.questmob == GET_MOB_VNUM(victim))
            {
                send_to_char("You have almost completed your QUEST!\n\r",ch);
                send_to_char("Return to the questmaster before your time runs out!\n\r",ch);         
                (ch)->player_specials->saved.questmob = -1;
            }   
	  }


	  if (exp > 1) {
	    sprintf(buf2, "You receive %d experience points.\r\n", exp);
	    send_to_char(buf2, ch);
  	  } else if (exp == 1) {
	    send_to_char("You receive one lousy experience point.\r\n", ch);
          } else if (exp == 0) {
            send_to_char("You didn't get any experience at all!\r\n", ch);
          } else {
            send_to_char("Wow you LOST experience from that kill!\r\n", ch);
          }
          if (ch != victim)
	    gain_exp(ch, exp);

	  change_alignment(ch, victim);
        }
      }
    if (!IS_NPC(victim)) {
      sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
	      (victim->in_room == NOWHERE) ? "Nowhere" :
              world[victim->in_room].name);
      if (victim->in_room != NOWHERE) {
        if (IS_CHAOS_ROOM(victim->in_room) && arena_deathmatch_mode) {
          mudlog(buf2, CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
        } else {
      mudlog(buf2, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
        }
      } else {
        mudlog(buf2, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      }
      /* clanlog */
      sprintf(buf2, "%s has died.", GET_NAME(victim));
      clanlog(buf2, victim);

      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }

    die(victim,ch);
  }
}



/*
 * This returns a number of attacks that is very raw
 * and 100 times too large ... ie 3 1/2 attacks would be returned
 * as '350' <- 3.5 attacks.
 * This is also a great place to move wield_2 weapons to wield
 * (the normal slot)
 */
int percent_number_of_attacks(struct char_data * ch)
{
  struct obj_data *wielded;
  struct obj_data *wielded_2;
  int percent_attacks;
  
  if (IS_THRIKREEN(ch)) {
    wielded = ch->equipment[THRI_WEAR_WIELD_R];
    wielded_2 = ch->equipment[THRI_WEAR_WIELD_L];
  } else {
    wielded = ch->equipment[WEAR_WIELD];
    wielded_2 = ch->equipment[WEAR_WIELD_2];
  }

  percent_attacks = 100;                /* start with one attack.. */

  if (IS_NPC(ch)) {
    percent_attacks += GET_LEVEL(ch) * 3;
  } else switch (GET_CLASS(ch)) {
    case CLASS_DEATHKNIGHT:
      percent_attacks += GET_LEVEL(ch) * 3;
/*      percent_attacks += 100; */
      percent_attacks += number_range(1, 100);

      break;
/*
    case CLASS_DRAGON:
    case CLASS_DEATH_KNIGHT:
      percent_attacks += GET_LEVEL(ch) * 7 / 2;
      break;
*/
    case CLASS_WARRIOR:
      percent_attacks += GET_LEVEL(ch) * 3;
      break;
/*
    case CLASS_GHOUL:
    case CLASS_GHOST:
      percent_attacks += GET_LEVEL(ch) * 5 / 2;
      break;
*/
    case CLASS_BARD:
    case CLASS_THIEF:
      percent_attacks += GET_LEVEL(ch) * 2;
      break;
    case CLASS_VAMPIRE:
/*
    case CLASS_LICH:
*/
      percent_attacks += GET_LEVEL(ch) * 3 / 2;
      break;
    case CLASS_CLERIC:
      percent_attacks += GET_LEVEL(ch) * 1;
      break;
    case CLASS_MAGIC_USER:
      percent_attacks += GET_LEVEL(ch) * 1 / 2;
      break;
    default:
      break;
  }

  /* raging warriors get an extra 1/2 attack */
  if (IS_AFFECTED(ch, AFF_RAGE))
/*    percent_attacks += 50; */
    percent_attacks += number_range(1, 50);

  /* fix the weapons in case primary wield is empty
     but secondary wield has something (sigh) */
  if (!wielded && wielded_2) {
    wielded = wielded_2;
    wielded_2 = NULL;
  }

  /* lightweight weapons are a LOT better for thieves
    and heavy weapons suck bad */
  if (IS_THIEF(ch) && wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
    percent_attacks += number_range(1, 25) + GET_LEVEL(ch)
/*    percent_attacks += 25 + GET_LEVEL(ch) */
        - ((GET_OBJ_WEIGHT(wielded) * GET_OBJ_WEIGHT(wielded)) / 2);
  }

  /* dual wield gives an extra attack */
  if (wielded && wielded_2 && (GET_SKILL(ch, SKILL_DUALWIELD) ||
                               IS_THRIKREEN(ch)))
    percent_attacks += GET_LEVEL(ch) * 2;

                        /* check for Haste and Slow spell affects */
  if (AFF_FLAGGED(ch,AFF_HASTE))
/*    percent_attacks += 100; */
    percent_attacks += number_range(1, 100);
  if (AFF_FLAGGED(ch,AFF_SLOW))
/*    percent_attacks -= 100; */
    percent_attacks -= number_range(1, 100);

                        /*
                         * for players only:  
                         * have dex affect the number of attacks they get
                         * take an attack off, and make that last attack
                         * dependant on how good your dex is.
                         * the formula is: (2/3 p) + (1/3 p * dex/25)
                         * but obviously simplified
                         * p = percent_attacks, d = ch's dex
                         */
  if (!IS_NPC(ch))
    percent_attacks = ((GET_DEX(ch)*percent_attacks) + (50*percent_attacks)) /
                                                75;

                        /* give them at least one attack, no matter what */
  if (percent_attacks < 100)
    percent_attacks = 100;

  return percent_attacks;
}



int number_of_attacks(struct char_data * ch)
{
  int percent_attacks;
  int num_attacks;


  /*
   * if their attacks are blocked, then clear them, but
   * dont let them get any attacks.
   */
  if (GET_BLOCKED(ch)) {
    GET_BLOCKED(ch) = 0;
    return 0;
  }

  /* continue as normal... */

  /* percent_number_of_attacks will always be at least 100 (1.00 attack) */
  percent_attacks = percent_number_of_attacks(ch);

  num_attacks = percent_attacks / 100;
  percent_attacks %= 100;

  if (number(0, 99) < percent_attacks)
    num_attacks++;

  return num_attacks;
}



void make_pool(struct char_data * ch)
{
  struct obj_data *pool;
  extern int max_npc_corpse_time;


  pool = create_obj();

  pool->item_number = NOTHING;
  pool->in_room = NOWHERE;
  pool->name = str_dup("pool blood");
  
  sprintf(buf2, "a pool of %s's blood lies here.", GET_NAME(ch));
  pool->description = str_dup(buf2);

  sprintf(buf2, "a pool of %s's blood", GET_NAME(ch));
  pool->short_description = str_dup(buf2);

  GET_OBJ_TYPE(pool) = ITEM_FOUNTAIN;
  GET_OBJ_WEAR(pool) = 0;
  GET_OBJ_EXTRA(pool) = ITEM_NODONATE;
  GET_OBJ_VAL(pool, 0) = 2;
  GET_OBJ_VAL(pool, 1) = 2;
  GET_OBJ_VAL(pool, 2) = LIQ_BLOOD;
  GET_OBJ_VAL(pool, 3) = 0;
  GET_OBJ_WEIGHT(pool) = 99999;
  GET_OBJ_RENT(pool) = 100000;
  GET_OBJ_TIMER(pool) = max_npc_corpse_time;

  obj_to_room(pool, ch->in_room);
}



void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded;
  struct obj_data *wielded_2;
  struct obj_data *wielded_temp;
  bool wielded_juggling = FALSE;
  struct obj_data *readied;
  int w_type;
  int victim_ac, calc_thaco;
  int dam;
  byte diceroll;
  int num_attacks;
  int i;
  int dir; /* so mobs can hunt down a player if they're being plinked */
  int critical_hit_chance;
  int missed_the_backstab = 0;


  /* External variables */
  extern int thaco[NUM_CLASSES][LVL_IMPL + 1];
  extern byte backstab_mult[];
  extern struct str_app_type str_app[];
  extern struct dex_app_type dex_app[];

  /* External functions */
  extern int find_first_step(sh_int src, sh_int target);
  extern bool is_nearby(struct char_data * ch, struct char_data * i);

  if (IS_THRIKREEN(ch)) {
    readied = ch->equipment[THRI_WEAR_READY];
  } else {
    readied = ch->equipment[WEAR_READY];
  }

  /* hit code */

  /* if ch or victim is not there (a kind of mob_prog mob), bail immediately */
/* combined within lower check 
  if (MOB_FLAGGED(ch, MOB_NOTTHERE)) {
    stop_fighting(ch);
    return;
  }
*/

  if (MOB_FLAGGED(victim, MOB_NOTTHERE) || MOB_FLAGGED(ch, MOB_NOTTHERE)) {
    stop_fighting(victim);
    stop_fighting(ch);
    return;
  }

  /* fix wielded and wielded_2 (sigh) */
  if (IS_THRIKREEN(ch)) {
    wielded = ch->equipment[THRI_WEAR_WIELD_R];
    wielded_2 = ch->equipment[THRI_WEAR_WIELD_L]; 
  } else {
    wielded = ch->equipment[WEAR_WIELD];
    wielded_2 = ch->equipment[WEAR_WIELD_2];
  }
  if (!wielded && wielded_2) {
    wielded = wielded_2;
    wielded_2 = NULL;
    ch->equipment[WEAR_WIELD] = ch->equipment[WEAR_WIELD_2];
    ch->equipment[WEAR_WIELD_2] = NULL;
  }

  /* MOBProg foo */
  mprog_hitprcnt_trigger(ch, FIGHTING(ch));
  mprog_fight_trigger(ch, FIGHTING(ch));

/* HACKED to not let players use ranged weapons on !plink mobs */
/* and to not let people plink across zones (leading mobs across zone lines) */
  if (ch->in_room != victim->in_room) {
    stop_fighting(ch);
/*
    if (MOB_FLAGGED(victim, MOB_NOPLINK) && IS_NPC(victim)) {
      send_to_char("You can't get a good shot!\r\n", ch);
      return;
    }
*/
  } 
/* end of hack */

  /* Figure out what weapon the ch is using */
  /* NOTE: this code means that mobs without ranged weapons can't defend */
  if (ch->in_room == victim->in_room) {
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
    else {
      if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
        w_type = ch->mob_specials.attack_type + TYPE_HIT;
      else
        w_type = TYPE_HIT;
    }
/*
  } else if (readied && is_nearby(ch, victim)) {
    if (readied && GET_OBJ_TYPE(readied) == ITEM_FIREWEAPON) {
      w_type = GET_OBJ_VAL(readied, 3) + TYPE_HIT;
    } else {
      if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
        w_type = ch->mob_specials.attack_type + TYPE_HIT;
      else
        w_type = TYPE_HIT;
    }
*/
  } else {
    if (FIGHTING(victim)) {
      if (IS_NPC(ch)) {
        dir = find_first_step(ch->in_room, victim->in_room);
        switch (dir) {
          case BFS_ERROR:
          case BFS_ALREADY_THERE:
          case BFS_NO_PATH:
            if (FIGHTING(ch))
              stop_fighting(ch);
            if (FIGHTING(victim) == ch)
              stop_fighting(victim);
            return;
            break;
          default:
            if (!(do_simple_move(ch, dir, TRUE))) {
              do_flee(victim, "", 0, 0);
            } else {
		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
		    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
		} else {
		    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
			w_type = ch->mob_specials.attack_type + TYPE_HIT;
		    else
			w_type = TYPE_HIT;
		}
	    }
            break;
        }  
      }
    } else {
      if (FIGHTING(ch))
        stop_fighting(ch);
      return;
    }
/* end of Hack */
  } 

  /* Calculate the raw armor including magic armor.  Lower AC is better. */
  if (!IS_NPC(ch))
    calc_thaco = thaco[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)];
  else
    /* THAC0 for monsters is set in the HitRoll */
    calc_thaco = 20;

  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  calc_thaco -= GET_HITROLL(ch);

  victim_ac = GET_AC(victim) / 10;

  if (AWAKE(victim))        
    victim_ac += dex_app[GET_DEX(victim)].defensive;

  victim_ac = MAX(-10, victim_ac);      /* -10 is lowest */

  switch (type) {
      case SKILL_RIPOSTE:
      case SKILL_AVENGING_BLOW:
      case SKILL_BERSERK:
          num_attacks = 1;
          break;
      case SKILL_BACKSTAB:
	  if (IS_AFFECTED(ch, AFF_STONESKIN)) {
	    num_attacks = number_of_attacks(ch) / 2;
	  } else {
	    num_attacks = number_of_attacks(ch);
	  }
      default:
          num_attacks = number_of_attacks(ch);
          break;
  }

  /* do the actual attacks */
  for (i = 0; i < num_attacks; i++) {

    /* switch to the wielded_2 weapon if they are dual_wielding,
      and they are in the last half of their attacks,
      and they are not using ranged combat.
      set it all back further down below and set the wielded_juggling
      to true */
    if ((wielded_2) &&
        (GET_OBJ_TYPE(wielded_2) == ITEM_WEAPON) &&
        (ch->in_room == victim->in_room) &&
        (i > (num_attacks / 2))) {
      w_type = GET_OBJ_VAL(wielded_2, 3) + TYPE_HIT;
      wielded_temp = wielded;
      wielded = wielded_2;
      wielded_2 = wielded_temp;
      wielded_juggling = TRUE; 
    }

    /*
     * sever chance =
     * .5% if they use a chopping weapon,
     * .3% otherwise
     * knights get a whole extra 1.0% chance!!
     */
    if (IS_WARRIOR(ch) || IS_DEATHKNIGHT(ch)) {

    int cdam = 0;

      critical_hit_chance = 3;

#ifdef NO_ACTIVE_DEATH_KNIGHTS
      if (IS_DEATHKNIGHT(ch))
        critical_hit_chance += 10;
#endif

      if (IS_AFFECTED(victim, AFF_STONESKIN))
	critical_hit_chance = 0;

      if (number(0, 100) < critical_hit_chance) {
        if (GET_SKILL(ch, SKILL_CRITICAL_HIT) > 50) {
          send_to_char("You score a critical hit, "
              "which results in a spurt of blood!\r\n", ch);
          act("$N scores a critical hit on you, "
              "and causes a shower of blood!", FALSE, victim, 0, ch, TO_CHAR);
          act("$n hits $N so hard they spurt blood!",
              FALSE, ch, 0, victim, TO_NOTVICT);
          make_pool(victim);
	  if(wielded)
	      cdam = dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
	  cdam += GET_DAMROLL(ch);
	  cdam *= 2;
          damage(ch, victim, cdam, w_type);
          if (GET_POS(victim) <= POS_DEAD)
            return;
        }
      }
    }

    /* into the main attacks!!! */
    diceroll = number(1, 20);

    if ((((diceroll < 20) && AWAKE(victim)) &&
         ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
      /* ch missed */
      if (type == SKILL_BACKSTAB) {
        if (i == 0) /* first attack */ {
          damage(ch, victim, 0, SKILL_BACKSTAB);
          missed_the_backstab = 1;
	  return;
        } else {
	  log("SYSERR: fight.c, should not be here.");
	  return;
/*
          if (missed_the_backstab)
            damage(ch, victim, 0, w_type);
          else
            damage(ch, victim, 0, SKILL_TWIST);
*/
        }
      } else if (type == SKILL_CIRCLE) {
        damage(ch, victim, 0, SKILL_CIRCLE);
	return;
      } else {
        damage(ch, victim, 0, w_type);
      }
    } else /* ch hit */ {

      dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
      dam += GET_DAMROLL(ch);

      if (ch->in_room == victim->in_room) {
        if (wielded) {
          dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
          if (IS_NPC(ch)) {
            dam = dam +
              dice(ch->mob_specials.damnodice,ch->mob_specials.damsizedice);
          }
	} else {
	    if (IS_NPC(ch)) {
		dam += dice(ch->mob_specials.damnodice,ch->mob_specials.damsizedice);
	    } else {

		switch (GET_CLASS(ch)) {
		    case CLASS_DEATHKNIGHT:
			dam += (int) number(1, MAX(GET_LEVEL(ch) / 1.5, 1));
			break;
		    case CLASS_WARRIOR:
			dam += (int) number(1, MAX(GET_LEVEL(ch) / 1.5, 1));
			break;
		    default:
			dam += number(0, 2);	/* Max. 2 dam with bare hands */
			break;
		}
	    }
	}
/*
      } else if (readied) {
          dam += dice(GET_OBJ_VAL(readied, 1), GET_OBJ_VAL(readied, 2));
*/
      } else {
        return;
      }


      if (GET_POS(victim) < POS_FIGHTING)
        dam *= 1 + (POS_FIGHTING - GET_POS(victim))  /  1.5  ;
      /* Position  sitting  x 1.33 */
      /* Position  resting  x 1.66 */
      /* Position  sleeping x 2.00 */
      /* Position  stunned  x 2.33 */
      /* Position  incap    x 2.66 */
      /* Position  mortally x 3.00 */


      dam = MAX(1, dam);		/* Not less than 0 damage */

      /* don't let them hurt a corpse */
      if (GET_POS(victim) > POS_DEAD) {
        if ((type == SKILL_BACKSTAB) && (i == 0)) {
          if (IS_AFFECTED(ch, AFF_SNEAK)) {
            sprintf(buf, "You sneaked in and %s doesn't notice you at all!\r\n",
                GET_NAME(victim));
            send_to_char(buf, ch);
            dam *= backstab_mult[(int) GET_LEVEL(ch)] + 1;
          } else {
            dam *= backstab_mult[(int) GET_LEVEL(ch)];
          }
          damage(ch, victim, dam, SKILL_BACKSTAB);
	  return;
        } else if (type == SKILL_BACKSTAB) {
          if (missed_the_backstab) {
            damage(ch, victim, dam, w_type);
          } else {
            damage(ch, victim, dam * GET_LEVEL(ch) / 15, SKILL_TWIST);
	    return;
	  }
        } else if (type == SKILL_CIRCLE) {
          dam *= 3;
          damage(ch, victim, dam, SKILL_CIRCLE);
	  return;
        } else if (type == SKILL_AVENGING_BLOW) {
          dam *= (number_of_attacks(ch) + 2);
          damage(ch, victim, dam, SKILL_AVENGING_BLOW);
        } else {
          damage(ch, victim, dam, w_type);
	}
      }
    }  /* ch hit */

    /* switch back to the wielded weapon if they are dual_wielding,
      and they are in the last half of their attacks,
      set it all back to normal and set the juggling to false ;) */
    if (wielded_juggling) {
      wielded_temp = wielded_2;
      wielded_2 = wielded;
      wielded = wielded_temp;
      wielded_juggling = FALSE;
    }

    /* if ch killed victim, lets bail */
    if (FIGHTING(ch) == NULL)
      return;
  }

  /* let the poor bastard flee now that he's been beaten on */
  if (FIGHTING(ch))
    if (GET_CANT_WIMPY(FIGHTING(ch)) == 1)
      GET_CANT_WIMPY(FIGHTING(ch)) = 0;
}


void autoassist_victim_group(struct char_data * killer, struct char_data * ch)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;


  if (ch == NULL)
    return;

  /* uncharmed, ie normal, mobs dont get autoassisted by anything */
  if (IS_NPC(ch) && !IS_AFFECTED(ch, AFF_CHARM))
    return;

  /* you only assist people you're grouped with */ 
  if (!IS_AFFECTED(ch, AFF_GROUP))
    return;

  /* find the head of the group */
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;

  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;

    if (tch->in_room != ch->in_room)
      continue;
    if (!IS_AFFECTED(tch, AFF_GROUP))
      continue;
    if (!FIGHTING(tch) && PRF_FLAGGED(tch, PRF_AUTOASSIST)) {
      if (!IS_NPC(tch) && !IS_NPC(killer)) {
        send_to_char("You want to help, but you're no KILLER!\r\n", tch);
      } else {
        set_fighting(tch, killer);
        send_to_char("You join the fight!\r\n", tch);
      }
    }
  }

  if ((k != ch) && IS_AFFECTED(k, AFF_GROUP) &&
	!FIGHTING(k) && PRF_FLAGGED(k, PRF_AUTOASSIST) &&
	(k->in_room == ch->in_room)) {
    if (!IS_NPC(k) && !IS_NPC(killer)) {
      send_to_char("You want to help, but you're no KILLER!\r\n", k);
    } else {
      set_fighting(k, killer);
      send_to_char("You join the fight!\r\n", k);
    }
  }
  /* ch is presumably fighting someone, usually the attacker */
}



/* do the weapon spells */
void call_weapon_spells(struct char_data * ch, struct obj_data * weapon)
{
  int i;
  
  if (!ch) return;
  if (!FIGHTING(ch)) return;


  if (weapon) {
    if (weapon->spell_affect[0].spelltype > 0) {
      for (i = 0; i < MAX_SPELL_AFFECT; i++) {
        if (weapon->spell_affect[i].spelltype > 0) {
          if (number(0,99) < (weapon->spell_affect[i].percentage)) {
            act("Your $p hums violently in your hands!", FALSE, ch,
                weapon, 0, TO_CHAR);
            act("$p hums violently in the hands of $n!", FALSE, ch,
                weapon, 0, TO_ROOM);
            if (weapon->spell_affect[i].level > 60)
              call_magic(ch, ch, NULL, NULL,
                  weapon->spell_affect[i].spelltype,
                  (weapon->spell_affect[i].level) - 60,
                  CAST_SPELL);
            else
              call_magic(ch, FIGHTING(ch), NULL, NULL,
                  weapon->spell_affect[i].spelltype,
                  weapon->spell_affect[i].level,
                  CAST_SPELL);
          }
        }
      }
    }
  }
}



/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
/* struct char_data *victim; */
  ACMD(do_flee);

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL) {
      stop_fighting(ch);
    } else if (MOB_FLAGGED(ch, MOB_WILL_SELF_PURGE)) {
      if (FIGHTING(FIGHTING(ch)) == ch)
        stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
      act("$n vanishes in a poof of smoke!", FALSE, ch, 0, 0, TO_ROOM);
      extract_char(ch);
    } else {
#if 0
      /* switch attacks? the chance of switching attacks is equal to
         1% * the level of the mob */
      /* HACKED further I hope to cut the chance of switch in half */
      if (IS_NPC(ch)) {
        for (switch_victim = world[ch->in_room].people;
             switch_victim; switch_victim = switch_victim->next_in_room)
          if ((FIGHTING(switch_victim) == ch) && 
              (number(0, 299) < GET_LEVEL(ch)))
            break;
        if ((switch_victim != ch) && (switch_victim != NULL))
          FIGHTING(ch) = switch_victim;
      }
      /* end of switch attacks */
#endif

      if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) &&
          (ch->in_room != ch->master->in_room)) {
	SET_BIT(MOB_FLAGS(ch), MOB_MEMORY);
	remember(ch->master, ch);
	SET_BIT(AFF2_FLAGS(ch), AFF2_WASCHARMED);
	stop_follower(ch);

/* Oh, let's duke it out.  I like to fight
	if (FIGHTING(ch) != NULL)
	{
	    victim = FIGHTING(ch);
	    stop_fighting(victim);
	}
	stop_fighting(ch);
        do_flee(ch, "", 0, 0);
*/
        continue;
      }
      /* end of flee */
      /* If you're a mob, fighting someone, and you aren't charmed, and you're
         fighting a mob who also isn't charmed, stop it! */
      /* Let switched mobs keep fighting */
/*  But I WANT to fight
      if (IS_NPC(ch) && !IS_AFFECTED(ch, AFF_CHARM) && IS_NPC(FIGHTING(ch)) &&
          !IS_AFFECTED(FIGHTING(ch), AFF_CHARM) && 
          !ch->desc && !(FIGHTING(ch)->desc)) {
        if (FIGHTING(FIGHTING(ch)) == ch) {
          stop_fighting(FIGHTING(ch));
        }
        stop_fighting(ch);
        continue;
      }
*/
#if 0
      /* trigger mobprogs */
      mprog_hitprcnt_trigger(ch, FIGHTING(ch));
      mprog_fight_trigger(ch, FIGHTING(ch));
#endif

      /* hit their target */
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED);

      /* weapon spells */
      if (IS_THRIKREEN(ch)) {
        if (ch->equipment[THRI_WEAR_WIELD_R])
          call_weapon_spells(ch, ch->equipment[THRI_WEAR_WIELD_R]);
        if (ch->equipment[THRI_WEAR_WIELD_L])
          call_weapon_spells(ch, ch->equipment[THRI_WEAR_WIELD_L]);
      } else {
        if (ch->equipment[WEAR_WIELD])
          call_weapon_spells(ch, ch->equipment[WEAR_WIELD]);
        if (ch->equipment[WEAR_WIELD_2])
          call_weapon_spells(ch, ch->equipment[WEAR_WIELD_2]);
      }

      /* if they're a mob, do their special procedure */
      if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
        (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");

      /* make friends of their target auto-assist */
      autoassist_victim_group(ch, FIGHTING(ch));

      /* Show auto group report */
      if (!IS_NPC(ch))
        if (PRF2_FLAGGED(ch, PRF2_AUTOGROUP) && IS_AFFECTED(ch, AFF_GROUP))
          print_group(ch);
    }
  }
}

void clear_deathmatch_scores() {
  struct dm_score_data *score, *nextscore;
  
  score = dm_scores;
  
  while (score != NULL) {
    nextscore = score->next;
    free(score);
    score = nextscore;
  }
  
  dm_scores = NULL;
}

struct dm_score_data *find_deathmatch_score( struct char_data *ch ) {
  struct dm_score_data *cur = dm_scores;
  
  /* is ch in the list of scores yet? */
  while (cur != NULL) {
    if (!strcmp( cur->plrname, GET_NAME(ch) ))
      return cur;
    cur = cur->next;
  }
  
  /* If not, add him! */
  cur = malloc(sizeof(struct dm_score_data));
  strcpy(cur->plrname, GET_NAME(ch));
  cur->kills = 0;
  cur->deaths = 0;
  cur->killscore = 0;
  cur->next = dm_scores;
  dm_scores = cur;
  
  return cur;
}

void log_deathmatch_kill( struct char_data *ch, struct char_data *vict) {
  ACMD(do_qcomm);
  struct dm_score_data *targ, *cur, *hipos, tmp;
  int hi;
  struct dm_score_data *ch_score, *vict_score;
  
/*  if (IS_NPC(ch) || IS_NPC(vict)) return;  */
  
  /* NPCs always get logged, but if one person in a player above the max level,
     don't do anything */
  if ((!IS_NPC(ch) && GET_LEVEL(ch) > arena_deathmatch_level) ||
      (!IS_NPC(vict) && GET_LEVEL(vict) > arena_deathmatch_level)) return;
    
  vict_score = find_deathmatch_score ( vict );
  vict_score->deaths++;
  vict_score->killscore -= GET_LEVEL(vict);

  if (vict == ch) {
    sprintf(buf, "[ %s dies horribly in the arena! (%d / %d) ]", GET_NAME(ch),
      vict_score->kills, vict_score->deaths);
      vict_score->killscore -= GET_LEVEL(vict);
  } else {
    ch_score = find_deathmatch_score( ch );
    ch_score->kills++;
    ch_score->killscore += (GET_LEVEL(vict) * 2);
    sprintf(buf, "[ %s (%d) killed by %s (%d) ]",
      GET_NAME(vict), vict_score->killscore,
      GET_NAME(ch), ch_score->killscore);
  }  
  
  do_qcomm(ch, buf, 0, SCMD_QECHO);
  
  /* New code: Sort the DM scores by number of kills, high to low. */
  /* Using a selection sort - slower, but who cares. Less likely that I
     screw up the list, and it's late at night... */

  for (targ = dm_scores; targ->next; targ = targ->next) {
    hi = targ->killscore;
    hipos = targ;
    for (cur = targ->next; cur; cur = cur->next) {
      if (cur->killscore > hi) {
        hi = cur->killscore;
        hipos = cur;
      }
    }
    if (hipos != targ) {
      tmp.killscore = hipos->killscore;
      tmp.kills = hipos->kills;
      tmp.deaths = hipos->deaths;
      strcpy(tmp.plrname, hipos->plrname);
      hipos->killscore = targ->killscore;
      hipos->kills = targ->kills;
      hipos->deaths = targ->deaths;
      strcpy(hipos->plrname, targ->plrname);
      targ->killscore = tmp.killscore;
      targ->kills = tmp.kills;
      targ->deaths = tmp.deaths;
      strcpy(targ->plrname, tmp.plrname);
    }
  }
/*
  for (targ = dm_scores; targ->next; targ = targ->next) {
    hi = targ->kills - targ->deaths;
    hipos = targ;
    for (cur = targ->next; cur; cur = cur->next) {
      if (cur->kills - cur->deaths > hi) {
        hi = cur->kills - cur->deaths;
        hipos = cur;
      }
    }
    if (hipos != targ) {
      tmp.kills = hipos->kills;
      tmp.deaths = hipos->deaths;
      strcpy(tmp.plrname, hipos->plrname);
      hipos->kills = targ->kills;
      hipos->deaths = targ->deaths;
      strcpy(hipos->plrname, targ->plrname);
      targ->kills = tmp.kills;
      targ->deaths = tmp.deaths;
      strcpy(targ->plrname, tmp.plrname);
    }
  }
*/
}

bool is_safe( struct char_data *ch, struct char_data *victim, bool show_messg)
{
    if ((ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) ||
		(ROOM_FLAGGED(victim->in_room, ROOM_PEACEFUL)))
    {
        if ( show_messg ) {
	    send_to_char("A magical force prevents you from attacking.\n\r", ch );
	}
	return TRUE;
    }
    
    if (MOB_FLAGGED(ch, MOB_SAFE))
    {
        if ( show_messg ) {
    	send_to_char("You are a pacifist and will not fight.\n\r", ch);
	}
	return TRUE;
    }

    if (MOB_FLAGGED(victim, MOB_SAFE))
    {
        if ( show_messg ) {
            sprintf(buf, "%s is a pacifist and will not fight.\n\r",
                GET_NAME(victim));
        send_to_char( buf, ch);
	}
        return TRUE;
    }
    return FALSE;
}

ACMD(do_deathpost) {
  /* Posts the dq scores, if any, to the mortal board (#0) */
  /* lots of this is cloned from boards.c from Board_write_message() */
  
  extern int find_slot();

  extern int num_of_msgs[NUM_OF_BOARDS];
  extern char *msg_storage[INDEX_SIZE];
  extern struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];
  
  time_t ct;
  char *tmstr;
  int len;
  struct dm_score_data *cur;
  
  /* Normal mobs are allowed to do this, but not jarred or charmed */
  if (IS_NPC(ch) && (ch->desc || ch->master)) return;
  
  if (!dm_scores) {
    send_to_char("Sorry, there aren't any deathmatch scores to post.\r\n", ch);
    return;
  }
  
  if (num_of_msgs[3] >= MAX_BOARD_MESSAGES) {
    send_to_char("Sorry, the board's full!", ch);
    return;
  }
  
  if ((NEW_MSG_INDEX(3).slot_num = find_slot()) == -1) {
    send_to_char("The board is malfunctioning - sorry.\r\n", ch);
    log("SYSERR: Board: failed to find empty slot on write.");
    return;
  }

  ct = time(0);
  tmstr = (char *) asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  sprintf(buf2, "(%s)", GET_NAME(ch));
  sprintf(buf, "%6.10s %-12s :: Deathmatch scores", tmstr, buf2);
  len = strlen(buf) + 1;
  if (!(NEW_MSG_INDEX(3).heading = (char *) malloc(sizeof(char) * len))) {
    send_to_char("The board is malfunctioning - sorry.\r\n", ch);
    return;
  }
  strcpy(NEW_MSG_INDEX(3).heading, buf);
  NEW_MSG_INDEX(3).heading[len - 1] = '\0';
  NEW_MSG_INDEX(3).level = GET_LEVEL(ch);
  NEW_MSG_INDEX(3).class = GET_CLASS(ch);
  NEW_MSG_INDEX(3).clan = GET_CLAN(ch);

  /* &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]); 
     MAX_MESSAGE_LENGTH */
  
  /* Ok, we've got the message created on the board...let's make our
     post. This is cloned from do_deaths() */
  
  *buf = '\0';
  cur = dm_scores;
  
  if (arena_deathmatch_mode) {
    sprintf(buf, "Deathmatch results so far:\r\n\r\n");
  } else {
    sprintf(buf, "Final deathmatch results:\r\n\r\n");
  }
  sprintf(buf, "%s%-35s%-10s%-10s%-10s\r\n", buf, "Name", "Kills", "Deaths", "Score");
  sprintf(buf, "%s%-35s%-10s%-10s%-10s\r\n", buf, "----", "-----", "------", "-----");
  
  while (cur != NULL) {
    sprintf(buf, "%s%-35s%-10d%-10d%-10d\r\n", buf,
          cur->plrname, cur->kills, cur->deaths, cur->killscore);
    cur = cur->next;
  }
  
  /* Allllllrighty then, the text is in buf and the message is set up
     on the board. All we gotta do is stick the two together... */
 
  msg_storage[NEW_MSG_INDEX(3).slot_num] = strdup(buf);
  num_of_msgs[3]++;
  Board_save_board(3);
  
  send_to_char("Posted.\r\n", ch);
}
