/* ************************************************************************
*   File: screen.c                                      Part of CircleMUD *
*  Usage: header file with ANSI color codes for online color              *
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
#include "screen.h"



/* universally used arrays */
const char *colors[] = {
  "normal", "red", "green", "yellow",
  "blue", "magenta", "cyan", "white",
  "bold", "bright red", "bright green", "bright yellow",
  "bright blue", "bright magenta", "bright cyan", "bright white",
  "\n"
};

const char *true_color_codes[] = {
  KNRM, KRED, KGRN, KYEL,
  KBLU, KMAG, KCYN, KWHT,
  KB, KBRED, KBGRN, KBYEL,
  KBBLU, KBMAG, KBCYN, KBWHT,
};

const char *color_codes[] = {
  "^n", "^r", "^g", "^y",
  "^b", "^m", "^c", "^w",
  "^n", "^R", "^G", "^Y",
  "^B", "^M", "^C", "^W",
};

const char *color_fields[] = {
  "warning", "alert", "info", "holler", "shout", "gossip", "auction", "gratz",
  "tell", "page", "gsay", "qsay", "clansay", "roomname", "roomdesc", "objects",
  "players", "gods", "exits", "runes", "blade", "quillions", "jewels", "hilt",
  "pommel", "wiznet", "prompthit", "promptmana", "promptmove", "promptgold",
  "promptxp",
  "\n"
};



void color_setup(struct char_data * ch)
{
  SET_BIT(PRF_FLAGS(ch), PRF_COLOR);
  ch->player_specials->saved.color_prefs[0] = KLRED;  /* warning */
  ch->player_specials->saved.color_prefs[1] = KLYEL;  /* alert */
  ch->player_specials->saved.color_prefs[2] = KLGRN;  /* info */
  ch->player_specials->saved.color_prefs[3] = KLYEL;  /* holler */
  ch->player_specials->saved.color_prefs[4] = KLYEL;  /* shout */
  ch->player_specials->saved.color_prefs[5] = KLGRN;  /* gossip */
  ch->player_specials->saved.color_prefs[6] = KLMAG;  /* auction */
  ch->player_specials->saved.color_prefs[7] = KLCYN;  /* gratz */
  ch->player_specials->saved.color_prefs[8] = KLRED;  /* tell */
  ch->player_specials->saved.color_prefs[9] = KLRED;  /* page */
  ch->player_specials->saved.color_prefs[10] = KLRED; /* gsay */
  ch->player_specials->saved.color_prefs[11] = KLCYN; /* qsay */
  ch->player_specials->saved.color_prefs[12] = KLRED; /* clansay */
  ch->player_specials->saved.color_prefs[13] = KLCYN; /* roomname */
  ch->player_specials->saved.color_prefs[14] = KLRED; /* roomdesc */
  ch->player_specials->saved.color_prefs[15] = KLGRN; /* objects */
  ch->player_specials->saved.color_prefs[16] = KLYEL; /* players */
  ch->player_specials->saved.color_prefs[17] = KLRED; /* gods */
  ch->player_specials->saved.color_prefs[18] = KLCYN; /* exits */
  ch->player_specials->saved.color_prefs[19] = KLYEL; /* runes */
  ch->player_specials->saved.color_prefs[20] = KLWHT; /* blade */
  ch->player_specials->saved.color_prefs[21] = KLCYN; /* quillions */
  ch->player_specials->saved.color_prefs[22] = KLRED; /* jewels */
  ch->player_specials->saved.color_prefs[23] = KLMAG; /* hilt */
  ch->player_specials->saved.color_prefs[24] = KLRED; /* pommel */
  ch->player_specials->saved.color_prefs[25] = KLCYN; /* wiznet */
  ch->player_specials->saved.color_prefs[26] = KLRED; /* prompt-hit */
  ch->player_specials->saved.color_prefs[27] = KLMAG; /* prompt-mana */
  ch->player_specials->saved.color_prefs[28] = KLGRN; /* prompt-move */
  ch->player_specials->saved.color_prefs[29] = KLYEL; /* prompt-gold */
  ch->player_specials->saved.color_prefs[30] = KLBWHT; /* prompt-xp */
}



