/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"


/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct command_info cmd_info[];
extern struct obj_data *object_list;
extern char *dirs[];

/* extern functions */
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_manacost(struct char_data * ch, int spellnum);
void mprog_death_trigger(struct char_data * mob, struct char_data * killer);



struct social_type {
  char *cmd;
  int next_line;
};


/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

int spell_sort_info[MAX_SKILLS+1];

extern char *spells[];

void sort_spells(void)
{
  int a, b, tmp;

  /* initialize array */
  for (a = 1; a < MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < MAX_SKILLS - 1; a++)
    for (b = a + 1; b < MAX_SKILLS; b++)
      if (strcmp(spells[spell_sort_info[a]], spells[spell_sort_info[b]]) > 0) {
	tmp = spell_sort_info[a];
	spell_sort_info[a] = spell_sort_info[b];
	spell_sort_info[b] = tmp;
      }
}



char *how_good(int percent)
{
  static char buf[256];

  if (percent == 0)
    strcpy(buf, " (not learned)");
  else if (percent <= 10)
    strcpy(buf, " (awful)");
  else if (percent <= 20)
    strcpy(buf, " (bad)");
  else if (percent <= 40)
    strcpy(buf, " (poor)");
  else if (percent <= 55)
    strcpy(buf, " (average)");
  else if (percent <= 70)
    strcpy(buf, " (fair)");
  else if (percent <= 80)
    strcpy(buf, " (good)");
  else if (percent <= 85)
    strcpy(buf, " (very good)");
  else
    strcpy(buf, " (superb)");

  return (buf);
}

char *prac_types[] = {
  "spell",
  "skill"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

/* actual prac_params are in class.c */
extern int prac_params[4][NUM_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])



/* HACKED to take out practices */
void list_skills(struct char_data * ch)
{
  extern char *spells[];
  extern struct spell_info_type spell_info[];
  int i, sortpos;

  *buf = '\0';
  
  if(IS_NPC(ch)) {
    send_to_char("Mobs don't need to practice! They have instinct!", ch);
    return;
  }

/* HACKED to take out practices */
/*
  if (!GET_PRACTICES(ch))
    strcpy(buf, "You have no practice sessions remaining.\r\n");
  else
    sprintf(buf, "You have %d practice session%s remaining.\r\n",
	    GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));
*/
/* end of hack */

  sprintf(buf, "%sYou know of the following %ss:\r\n"
               "SPELLNAME               LEVEL  COST\r\n", buf, SPLSKL(ch));

  strcpy(buf2, buf);

  for (sortpos = 1; sortpos < MAX_SKILLS; sortpos++) {
    i = spell_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) {
      if (mag_manacost(ch, i) == 0) {
        sprintf(buf, "%-20s %5d   n/a\r\n",
            spells[i],
            /* how_good(GET_SKILL(ch, i)), end practices */
            spell_info[i].min_level[(int) GET_CLASS(ch)]);
      } else {
        sprintf(buf, "%-20s %5d %5d\r\n",
            spells[i],
            /* how_good(GET_SKILL(ch, i)), end practices */
            spell_info[i].min_level[(int) GET_CLASS(ch)], mag_manacost(ch, i));
      }
      strcat(buf2, buf);
    }
  }
  page_string(ch->desc, buf2, 1);
}
/* end of hack */



/* HACKED to take out practices */
/*
SPECIAL(guild)
{
  int skill_num, percent;

  extern struct spell_info_type spell_info[];
  extern struct int_app_type int_app[26];

  if (IS_NPC(ch) || !CMD_IS("practice"))
    return 0;

  skip_spaces(&argument);

  if (!*argument) {
    list_skills(ch);
    return 1;
  }
  if (GET_PRACTICES(ch) <= 0) {
    send_to_char("You do not seem to be able to practice now.\r\n", ch);
    return 1;
  }

  skill_num = find_skill_num(argument);

  if (skill_num < 1 ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
    sprintf(buf, "You do not know of that %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return 1;
  }
  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You are already learned in that area.\r\n", ch);
    return 1;
  }
  send_to_char("You practice for a while...\r\n", ch);
  GET_PRACTICES(ch)--;

  percent = GET_SKILL(ch, skill_num);
  percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));

  SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);

  return 1;
}
*/
/* end of hack */



SPECIAL(chasm)
{
 
 int jumpchance = 0;

 if (!(CMD_IS("jump") || (CMD_IS("east"))))
   return FALSE;
 
 if (CMD_IS("east")) {
   send_to_char("A wide chasm yawns before you, you'll have to jump it!\r\n",
    ch);
   return TRUE;
 }
 if (CMD_IS("jump")) {

 jumpchance = number(0, 99);

 if (IS_AFFECTED(ch, AFF_FLY)) {
   jumpchance += 10;
 }
 if (jumpchance < 10) {
   act("$n tries to jump the chasm, and falls to certain death!", TRUE, ch, 
       0, 0, TO_ROOM);
   send_to_char("You attempt to jump the chasm, but fall brutally to your"
       " death!\r\n", ch);
   perform_move(ch, SOMEWHERE, 0); 
   return TRUE;
 }
 if (jumpchance == 100) {
   act("$n starts to fly across the chasm, but hits a downdraft and "
     "plummets to \r\ncertain death below!", TRUE, ch, 0, 0,TO_ROOM);
   send_to_char("You hit a draft of wind and fall to your death!\r\n", ch);
   perform_move(ch, SOMEWHERE, 0);
   return TRUE;
 }
 if (jumpchance > 10) {
   perform_move(ch, EAST, 0);
   return TRUE;
  }
 }
return FALSE;
} 



/*
SPECIAL(echo)
{

if (!(CMD_IS("say"))

  world[ch->in_room].number
*/



/*
SPECIAL(night_up_gate)
{
  if (!CMD_IS("down")) 
    return FALSE;

  if ((time_info.hours < 5) || (time_info.hours >= 22)) {
    perform_move(ch, SOMEWHERE, 0);
    return TRUE;
  } else {
    perform_move(ch, DOWN, 0);
    return TRUE;
  }
}
*/



/*
SPECIAL(night_west_gate)
{
  if (!CMD_IS("east")) 
    return FALSE;

  if ((time_info.hours < 5) || (time_info.hours >= 22)) {
    perform_move(ch, SOMEWHERE, 0);
    return TRUE;
  } else {
    perform_move(ch, EAST, 0);
    return TRUE;
  }
}
*/



