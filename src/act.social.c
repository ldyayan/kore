/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern char *color_codes[];
extern struct social_info soc_info[];

/* extern functions */
char *fread_action(FILE * fl, int nr);

/* local globals */
int list_top = -1;
bool socials_are_buggy;

/* local functions */
void sort_social_messages(void);
int save_social_messages(void);

struct social_messg *soc_mess_list = NULL;


int find_social(char *command) {
  
  int i;
  
  for (i = 0; i < list_top; i++)
    if (!strcmp(soc_info[i].command, command)) return i;
    
  return 0;
}

/*int find_action(int cmd)
{
  int bot, top, mid;

  bot = 0;
  top = list_top;

  if (top < 0)
    return (-1);

  for (;;) {
    mid = (bot + top) >> 1;

    if (soc_mess_list[mid].act_nr == cmd)
      return (mid);
    if (bot >= top)
      return (-1);

    if (soc_mess_list[mid].act_nr > cmd)
      top = --mid;
    else
      bot = ++mid;
  }
}*/


/*ACMD(do_action)
{
  struct social_messg *action;
  struct char_data *vict;

  action = &soc_mess_list[cmd];

  if (action->char_found)
    one_argument(argument, buf);
  else
    *buf = '\0';

  if (!*buf) {
    send_to_char(action->char_no_arg, ch);
    send_to_char("\r\n", ch);
    act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
    return;
  }
  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char(action->not_found, ch);
    send_to_char("\r\n", ch);
  } else if (vict == ch) {
    send_to_char(action->char_auto, ch);
    send_to_char("\r\n", ch);
    act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
  } else {
    if (GET_POS(vict) < action->min_victim_position)
      act("$N is not in a proper position for that.",
	  FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else {
      act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
      act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
      act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
    }
  }
}*/




ACMD(do_action)
{
  struct social_messg *action;
  struct char_data *vict;
  int stopped = 0;

  int mprog_social_trigger(struct char_data *vict, struct char_data *ch,
      char *social_name);

  if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL) {
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
    return;
  }

  action = &soc_mess_list[cmd];

  if (action->char_found)
    argument = one_argument(argument, buf);
  else
    *buf = '\0';

  if (!*buf) {
    send_to_char(action->char_no_arg, ch);
    send_to_char("\r\n", ch);
    act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf))) {
    if (!(vict = get_char_vis(ch, buf))) {
      send_to_char(action->not_found, ch);
      send_to_char("\r\n", ch);
    } else {
      if (IS_NPC(ch) && !ch->desc) {
        send_to_char("Mobs shouldnt be sending socials!\r\n", ch);
        return;
      }

      if (IS_NPC(vict)) {
	send_to_char(action->not_found, ch);
	send_to_char("\r\n", ch);
        return;
      }

      if (PRF_FLAGGED(ch, PRF_NOTELL)) {
        send_to_char("You can't long distance social to other people "
            "while you have notell on.\r\n", ch);
        return;
      }
      if (PRF_FLAGGED(vict, PRF_NOTELL)) {
        act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
        return;
      }
      sprintf(buf, "%sLong distance to %s: ", CCTELL(ch), GET_NAME(vict));
      send_to_char(buf, ch);
      act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
      send_to_char(CCNRM(ch), ch);
      sprintf(buf, "%sFrom afar, ", CCTELL(vict));
      send_to_char(buf, vict);
      act(action->vict_found, action->hide, ch, 0, vict, TO_VICT | TO_SLEEP);
      send_to_char(CCNRM(vict), vict);
      /* let people reply to ranged socials */
      GET_LAST_TELL(vict) = GET_IDNUM(ch);
    }
  } else if (vict == ch) {
    send_to_char(action->char_auto, ch);
    send_to_char("\r\n", ch);
    act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
  } else {
    if (GET_POS(vict) < action->min_victim_position)
      act("$N is not in a proper position for that.",
	  FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else {
      stopped = 0;
      if (IS_NPC(vict)) {
        if (!*argument)
          sprintf(buf, " %s", soc_info[cmd].command);
        else
          sprintf(buf, " %s%s", soc_info[cmd].command, argument);
        /* don't retrigger from mpdosocial */
        if (subcmd != 1) stopped = mprog_social_trigger(vict, ch, buf);
      }
      if (!stopped) {
        act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
        act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
        act(action->vict_found, action->hide, ch, 0, vict, TO_VICT | TO_SLEEP);
      }
    }
  }
}