ACMD(do_color)
{
  int i, l, m;
  char field[MAX_INPUT_LENGTH];
  char color[MAX_INPUT_LENGTH];
  bool virgin_color;


  virgin_color = TRUE;
  for (i = 0; i < 40; i++)
    if (ch->player_specials->saved.color_prefs[i] != 0)
      virgin_color = FALSE;

  half_chop(argument, field, buf);

  if (!strcmp(field, "defaults") || !strcmp(field, "reset") || !*field
      || (!strcmp(field, "on") && virgin_color)) {
   if (*field) {
     color_setup(ch);
     send_to_char("Color enabled and set to default values:", ch);
   } else {
     send_to_char("Color values are currently set to:", ch);
   }
    for (i = 0; *(color_fields[i]) != '\n'; i++) {
      if ((i % 6) == 0)
        strcat(buf, "\r\n");
      if (PRF_FLAGGED(ch, PRF_COLOR)) {
        strcat(buf, color_codes[(ch->player_specials->saved.color_prefs[i])]);
      }
      sprintf(buf2, "%-12s%s", color_fields[i], CCNRM(ch));
      strcat(buf, buf2);
    }
    strcat(buf, "\r\n(Type 'help colors' for more information.)\r\n");
    send_to_char(buf, ch);
    return;
  } else if (!strcmp(field, "on")) {
    SET_BIT(PRF_FLAGS(ch), PRF_COLOR);
    sprintf(buf, "%sColor on.%s\r\n", CCINFO(ch), CCNRM(ch));
    send_to_char(buf, ch);
    return;
  } else if (!strcmp(field, "off")) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR);
    send_to_char("Color off.\r\n", ch);
    return;
  }

  strcpy(color, buf);

  for (l = 0; *(color_fields[l]) != '\n'; l++)
    if (!strncmp(field, color_fields[l], strlen(field)))
      break;
  if (!strcmp(color_fields[l], "\n")) {
    sprintf(buf, "Unknown color field, try one of:");
    for (i = 0; *(color_fields[i]) != '\n'; i++) {
      if ((i % 6) == 0) 
        strcat(buf, "\r\n");
      if (PRF_FLAGGED(ch, PRF_COLOR)) {
          strcat(buf,
              color_codes[(ch->player_specials->saved.color_prefs[i])]);
      }
      sprintf(buf2, "%-12s%s", color_fields[i], CCNRM(ch));
      strcat(buf, buf2);
    }
    strcat(buf, "\r\n(Type 'help color' for more information.)\r\n");
    send_to_char(buf, ch);
    return;
  }

  for (m = 0; *(colors[m]) != '\n'; m++)
    if (!strncmp(color, colors[m], strlen(color)))
      break;
  if (!strcmp(colors[m], "\n")) {
    sprintf(buf, "Unknown color, try one of:");
    for (i = 0; *(colors[i]) != '\n'; i++) {
      if ((i % 4) == 0)
        strcat(buf, "\r\n");
      if (PRF_FLAGGED(ch, PRF_COLOR))
        strcat(buf, color_codes[i]);
      sprintf(buf2, "%-18s%s", colors[i], CCNRM(ch));
      strcat(buf, buf2);
    }
    strcat(buf, "\r\n(Type 'help color' for more information.)\r\n");
    send_to_char(buf, ch);
    return;
  }

  /* what they've typed looks ok! lets go ahead and set them up */
  ch->player_specials->saved.color_prefs[l] = m;

  send_to_char("Color set: ", ch);
  if (PRF_FLAGGED(ch, PRF_COLOR)) {
    sprintf(buf, "%s", color_codes[m]);
    send_to_char(buf, ch);
  }  
  sprintf(buf, "%s will now be %s.%s\r\n", color_fields[l], colors[m],
      CCNRM(ch));
  send_to_char(buf, ch);
}



/*
 * The color stack will sometimes need fixing because the nesting of colors
 * etc will go too deep (more than the current arbitrary limit of 4 colors)
 * and then have a hard time pulling out.. the problem could be avoided
 * if before each color change a person popped in a normal code to turn it
 * back to some kind of normal, but this isnt really possible...
 */
void fix_color_stack(struct char_data * ch)
{
  /* out of bounds? reset the stack */
  if (GET_COLOR_STACK_INDEX(ch) < 0) {
    GET_COLOR_STACK_INDEX(ch) = 0;
    GET_COLOR_STACK(ch) = KLNRM;
    return;
  }

  /* too high? reset almost to the bottom (hope thats a good default) */
  if (GET_COLOR_STACK_INDEX(ch) >= 9) {
    GET_COLOR_STACK_INDEX(ch) = 2;  /* almost normal */
 /*   GET_COLOR_STACK(ch) = KLNRM; */
    return;
  }
}