/*
SPECIAL(night_east_gate)
{
  if (!CMD_IS("west")) 
    return FALSE;

  if ((time_info.hours < 5) || (time_info.hours >= 22)) {
    perform_move(ch, SOMEWHERE, 0);
    return TRUE;
  } else {
    perform_move(ch, WEST, 0);
    return TRUE;
  }
}
*/



/*
SPECIAL(night_north_gate)
{
  if (!CMD_IS("south")) 
    return FALSE;

  if ((time_info.hours < 5) || (time_info.hours >= 22)) {
    perform_move(ch, SOMEWHERE, 0);
    return TRUE;
  } else {
    perform_move(ch, SOUTH, 0);
    return TRUE;
  }
}
*/



/*
SPECIAL(night_south_gate)
{
  if (!CMD_IS("north")) 
    return FALSE;

  if ((time_info.hours < 5) || (time_info.hours >= 22)) {
    perform_move(ch, SOMEWHERE, 0);
    return TRUE;
  } else {
    perform_move(ch, NORTH, 0);
    return TRUE;
  }
}
*/



SPECIAL(lounge)
{
  if (!CMD_IS("up"))
    return FALSE;

  if (GET_LEVEL(ch) < LVL_GRGOD) {
    send_to_char("You cannot go there!\r\n", ch);
    return TRUE;
  } else {
    perform_move(ch, UP, 0);
    return TRUE;
  }
}


  
SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  ACMD(do_drop);
  char *fname(char *namelist);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return 0;

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    act("You are awarded for outstanding performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return 1;
}



SPECIAL(mayor)
{
  ACMD(do_gen_door);

  static char open_path[] =
  "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

  static char close_path[] =
  "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static char *path;
  static int index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return FALSE;

  switch (path[index]) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    perform_move(ch, path[index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
    do_gen_door(ch, "gate", 0, SCMD_OPEN);
    break;

  case 'C':
    do_gen_door(ch, "gate", 0, SCMD_CLOSE);
    do_gen_door(ch, "gate", 0, SCMD_LOCK);
    break;

  case '.':
    move = FALSE;
    break;

  }

  index++;
  return FALSE;
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data * ch, struct char_data * victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;

  if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
    if (gold > 0) {
      GET_GOLD(ch) += gold;
      GET_GOLD(victim) -= gold;
    }
  }
}



SPECIAL(snake)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0)) {
    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_POISON,
      GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}



/* Possible crash bug in failed assertion FIGHTING(ch), dont know
  which line
SPECIAL(drain)
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
   
  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
    (number(0,3) == 0)) {
    act("$n touches $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n touches you with dead hands!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    GET_EXP(FIGHTING(ch)) -= 500;
    if (GET_EXP(FIGHTING(ch)) < 0)
      GET_EXP(FIGHTING(ch)) = 0;
    return TRUE;
  }
  return FALSE;
}
*/


/* possible crash bug on failed assertion of FIGHTING(ch) */
/*
SPECIAL(gate) 
{
  struct char_data *mob;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
   
  if (FIGHTING(ch) && (number(0,3) == 0))  {
    
    mob = read_mobile(6620, VIRTUAL);
    char_to_room(mob, FIGHTING(ch)->in_room);

    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
    add_follower(mob, ch);

    act("The Fiend summons help!", 1, ch, 0, FIGHTING(ch),
      TO_NOTVICT | TO_VICT);
    
    set_fighting(mob, FIGHTING(ch));
    return TRUE;
  }  
  return FALSE; 
}
*/



SPECIAL(chaos_guard)
{
  struct char_data *mob;
  struct char_data *vict;

  if (CMD_IS("north"))
      act("The Rynathi gate guard pushes you back in line!", 1, ch, 0, ch,
         TO_CHAR);
      act("The Rynathi gate guard pushes $N back in line!", 1, ch, 0, ch,
         TO_ROOM);
   return FALSE;

  if (FIGHTING(ch) && (number(0,3) == 0))  {
    
    vict = FIGHTING(ch);
    mob = read_mobile(23601, VIRTUAL);
    char_to_room(mob, FIGHTING(ch)->in_room);

    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
    add_follower(mob, ch);

    act("The guard summons help!", 1, ch, 0, FIGHTING(ch),
      TO_NOTVICT | TO_VICT);
    
    set_fighting(mob, FIGHTING(ch));
    return TRUE;
   }
  return FALSE;
}


SPECIAL(chaos) 
{
  
  if (CMD_IS("bribe")) {
    if (GET_GOLD(ch) > 5000) 
     perform_move(ch, 3, 1);
     act("The guard grins at you and waves you through!", 1, ch, 0, ch,
         TO_CHAR);
     act("The guard smiles at $N and waves them through!", 1, ch, 0, ch,
         TO_ROOM);
     if (GET_GOLD(ch) > 50000)
        GET_GOLD(ch) -= (GET_GOLD(ch) / 10);
     else
        GET_GOLD(ch) -= 5000;
    return TRUE;
    }
    else {
     act("Baah, that is not a bribe! It is an insult!", 1, ch, 0, ch,
        TO_CHAR);
     act("The Rynathi gate guard spits on $N and pushes them back!", 1, 
        ch, 0, ch, TO_ROOM);
     return FALSE;
   }
}



SPECIAL(stone)
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (number(0,2) == 0)) {
     cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_SLOW);
     cast_spell(ch, ch, NULL, NULL, SPELL_HASTE);
    return TRUE; 
  }
 return FALSE;
}



SPECIAL(iron)
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
 
  if (FIGHTING(ch) && (number(0,2) == 0)) {
     cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_PETRIFY);
    return TRUE; 
  }
 return FALSE;
}



SPECIAL(zeus)
{
  ACMD(do_trans);

  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (number(0,2) == 0)) {
    switch (number(0,2)) {
     case 2:
       if ((GET_HIT(ch) > 1000) || (GET_HIT(ch) < 200)) {
         cast_spell(ch, ch, NULL, NULL, SPELL_RESTORE);
       } else {
         act("$n reels in pain as $s head splits!!",
             FALSE, ch, 0, 0, TO_ROOM);
         do_trans(ch, "Athena", 0, 0);
       }
       break;
     }
    return TRUE;
  }
  return FALSE;
}