ACMD(do_insult)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (*arg) {
    if (!(victim = get_char_room_vis(ch, arg)))
      send_to_char("Can't hear you!\r\n", ch);
    else {
      if (victim != ch) {
	sprintf(buf, "You insult %s.\r\n", GET_NAME(victim));
	send_to_char(buf, ch);

	switch (number(0, 2)) {
	case 0:
	  if (GET_SEX(ch) == SEX_MALE) {
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT);
	  } else {		/* Ch == Woman */
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of having the smallest... (brain?)",
		  FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n tells you that you'd lose a beauty contest against a troll.",
		  FALSE, ch, 0, victim, TO_VICT);
	  }
	  break;
	case 1:
	  act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	default:
	  act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	}			/* end switch */

	act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
      } else {			/* ch == victim */
	send_to_char("You feel insulted.\r\n", ch);
      }
    }
  } else
    send_to_char("I'm sure you don't want to insult *everybody*...\r\n", ch);
}


char *fread_action(FILE * fl, int nr)
{
  char buf[MAX_STRING_LENGTH], *rslt;

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl)) {
    fprintf(stderr, "fread_action - unexpected EOF near action #%d", nr);
    exit(1);
  }
  if (*buf == '#')
    return (NULL);
  else {
    *(buf + strlen(buf) - 1) = '\0';
    CREATE(rslt, char, strlen(buf) + 1);
    strcpy(rslt, buf);
    return (rslt);
  }
}

void boot_social_messages(void)
{
  FILE *fl;
  int hide, min_pos, curr_soc = -1, minlev;
  char next_soc[MAX_SOCIAL_LEN];

  socials_are_buggy = FALSE;
  /* open social file */
  if (!(fl = fopen(SOCMESS_FILE, "r"))) {
    sprintf(buf, "SYSERR: can't open socials file '%s'", SOCMESS_FILE);
    perror(buf);
/*    exit(1);*/
    strcpy(soc_info[0].command, "\n");
    socials_are_buggy = TRUE;
    return;
  }

  /* Allocate space for socials */
  CREATE(soc_mess_list, struct social_messg, MAX_SOCIALS);

  /* now read 'em */
  for (;;) {
    fscanf(fl, " %s ", next_soc);
    if (*next_soc == '$')
      break;
    if (fscanf(fl, " %d %d %d\n", &hide, &min_pos, &minlev) != 3) {
      fprintf(stderr, "SYSERR: format error in social file near social '%s'\n",
	      next_soc);
/*      exit(1);*/
      list_top = MAX(curr_soc-1, -1);  /* Ignore the last social or two */
      strcpy(soc_info[list_top+1].command, "\n");
      socials_are_buggy = TRUE;
      return;
    }
    /* read the stuff */
    curr_soc++;
    strcpy(soc_info[curr_soc].command, next_soc);
    soc_info[curr_soc].minimum_position = POS_SLEEPING;
    soc_info[curr_soc].minimum_level = minlev;    
   
    soc_mess_list[curr_soc].hide = hide;
    soc_mess_list[curr_soc].min_victim_position = min_pos;

    soc_mess_list[curr_soc].char_no_arg = fread_action(fl, /*nr*/ curr_soc);
    soc_mess_list[curr_soc].others_no_arg = fread_action(fl, /*nr*/ curr_soc);
    soc_mess_list[curr_soc].char_found = fread_action(fl, /*nr*/ curr_soc);

    /* if no char_found, the rest is to be ignored */
    if (!soc_mess_list[curr_soc].char_found)
      continue;

    soc_mess_list[curr_soc].others_found = fread_action(fl, /*nr*/ curr_soc);
    soc_mess_list[curr_soc].vict_found = fread_action(fl, /*nr*/ curr_soc);
    soc_mess_list[curr_soc].not_found = fread_action(fl, /*nr*/ curr_soc);
    soc_mess_list[curr_soc].char_auto = fread_action(fl, /*nr*/ curr_soc);
    soc_mess_list[curr_soc].others_auto = fread_action(fl, /*nr*/ curr_soc);
  }

  /* close file & set top */
  fclose(fl);
  list_top = curr_soc;
  strcpy(soc_info[list_top+1].command, "\n");
  soc_info[list_top+1].minimum_position = 0;
  soc_info[list_top+1].minimum_level = 100;

  /* now, sort 'em */
  sort_social_messages();
}

void sort_social_messages(void)
{
  int min_pos, i, curr_soc;
  struct social_messg temp;
  struct social_info tempinfo;
  
  /* This is a selection sort: Darryl */
  for (curr_soc = 0; curr_soc < list_top; curr_soc++) {
    min_pos = curr_soc;
    for (i = curr_soc + 1; i <= list_top; i++)
      if (strcmp(soc_info[i].command, soc_info[min_pos].command) < 0)
	min_pos = i;
    if (curr_soc != min_pos) {
      temp = soc_mess_list[curr_soc];
      soc_mess_list[curr_soc] = soc_mess_list[min_pos];
      soc_mess_list[min_pos] = temp;
      tempinfo = soc_info[curr_soc];
      soc_info[curr_soc] = soc_info[min_pos];
      soc_info[min_pos] = tempinfo;
    }
  }
}

