/* ************************************************************************
*   File: textsave.c                                   Added to CircleMUD *
*  Usage: loading/saving player text for rent and crash-save              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  Based on objsave.c                                                     *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

/* Extern functions */
extern int file_to_string_alloc(char *name, char **buf);
extern ACMD(do_alias);
/* NeXTs need unlink defined for some reason */
#ifdef NeXT
int unlink(const char *);        /* /NextDeveloper/Headers/bsd/libc.h */
#endif /* NeXT */


/* return values:
	0 - successful load.
	1 - load failure of crash text -- perhaps new player.
*/
int Crash_load_text(struct char_data * ch) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char line[256];

  if (!get_filename(GET_NAME(ch), fname, ETEXT_FILE))
    return 1;

  if (!(fl = fopen(fname, "r"))) {
    /* couldnt open the file for reading ... they probably dont have one */
    /* could be a new player, or an old player that never made aliases */
    return 1;
  }

  while (!feof(fl)) {
    get_line(fl, line);
    if (feof(fl))
      break;
    do_alias(ch, line, 0, SCMD_QUIET_ALIAS);
  }

  fclose(fl);

  return 0;
}



int save_char_text(struct char_data * ch)
{
  struct alias *a;
  FILE *fl;
  char fname[MAX_STRING_LENGTH];

  /* if the player has no aliases, no need to save anything */
  if ((a = GET_ALIASES(ch)) == NULL)
    return 0;
 
  if (!get_filename(GET_NAME(ch), fname, ETEXT_FILE))
    return 1;

  if (!(fl = fopen(fname, "w"))) {
    /* couldnt open the file for writing... the disk is probably full */
    return 1;
  }

  while (a != NULL) {
    fprintf(fl, "%s%s\n", a->alias, a->replacement);
    a = a->next;
  }

  fclose(fl);

  return 0;
}