SPECIAL(tyrant)
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (FIGHTING(ch) && (number(0,2) == 0))  {

    switch (number(0,2)) {
      case 0:
        GET_POS(FIGHTING(ch)) = POS_MORTALLYW;
        switch (number(0,1)) {
          case 0:  
           act("$n crushes $N's legs and knocks them to the ground!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
           act("$n brutally knocks you to the ground with his staff!", 1, ch, 0, FIGHTING(ch), TO_VICT);
           break;
	 case 1:
           act("$n brings his staff across $N's chest and knocks them to the ground!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
           act("$n brutally knocks you to the ground with his staff!", 1, ch, 0, FIGHTING(ch), TO_VICT);
           break;
	 }
        break;
      case 1:
        cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_DISPEL_MAGIC);
        break;

      case 2:
        cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_WORD_OF_DEATH);
	break;      
    }
    return TRUE;
  }
 
  return FALSE;
}   
        


SPECIAL(area_bash)
{
  if (cmd || !AWAKE(ch) || (GET_POS(ch) != POS_FIGHTING))
    return FALSE;

  /* in mag areas, they get no save at all */
  if (number(0, 5) == 0) {
    mag_areas(GET_LEVEL(ch), ch, FIGHTING(ch), SKILL_BASH, SAVING_NO_SAVE);
    return TRUE;
  }
   
  return FALSE;
}



SPECIAL(warrior)
{
 
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (FIGHTING(ch) && (number(0,4) == 0))  {
      GET_HIT(FIGHTING(ch)) -= (GET_LEVEL(ch) * 3);
      act("$n staggers $N with a fearsome blow!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
      act("$n hits you with crushing force!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    return TRUE;
  }

  return FALSE;
} 



SPECIAL(area_word_death)
{
  struct char_data *vict;
  int found = 0;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (number(0, 3) == 0) {
    act("$n shrieks an ear-shattering keen!", FALSE, ch, 0, 0, TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
      /* charmed pets are ok targets */
      if (IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
        continue;
      /* dont hurt immortals */
      if (GET_LEVEL(vict) >= LVL_IMMORT)
        continue;
      /* hurt them */
      found = 1;
      /* noises */
      act("$n takes $N to $S knees with $s wail!",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("$s keen rends your soul to pieces and sends you to your knees!",
          TRUE, ch, 0, vict, TO_VICT);
      /* affect them */
      call_magic(ch, vict, NULL, NULL, SPELL_WORD_OF_DEATH,
          GET_LEVEL(ch), CAST_SPELL);
    }
  }

  if (found) 
    return TRUE;

  return FALSE; 
}



SPECIAL(area_dispel_magic)
{
  struct char_data *vict;
  int found = 0;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  
  if (number(0, 3) == 0) {
    act("$n glares and utters the words, 'qirr marsor kariq'!",
        FALSE, ch, 0, 0, TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
      /* charmed pets are ok targets */
      if (IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
        continue;
      /* dont target immortals */
      if (GET_LEVEL(vict) >= LVL_IMMORT)
        continue;
      /* valid target them */
      found = 1;
      /* affect them */
      call_magic(ch, vict, NULL, NULL, SPELL_DISPEL_MAGIC,
          GET_LEVEL(ch), CAST_SPELL);
    }
  }

  if (found) 
    return TRUE;

  return FALSE; 
}



SPECIAL(slap_remove_eq)
{
  struct char_data *vict;
  int i, j;
  int where;

  void perform_remove(struct char_data *ch, int where);

  if (cmd || !AWAKE(ch) || (GET_POS(ch) != POS_FIGHTING))
    return(FALSE);

  if (number(1, 6) == 1) {
    vict = FIGHTING(ch);

    act("$n slaps $N senseless!", TRUE, ch, 0, vict, TO_NOTVICT);
    act("$n slaps you senseless!", TRUE, ch, 0, vict, TO_VICT);

    /* if i chose an immortal then return without affecting them */
    if (GET_LEVEL(vict) >= LVL_IMMORT)
      return TRUE;

    /* affect them */
    i = number(1, 3);
    for (j = 0; j < i; j++) {
      where = number(0, GET_NUM_WEARS(vict) - 1);
      if (vict->equipment[where])
        perform_remove(vict, where);
    }

    return TRUE;
  }

  return FALSE;
}



/* area_dispel_magic and slap_remove_eq */
SPECIAL(bitch_queen)
{
  struct char_data *vict;
  int found = 0;
  int i, j;
  int where;

  void perform_remove(struct char_data *ch, int where);

  if (cmd || !AWAKE(ch) || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  switch (number(1, 20)) {
/* No more dispel magic, now cast from prog */
#if 0
    case 1:
        act("$n glares and utters the words, 'qirr marsor kariq'!",
            FALSE, ch, 0, 0, TO_ROOM);
        for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
          /* charmed pets are ok targets */
          if (IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
            continue;
          /* dont target immortals */
          if (GET_LEVEL(vict) >= LVL_IMMORT)
            continue;
          if (GET_LEVEL(vict) >= LVL_IMMORT)
            continue;
          /* valid target them */
          found = 1;
          /* affect them */
          call_magic(ch, vict, NULL, NULL, SPELL_DISPEL_MAGIC,
              GET_LEVEL(ch), CAST_SPELL);
        }
        break;
#endif
    case 1: case 2: case 3: case 4: case 5: case 6:
        found = 1;

        vict = FIGHTING(ch);

        act("$n slaps $N senseless!", TRUE, ch, 0, vict, TO_NOTVICT);
        act("$n slaps you senseless!", TRUE, ch, 0, vict, TO_VICT);

        /* if i chose an immortal then return without affecting them */
        if (GET_LEVEL(vict) >= LVL_IMMORT)
          return TRUE;

        /* affect them */
        i = number(1, 3);
        for (j = 0; j < i; j++) {
          where = number(0, GET_NUM_WEARS(vict) - 1);
          if (vict->equipment[where])
            perform_remove(vict, where);
        }

        break;

    default: 
        break;
  }

  if (found) 
    return TRUE;

  return FALSE; 
}



SPECIAL(area_scare_hunt)
{
  struct char_data *vict;
  int found = 0;
 
  ACMD(do_flee);

  if (cmd || !AWAKE(ch) || (GET_POS(ch) != POS_FIGHTING))
    return(FALSE);

  if (number(1, 15) == 1) {
    act("$n howls and your blood freezes as fear strikes your heart!",
         TRUE, ch, 0, 0, TO_ROOM);

    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
      /* charmed pets are ok targets */
      if (IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
        continue;
      /* dont target immortals */
      if (GET_LEVEL(vict) >= LVL_IMMORT)
        continue;
      /* valid target them */
      found = 1;
      /* affect them */
      do_flee(vict, "", 0, 0);
    }
  }

  if (found)
    return TRUE;

  return FALSE;
}



SPECIAL(spit_blind)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) != POS_FIGHTING))
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return FALSE;

  /* if i chose myself, then quit */
  if (vict == ch)
    return FALSE;
 
  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return FALSE;

  act("$n sprays a blast of muddy water into $N's eyes!!",
      TRUE, ch, 0, vict, TO_NOTVICT);
  send_to_char("Someone sprayed a blast of muddy water into your eyes!!\r\n"
               "You can't see!!\r\n", vict);

  /* if i chose an immortal then return without affecting them */
  if (GET_LEVEL(vict) >= LVL_IMMORT)
    return TRUE;

  /* affect them */
  call_magic(ch, vict, NULL, NULL, SPELL_BLINDNESS,
      GET_LEVEL(ch), CAST_SPELL);


  return TRUE;
}



SPECIAL(word_of_death)
{

  if (cmd)
    return FALSE;
  if (!(FIGHTING(ch)))
    return FALSE;

  if (FIGHTING(ch) && (number(0,4) == 0))  {
       cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_WORD_OF_DEATH);
    return TRUE;
  }  
  if (FIGHTING(ch) && (number(0,10) == 0))  {
       cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_DISPEL_MAGIC);     
       cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_WORD_OF_DEATH);
    return TRUE;
  }
 return FALSE;
}