ACMD(do_socedit) {
  int i, curr_soc = -1;
  char field[15];
  char **fieldptr = NULL;
 
  argument = one_argument(argument, buf);
  if (!strcmp(buf, "")) {
    send_to_char("Usage: sedit social command [text]\r\n   or: sedit save\r\n", ch);
    return;
  }
  
  if (!strcmp(buf, "save")) {
    if (socials_are_buggy)
      send_to_char("The socials file is buggy. Saving would be a bad idea.\r\n", ch);
    else
      if (save_social_messages() == 0)
        send_to_char("Social messages saved.\r\n", ch);
      else
        send_to_char("Woah...couldn't save...Dave...my mind is going...\r\n", ch);
    return;
  }
  
  for (i = 0; i <= list_top; i++)
    if (!strcmp(soc_info[i].command, buf))
      curr_soc = i;
  if (curr_soc == -1) {
    send_to_char("Social not found.\r\n", ch);
    return;
  }
  
  argument = one_argument(argument, buf);
  strcpy(field, buf);
  if (!strcmp(field, "")) {
    send_to_char("Missing sedit command.\r\n",ch);
    return;
  }
  if (!strcmp(field, "list")) {
    sprintf(buf, "Social '%s' (level %d) defined as:\r\n",
        soc_info[curr_soc].command,
        soc_info[curr_soc].minimum_level);
    sprintf(buf+strlen(buf), "cnoarg: %s\r\nonoarg: %s\r\n",
        soc_mess_list[curr_soc].char_no_arg,
        soc_mess_list[curr_soc].others_no_arg);
    sprintf(buf+strlen(buf), "cvict: %s\r\novict: %s\r\nvict: %s\r\n",
        soc_mess_list[curr_soc].char_found,
        soc_mess_list[curr_soc].others_found,
        soc_mess_list[curr_soc].vict_found
    );
    sprintf(buf+strlen(buf), "cnvict: %s\r\nccvict: %s\r\nocvict: %s\r\n",
        soc_mess_list[curr_soc].not_found,
        soc_mess_list[curr_soc].char_auto,
        soc_mess_list[curr_soc].others_auto
    );
    send_to_char(buf, ch);
  } else if (!strcmp(field, "cnoarg"))
    fieldptr = &soc_mess_list[curr_soc].char_no_arg;
  else if (!strcmp(field, "onoarg"))
    fieldptr = &soc_mess_list[curr_soc].others_no_arg;
  else if (!strcmp(field, "cvict"))
    fieldptr = &soc_mess_list[curr_soc].char_found;
  else if (!strcmp(field, "ovict"))
    fieldptr = &soc_mess_list[curr_soc].others_found;
  else if (!strcmp(field, "vict"))
    fieldptr = &soc_mess_list[curr_soc].vict_found;
  else if (!strcmp(field, "cnvict"))
    fieldptr = &soc_mess_list[curr_soc].not_found;
  else if (!strcmp(field, "ccvict"))
    fieldptr = &soc_mess_list[curr_soc].char_auto;
  else if (!strcmp(field, "ocvict"))
    fieldptr = &soc_mess_list[curr_soc].others_auto;
  else if (!strcmp(field, "level")) {
    i = atoi(argument);
    if (i < 0 || i > LVL_IMPL) {
      send_to_char("Invalid level.\r\n", ch);
      return;
    }
    soc_info[curr_soc].minimum_level = i;
    send_to_char("Ok.\r\n", ch);
    return;
  } else send_to_char("Unknown sedit command.\n", ch);
  
  if (fieldptr != NULL) {
    skip_spaces(&argument);
    delete_doubledollar(argument);
    if (!strcmp(argument, "")) {
      send_to_char("Field removed.\r\n", ch);
      free(*fieldptr);
      *fieldptr = NULL;
    } else {
      send_to_char("Ok.\r\n", ch);
      free(*fieldptr);
      CREATE(*fieldptr, char, strlen(argument)+1);
      strcpy(*fieldptr, argument);
    };
  };
}

