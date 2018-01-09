/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"


char buf2[MAX_STRING_LENGTH];
char null_str[] = "";
struct char_data *script_list = NULL;

struct mobprog_var_data *mobprog_vars;

/* external variables */
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern char *clan_names[];
extern char *clan_levels[];
extern char *genders[];
extern char *pc_class_types[];
extern char *item_types[];
extern char *affected_bits[];
extern char *affected2_bits[];
extern char *pc_race_types[];
extern struct time_info_data time_info;


/* external functions */
void death_cry (struct char_data *ch);
bool str_prefix(const char *astr, const char *bstr);
int number_range(int from, int to);

void perform_mpbroadcast(struct char_data *ch, char *arg,
      struct char_data *actor, struct char_data *rndm, struct char_data *vict);

#define bug(x, y) { sprintf(buf2, (x), (y)); log(buf2); }


/*
 * Local function prototypes
 */

struct char_data * first_in_zone(int zone);
struct char_data * rndm_in_zone(int zone);
char * mprog_next_command(char* clist);
int mprog_seval(char* lhs, char* opr, char* rhs);
int mprog_veval(int lhs, char* opr, int rhs);
int mprog_do_ifchck(char* ifchck, struct char_data* mob,
	struct char_data* actor, struct obj_data* obj,
	void* vo, struct char_data* rndm);
char * mprog_process_if(char* ifchck, char* com_list, 
	struct char_data* mob, struct char_data* actor,
	struct obj_data* obj, void* vo,
	struct char_data* rndm);
void mprog_translate(char ch, char* t, struct char_data* mob,
	struct char_data* actor, struct obj_data* obj,
	void* vo, struct char_data* rndm);
void mprog_process_cmnd(char* cmnd, struct char_data* mob, 
	struct char_data* actor, struct obj_data* obj,
	void* vo, struct char_data* rndm);
void mprog_driver(char* com_list, struct char_data* mob,
	struct char_data* actor, struct obj_data* obj,
	void* vo, int self_triggered);
int is_scripting(struct char_data *mob);



/***************************************************************************
 * Local function code and brief comments.                                 *
 ***************************************************************************/

/*
 * first_in_zone returns the first character found in a zone
 * needed for randprogs so there's always a valid $n
 * based on is_empty() function
 * returns NULL if no one found
 */
struct char_data *first_in_zone(int zone_nr)
{
  struct descriptor_data *i;


  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr)
        return i->character;

  return NULL;
}



/*
 * rndm_in_zone returns the first character found in a zone
 * needed for randprogs so there's always a valid $r
 * based on is_empty() function
 * returns NULL if no one found
 * (for now this is just like first_in_zone)
 */
struct char_data *rndm_in_zone(int zone_nr)
{
  struct descriptor_data *i;
  struct char_data *ch = NULL;
  int count = 0;

 
  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr) {
        if (number_range(0, count) == 0)
          ch = i->character;
        count++;
      }

  return ch;
}



/* if you dont have these functions, you damn well should... */

#ifdef DUNNO_STRSTR
char *strstr(s1,s2)
const char *s1;
const char *s2;
{
  char *cp;
  int i, j = strlen(s1) - strlen(s2), k = strlen(s2);


  if (j < 0)
    return NULL;

  for (i = 0; i <= j && strncmp(s1++, s2, k) != 0; i++);

  return (i > j) ? NULL : (s1 - 1);
}
#endif



/*
 * Used to get sequential lines of a multi line string (separated by "\n\r")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */
char *mprog_next_command(char *clist)
{
  char *pointer = clist;


  while (*pointer != '\n' && *pointer != '\0')
    pointer++;
  if (*pointer == '\n')
    *pointer++ = '\0';
  if (*pointer == '\r')
    *pointer++ = '\0';

  return (pointer);
}



/* we need str_infix here because strstr is not case insensitive */

bool str_infix(const char *astr, const char *bstr)
{
  int sstr1;
  int sstr2;
  int ichar;
  char c0;


  if ((c0 = LOWER(astr[0])) == '\0')
    return FALSE;

  sstr1 = strlen(astr);
  sstr2 = strlen(bstr);

  for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
    if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar))
      return FALSE;
  }

  return TRUE;
}



/*
 * These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 * Additionally, rhs often has a '\r' on the end, so this has been
 * modified to trim the rhs side by one character
 */
int mprog_seval(char *lhs, char *opr, char *rhs)
{
  /* trim the right string so that "string\r" becomes "string" */
  rhs[strlen(rhs) - 1] = '\0';

  /*
   * now depending on the operator (opr), compare the left string
   * to the right string 
   */
  if (!str_cmp(opr, "=="))
    return (!str_cmp(lhs, rhs));
  if (!str_cmp(opr, "!="))
    return (!(!str_cmp(lhs, rhs)));
  if (!str_cmp(opr, "/"))
    return (!str_infix(rhs, lhs));
  if (!str_cmp(opr, "!/"))
    return (str_infix(rhs, lhs));

  bug("Improper MOBprog operator '%s'\n\r", opr);
  return 0;
}



int mprog_veval(int lhs, char *opr, int rhs)
{
  if (!str_cmp(opr, "=="))
    return (lhs == rhs);
  if (!str_cmp(opr, "!="))
    return (lhs != rhs);
  if (!str_cmp(opr, ">"))
    return (lhs > rhs);
  if (!str_cmp(opr, "<"))
    return (lhs < rhs);
  if (!str_cmp(opr, ">="))
    return (lhs <= rhs);
  if (!str_cmp(opr, ">="))
    return (lhs >= rhs);
  if (!str_cmp(opr, "&"))
    return (lhs & rhs);
  if (!str_cmp(opr, "|"))
    return (lhs | rhs);

  bug("Improper MOBprog operator '%s'\n\r", opr);
  return 0;
}


int get_var(char *varname) {
  struct mobprog_var_data *tmpvar;

  for (tmpvar = mobprog_vars; tmpvar; tmpvar = tmpvar->next)
    if (!str_cmp(varname, tmpvar->name)) break;

  if (!tmpvar) return 0;
  return tmpvar->val;
}


/*
 * This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return -1 otherwise return boolean 1,0
 */
int mprog_do_ifchck( char *ifchck, struct char_data *mob,
		struct char_data *actor,
		struct obj_data *obj, void *vo, struct char_data *rndm)
{
  char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  char opr[MAX_INPUT_LENGTH];
  char val[MAX_INPUT_LENGTH];
  struct char_data *vict = (struct char_data *) vo;
  struct obj_data *v_obj = (struct obj_data *) vo;
  char *bufpt = buf;
  char *argpt = arg;
  char *oprpt = opr;
  char *valpt = val;
  char *point = ifchck;
  int lhsvl;
  int rhsvl;
  int i;
  unsigned long affect;
  unsigned long affect2;
  struct char_data *tmp_char;
  int invert_opr;
  struct obj_data *tmp_obj;
  memory_rec *names; int targid;  /* These are for ismemory() */


  if (*point == '\0') {
    bug("Mob: %d null ifchck", (int) mob_index[mob->nr].virtual); 
    return -1;
  }   

  /* skip leading spaces */
  while (*point == ' ')
    point++;

  /* get whatever comes before the left paren.. ignore spaces */
  while (*point != '(') {
    if (*point == '\0') {
      bug("Mob: %d ifchck syntax error left paren scan",
              mob_index[mob->nr].virtual); 
      return -1;
    } else {
      if (*point == ' ')
	point++;
      else 
	*bufpt++ = *point++; 
    }
  }

  *bufpt = '\0';
  point++;

  /* get whatever is in between the parens.. ignore spaces */
  while (*point != ')') {
    if (*point == '\0') {
      bug("Mob: %d ifchck syntax error right paren scan",
              mob_index[mob->nr].virtual); 
      return -1;
    } else {
      if (*point == ' ')
	point++;
      else 
	*argpt++ = *point++; 
    }
  }

  *argpt = '\0';
  point++;

  /* check to see if there is an operator */
  while (*point == ' ')
    point++;
  if (*point == '\0') {
      *opr = '\0';
      *val = '\0';
  } else { /* there should be an operator and value, so get them */
    while ((*point != ' ') && (!isalnum(*point))) 
      if (*point == '\0') {
/* HACKED to skip this and continue, setting the pointers to nul bufs */
/*
        bug ( "Mob: %d ifchck operator without value",
                mob_index[mob->nr].virtual ); 
        return -1;
*/
        *opr = '\0';
        *val = '\0';
        goto set_pointers_to_bufs;
/* end of hack */
      } else {
        *oprpt++ = *point++; 
      }

    *oprpt = '\0';
 
    /* finished with operator, skip spaces and then get the value */
    while (*point == ' ')
      point++;
    for ( ; ; ) {
      if ((*point != ' ') && (*point == '\0'))
        break;
      else
        *valpt++ = *point++; 
    }

    *valpt = '\0';
  }

set_pointers_to_bufs:
  bufpt = buf;
  argpt = arg;
  oprpt = opr;
  valpt = val;

  /* Ok... now buf contains the ifchck, arg contains the inside of the
   *  parentheses, opr contains an operator if one is present, and val
   *  has the value if an operator was present.
   *  So.. basically use if statements and run over all known ifchecks
   *  Once inside, use the argument and expand the lhs. Then if need be
   *  send the lhs,opr,rhs off to be evaluated.
   */

  if (!str_cmp(buf, "rand")) {
    return (number(0, 99) < atoi(arg));
  }

  if (!str_cmp(buf, "timeis")) {
    return (time_info.hours == atoi(arg));
  }