SPECIAL(restore)
{
  if (cmd)
    return FALSE;
  if (!(FIGHTING(ch)))
    return FALSE;

  if (FIGHTING(ch) && (number(0,84) == 0))  {
    cast_spell(ch, ch, NULL, NULL, SPELL_RESTORE);
    return TRUE;
  }
  if (FIGHTING(ch) && (number(0,100) == 0))  {
    cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_DISPEL_SANCTUARY);
    cast_spell(ch, ch, NULL, NULL, SPELL_RESTORE);
    return TRUE;
  }

  return FALSE;
}



SPECIAL(bash)
{
  struct char_data *vict;
  ACMD(do_bash);

  if (cmd)
    return FALSE;
  if (!(FIGHTING(ch)))
    return FALSE;
  vict = FIGHTING(ch);

  if (FIGHTING(ch) && (number(0,2) == 0))  {
    do_bash(ch, GET_NAME(vict), 0, 0);
    return TRUE;
  }
  return FALSE;
}



SPECIAL(backstab)
{
  struct char_data *vict;
  ACMD(do_backstab);

  if (cmd)
    return FALSE;
  if (!(FIGHTING(ch)))
    return FALSE;
  vict = FIGHTING(ch);

  if (FIGHTING(ch) && (number(0,3) == 0))  {
     stop_fighting(ch);
     stop_fighting(vict);
     do_backstab(ch, GET_NAME(vict), 0, 0);
     return TRUE;
  }
  return FALSE;
}



SPECIAL(disarm)
{
  struct char_data *vict;
  ACMD(do_disarm);

  if (cmd)
    return FALSE;
  if (!(FIGHTING(ch)))
    return FALSE;
  vict = FIGHTING(ch);

  if (FIGHTING(ch) && (number(0,2) == 0))  {
     cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_DISPEL_SANCTUARY);
     do_disarm(ch, GET_NAME(vict), 0, 0);
     return TRUE;
  }
  return FALSE;
}



SPECIAL(golem) 
{
  struct char_data *mob;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
   
  if (FIGHTING(ch) && (number(0,5) == 0))  {
    
    switch (number(0,2)) {
      case 0:
        mob = read_mobile(10010, VIRTUAL);
        break;
      case 1:
        mob = read_mobile(10011, VIRTUAL);
        break;
      case 2:
        mob = read_mobile(10012, VIRTUAL);
        break;
      default:
        break;
    }
    
    char_to_room(mob, FIGHTING(ch)->in_room);

    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
    add_follower(mob, ch);

send_to_room("The Iron Golem's eyes flare red and starts to move!\r\n", ch->in_room);

    return TRUE;
  }  
  return FALSE; 
} 



SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_STANDING)
    return FALSE;

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4))) {
      npc_steal(ch, cons);
      return TRUE;
    }
  return FALSE;
}



SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
    cast_spell(ch, vict, NULL, NULL, SPELL_SLEEP);

  if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
    cast_spell(ch, vict, NULL, NULL, SPELL_BLINDNESS);

  if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, NULL, SPELL_ENERGY_DRAIN);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, NULL, SPELL_DISPEL_EVIL);
  }
  if (number(0, 4))
    return TRUE;

  switch (GET_LEVEL(ch)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, NULL, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, NULL, SPELL_COLOR_SPRAY);
    break;
  default:
    cast_spell(ch, vict, NULL, NULL, SPELL_FIREBALL);
    break;
  }
  return TRUE;

}



SPECIAL(color_spray_fireball)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;
 
  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  if (number(0, 4) == 0) {
    switch (number(0, 1)) {
      case 0:
          cast_spell(ch, vict, NULL, NULL, SPELL_COLOR_SPRAY);
          break;
      case 1:
      default:
          cast_spell(ch, vict, NULL, NULL, SPELL_FIREBALL);
          break;
    }
  }

  return TRUE;
}



/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(ronk)
{
  struct char_data *vict;
  ACMD(do_backstab);

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (!AWAKE(vict) && !IS_NPC(vict))
      break;
  if (vict == NULL)
    return(FALSE);
  if (vict == ch)
    return(FALSE);
  do_backstab(ch, GET_NAME(vict), 0, 0);
    return(TRUE);
}



/*
SPECIAL(antimagic_bugs)
{
  if (!CMD_IS("cast") && !CMD_IS("sing"))
    return FALSE;

  send_to_char("You cant concentrate with all these bugs around!\r\n", ch);

  return TRUE;   
}
*/