ACMD(do_screate) {
  int i, curr_soc = -1;
  char newsoc[MAX_SOCIAL_LEN];
  
  if (!strcmp(argument, "")) {
    send_to_char("Usage: screate social\r\n", ch);
    return;
  }
  
  if (list_top+1 == MAX_SOCIALS) {
    send_to_char("No more room for socials! Sorry!\r\n", ch);
    return;
  }
  
  one_argument(argument, newsoc);
  
  for (i = 0; i <= list_top; i++)
    if (!strcmp(soc_info[i].command, newsoc))
      curr_soc = i;
  if (curr_soc != -1) {
    send_to_char("That social already exists!\r\n", ch);
    return;
  }

  i = ++list_top;
  strcpy(soc_info[i].command, newsoc);
  soc_info[i].minimum_position = POS_SLEEPING;
  soc_info[i].minimum_level = 0;
  soc_mess_list[i].hide = 0;
  soc_mess_list[i].min_victim_position = POS_DEAD;
  soc_mess_list[i].char_no_arg = NULL;
  soc_mess_list[i].others_no_arg = NULL;
  soc_mess_list[i].char_found = NULL;
  soc_mess_list[i].others_found = NULL;
  soc_mess_list[i].vict_found = NULL;
  soc_mess_list[i].not_found = NULL;
  soc_mess_list[i].char_auto = NULL;
  soc_mess_list[i].others_auto = NULL;

  strcpy(soc_info[i+1].command, "\n");
  sort_social_messages();
  send_to_char("Ok.\r\n", ch);
}

int save_social_messages() {
  FILE *fp;
  int i;
    
  if (!(fp = fopen(SOCMESS_FILE, "w"))) {
    sprintf(buf, "SYSERR: can't open socials file '%s' for save", SOCMESS_FILE);
    perror(buf);
    return 1;  /* this is non-fatal */
  }
  
  for (i = 0; i <= list_top; i++) {
    fprintf( fp, "%s %d %d %d\n", soc_info[i].command,
                              soc_mess_list[i].hide,
                              soc_mess_list[i].min_victim_position,
                              soc_info[i].minimum_level
    );
    fprintf( fp, "%s\n", soc_mess_list[i].char_no_arg ?
                     soc_mess_list[i].char_no_arg :
                     "#"
    );
    fprintf( fp, "%s\n", soc_mess_list[i].others_no_arg ?
                     soc_mess_list[i].others_no_arg :
                     "#"
    );
    fprintf( fp, "%s\n", soc_mess_list[i].char_found ?
                     soc_mess_list[i].char_found :
                     "#"
    );
    if (soc_mess_list[i].char_found) {
      fprintf( fp, "%s\n", soc_mess_list[i].others_found ?
                       soc_mess_list[i].others_found :
                       "#"
      );
      fprintf( fp, "%s\n", soc_mess_list[i].vict_found ?
                       soc_mess_list[i].vict_found :
                       "#"
      );
      fprintf( fp, "%s\n", soc_mess_list[i].not_found ?
                       soc_mess_list[i].not_found :
                       "#"
      );
      fprintf( fp, "%s\n", soc_mess_list[i].char_auto ?
                       soc_mess_list[i].char_auto :
                       "#"
      );
      fprintf( fp, "%s\n", soc_mess_list[i].others_auto ?
                       soc_mess_list[i].others_auto :
                       "#"
      );
    }
    fprintf( fp, "\n" );
  }
  fprintf( fp, "$\n" );
  fclose(fp);
  return 0;  
}

ACMD(do_sremove) {
  int i, curr_soc = -1;
  char delsoc[MAX_SOCIAL_LEN];
  
  if (!strcmp(argument, "")) {
    send_to_char("Usage: sremove social\r\n", ch);
    return;
  }
  
  one_argument(argument, delsoc);
  
  for (i = 0; i <= list_top; i++)
    if (!strcmp(soc_info[i].command, delsoc))
      curr_soc = i;
  if (curr_soc == -1) {
    send_to_char("That social does not exist!\r\n", ch);
    return;
  }
  
  if (curr_soc != list_top) {
    soc_info[curr_soc] = soc_info[list_top];
    soc_mess_list[curr_soc] = soc_mess_list[list_top];
  }
  if (soc_mess_list[list_top].char_no_arg != NULL) free(soc_mess_list[list_top].char_no_arg);
  if (soc_mess_list[list_top].others_no_arg != NULL) free(soc_mess_list[list_top].others_no_arg);
  if (soc_mess_list[list_top].char_found != NULL) free(soc_mess_list[list_top].char_found);
  if (soc_mess_list[list_top].others_found != NULL) free(soc_mess_list[list_top].others_found);
  if (soc_mess_list[list_top].vict_found != NULL) free(soc_mess_list[list_top].vict_found);
  if (soc_mess_list[list_top].not_found != NULL) free(soc_mess_list[list_top].not_found);
  if (soc_mess_list[list_top].char_auto != NULL) free(soc_mess_list[list_top].char_auto);
  if (soc_mess_list[list_top].others_auto != NULL) free(soc_mess_list[list_top].others_auto);
  list_top--;
  sort_social_messages();
  send_to_char("Removed.\r\n", ch);
}