  if (!str_cmp(buf, "ispc")) {
    switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return 0;
	case 'n': if (actor)
 	            return (!IS_NPC(actor));
	          else return -1;
	case 't': if (vict)
                    return (!IS_NPC(vict));
	          else return -1;
	case 'r': if (rndm)
                    return (!IS_NPC(rndm));
	          else return -1;
	default:
	    bug("Mob: %d bad argument to 'ispc'",
                    mob_index[mob->nr].virtual); 
            return -1;
    }
  }

  if (!str_cmp(buf, "isnpc")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return 1;
	case 'n': if (actor)
	             return IS_NPC(actor);
	          else return -1;
	case 't': if (vict)
                     return IS_NPC(vict);
	          else return -1;
	case 'r': if (rndm)
	             return IS_NPC(rndm);
	          else return -1;
	default:
	  bug("Mob: %d bad argument to 'isnpc'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "isjarred")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return 0;
	case 'n': if (actor)
	             return (actor->desc != NULL);
	          else return -1;
	case 't': if (vict)
                     return (vict->desc != NULL);
	          else return -1;
	case 'r': if (rndm)
	             return (rndm->desc != NULL);
	          else return -1;
	default:
	  bug("Mob: %d bad argument to 'isjarred'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "isgood")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return IS_GOOD(mob);
	case 'n': if (actor)
	             return IS_GOOD(actor);
	          else return -1;
	case 't': if (vict)
	             return IS_GOOD(vict);
	          else return -1;
	case 'r': if (rndm)
	             return IS_GOOD(rndm);
	          else return -1;
	default:
	  bug("Mob: %d bad argument to 'isgood'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "isneutral")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return IS_NEUTRAL(mob);
        case 'n': if (actor)
                     return IS_NEUTRAL(actor);
                  else return -1;
        case 't': if (vict)
                     return IS_NEUTRAL(vict);
                  else return -1;
        case 'r': if (rndm)
                     return IS_NEUTRAL(rndm);
                  else return -1;
        default:
          bug("Mob: %d bad argument to 'isneutral'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  if (!str_cmp(buf, "isevil")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return IS_EVIL(mob);
        case 'n': if (actor)
                     return IS_EVIL(actor);
                  else return -1;
        case 't': if (vict)
                     return IS_EVIL(vict);
                  else return -1;
        case 'r': if (rndm)
                     return IS_EVIL(rndm);
                  else return -1;
        default:
          bug("Mob: %d bad argument to 'isevil'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  if (!str_cmp(buf, "ismemory")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return 0;
        case 'n': if (actor)
                    if (IS_NPC(actor)) return 0;
                    else {
                      targid = GET_IDNUM(actor);
                      break;
                    }
        case 't': if (vict)
                    if (IS_NPC(vict)) return 0;
                    else {
                      targid = GET_IDNUM(vict);
                      break;
                    }
        case 'r': if (rndm)
                    if (IS_NPC(rndm)) return 0;
                    else {
                      targid = GET_IDNUM(rndm);
                      break;
                    }
        default:
          bug("Mob: %d bad argument to 'ismemory'",
              mob_index[mob->nr].virtual);
          return -1;
      }

      /* Ok, targid is the person we're trying to trigger on */

      if (!MEMORY(mob)) return 0; /* I don't remember ANYONE */

      for (names = MEMORY(mob); names; names = names->next)
        if (names->id == targid) return 1;
      return 0;
  }

  if (!str_cmp(buf, "isfight")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return FIGHTING(mob) ? 1 : 0;
	case 'n': if (actor)
	             return FIGHTING(actor) ? 1 : 0;
	          else return -1;
	case 't': if (vict)
	             return FIGHTING(vict) ? 1 : 0;
	          else return -1;
	case 'r': if (rndm)
	             return FIGHTING(rndm) ? 1 : 0;
	          else return -1;
	default:
	  bug("Mob: %d bad argument to 'isfight'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "isscript")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return is_scripting(mob) ? 1 : 0;
        case 'n': if (actor)
                     return is_scripting(actor) ? 1 : 0;
                  else return -1;
        case 't': if (vict)
                     return is_scripting(vict) ? 1 : 0;
                  else return -1;
        case 'r': if (rndm)
                     return is_scripting(rndm) ? 1 : 0;
                  else return -1;
        default:
          bug("Mob: %d bad argument to 'isscript'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  if (!str_cmp(buf, "isimmort")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return (GET_LEVEL(mob) >= LVL_IMMORT);
	case 'n': if (actor)
	             return (GET_LEVEL(actor) >= LVL_IMMORT);
  	          else return -1;
	case 't': if (vict)
	             return (GET_LEVEL(vict) >= LVL_IMMORT);
                  else return -1;
	case 'r': if (rndm)
	             return (GET_LEVEL(rndm) >= LVL_IMMORT);
                  else return -1;
	default:
	  bug("Mob: %d bad argument to 'isimmort'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "isremort")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return (GET_RACE(mob) == RACE_UNDEAD ||
                          GET_RACE(mob) == RACE_DRAGON ||
                          GET_RACE(mob) == RACE_ANGEL ||
                          GET_RACE(mob) == RACE_THRIKREEN);
        case 'n': if (actor)
                     return (GET_RACE(actor) == RACE_UNDEAD ||
                             GET_RACE(actor) == RACE_DRAGON ||
                             GET_RACE(actor) == RACE_ANGEL ||
                             GET_RACE(actor) == RACE_THRIKREEN);
                  else return -1;
        case 't': if (vict)
                     return (GET_RACE(vict) == RACE_UNDEAD ||
                             GET_RACE(vict) == RACE_DRAGON ||
                             GET_RACE(vict) == RACE_ANGEL ||
                             GET_RACE(vict) == RACE_THRIKREEN);
                  else return -1;
        case 'r': if (rndm)
                     return (GET_RACE(rndm) == RACE_UNDEAD ||
                             GET_RACE(rndm) == RACE_DRAGON ||
                             GET_RACE(rndm) == RACE_ANGEL ||
                             GET_RACE(rndm) == RACE_THRIKREEN);
                  else return -1;
        default:
          bug("Mob: %d bad argument to 'isremort'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  if (!str_cmp(buf, "ischarmed")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return IS_AFFECTED(mob, AFF_CHARM);
	case 'n': if (actor)
	             return IS_AFFECTED(actor, AFF_CHARM);
	          else return -1;
	case 't': if (vict)
	             return IS_AFFECTED(vict, AFF_CHARM);
	          else return -1;
	case 'r': if (rndm)
	             return IS_AFFECTED(rndm, AFF_CHARM);
	          else return -1;
	default:
	  bug("Mob: %d bad argument to 'ischarmed'",
	      mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "ispoisoned")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return IS_AFFECTED(mob, AFF_POISON);
        case 'n': if (actor)
                     return IS_AFFECTED(actor, AFF_POISON);
                  else return -1;
        case 't': if (vict)
                     return IS_AFFECTED(vict, AFF_POISON);
                  else return -1;
        case 'r': if (rndm)
                     return IS_AFFECTED(rndm, AFF_POISON);
                  else return -1;
        default:
          bug("Mob: %d bad argument to 'ispoisoned'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  /*
   * isaffected is unusual in that it takes two arguments:
   * a target and an affect name...
   * arg should come in the form "$n,<affect>"
   * like: if isaffected($n, blind)
   *         [do stuff]
   *       endif
   */
  if (!str_cmp(buf, "isaffected")) {

      /* store the name of the affect in buf2 */
      if ((arg[2] == ',') && (strlen(arg) > 3)) {
        strcpy(buf2, arg + 3);
      } else {
        bug("Mob: %d badly formed arguments to 'isaffected'",
            mob_index[mob->nr].virtual);
        return -1;
      }

      /* find the affect */
      affect = 0;
      affect2 = 0;
      for (i = 0; *(affected_bits[i]) != '\n'; i++)
        if (!strncasecmp(buf2, affected_bits[i], strlen(buf2))) {
          affect = (1 << i);
          break;
        }
      for (i = 0; *(affected2_bits[i]) != '\n'; i++)
        if (!strncasecmp(buf2, affected2_bits[i], strlen(buf2))) {
          affect2 = (1 << i);
          break;
        }
      if ((affect == 0) && (affect2 == 0)) {
        bug("Mob: %d unknown affect to 'isaffected'",
            mob_index[mob->nr].virtual);
        return -1;
      } 

      /* see if the target is affected */
      switch (arg[1]) {
        case 'i': if (affect) {
	    return IS_AFFECTED(mob, affect);
	} else {
	    return IS_AFFECTED2(mob, affect2);
	}
        case 'n': if (actor) {
	    if (affect) {
		return IS_AFFECTED(actor, affect);
	    } else {
		return IS_AFFECTED2(actor, affect2);
	    }
	} else {
	    return -1;
	}
        case 't': if (vict) {
	    if (affect) {
		return IS_AFFECTED(vict, affect);
	    } else {
		return IS_AFFECTED2(actor, affect2);
	    }
	} else {
	    return -1;
	}
        case 'r': if (rndm) {
	    if (affect) {
		return IS_AFFECTED(rndm, affect);
	    } else {
		return IS_AFFECTED2(rndm, affect2);
	    }
	} else {
	    return -1;
	}
        default:
          bug("Mob: %d bad argument to 'isaffected'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  if (!str_cmp(buf, "isfollow")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return (mob->master != NULL
			  && mob->master->in_room == mob->in_room);
	case 'n': if (actor)
	             return (actor->master != NULL
			     && actor->master->in_room == actor->in_room);
	          else return -1;
	case 't': if (vict)
	             return (vict->master != NULL
			     && vict->master->in_room == vict->in_room);
	          else return -1;
	case 'r': if (rndm)
	             return (rndm->master != NULL
			     && rndm->master->in_room == rndm->in_room);
	          else return -1;
	default:
	  bug("Mob: %d bad argument to 'isfollow'", 
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "isaffected")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return (AFF_FLAGGED(mob, atoi(arg)));
	case 'n': if (actor)
                     return (AFF_FLAGGED(actor, atoi(arg)));
	          else return -1;
	case 't': if (vict)
                     return (AFF_FLAGGED(vict, atoi(arg)));
	          else return -1;
	case 'r': if (rndm)
                     return (AFF_FLAGGED(rndm, atoi(arg)));
	          else return -1;
	default:
	  bug("Mob: %d bad argument to 'isaffected'",
	      mob_index[mob->nr].virtual); 
	  return -1;
      }
  }
  
  if (!strcmp(buf, "isanycharhere")) {
    for (tmp_char = world[mob->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
      if (!IS_NPC(tmp_char)) return 1;
    return 0;
  }

  if (!strcmp(buf, "isanycharat")) {
    i = real_room(atoi(arg));
    if (i == NOWHERE) {
      bug ("Mob %d: bad room vnum to isanycharat", mob_index[mob->nr].virtual);
      return -1;
    }
    for (tmp_char = world[i].people; tmp_char; tmp_char = tmp_char->next)
      if (!IS_NPC(tmp_char)) return 1;
    return 0;
  }
      
  if (!str_cmp(buf, "ischarhere")) {
      if (arg[0] == '$') {
          bug("Mob: %d bad argument to 'ischarhere' no $*",
              mob_index[mob->nr].virtual);
          return -1;
      }
      if (!*arg) {
          bug("Mob: %d empty argument to 'ischarhere'",
              mob_index[mob->nr].virtual);
          return -1;
      }
      if ((tmp_char = get_char_room(arg, mob->in_room)) != NULL)
          return 1;
      else
          return 0;
  }

  if (!str_cmp(buf, "isanycharhere")) {
      if (arg[0] == '$') {
          bug("Mob: %d bad argument to 'isanycharhere' no $*",
              mob_index[mob->nr].virtual);
          return -1;
      }
      if (!*arg) {
          bug("Mob: %d empty argument to 'isanycharhere'",
              mob_index[mob->nr].virtual);
          return -1;
      }
      if ((tmp_char = get_char_room(arg, mob->in_room)) != NULL)
          return 1;
      else
          return 0;
  }

  if (!str_cmp(buf, "hitprcnt")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = 100 * mob->points.hit / mob->points.max_hit;
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
		    lhsvl = 100 * actor->points.hit / actor->points.max_hit;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
		    lhsvl = 100 * vict->points.hit / vict->points.max_hit;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = 100 * rndm->points.hit / rndm->points.max_hit;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'hitprcnt'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  /*
   * as usual, val has got an extra \r tacked onto the end, so that had
   * to be taken into account and "here" becomes "here\r"
   * lousy progs
   * future ideas for expansion: add in a "nearby\r" check too
   */
  if (!str_cmp(buf, "inroom")) {

    if (!str_cmp(val, "here\r")) {
      rhsvl = mob->in_room;
    } else {
      rhsvl = real_room(atoi(val));
      if (rhsvl == NOWHERE) {
        bug("Mob: %d nonexistant room passed to 'inroom'",
            mob_index[mob->nr].virtual);
        return -1;
      }
    }

    switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = mob->in_room;
	          return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
		    lhsvl = actor->in_room;
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
		    lhsvl = vict->in_room;
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = rndm->in_room;
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'inroom'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "inzone")) {

    if (!str_cmp(val, "here\r")) {
      rhsvl = world[mob->in_room].number / 100;
    } else {
      rhsvl = atoi(val);
      if (rhsvl == NOWHERE) {
        bug("Mob: %d nonexistant zone passed to 'inzone'",
            mob_index[mob->nr].virtual);
        return -1;
      }
    }

    switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = world[mob->in_room].number / 100;
	          return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
		    lhsvl = world[actor->in_room].number / 100;
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
		    lhsvl = world[vict->in_room].number / 100;
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = world[rndm->in_room].number / 100;
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'inzone'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "sex")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return mprog_seval(genders[(int) GET_SEX(mob)], opr, val);
	case 'n': if (actor)
		    return mprog_seval(genders[(int) GET_SEX(actor)], opr, val);
		  else 
		    return -1;
	case 't': if (vict)
		    return mprog_seval(genders[(int) GET_SEX(vict)], opr, val);
	          else
		    return -1;
	case 'r': if (rndm)
		    return mprog_seval(genders[(int) GET_SEX(rndm)], opr, val);
	          else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'sex'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "position")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_POS(mob);
	          rhsvl = atoi(val);
	          return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
		    lhsvl = GET_POS(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
		    lhsvl = GET_POS(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_POS(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'position'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "level")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_LEVEL(mob);
	          rhsvl = atoi(val);
	          return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
		    lhsvl = GET_LEVEL(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
		    lhsvl = GET_LEVEL(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_LEVEL(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'level'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }


  if (!str_cmp(buf, "class")) {
      switch (arg[1]) {
	case 'i': return mprog_seval(pc_class_types[(int) GET_CLASS(mob)],
	                     opr, val);
	case 'n': if (actor)
		    return mprog_seval(pc_class_types[(int) GET_CLASS(actor)],
                               opr, val);
	          else 
		    return -1;
	case 't': if (vict)
		    return mprog_seval(pc_class_types[(int) GET_CLASS(vict)],
                               opr, val);
	          else
		    return -1;
	case 'r': if (rndm)
		    return mprog_seval(pc_class_types[(int) GET_CLASS(rndm)],
                               opr, val);
	          else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'class'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "race")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': if (GET_RACE(mob) == RACE_UNDEFINED)
                    strcpy(buf2, "undefined");
                  else
                    strcpy(buf2, pc_race_types[(int) GET_RACE(mob)]);
                  return mprog_seval(buf2, opr, val);
        case 'n': if (actor) {
                    if (GET_RACE(actor) == RACE_UNDEFINED)
                      strcpy(buf2, "undefined");
                    else
                      strcpy(buf2, pc_race_types[(int) GET_RACE(actor)]);
                    return mprog_seval(buf2, opr, val);
                  } else
                    return -1;
        case 't': if (vict) {
                    if (GET_RACE(vict) == RACE_UNDEFINED)
                      strcpy(buf2, "undefined");
                    else
                      strcpy(buf2, pc_race_types[(int) GET_RACE(vict)]);
                    return mprog_seval(buf2, opr, val);
                  } else
                    return -1;
        case 'r': if (rndm) {
                   if (GET_RACE(rndm) == RACE_UNDEFINED)
                      strcpy(buf2, "undefined");
                    else
                      strcpy(buf2, pc_race_types[(int) GET_RACE(rndm)]);
                    return mprog_seval(buf2, opr, val);
                  } else
                    return -1;
        default:
          bug("Mob: %d bad argument to 'race'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  if (!str_cmp(buf, "goldamt")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = mob->points.gold;
                  rhsvl = atoi(val);
                  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
		    lhsvl = actor->points.gold;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
		    lhsvl = vict->points.gold;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = rndm->points.gold;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'goldamt'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "objtype")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'o': if (obj)
		    return mprog_seval(item_types[(int) GET_OBJ_TYPE(obj)],
                               opr, val);
	          else
		   return -1;
	case 'p': if (v_obj)
		    return mprog_seval(item_types[(int) GET_OBJ_TYPE(v_obj)],
                               opr, val);
	          else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'objtype'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "objval0")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'o': if (obj) {
		    lhsvl = obj->obj_flags.value[0];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'p': if (v_obj) {
		    lhsvl = v_obj->obj_flags.value[0];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'objval0'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "objval1")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'o': if (obj) {
		    lhsvl = obj->obj_flags.value[1];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'p': if (v_obj) {
		    lhsvl = v_obj->obj_flags.value[1];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'objval1'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "objval2")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'o': if (obj) {
		    lhsvl = obj->obj_flags.value[2];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'p': if (v_obj) {
		    lhsvl = v_obj->obj_flags.value[2];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'objval2'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "objval3")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'o': if (obj) {
		    lhsvl = obj->obj_flags.value[3];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'p': if (v_obj) {
		    lhsvl = v_obj->obj_flags.value[3];
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'objval3'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "number")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = mob->points.gold;
	          rhsvl = atoi(val);
	          return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
		    if IS_NPC(actor) {
		      lhsvl = mob_index[actor->nr].virtual;
		      rhsvl = atoi(val);
		      return mprog_veval(lhsvl, opr, rhsvl);
		    }
		  } else
		    return -1;
	case 't': if (vict) {
		    if IS_NPC(actor) {
		      lhsvl = mob_index[vict->nr].virtual;
		      rhsvl = atoi(val);
		      return mprog_veval(lhsvl, opr, rhsvl);
		    }
		  } else
		    return -1;
	case 'r': if (rndm) {
		    if IS_NPC(actor) {
		      lhsvl = mob_index[rndm->nr].virtual;
		      rhsvl = atoi(val);
		      return mprog_veval(lhsvl, opr, rhsvl);
		    }
		  } else
	            return -1;
	case 'o': if (obj) {
		    lhsvl = obj_index[obj->item_number].virtual;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'p': if (v_obj) {
		    lhsvl = obj_index[v_obj->item_number].virtual;
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'number'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "name")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': return mprog_seval(mob->player.name, opr, val);
	case 'n': if (actor)
	            return mprog_seval(actor->player.name, opr, val);
	          else
		    return -1;
	case 't': if (vict)
	            return mprog_seval(vict->player.name, opr, val);
	          else
		    return -1;
	case 'r': if (rndm)
	            return mprog_seval(rndm->player.name, opr, val);
	          else
		    return -1;
	case 'o': if (obj)
	            return mprog_seval(obj->name, opr, val);
	          else
		    return -1;
	case 'p': if (v_obj)
	            return mprog_seval(v_obj->name, opr, val);
	          else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'name'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "clan")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return -1;
        case 'n': if (actor)
                    if (!IS_NPC(actor))
                      return mprog_seval(clan_names[(int) GET_CLAN(actor)],
                                 opr, val);
                  else
                    return -1;
        case 't': if (vict)
                    if (!IS_NPC(vict))
                      return mprog_seval(clan_names[(int) GET_CLAN(vict)],
                                 opr, val);
                  else
                    return -1;
        case 'r': if (rndm)
                    if (!IS_NPC(rndm))
                      return mprog_seval(clan_names[(int) GET_CLAN(rndm)],
                                 opr, val);
                  else
                    return -1;
        default:
          bug("Mob: %d bad argument to 'clan'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  if (!str_cmp(buf, "clanlevel")) {
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': return -1;
        case 'n': if (actor)
                    if (!IS_NPC(actor))
                      return
                          mprog_seval(clan_levels[(int) GET_CLAN_LEVEL(actor)],
                              opr, val);
                  else
                    return -1;
        case 't': if (vict)
                    if (!IS_NPC(vict))
                      return
                          mprog_seval(clan_levels[(int) GET_CLAN_LEVEL(vict)],
                              opr, val);
                  else
                    return -1;
        case 'r': if (rndm)
                    if (!IS_NPC(rndm))
                      return
                          mprog_seval(clan_levels[(int) GET_CLAN_LEVEL(rndm)],
                              opr, val);
                  else
                    return -1;
        default:
          bug("Mob: %d bad argument to 'clanlevel'",
              mob_index[mob->nr].virtual);
          return -1;
      }
  }

  /*
   * for hasworn, it checks to see if the target is wearing a
   * certain piece of eq.  The only meaningful operators for the
   * check are == and !=.  (The others will work, they'll just return
   * garbage)  The value must be the vnum of the object.
   * (Cant use a name)
   */
  if (!str_cmp(buf, "hasworn")) {
      if (!str_cmp(opr, "!="))
        invert_opr = 1;
      else
        invert_opr = 0;

      /* trim the '\r' off the end of val */
      if (strlen(val) > 0)
        val[strlen(val)-1] = '\0';

      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': tmp_obj = get_object_in_equip_vis(mob, val, mob->equipment, &i);
                  break;
        case 'n': if (actor)
                    tmp_obj = get_object_in_equip_vis(actor, val, actor->equipment, &i);
                  else
                    return -1;
                  break;
        case 't': if (vict)
                    tmp_obj = get_object_in_equip_vis(vict, val, vict->equipment, &i);
                  else
                    return -1;
                  break;
        case 'r': if (rndm)
                    tmp_obj = get_object_in_equip_vis(rndm, val, rndm->equipment, &i);
                  else
                    return -1;
                  break;
        default:
          bug("Mob: %d bad argument to 'hasworn'",
              mob_index[mob->nr].virtual);
          return -1;
          break;
      }
      if (tmp_obj == NULL) {
        if (invert_opr)
          return 1;
        else
          return 0;
      } else {
        if (invert_opr)
          return 0;
        else
          return 1;
      }
  }

  /*
   * for hasobj, it checks to see if the target has a certain piece of eq
   * in inventory.  The only meaningful operators for the
   * check are == and !=.  (The others will work, they'll just return
   * garbage)
   * Unlike hasworn, the value can be a word and not just a vnum.
   */
  if (!str_cmp(buf, "hasobj")) {
      if (!str_cmp(opr, "!="))
        invert_opr = 1;
      else
        invert_opr = 0;

      /* trim the '\r' off the end of val */
      if (strlen(val) > 0)
        val[strlen(val)-1] = '\0';

      switch (arg[1]) { /* arg should be "$*" so just get the letter */
        case 'i': tmp_obj = get_obj_in_list_vis(mob, val, mob->carrying);
                  break;
        case 'n': if (actor)
                    tmp_obj = get_obj_in_list_vis(actor, val, actor->carrying);
                  else
                    return -1;
                  break;
        case 't': if (vict)
                    tmp_obj = get_obj_in_list_vis(vict, val, vict->carrying);
                  else
                    return -1;
                  break;
        case 'r': if (rndm)
                    tmp_obj = get_obj_in_list_vis(rndm, val, rndm->carrying);
                  else
                    return -1;
                  break;
        default:
          bug("Mob: %d bad argument to 'hasobj'",
              mob_index[mob->nr].virtual);
          return -1;
          break;
      }
      if (tmp_obj == NULL) {
        if (invert_opr)
          return 1;
        else
          return 0;
      } else {
        if (invert_opr)
          return 0;
        else
          return 1;
      }
  }


  if (!str_cmp(buf, "int")) { 
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_INT(mob);
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
	            lhsvl = GET_INT(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
	            lhsvl = GET_INT(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_INT(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'int'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "wis")) { 
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_WIS(mob);
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
	            lhsvl = GET_WIS(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
	            lhsvl = GET_WIS(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_WIS(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'wis'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "cha")) { 
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_CHA(mob);
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
	            lhsvl = GET_CHA(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
	            lhsvl = GET_CHA(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_CHA(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'cha'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "str")) { 
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_STR(mob);
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
	            lhsvl = GET_STR(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
	            lhsvl = GET_STR(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_STR(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'str'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "con")) { 
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_CON(mob);
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
	            lhsvl = GET_CON(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
	            lhsvl = GET_CON(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_CON(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'con'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "dex")) { 
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_DEX(mob);
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
	            lhsvl = GET_DEX(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
	            lhsvl = GET_DEX(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_DEX(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'dex'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }

  if (!str_cmp(buf, "stradd")) { 
      switch (arg[1]) { /* arg should be "$*" so just get the letter */
	case 'i': lhsvl = GET_ADD(mob);
	          rhsvl = atoi(val);
         	  return mprog_veval(lhsvl, opr, rhsvl);
	case 'n': if (actor) {
	            lhsvl = GET_ADD(actor);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 't': if (vict) {
	            lhsvl = GET_ADD(vict);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	case 'r': if (rndm) {
		    lhsvl = GET_ADD(rndm);
		    rhsvl = atoi(val);
		    return mprog_veval(lhsvl, opr, rhsvl);
		  } else
		    return -1;
	default:
	  bug("Mob: %d bad argument to 'stradd'",
              mob_index[mob->nr].virtual); 
	  return -1;
      }
  }
  if (!str_cmp(buf, "var")) { 
    if (!*arg) {
      bug("Mob: %d bad argument to 'var'", mob_index[mob->nr].virtual); 
      return -1;
    }
    lhsvl = get_var(arg);
    rhsvl = atoi(val);
    return mprog_veval(lhsvl, opr, rhsvl);
  }


  /*
   * Ok... all the ifchcks are done, so if we didnt find ours then something
   * odd happened.  So report the bug and abort the MOBprogram (return error)
   */
  bug("Mob: %d unknown ifchck", mob_index[mob->nr].virtual);
  return -1;
}



/*
 * Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */
char *mprog_process_if(char *ifchck, char *com_list, struct char_data *mob,
		       struct char_data *actor, struct obj_data *obj, void *vo,
		       struct char_data *rndm)
{
  char buf[MAX_INPUT_LENGTH];
  char *morebuf = '\0';
  char *cmnd = '\0';
  int loopdone = FALSE;
  int flag = FALSE;
  int legal;
  int nest;


  /* check for trueness of the ifcheck */
  if ((legal = mprog_do_ifchck(ifchck, mob, actor, obj, vo, rndm))) {
    if (legal > 0) {
      flag = TRUE;
    } else {
      return null_str;
    }
  }

 while( loopdone == FALSE ) /*scan over any existing or statements */
 {
     cmnd     = com_list;
     com_list = mprog_next_command( com_list );
     while ( *cmnd == ' ' )
       cmnd++;
     if ( *cmnd == '\0' )
     {
	 bug ( "Mob: %d no commands after IF/OR", mob_index[mob->nr].virtual ); 
         return null_str;
     }
     morebuf = one_argument( cmnd, buf );
     if ( !str_cmp( buf, "or" ) )
     {
	 if ( ( legal = mprog_do_ifchck( morebuf,mob,actor,obj,vo,rndm ) ) )
	   if ( legal > 0 )
	     flag = TRUE;
	   else
             return null_str;
     }
     else
       loopdone = TRUE;
 }
 
 if ( flag )
   for ( ; ; ) /*ifcheck was true, do commands but ignore else to endif*/ 
   {
       if ( !str_cmp( buf, "if" ) )
       { 
	   com_list = mprog_process_if(morebuf,com_list,mob,actor,obj,vo,rndm);
	   while ( *cmnd==' ' )
	     cmnd++;
	   if ( *com_list == '\0' )
             return null_str;
	   cmnd     = com_list;
	   com_list = mprog_next_command( com_list );
	   morebuf  = one_argument( cmnd,buf );
	   continue;
       }
       if ( !str_cmp( buf, "break" ) )
	 return null_str;
       if ( !str_cmp( buf, "endif" ) )
	 return com_list; 
       if ( !str_cmp( buf, "else" ) ) 
       {
	   while ( str_cmp( buf, "endif" ) ) 
	   {
	       cmnd     = com_list;
	       com_list = mprog_next_command( com_list );
	       while ( *cmnd == ' ' )
		 cmnd++;
	       if ( *cmnd == '\0' )
	       {
		   bug ( "Mob: %d missing endif after else",
			mob_index[mob->nr].virtual );
                   return null_str;
	       }
	       morebuf = one_argument( cmnd,buf );
	   }
	   return com_list; 
       }
       mprog_process_cmnd( cmnd, mob, actor, obj, vo, rndm );
       cmnd     = com_list;
       com_list = mprog_next_command( com_list );
       while ( *cmnd == ' ' )
	 cmnd++;
       if ( *cmnd == '\0' )
       {
           bug ( "Mob: %d missing else or endif", mob_index[mob->nr].virtual ); 
           return null_str;
       }
       morebuf = one_argument( cmnd, buf );
   }
 else /*false ifcheck, find else and do existing commands or quit at endif*/
   {
/* HACKED to find the right nesting level */
/* old code
     while ( ( str_cmp( buf, "else" ) ) && ( str_cmp( buf, "endif" ) ) )
       {
	 cmnd     = com_list;
	 com_list = mprog_next_command( com_list );
	 while ( *cmnd == ' ' )
	   cmnd++;
	 if ( *cmnd == '\0' )
	   {
	     bug ( "Mob: %d missing an else or endif",
		  mob_index[mob->nr].virtual ); 
             return null_str;
	   }
	 morebuf = one_argument( cmnd, buf );
       }
*/
     nest = 0;
     while (1) {
       if (!str_cmp(buf, "else")) {
         if (nest == 0)
           break;
       }
       if (!str_cmp(buf, "endif")) {
         if (nest == 0)
           break;
         else
           nest--;
       }
       if (!str_cmp(buf, "if"))
         nest++;
       cmnd = com_list;
       com_list = mprog_next_command(com_list);
       while (*cmnd == ' ')
         cmnd++;
       if (*cmnd == '\0') {
         bug ("Mob: %d missing an else or endif", mob_index[mob->nr].virtual);
         return null_str;
       }
       morebuf = one_argument(cmnd, buf);
     }
/* end of hack */

     /* found either an else or an endif.. act accordingly */
     if ( !str_cmp( buf, "endif" ) )
       return com_list;
     cmnd     = com_list;
     com_list = mprog_next_command( com_list );
     while ( *cmnd == ' ' )
       cmnd++;
     if ( *cmnd == '\0' )
       { 
	 bug ( "Mob: %d missing endif", mob_index[mob->nr].virtual ); 
         return null_str;
       }
     morebuf = one_argument( cmnd, buf );
     
     for ( ; ; ) /*process the post-else commands until an endif is found.*/
       {
	 if ( !str_cmp( buf, "if" ) )
	   { 
	     com_list = mprog_process_if( morebuf, com_list, mob, actor,
					 obj, vo, rndm );
	     while ( *cmnd == ' ' )
	       cmnd++;
	     if ( *com_list == '\0' )
               return null_str;
	     cmnd     = com_list;
	     com_list = mprog_next_command( com_list );
	     morebuf  = one_argument( cmnd,buf );
	     continue;
	   }
	 if ( !str_cmp( buf, "else" ) ) 
	   {
/*	     bug ( "Mob: %d found else in an else section",
		  mob_index[mob->nr].virtual ); */
             return null_str;
	   }
	 if ( !str_cmp( buf, "break" ) )
           return null_str;
	 if ( !str_cmp( buf, "endif" ) )
	   return com_list; 
	 mprog_process_cmnd( cmnd, mob, actor, obj, vo, rndm );
	 cmnd     = com_list;
	 com_list = mprog_next_command( com_list );
	 while ( *cmnd == ' ' )
	   cmnd++;
	 if ( *cmnd == '\0' )
	   {
	     bug ( "Mob:%d missing endif in else section",
		  mob_index[mob->nr].virtual ); 
             return null_str;
	   }
	 morebuf = one_argument( cmnd, buf );
       }
   }
}



/*
 * This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object 
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
void mprog_translate( char ch, char *t, struct char_data *mob,
		struct char_data *actor,
		struct obj_data *obj, void *vo, struct char_data *rndm )
{
 struct char_data   *vict             = (struct char_data *) vo;
 struct obj_data    *v_obj            = (struct obj_data  *) vo;

 *t = '\0';
 switch ( ch ) {
     case 'i':
         one_argument( mob->player.name, t );
      break;

     case 'I':
         strcpy( t, mob->player.short_descr );
      break;

     case 'n':
         if ( actor ) {
	   if ( CAN_SEE( mob,actor ) ) {
             if ( !IS_NPC( actor ) ) {
               strcpy(t, actor->player.name);
             } else
	       one_argument( actor->player.name, t );
           } else
             strcpy(t, "Someone");
         }
      break;

     case 'N':
         if ( actor ) 
            if ( CAN_SEE( mob, actor ) )
	       if ( IS_NPC( actor ) )
		 strcpy( t, actor->player.short_descr );
	       else
	       {
/* HACKED to not give titles */
		   strcpy( t, actor->player.name );
/*
		   strcat( t, " " );
		   strcat( t, actor->player.title );
*/
/* end of hack */
	       }
	    else
	      strcpy( t, "someone" );
	 break;

     case 't':
         if ( vict ) {
	   if ( CAN_SEE( mob, vict ) ) {
             if ( !IS_NPC( vict ) )
               strcpy(t, vict->player.name);
             else
	       one_argument( vict->player.name, t );
           } else
             strcpy(t, "Someone");
         }
	 break;

     case 'T':
         if ( vict ) 
            if ( CAN_SEE( mob, vict ) )
	       if ( IS_NPC( vict ) )
		 strcpy( t, vict->player.short_descr );
	       else
	       {
		 strcpy( t, vict->player.name );
/* HACKED to not give titles */
/*
		 strcat( t, " " );
		 strcat( t, vict->player.title );
*/
/* end of hack */
	       }
	    else
	      strcpy( t, "someone" );
	 break;
     
     case 'r':
         if ( rndm ) {
	   if ( CAN_SEE( mob, rndm ) ) {
             if ( !IS_NPC( rndm ) )
               strcpy(t, rndm->player.name);
	     else
               one_argument( rndm->player.name, t );
           } else
             strcpy(t, "Someone");
         }
      break;

     case 'R':
         if ( rndm ) 
            if ( CAN_SEE( mob, rndm ) )
	       if ( IS_NPC( rndm ) )
		 strcpy(t,rndm->player.short_descr);
	       else
	       {
		 strcpy( t, rndm->player.name );
/* HACKED to not give titles */
/*
		 strcat( t, " " );
		 strcat( t, rndm->player.title );
*/
/* end of hack */
	       }
	    else
	      strcpy( t, "someone" );
	 break;

     case 'e':
         if ( actor )
	   CAN_SEE( mob, actor ) ? strcpy( t, HSSH(actor) )
	                         : strcpy( t, "someone" );
	 break;
  
     case 'm':
         if ( actor )
	   CAN_SEE( mob, actor ) ? strcpy( t, HMHR(actor) )
                                 : strcpy( t, "someone" );
	 break;
  
     case 's':
         if ( actor )
	   CAN_SEE( mob, actor ) ? strcpy( t, HSHR(actor) )
	                         : strcpy( t, "someone's" );
	 break;
     
     case 'E':
         if ( vict )
	   CAN_SEE( mob, vict ) ? strcpy( t, HSSH(vict) )
                                : strcpy( t, "someone" );
	 break;
  
     case 'M':
         if ( vict )
	   CAN_SEE( mob, vict ) ? strcpy( t, HMHR(vict) )
                                : strcpy( t, "someone" );
	 break;
  
     case 'S':
         if ( vict )
	   CAN_SEE( mob, vict ) ? strcpy( t, HSHR(vict) )
                                : strcpy( t, "someone's" ); 
	 break;

     case 'j':
	 strcpy( t, HSSH(mob) );
	 break;
  
     case 'k':
	 strcpy( t, HMHR(mob) );
	 break;
  
     case 'l':
	 strcpy( t, HSHR(mob) );
	 break;

     case 'J':
         if ( rndm )
	   CAN_SEE( mob, rndm ) ? strcpy( t, HSSH(rndm) )
	                        : strcpy( t, "someone" );
	 break;
  
     case 'K':
         if ( rndm )
	   CAN_SEE( mob, rndm ) ? strcpy( t, HMHR(rndm) )
                                : strcpy( t, "someone" );
	 break;
  
     case 'L':
         if ( rndm )
	   CAN_SEE( mob, rndm ) ? strcpy( t, HSHR(rndm) )
	                        : strcpy( t, "someone's" );
	 break;

     case 'o':
         if ( obj )
	   CAN_SEE_OBJ( mob, obj ) ? one_argument( obj->name, t )
                                   : strcpy( t, "something" );
	 break;

     case 'O':
         if ( obj )
	   CAN_SEE_OBJ( mob, obj ) ? strcpy( t, obj->short_description )
                                   : strcpy( t, "something" );
	 break;

     case 'p':
         if ( v_obj )
	   CAN_SEE_OBJ( mob, v_obj ) ? one_argument( v_obj->name, t )
                                     : strcpy( t, "something" );
	 break;

     case 'P':
         if ( v_obj )
	   CAN_SEE_OBJ( mob, v_obj ) ? strcpy( t, v_obj->short_description )
                                     : strcpy( t, "something" );
      break;

     case 'a':
         if ( obj ) 
          switch ( *( obj->name ) )
	  {
	    case 'a': case 'e': case 'i':
            case 'o': case 'u': strcpy( t, "an" );
	      break;
            default: strcpy( t, "a" );
          }
	 break;

     case 'A':
         if ( v_obj ) 
          switch ( *( v_obj->name ) )
	  {
            case 'a': case 'e': case 'i':
	    case 'o': case 'u': strcpy( t, "an" );
	      break;
            default: strcpy( t, "a" );
          }
	 break;

     case '$':
         strcpy( t, "$" );
	 break;

     default:
         bug( "Mob: %d bad $var", mob_index[mob->nr].virtual );
	 break;
       }

 return;

}



/*
 * This procedure simply copies the cmnd to a buffer while expanding
 * any variables by calling the translate procedure.  The observant
 * code scrutinizer will notice that this is taken from act()
 */
/* HACKED by Culvan to handle mpbroadcast specially : Instead of the $*
 * being translated to a name, and then using get_char_vis, the pointer
 * is passed directly to perform_mpbroadcast. Note that because of the way
 * this works, you can NOT abbreviate mpbroadcast!
 */
void mprog_process_cmnd( char *cmnd, struct char_data *mob,
	struct char_data *actor,
	struct obj_data *obj, void *vo, struct char_data *rndm )
{
  char buf[ MAX_INPUT_LENGTH ];
  char tmp[ MAX_INPUT_LENGTH ];
  char *str;
  char *i;
  char *point;
  
  str = cmnd;
  skip_spaces(&str);
  if (!strn_cmp(str, "mpbroadcast ", 12)) {
    perform_mpbroadcast(mob, str + 12, actor, rndm, (struct char_data *)vo);
    return;
  }

  point   = buf;
  str     = cmnd;

  while ( *str != '\0' )
  {
    if ( *str != '$' )
    {
      *point++ = *str++;
      continue;
    }
    str++;
    mprog_translate( *str, tmp, mob, actor, obj, vo, rndm );
    i = tmp;
    ++str;
    while ( ( *point = *i ) != '\0' )
      ++point, ++i;
  }
  *point = '\0';

/* HACKED to take extra \r's off the end of buf before sending to 
  command interpreter cheesily, all I do is make the end \r into
  a \0.  I have two then, but I'm not keeping buf anyways. */

  buf[strlen(buf) - 1] = '\0';

/* end of hack */

  command_interpreter( mob, buf );

  return;
}



/*
 * The main focus of the MOBprograms.  This routine is called 
 * whenever a trigger is successful.  It is responsible for parsing
 * the command list and figuring out what to do. However, like all
 * complex procedures, everything is farmed out to the other guys.
 * the variable 'self_triggered' is usually 0, but is set to 1
 * for time_progs and load_progs.. it will try to fill in $n and $r variables,
 * but if it cant find anyone in the zone, the mob will use itself
 * as those targets..
 */
void mprog_driver ( char *com_list, struct char_data *mob,
		struct char_data *actor,
		struct obj_data *obj, void *vo,
		int self_triggered)
{
  char tmpcmndlst[MAX_STRING_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  char *morebuf;
  char *command_list;
  char *cmnd;
  struct char_data *rndm = NULL;
  struct char_data *vch = NULL;
  int count = 0;


  if (IS_AFFECTED(mob, AFF_CHARM) || (mob->desc))
    return;

  /* get a random visable mortal player who is in the room with the mob */
  /*
   * the check to exclude immortals is disabled and here's why:
   * if you are an immortal alone in a room with a mob you can easily
   * trigger a prog (its greet_prog for example) but the $r would be 
   * NULL so further commands crash the mud:
   * example: if isgood($r) say hi endif
   * would crash the mud if an immortal triggered it
   * if either actor or vch are null, make actor be the first visible
   * actor in that zone, and make rndm be a random actor in that zone
   */
  for ( vch = world[mob->in_room].people; vch; vch = vch->next_in_room )
    if ( !IS_NPC( vch )
/*      &&  vch->player.level < LVL_IMMORT   */
        &&  CAN_SEE( mob, vch ) )
      {
        if ( number_range( 0, count ) == 0 )
          rndm = vch;
        count++;
      }

  if (actor == NULL) {
    actor = first_in_zone(world[mob->in_room].zone);
    /* still null? bail! ... unless self-triggered ... */
    if (actor == NULL) {
      if (self_triggered)
        actor = mob;
      else 
        return;
    }
  }
  if (rndm == NULL) {
    rndm = rndm_in_zone(world[mob->in_room].zone);
    /* still null? bail! ... unless self-triggered ... */
    if (rndm == NULL) {
      if (self_triggered)
        rndm = mob;
      else
        return;
    }
  }
 
  strcpy(tmpcmndlst, com_list);
  command_list = tmpcmndlst;
  cmnd = command_list;
  command_list = mprog_next_command(command_list);
  while (*cmnd != '\0') {
    morebuf = one_argument(cmnd, buf);
    if (!str_cmp(buf, "if"))
      command_list = mprog_process_if(morebuf, command_list, mob,
                         actor, obj, vo, rndm);
    else
      mprog_process_cmnd(cmnd, mob, actor, obj, vo, rndm);
    cmnd = command_list;
    command_list = mprog_next_command(command_list);
  }

  return;
}



/***************************************************************************
 * Global function code and brief comments.                                *
 ***************************************************************************/

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
void mprog_wordlist_check( char *arg, struct char_data *mob, 
	struct char_data *actor, struct obj_data *obj, void *vo, int type )
{
  char        temp1[ MAX_STRING_LENGTH ];
  char        temp2[ MAX_INPUT_LENGTH ];
  char        word[ MAX_INPUT_LENGTH ];
  MPROG_DATA *mprg;
  char       *list;
  char       *start;
  char       *dupl;
  char       *end;
  int         i;

  for ( mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next )
    if ( mprg->type & type )
      {
	strcpy( temp1, mprg->arglist );
	list = temp1;
        while(isspace(*list)) list++;
	for ( i = 0; i < strlen( list ); i++ )
	  list[i] = LOWER( list[i] );
	strcpy( temp2, arg );
	dupl = temp2;
	for ( i = 0; i < strlen( dupl ); i++ )
	  dupl[i] = LOWER( dupl[i] );
	if ( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
	  {
	    list += 2;
	    while ( ( start = strstr( dupl, list ) ) )
	      if ( (start == dupl || *(start-1) == ' ' )
		  && ( *(end = start + strlen( list ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || end == NULL ) )
		{
		  mprog_driver( mprg->comlist, mob, actor, obj, vo, 0 );
		  break;
		}
	      else
		dupl = start+1;
	  }
	else
	  {
	    list = one_argument( list, word );
	    for( ; word[0] != '\0'; list = one_argument( list, word ) )
	      while ( ( start = strstr( dupl, word ) ) )
		if ( ( start == dupl || *(start-1) == ' ' )
		    && ( *(end = start + strlen( word ) ) == ' '
			|| *end == '\n'
			|| *end == '\r'
			|| end == NULL ) )
		  {
		    mprog_driver( mprg->comlist, mob, actor, obj, vo, 0 );
		    break;
		  }
		else
		  dupl = start+1;
	  }
      }

  return;

}



/*
 * Even if $n or $r is null at this point (from rand_prog, greet_prog,
 * or entry_prog) thats OK because mprog_driver will find *someone* who
 * is valid, $n is the first in the room, $r is a random in the room
 * if they can be found, otherwise $n is first in the zone, and $r
 * is a random person in the zone (thats gotta work)
 */
void mprog_percent_check( struct char_data *mob, struct char_data *actor,
	struct obj_data *obj, void *vo, int type)
{
  MPROG_DATA * mprg;

  for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next)
    if ((mprg->type & type) && (number(0, 99) < atoi(mprg->arglist))) {
      mprog_driver(mprg->comlist, mob, actor, obj, vo, (type == LOAD_PROG) ? 1 : 0);
      /* This next line means that a single mob can fire more than one
         greet-related prog on each char, but will only fire one other prog */
      if (type != GREET_PROG && type != ALL_GREET_PROG &&
          type != GREET_EVERY_PROG && type != ALL_GREET_EVERY_PROG)
        break;
    }

  return;
}



/*
 * The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void mprog_act_trigger( char *buf, struct char_data *mob, struct char_data *ch,
		       struct obj_data *obj, void *vo)
{
  MPROG_ACT_LIST * tmp_act;

  if ( IS_NPC( mob ) &&
       ( mob_index[mob->nr].progtypes & ACT_PROG ) ) {

    tmp_act = malloc( sizeof( MPROG_ACT_LIST ) );
    if ( mob->mpactnum > 0 )
      tmp_act->next = mob->mpact->next;
    else
      tmp_act->next = NULL;

    mob->mpact      = tmp_act;
    mob->mpact->buf = str_dup( buf );
    mob->mpact->ch  = ch; 
    mob->mpact->obj = obj; 
    mob->mpact->vo  = vo; 
    mob->mpactnum++;
  }

  return;
}



void mprog_bribe_trigger(struct char_data *mob, struct char_data *ch,
	int amount)
{
  MPROG_DATA *mprg;
  struct obj_data *obj;

  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & BRIBE_PROG)) {
    obj = create_money(amount);
    obj_to_char(obj, mob);
    mob->points.gold -= amount;

    for (mprg = mob_index[mob->nr].mobprogs;
         mprg != NULL; mprg = mprg->next) {
      if ((mprg->type & BRIBE_PROG) && (amount >= atoi(mprg->arglist))) {
        mprog_driver( mprg->comlist, mob, ch, obj, NULL, 0);
        break;
      }
    }
  }
  
  return;
}



void mprog_kill_trigger(struct char_data *mob, struct char_data *victim)
{
  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & KILL_PROG)) {
    mprog_percent_check(mob, victim, NULL, NULL, KILL_PROG);
  }

  return;
}



void mprog_death_trigger(struct char_data *mob, struct char_data *killer)
{
  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & DEATH_PROG)) {
    mprog_percent_check(mob, killer, NULL, NULL, DEATH_PROG);
  }

  /* death_cry(mob); */
  return;
}



/*
 * although entry triggers will use $r, they wont use $n.  The code has
 * been changed so that $n is a valid target -- it just takes as $n the
 * first person it finds in the list of players in the room.
 */
void mprog_entry_trigger( struct char_data *mob )
{
    struct char_data *first_ch = NULL;

    if ( IS_NPC( mob ) && ( mob_index[mob->nr].progtypes & ENTRY_PROG ) ) {

   /* cut and paste some code here */
   /*
    * get the first visable mortal player who is in the room with the mob
    * pretend that the first charcter in the room 'triggered' the mob to act
    * when it enters and sees them there
    */

	for ( first_ch = world[mob->in_room].people; first_ch;
					first_ch = first_ch->next_in_room ) {
	    if ( !IS_NPC( first_ch )
	    && mob != first_ch
	    && CAN_SEE( mob, first_ch ) ) {

		mprog_percent_check( mob, first_ch, NULL, NULL, ENTRY_PROG );
		break;
	    }
	}
    }
    return;
}



void mprog_fight_trigger(struct char_data *mob, struct char_data *ch)
{
  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & FIGHT_PROG))
    mprog_percent_check( mob, ch, NULL, NULL, FIGHT_PROG );

  return;
}



void mprog_give_trigger(struct char_data *mob, struct char_data *ch,
		struct obj_data *obj)
{
  char buf[MAX_INPUT_LENGTH];
  MPROG_DATA *mprg;

  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & GIVE_PROG))
    for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next) {
      one_argument( mprg->arglist, buf );
      if ((mprg->type & GIVE_PROG) &&
 	  ((!str_infix(obj->name, mprg->arglist))
	   || (isname(buf, obj->name))
 	   || (!str_cmp("all", buf))
           || (atoi(buf) == GET_OBJ_VNUM(obj)) )) {
	mprog_driver(mprg->comlist, mob, ch, obj, NULL, 0);
	break;
      }
    }

  return;
}



void mprog_greet_trigger( struct char_data *ch )
{
  struct char_data *vmob;


  /* dont trigger multiple times when a group walks in */
  if (ch->master && (ch->master->in_room == ch->in_room))
    return;
  
  if (GET_POS(ch) == POS_DEAD) return;
  
  for ( vmob = world[ch->in_room].people; vmob != NULL; vmob = vmob->next_in_room )
    if ( IS_NPC( vmob ) && ch != vmob && CAN_SEE( vmob, ch )
        && !FIGHTING( vmob ) && AWAKE( vmob )
        && ( mob_index[vmob->nr].progtypes & GREET_PROG) ) {
           mprog_percent_check( vmob, ch, NULL, NULL, GREET_PROG );
    } else if ( IS_NPC( vmob ) && !FIGHTING( vmob ) && AWAKE( vmob )
      && ( mob_index[vmob->nr].progtypes & ALL_GREET_PROG ) ) {
        mprog_percent_check(vmob,ch,NULL,NULL,ALL_GREET_PROG);
    }
  return;
}

void mprog_greet_every_trigger( struct char_data *ch )
{
  struct char_data *vmob;

  if (GET_POS(ch) == POS_DEAD) return;

  for ( vmob = world[ch->in_room].people; vmob != NULL; vmob = vmob->next_in_room )
    if ( IS_NPC( vmob ) && ch != vmob && CAN_SEE( vmob, ch )
        && !FIGHTING( vmob ) && AWAKE( vmob )
        && ( mob_index[vmob->nr].progtypes & GREET_EVERY_PROG) ) {
           mprog_percent_check( vmob, ch, NULL, NULL, GREET_EVERY_PROG );
    } else if ( IS_NPC( vmob ) && !FIGHTING( vmob ) && AWAKE( vmob )
      && ( mob_index[vmob->nr].progtypes & ALL_GREET_EVERY_PROG ) ) {
        mprog_percent_check(vmob,ch,NULL,NULL,ALL_GREET_EVERY_PROG);
    }
  return;
}



void mprog_hitprcnt_trigger( struct char_data *mob, struct char_data *ch)
{
  MPROG_DATA *mprg;

  if ( IS_NPC( mob )
      && ( mob_index[mob->nr].progtypes & HITPRCNT_PROG ) )
    for ( mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next )
      if ( ( mprg->type & HITPRCNT_PROG )
           && ( ( 100*mob->points.hit / mob->points.max_hit ) < atoi( mprg->arglist ) ) )
      {
        mprog_driver( mprg->comlist, mob, ch, NULL, NULL, 0);
        break;
      }
 
  return;
}



/*
 * although randoms dont trigger specifically off of anything, they could
 * be said to be "triggered" by the first player in the room list who
 * looks like a valid target
 */
void mprog_random_trigger(struct char_data *mob)
{
  struct char_data *first_ch = NULL;

  /* if the mob *has* a randprog, take a look */
  if (mob_index[mob->nr].progtypes & RAND_PROG) {

   /* cut and paste some code here */
   /*
    * get the first visable mortal player who is in the room with the mob,
    * and pretend that they 'triggered' the mob
    */
   /*
    * the check to exclude immortals is disabled and here's why:
    * if you are an immortal alone in a room with a mob you can easily
    * trigger a prog (its greet_prog for example) but the $r would be
    * NULL so further commands crash the mud:
    * example: if isgood($r) say hi endif
    * would crash the mud if an immortal triggered it
    */
   for ( first_ch = world[mob->in_room].people;
         first_ch;
         first_ch = first_ch->next_in_room )
     if ( !IS_NPC( first_ch )
/*       &&  first_ch->player.level < LVL_IMMORT   */
         &&  CAN_SEE( mob, first_ch ) )
       break;

    /* if first_ch is null mprog_percent_check calls mprog_driver, and
       mprog_driver will find someone */

    mprog_percent_check(mob, first_ch, NULL, NULL, RAND_PROG);
  }

  return;
}



/* HACKED, commented out and replaced below */
/* this stock code doesnt work at all, fucking crap code */
/*
void mprog_speech_trigger( char *txt, struct char_data *mob )
{
  struct char_data *vmob;

  for ( vmob = world[mob->in_room].people;
        vmob != NULL;
        vmob = vmob->next_in_room )
    if ( IS_NPC( vmob ) && ( mob_index[vmob->nr].progtypes & SPEECH_PROG ) )
      mprog_wordlist_check( txt, vmob, mob, NULL, NULL, SPEECH_PROG );
  
  return;
}
*/



/*
 * This isn't perfect at all: one problem is if you use keywords,
 * the 'p' option, then it works correctly if they actor types one of the
 * keywords, but if they type two of them and out of order, it doesn't work
 * the reason why is because i did a subtring search on everything they
 * typed, not a word for word check like maybe I should have. I dunno.
 */
void mprog_speech_trigger( char *txt, struct char_data *ch )
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vmob;
  struct char_data *next_vmob;
  MPROG_DATA *mprg;


  for (vmob = world[ch->in_room].people;
       vmob != NULL;
       vmob = next_vmob) {
    next_vmob = vmob->next_in_room;
    if (IS_NPC(vmob) && (mob_index[vmob->nr].progtypes & SPEECH_PROG)) {
      for (mprg = mob_index[vmob->nr].mobprogs;
           mprg != NULL; mprg = mprg->next) {
        if (mprg->type & SPEECH_PROG) {
          one_argument(mprg->arglist, buf);
          if (!strcmp(buf, "p")) {
            sprintf(buf, " p %s", txt);
            if (!str_cmp(buf, mprg->arglist)) {
              mprog_driver(mprg->comlist, vmob, ch, NULL, NULL, 0);
              break;
            }
          } else /* this is not perfect */ {
            if (!str_infix(txt, mprg->arglist)) {
              mprog_driver(mprg->comlist, vmob, ch, NULL, NULL, 0);
              break;
            }
          }
        }
      }
    }
  }

  return;
}



/*
 * A trigger for socials, only the mob itself triggers
 * use like: social_prog <socialname>
 * oops, it always goes off 100%, maybe it should take a % chance as well
 * a side note: the social name is sent with an extra space at the beginning
 * ie "kiss" is sent in as " kiss" so that it matches the args that mprg is
 * using.  Just remember social_name should have a leading space.
 * its ok for additional arguments to be passed along with the social name
 * how those are handled are:
 * chat king kingdom   ....
 * is sent into here as " chat kingdom"
 * HACKED to use mpstopcommand
 */
int mprog_social_trigger(struct char_data *mob, struct char_data *ch,
    char *social_name)
{
  MPROG_DATA *mprg;

  MOBHandled = 0;

  if (!IS_NPC(mob) || IS_AFFECTED(mob, AFF_CHARM) || mob->desc)
    return 0;

  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & SOCIAL_PROG)) {
    for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next) {
      if (mprg->type & SOCIAL_PROG) {
        if (!str_cmp(social_name, mprg->arglist)) {
          mprog_driver(mprg->comlist, mob, ch, NULL, NULL, 0);
          break;
        }
      }
    }
  }

  return MOBHandled;
}



/*
 * return a 1 if we consider the action handled, a 0 if we want the
 * command to go ahead and go through
 * Sorry to use the global MOBHandled, but it was getting hard to
 * hack :<  -- all them nested ifs and stuff :((((
 * by default of course, the commands are NOT handled
 * use mpstopcommand to have the mob 'handle' the command
 */
int mprog_command_trigger(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *vmob;
  MPROG_DATA *mprg;


  MOBHandled = 0;

  for (vmob = world[ch->in_room].people;
       vmob != NULL;
       vmob = vmob->next_in_room) {
    if (vmob == ch)
      continue;
    if (vmob &&
        IS_NPC(vmob) &&
        !IS_AFFECTED(vmob, AFF_CHARM) &&
        !vmob->desc &&
        (mob_index[vmob->nr].progtypes & COMMAND_PROG)) {
      for (mprg = mob_index[vmob->nr].mobprogs;
           mprg != NULL; mprg = mprg->next) {
        if (mprg->type & COMMAND_PROG) {
          if (mprg->arglist == NULL) {
            break;
          }
          skip_spaces(&arg);
          if (*arg) {
            sprintf(buf, " %s %s", cmd_info[cmd].command, arg);
/*
            if (!strn_cmp(buf, mprg->arglist, strlen(buf))) {
*/
            if (!str_prefix(mprg->arglist, buf)) {
              mprog_driver(mprg->comlist, vmob, ch, NULL, NULL, 0);
              break;
            }
          } else {
            sprintf(buf, " %s", cmd_info[cmd].command);
            if (!str_cmp(buf, mprg->arglist)) {
              mprog_driver(mprg->comlist, vmob, ch, NULL, NULL, 0);
              break;
            }
          }
        }
      }
    }
  }

  return MOBHandled;
}

/* Just like command_trigger, but with spells - format is
   >spell_prog earthquake~
   or somesuch. Uses MOBHandled & mpstopcommand too. Works better than a
   command_prog, because the above example will trigger on "cast 'earthq'" or
   "sing 'earthquake'" as well as "cast 'earthquake'"
*/
int mprog_spell_trigger(struct char_data *ch, char *arg)
{
  struct char_data *vmob;
  MPROG_DATA *mprg;


  MOBHandled = 0;

  for (vmob = world[ch->in_room].people;
       vmob != NULL;
       vmob = vmob->next_in_room) {
    if (vmob == ch)
      continue;
    if (vmob &&
        IS_NPC(vmob) &&
        !IS_AFFECTED(vmob, AFF_CHARM) &&
        !vmob->desc &&
        (mob_index[vmob->nr].progtypes & SPELL_PROG)) {
      for (mprg = mob_index[vmob->nr].mobprogs;
           mprg != NULL; mprg = mprg->next) {
        if (mprg->type & SPELL_PROG) {
          if (mprg->arglist == NULL) {
            break;
          }
          skip_spaces(&arg);
          sprintf(buf, " %s", arg);
          if (!str_prefix(mprg->arglist, buf)) {
            mprog_driver(mprg->comlist, vmob, ch, NULL, NULL, 0);
            break;
          }
        }
      }
    }
  }

  return MOBHandled;
}

void mprog_load_trigger(struct char_data *mob)
{
  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & LOAD_PROG)) {
    mprog_percent_check(mob, NULL, NULL, NULL, LOAD_PROG);
  }

  /* death_cry(mob); */
  return;
}

int is_scripting(struct char_data *mob)
{
  struct char_data *tmob;  /* temp mob, just to check the script_list */


  /* only mobs can script */
  if (!IS_NPC(mob))
    return 0;

  /* check the list for mob */
  if (script_list != NULL) {
    for (tmob = script_list; tmob; tmob = tmob->mpnextscript)
      if (tmob == mob) {
        return 1;
      }
  }

  return 0;
}



void set_script(struct char_data *mob, struct char_data *actor,
                int whichscript)
{

  if (!IS_NPC(mob) || IS_AFFECTED(mob, AFF_CHARM) || mob->desc)
    return;

  /* if i am already in the list, then bail */
  if (is_scripting(mob))
    return;

  /* remember who started it */
  mob->mpscriptactor = actor;

  /* remember which script this is (and that mob is running one) */
  /* and reset which step they're on */
  mob->mpscriptnum = whichscript;
  mob->mpscriptstep = 0;

  /* add them to the head of the list */
  mob->mpnextscript = script_list;
  script_list = mob;
}



void stop_script(struct char_data *mob)
{
  struct char_data *temp;


  if (!IS_NPC(mob) || IS_AFFECTED(mob, AFF_CHARM) || mob->desc)
    return;

  /* clear out their script memory */
  mob->mpscriptnum = 0;
  mob->mpscriptstep = 0;
  mob->mpscriptactor = NULL;

  REMOVE_FROM_LIST(mob, script_list, mpnextscript);
  mob->mpnextscript = NULL;
}



/*
 * A trigger for scripts, only the mob itself triggers
 * use like: >script_prog <scriptname>~
 * a side note: the script_name is sent with an extra space at the beginning
 * ie "kiss" is sent in as " kiss" so that it matches the args that mprg is
 * using.  Just remember scriptname should have a leading space.
 * There are two ways into this script:
 * the first way is to get triggered by the actions of a player
 * via mptrigger $n <scriptname>
 * the second way is, the script is running, and the values for ch (actor)
 * etc have to come from the mobs memory.
 */
void mprog_script_trigger(struct char_data *mob, struct char_data *ch,
    char *script_name)
{
  MPROG_DATA *mprg;
  int whichscript = 0;
  int step;
  struct char_data *actor;
  char *newcomlist;
  char *curtok;
  char separator[2];  /* either a carriage return or a '@' */


  /* check if the mob can run their script */
  if (!IS_NPC(mob) || IS_AFFECTED(mob, AFF_CHARM) || mob->desc) {
    if (IS_NPC(mob))
      stop_script(mob);
    return;
  }

  /* if they have a script */
  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & SCRIPT_PROG)) {
    
    /* check each prog */
    for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next) {

      /* if its a script prog.. */
      if (mprg->type & SCRIPT_PROG) {
        whichscript++;

        /*
         * and it matches either the name (the first time)
         * or the scriptnumber (coming in on the pulse)
         */
        if (!str_cmp(script_name, mprg->arglist) ||
            whichscript == mob->mpscriptnum) {

          /* find the actor */
          if (ch != NULL) {                 /* the first time */
            set_script(mob, ch, whichscript);
          }

          actor = mob->mpscriptactor;     /* so remember him from before */
          
          /*
           * find which step of the script to run, and run
           * just that one step
           */
          mob->mpscriptstep++;       /* remember which step we're looking for */
                                     /* first time in, this is 0, so then */
                                     /* we'd be looking for step 1 */
          step = 1;

          /*
           * usually, break up scripts line by line,
           * but in the case of if..thens or any time when more than
           * one line needs to be processed in one step,
           * then use a '@' on a line by itself to delimit a block
           */
          newcomlist = strdup(mprg->comlist);  /* need a copy for strtok */ 

          if (strchr(newcomlist, '@') == NULL)
            strcpy(separator, "\n");   /* line by line */
          else
            strcpy(separator, "@");    /* block by block */

          for (curtok = strtok(newcomlist, separator);
               curtok;
               curtok = strtok(NULL, separator)) {
            if (curtok && (step == mob->mpscriptstep)) {            
              mprog_driver(curtok, mob, actor, NULL, NULL, 0);
              free(newcomlist); 
              return;
            }
            step++;  /* check the next curtok maybe */
          }

          /* if the script got to here, then its out of steps, reset 
             and return */
          stop_script(mob);
          free(newcomlist);

          return;
        }
      }
    }
  }

  return;
}



/* called every 2 seconds from comm.c */
void perform_scripts(void)
{
  struct char_data *mob;
  struct char_data *next_mob;


  /*
   * this has to be done this strange way because mob
   * can wind up finishing their script leaving the next
   * to null, leaving you with no list at all!
   * hence we remember the next mob running a script 
   * before we do the trigger
   */

  if (script_list != NULL) {

    for (mob = script_list; mob; mob = next_mob) {
      next_mob = mob->mpnextscript;
      mprog_script_trigger(mob, NULL, "");
    }
  }
}



/*
 * special times of day: 
 *   hour 5: the sky gets light out
 *   hour 6: the dawn
 *   hour 21: the sky gets kind of dark
 *   hour 22: nighttime
 */
void mprog_time_trigger(struct char_data *mob)
{
  MPROG_DATA *mprg;
  int time;
  /* compare to time_info.hours */


  if (!IS_NPC(mob) || IS_AFFECTED(mob, AFF_CHARM) || mob->desc)
    return;

  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & TIME_PROG)) {
    for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next) {
      if (mprg->type & TIME_PROG) {
        sscanf(mprg->arglist, " %d ", &time);
        if (time == time_info.hours) {
          mprog_driver(mprg->comlist, mob, NULL, NULL, NULL, 1);
          break;
        }
      }
    }
  }

  return;
}