/*
SPECIAL(plague_rat)
{
  void raw_kill(struct char_data * ch, struct char_data * killer); 
  struct char_data *vict;
  struct char_data *mob;
  int num_mobs, max_mobs;
  bool found = FALSE;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return FALSE;
  if (ch->desc)
    return FALSE;
  for (vict = world[ch->in_room].people; vict && !found;
       vict = vict->next_in_room) {
    if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
      continue;
    if (GET_LEVEL(vict) < 6) {
      send_to_char("The plague rat looks at you viciously.\r\n", vict);
      return TRUE;
    } else {
      found = TRUE;
      max_mobs = (GET_LEVEL(vict) / 10) - 1;
      for (num_mobs = 0; num_mobs < max_mobs; num_mobs++) {
        mob = read_mobile(8291, VIRTUAL);
        char_to_room(mob, ch->in_room);
        IS_CARRYING_W(mob) = 0;
        IS_CARRYING_N(mob) = 0;
        act("A plague rat has arrived.", FALSE, ch, 0, 0, TO_ROOM);
        hit(mob, vict, TYPE_UNDEFINED);
      }
    }
  }

  if (found) {
   if (ch)
     mprog_death_trigger(vict, ch);
   raw_kill(ch, vict);
   return TRUE;
  }

  return FALSE;
}
*/



SPECIAL(temple_goddess)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  /* pseudo-randomly choose someone in the room who is not fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) != ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return(FALSE);

  /* if i chose myself, then quit */
  if (vict == ch)
    return(FALSE);
 
  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return(FALSE);
 
  if IS_AFFECTED(vict, AFF_POISON) {
    cast_spell(ch, vict, NULL, NULL, SPELL_REMOVE_POISON);
  }
  if (GET_MAX_HIT(vict) != GET_HIT(vict)) {
    cast_spell(ch, vict, NULL, NULL, SPELL_CURE_CRITIC);
    return(TRUE);
  } else
    return(FALSE);
}



SPECIAL(armor)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  /* pseudo-randomly choose someone in the room who is not fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) != ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return(FALSE);

  /* if i chose myself, then quit */
  if (vict == ch)
    return(FALSE);
 
  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return(FALSE);

  if (number(0, 10) == 1) {
    cast_spell(ch, vict, NULL, NULL, SPELL_ARMOR);
    return(TRUE);
  }

  return(FALSE);
}



SPECIAL(remove_poison)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  /* pseudo-randomly choose someone in the room who is not fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) != ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return(FALSE);

  /* if i chose myself, then quit */
  if (vict == ch)
    return(FALSE);
 
  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return(FALSE);
 
  if IS_AFFECTED(vict, AFF_POISON) {
    cast_spell(ch, vict, NULL, NULL, SPELL_REMOVE_POISON);
    return(TRUE);
  }

  return(FALSE);
}



SPECIAL(fly)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  /* pseudo-randomly choose someone in the room who is not fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) != ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return(FALSE);

  /* if i chose myself, then quit */
  if (vict == ch)
    return(FALSE);
 
  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return(FALSE);

  if (number(0, 5) == 1) {
    cast_spell(ch, vict, NULL, NULL, SPELL_FLY);
    return(TRUE);
  }

  return(FALSE);
}



SPECIAL(cure_critic)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  /* pseudo-randomly choose someone in the room who is not fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) != ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return(FALSE);

  /* if i chose myself, then quit */
  if (vict == ch)
    return(FALSE);
 
  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return(FALSE);
 
  if (GET_MAX_HIT(vict) != GET_HIT(vict)) {
    cast_spell(ch, vict, NULL, NULL, SPELL_CURE_CRITIC);
    return(TRUE);
  } else
    return(FALSE);
}



SPECIAL(infra)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  /* pseudo-randomly choose someone in the room who is not fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) != ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return(FALSE);

  /* if i chose myself, then quit */
  if (vict == ch)
    return(FALSE);
 
  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return(FALSE);

  if (number(0, 5) == 1) {
    cast_spell(ch, vict, NULL, NULL, SPELL_INFRAVISION);
    return(TRUE);
  }

  return(FALSE);
}



SPECIAL(invis)
{
  struct char_data *vict;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return(FALSE);

  /* pseudo-randomly choose someone in the room who is not fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) != ch && !number(0,4))
      break;

  /* if i didnt choose any of those, then quit */
  if (vict == NULL)
    return(FALSE);

  /* if i chose myself, then quit */
  if (vict == ch)
    return(FALSE);

  /* if i chose a mob, then quit */
  if (IS_NPC(vict))
    return(FALSE);

  if (number(0, 5) == 1) {
    cast_spell(ch, vict, NULL, NULL, SPELL_INVISIBLE);
    return(TRUE);
  }

  return(FALSE);
}



SPECIAL(guild_guard)
{
  int i;
  extern int guild_info[][3];
  struct char_data *guard = (struct char_data *) me;
  char *buf = "The guard humiliates you, and blocks your way.\r\n";
  char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND) ||
      (IS_NPC(ch) && !IS_AFFECTED(ch, AFF_CHARM)))
    return FALSE;

  for (i = 0; guild_info[i][0] != -1; i++) {
    if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
	world[ch->in_room].number == guild_info[i][1] &&
	cmd == guild_info[i][2]) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  return FALSE;
}



/*
SPECIAL(puff)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 20)) {
  case 0:
    do_say(ch, "FUCKU Zerkle!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "u are sooo clue", 0, 0);
    return (1);
  case 2:
    do_say(ch, "show me u dont have the intelligence of a yak.", 0, 0);
    return (1);
  case 3:
    do_say(ch, "u got piggydooshed! oink! oink! PIGGYDOOSH!", 0, 0);
    return (1);
  case 4:
    do_say(ch, "NO YOLKS.", 0, 0);
    return (1);
  case 5:
    do_say(ch, "Don't make fun of me because I do push-ups before bed time.", 0, 0);
    return (1);
  case 6:
    do_say(ch, "So does your school have any hot chicks?", 0, 0);
    return (1);
  case 7:
    do_say(ch, "Samuri Showdown rules!", 0, 0);
    return (1);
  case 8:
    do_say(ch, "How do I post to RGN?", 0, 0);
    return (1);
  case 9:
    do_say(ch, "Wait! Stop! It's a Polo outlet!", 0, 0);
    return (1);
  case 10:
    do_say(ch, "No Jitesh Is Fucking IrratE!~!", 0, 0);
    return (1);
  case 11:
    do_say(ch, "mniovew yd.", 0, 0);
    return (1);
  case 12:
    do_say(ch, "u lack male genetalia.", 0, 0);
    return (1);
  default:
    return (0);
  }
}
*/



SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for (temp = i->contains; temp; temp = next_obj) {
	next_obj = temp->next_content;
	obj_from_obj(temp);
	obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
/* HACKED to let fido's eat food thats laying around */
    if (GET_OBJ_TYPE(i) == ITEM_FOOD) {
      act("$n savagely devours $p.", FALSE, ch, i, 0, TO_ROOM);
      extract_obj(i);
      return (TRUE);
    }
/* end of hack */
  }
  return (FALSE);
}



SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return TRUE;
  }

  return FALSE;
}



SPECIAL(cityguard)
{
  struct char_data *tch, *evil;
  int max_evil;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  max_evil = 1000;
  evil = 0;

/* HACKED to not attack killers */
/*
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }
*/
/* end of hack */
/* HACKED to not attack thieves */
/*
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_THIEF)){
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }
*/
/* end of hack */

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
	  (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
	max_evil = GET_ALIGNMENT(tch);
	evil = tch;
      }
    }
  }

  if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED);
    return (TRUE);
  }
  return (FALSE);
}



#define NUM_TIAMAT_ATTACKS 2
SPECIAL(tiamat)
{
  int attack;
  bool acted = FALSE;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  for (attack = 0; attack < NUM_TIAMAT_ATTACKS; attack++) {
    switch(number(1,2)){
      case 1:
          switch(number(1,5)){
            case 1:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                act("$n breathes lightning at $N!", 1, ch, 0, FIGHTING(ch), 
                     TO_NOTVICT);
                act("$n breathes lightning at you!", 1, ch, 0, FIGHTING(ch), 
                     TO_VICT);
                call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_LIGHTNING_BOLT, 
                  GET_LEVEL(ch), CAST_SPELL);
                acted = TRUE;
              }
              break;
            case 2:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                act("$n breathes fire at $N!", 1, ch, 0, FIGHTING(ch), 
                      TO_NOTVICT);
                act("$n breathes fire at you!", 1, ch, 0, FIGHTING(ch), 
                      TO_VICT);
                call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_FIREBALL, 
                  GET_LEVEL(ch), CAST_SPELL);
                acted = TRUE;
              }
              break;
            case 3:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                act("$n breathes acid at $N!", 1, ch, 0, FIGHTING(ch), 
                      TO_NOTVICT);
                act("$n breathes acid at you!", 1, ch, 0, FIGHTING(ch), 
                      TO_VICT);
                call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_DISINTEGRATE, 
                  GET_LEVEL(ch), CAST_SPELL);
                acted = TRUE;
              }
              break;
            case 4:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                act("$n breathes poison gas at $N!", 1, ch, 0, FIGHTING(ch), 
                      TO_NOTVICT);
                act("$n breathes poison gas at you!", 1, ch, 0, FIGHTING(ch), 
                      TO_VICT);
                call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_CLOUDKILL, 
                  GET_LEVEL(ch), CAST_SPELL);
                acted = TRUE;
              }
              break;
            case 5:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                act("$n breathes frost at $N!", 1, ch, 0, FIGHTING(ch), 
                      TO_NOTVICT);
                act("$n breathes frost at you!", 1, ch, 0, FIGHTING(ch), 
                      TO_VICT);
                call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_CONE_OF_COLD, 
                  GET_LEVEL(ch), CAST_SPELL);
                acted = TRUE;
              }
              break;
            default:
              break; 
        }
        break;
      case 2:
          switch(number(1,7)){
            case 1:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_DISINTEGRATE);
                acted = TRUE;
              }
              break;
            case 2:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_WORD_OF_DEATH);
                acted = TRUE;
              }
              break;
            case 3:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_PETRIFY);
                acted = TRUE;
              }
              break;
            case 4:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_EARTHQUAKE);
                acted = TRUE;
              }
              break;
            case 5:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_CALL_LIGHTNING);
                acted = TRUE;
              }
              break;
            case 6:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                act("$n breathes lightning at $N!", 1, ch, 0, FIGHTING(ch), 
                     TO_NOTVICT);
                act("$n breathes lightning at you!", 1, ch, 0, FIGHTING(ch), 
                     TO_VICT);
                call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_LIGHTNING_BOLT, 
                  GET_LEVEL(ch), CAST_SPELL);
                acted = TRUE;
              }
              break;
            case 7:
              if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
                act("$n breathes lightning at $N!", 1, ch, 0, FIGHTING(ch), 
                     TO_NOTVICT);
                act("$n breathes lightning at you!", 1, ch, 0, FIGHTING(ch), 
                     TO_VICT);
                call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_LIGHTNING_BOLT, 
                  GET_LEVEL(ch), CAST_SPELL);
                acted = TRUE;
              }
              break;
            default:
              break;
        }
        break;
      default:
        break;
    }
  }

  return acted;
}


SPECIAL(blue_dragon)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  switch(number(1,5)){
    case 1:
      if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
        act("$n breathes lightning at $N!", 1, ch, 0, FIGHTING(ch), 
             TO_NOTVICT);
        act("$n breathes lightning at you!", 1, ch, 0, FIGHTING(ch), 
              TO_VICT);
        call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_LIGHTNING_BOLT, 
          GET_LEVEL(ch), CAST_SPELL);
        return TRUE;
      }
      break;
    case 2:
        switch(number(1,5)){
          case 1:
            if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
              cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_MAGIC_MISSILE);
              return TRUE;
            }
            break;
          case 2:
            if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
              cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_CHILL_TOUCH);
              return TRUE;
            }
            break;
          case 3:
            if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
              cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_FEEBLEMIND);
              return TRUE;
            }
            break;
          case 4:
            if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
              cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_EARTHQUAKE);
              return TRUE;
            }
            break;
          case 5:
            if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)){
              cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_SHOCKING_GRASP);
              return TRUE;
            }
            break;
          default:
            break;
      }
      break;
    case 3:
      break;
    case 4:
      break;
    case 5:
      break;
    default:
      break;
  }

  return FALSE;
}



SPECIAL(red_dragon)
{
  ACMD(do_bash);

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  switch (number(1,4)) {
    case 1:
      if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
        act("$n breathes fire at $N!", 1, ch, 0, FIGHTING(ch), 
             TO_NOTVICT);
        act("$n breathes fire at you!", 1, ch, 0, FIGHTING(ch), 
              TO_VICT);
        call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_FIREBALL, 
              GET_LEVEL(ch), CAST_SPELL);
        return TRUE;
      }
      break;
    case 2:
      break;
    case 3:
      break;
    case 4:
      break;
    default:
      break;
  }

  return FALSE;
}



SPECIAL(white_dragon)
{
  struct char_data *tank, *p;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(1, 3) == 1)) {
    act("$n breathes frost at $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n breathes frost at you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_FIREBALL, 
      GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }

  /* look for the tank in the room (presumably tiamat) and heal it */
  if (number(1, 3) == 1) {
    tank = ch;
    for (p = world[ch->in_room].people; p; p = p->next_in_room)
      if (IS_NPC(p) && !IS_AFFECTED(p, AFF_CHARM)
          && GET_LEVEL(p) > GET_LEVEL(tank))
        tank = p;
    cast_spell(ch, tank, NULL, NULL, SPELL_RESTORE);
    return TRUE;
  }

  return FALSE;
}



SPECIAL(black_dragon)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(1, 3) == 1)) {
    act("$n breathes acid at $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n breathes acid at you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_DISINTEGRATE, 
      GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}



SPECIAL(green_dragon)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(1, 3) == 1)) {
    act("$n breathes poison gas at $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n breathes poison gas at you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, NULL, SPELL_CLOUDKILL, 
      GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}


/*
SPECIAL(greb_guard)

{
  char *buf = "The guard slaps you upside the head and says, Hey, you're not Greb!\r\n";
  char *buf2 = "The guard slaps $n upside the head and glares at him intensely!";

  if (!CMD_IS("north"))
    return FALSE;

    if (GET_LEVEL(ch) < LVL_IMPL) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }

  return FALSE;
}
*/



/*
SPECIAL(newbie_guard)
{
  char *buf = "The wizened old man peers at you and says, 'You are too old to go further from here.\r\n";
  char *buf2 = "The wizened old man looks angry for a moment, but it passes"; 

  if (!CMD_IS("west"))
    return FALSE;

    if (GET_LEVEL(ch) > 8) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }

  return FALSE;
}
*/



SPECIAL(vampire)
{
  ACMD(do_say);

  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  switch (number(1,3)) {
    case 1:
      if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
        act("$n drains $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
        act("$n drains the life force out of you!", 1, ch, 0, 
                     FIGHTING(ch), TO_VICT);
        GET_EXP(FIGHTING(ch)) -= 50000;
        if (GET_EXP(FIGHTING(ch)) < 0)
          GET_EXP(FIGHTING(ch)) = 0;
        return TRUE;
      }
      break;
    case 2:
      break;
    case 3:
      break;
    default:
      break;
  }
     
  return FALSE;
}



SPECIAL(doom_orc)
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
 
  if (FIGHTING(ch) && (number(0,5) == 0)) {
     cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_FIREBALL);
    return TRUE; 
  }
 return FALSE;
}



SPECIAL(doom_demon)
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
 
  if (FIGHTING(ch) && (number(0,3) == 0)) {
     cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_FIREBALL);
    return TRUE; 
  }
 return FALSE;
}



SPECIAL(doom_beholder) 
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
 
  if (FIGHTING(ch) && (number(0,2) == 0)) {
     cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_FIREBALL);
    return TRUE; 
  }
 return FALSE;
}



SPECIAL(doom_cyberdaemon) 
{
  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
 
  if (FIGHTING(ch) && (number(0,3) == 0)) {
     cast_spell(ch, FIGHTING(ch), NULL, NULL, SPELL_WORD_OF_DEATH);
    return TRUE; 
  }
 return FALSE;
}



/* ********************************************************************
*  Special procedures for rooms                                       *
******************************************************************** */

/*
SPECIAL(water_life)
{

struct obj_data *obj;
char buf[MAX_STRING_LENGTH], water_name[256];

  if (CMD_IS("list")) {
    send_to_char("Available water containers are:\r\n", ch);
    send_to_char("   Canteen          ??\r\n   Barrel          ??\r\n   
Bottle          ??\r\n");
    }
  if (CMD_IS("buy")) {

   argument = one_argument(argument, buf);
   argument = one_argument(argument, water_name);
  
  
*/
  
SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  int pet_room;
  struct char_data *pet;
  struct follow_type *f;
  int num_followers = 0;
  pet_room = ch->in_room + 1;

  if (CMD_IS("list")) {
    send_to_char("Available pets are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", 3 * GET_EXP(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    argument = one_argument(argument, buf);
    argument = one_argument(argument, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such pet!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < (GET_EXP(pet) * 3)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    for (f = ch->followers; f; f = f->next) {
    if (IS_NPC(f->follower))
      num_followers++;
    }

    if (num_followers >= GET_CHA(ch)) {

      send_to_char("You haven't the charisma to lead more followers!\r\n", 
ch);
      return(TRUE);
    }
    GET_GOLD(ch) -= GET_EXP(pet) * 3;

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("May you enjoy your pet.\r\n", ch);
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }
  /* All commands except list and buy */
  return 0;
}



/* for corpse shop below, free for people level 15 or lower */
int corpse_cost(struct char_data * ch)
{
  int cost = 0;


  if (GET_LEVEL(ch) > 15)
    cost = GET_LEVEL(ch) * GET_LEVEL(ch) * GET_LEVEL(ch) * 10;

  return cost;
}

SPECIAL(corpse_shop)
{
  char buf[MAX_STRING_LENGTH];
  register struct obj_data *k;
  int num, wanted; 
  bool found;


  if (CMD_IS("list")) {
    send_to_char(" ##   Player Corpses                    "
                 "                             Cost\r\n", ch);
    send_to_char("----------------------------------------"
                 "---------------------------------\r\n", ch);
    for (num = 0, k = object_list; k; k = k->next) {
      if (isname("corpse", k->name)			/* keyword corpse */
          && (k->in_room != ch->in_room)		/* not right here */
          && (k->in_room > 0)				/* and room exists */
          && (GET_OBJ_TYPE(k) == ITEM_CONTAINER)	/* is container */
          && (GET_OBJ_VAL(k, 3) == 1)			/* is a corpse */
          && (GET_OBJ_VAL(k, 2) == -2))	{		/* is player corpse */
        num++;
        sprintf(buf, "%3d)  %-52s%15d\r\n", num, k->short_description,
            GET_OBJ_COST(k));
        send_to_char(buf, ch);
      }
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {
    argument = one_argument(argument, buf);
    if (*buf == '#') {
      wanted = atoi(buf + 1) - 1;
      found = FALSE;
      for (num = 0, k = object_list; k; k = k->next) { 
        if (isname("corpse", k->name)
            && (k->in_room != ch->in_room)
            && (k->in_room > 0)
            && (GET_OBJ_TYPE(k) == ITEM_CONTAINER)
            && (GET_OBJ_VAL(k, 3) == 1)
            && (GET_OBJ_VAL(k, 2) == -2)) {
          if (num == wanted) {
            found = TRUE;
            if (!isname(GET_NAME(ch), k->name)) {
              send_to_char("That's not your corpse, you scoundrel!\r\n", ch);
              return(TRUE);
            }
            if (GET_GOLD(ch) < GET_OBJ_COST(k)) {
              send_to_char("You don't have enough gold!\r\n", ch);
              return(TRUE);
            }
            GET_GOLD(ch) -= GET_OBJ_COST(k);
            send_to_char("The Caretaker says, 'Unhold wiedererlangung!'\r\n"
                         "A ghoul has arrived.\r\n", ch);
            obj_from_room(k);
            obj_to_room(k, ch->in_room);
            sprintf(buf, "A ghoul drops %s.\r\nA ghoul says, 'meep.'\r\n"
                         "The Caretaker gives the ghoul a tasty bone.\r\n",
                         k->short_description);
            send_to_char(buf, ch);
            break;
          } else {	/* not this one ... */
            num++;
          }
        }
      }
      if (!found) {
        send_to_char("The Caretaker says, "
                     "'sorry but I dont have that one'\r\n", ch);
      }
      
    } else {

      send_to_char("The Caretaker tells you, 'Use 'list' to get a list "
                   "of corpses for sale\r\nand buy them by number, "
                   "as in 'buy #1'.\r\n"
                   "You can only buy your *own* corpse, obviously.\r\n", ch);
    }

    return 1;
  }

  /* All commands except list and buy */
  return 0;
}



SPECIAL(clan_atrium)
{
  int i;
  extern int clan_info[][3];
  extern char *clan_names[];

  if (!IS_MOVE(cmd))
    return FALSE;

  /* let uncharmed, unjarred mobs in */
  if (IS_MOB(ch) && !IS_AFFECTED(ch, AFF_CHARM) && !ch->desc)
    return FALSE;

  /* this code lets wandering monsters in - make the room beyond !mob
    if you dont want mobs to wander in */
  for (i = 0; clan_info[i][0] != -1; i++) {
    if (((GET_CLAN(ch) != clan_info[i][0]) || (IS_NPC(ch) && ch->desc)) &&
        world[ch->in_room].number == clan_info[i][1] &&
        cmd == clan_info[i][2]) {
      sprintf(buf, "The power of %s rises, and blocks your way.\r\n",
          clan_names[clan_info[i][0]]);
      send_to_char(buf, ch);
      sprintf(buf2, "The power of %s rises, and blocks $s way.",
          clan_names[clan_info[i][0]]);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  return FALSE;
}



/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */


SPECIAL(bank)
{
  int amount;
  int deposit;
  int withdrawl;


  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is %d coins.\r\n",
	      GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) -= amount;
    deposit = amount;                /* no deposit fee */
    GET_BANK_GOLD(ch) += deposit;
    sprintf(buf, "You deposit %d coins.\r\n", deposit);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins deposited!\r\n", ch);
      return 1;
    }
    GET_BANK_GOLD(ch) -= amount;
    withdrawl = amount * 0.95;
    GET_GOLD(ch) += withdrawl;
    sprintf(buf, "You withdraw %d coins (The bank took %d in fees).\r\n",
        withdrawl, amount - withdrawl);
    send_to_char(buf, ch);
    sprintf(buf, "You now have a balance of %d coins.\r\n", GET_BANK_GOLD(ch));
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else
    return 0;
}



SPECIAL(bank_expensive)
{
  int amount;
  int deposit;
  int withdrawl;


  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is %d coins.\r\n",
              GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) -= amount;
    deposit = amount * 0.98;
    GET_BANK_GOLD(ch) += deposit;
    sprintf(buf, "You deposit %d coins (The bank took %d in fees).\r\n",
        deposit, amount - deposit);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins deposited!\r\n", ch);
      return 1;
    }
    GET_BANK_GOLD(ch) -= amount;
    withdrawl = amount * 0.95;
    GET_GOLD(ch) += withdrawl;
    sprintf(buf, "You withdraw %d coins (The bank took %d in fees).\r\n",
        withdrawl, amount - withdrawl);
    send_to_char(buf, ch);
    sprintf(buf, "You now have a balance of %d coins.\r\n", GET_BANK_GOLD(ch));
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else
    return 0;
}



#define COKE_VNUM	1216
SPECIAL(pop_dispenser)
{
  struct obj_data *obj = me, *drink;


  if (CMD_IS("list")) {
    send_to_char("To buy a coke, type 'buy coke'.\r\n", ch);
    return (TRUE);
  }

  if (CMD_IS("buy")) {
    if (GET_GOLD(ch) < 25) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    } else {
      drink = read_object(COKE_VNUM, VIRTUAL); 
      obj_to_char(drink, ch);
      send_to_char("You insert your money into the machine\r\n",ch);
      GET_GOLD(ch) -= 25; /* coke costs 25 gold */
      act("$n gets a pop can from $p.", FALSE, ch, obj, 0, TO_ROOM);
      send_to_char("You get a pop can from the machine.\r\n",ch);
    }
    return 1;
  }

  return 0;
}
