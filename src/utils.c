/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"

extern struct time_data time_info;
extern struct room_data *world;
extern char *color_codes[];

unsigned long my_rand(void);


/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  return ((my_rand() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int number, int size)
{
  int sum = 0;

  if (size <= 0 || number <= 0)
    return 0;

  while (number-- > 0)
    sum += ((my_rand() % size) + 1);

  return sum;
}


/* simulates a number range */
int number_range(int from, int to)
{
   return ((my_rand() % (to - from + 1)) + from);
}


/* simulates a percentile from 1 to 100 */
int number_percent(void)
{
   return ((my_rand() % 100) + 1);
}


int MIN(int a, int b)
{
  return a < b ? a : b;
}


int MAX(int a, int b)
{
  return a > b ? a : b;
}


#ifndef HAVE_STRDUP
/* Create a duplicate of a string */
char *strdup(const char *source)
{
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif /* HAVE_STRDUP */


#ifndef str_cmp
/* str_cmp: a case-insensitive version of strcmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(char *arg1, char *arg2)
{
  int chk, i;

  for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
      if (chk < 0)
	return (-1);
      else
	return (1);
  return (0);
}
#endif /* str_cmp */ 


#ifndef strn_cmp
/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(char *arg1, char *arg2, int n)
{
  int chk, i;

  for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
      if (chk < 0)
	return (-1);
      else
	return (1);

  return (0);
}
#endif /* strn_cmp */


/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  char buf[150];
  extern struct room_data *world;

  sprintf(buf, "%s hit death trap #%d (%s)", GET_NAME(ch),
	  world[ch->in_room].number, world[ch->in_room].name);
  mudlog(buf, BRF, LVL_IMMORT, TRUE);
}



/* writes a string to the log */
void basic_mud_log(char *str)
{
  time_t ct;
  char *tmstr;

  ct = time(0);
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  fprintf(stderr, "%-19.19s :: %s\n", tmstr, str);
}


/* the "touch" command, essentially. */
int touch(char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    perror(path);
    return -1;
  } else {
    fclose(fl);
    return 0;
  }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(char *str, char type, sbyte level, byte file)
{
  /* All this code is now in arealog() */
  arealog(str, type, level, file, -1);
#if(0)
  char buf[MAX_STRING_LENGTH];
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
  char *tmp, tp;
  time_t ct;

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (file)
    fprintf(stderr, "%-19.19s :: %s\n", tmp, str);
  if (level < 0)
    return;

  sprintf(buf, "[ %s ]\r\n", str);

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING)) {
      tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
	    (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));

      if ((GET_LEVEL(i->character) >= level) && (tp >= type)) {
	send_to_char(CCINFO(i->character), i->character);
	send_to_char(buf, i->character);
	send_to_char(CCNRM(i->character), i->character);
      }
    }
#endif
}



/* HACKED to show all 32 bits */
void sprintbit(unsigned long vektor, char *names[], char *result)
{
  unsigned long nr;

  *result = '\0';

/*
  if (vektor < 0) {
    strcpy(result, "SPRINTBIT ERROR!");
    return;
  }
*/
  for (nr = 0; vektor; vektor >>= 1) {
    if (IS_SET(1, vektor)) {
      if (*names[nr] != '\n') {
	strcat(result, names[nr]);
	strcat(result, " ");
      } else
	strcat(result, "UNDEFINED ");
    }
    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    strcat(result, "NOBITS ");
}



void sprinttype(int type, char *names[], char *result)
{
  int nr;

  for (nr = 0; (*names[nr] != '\n'); nr++);
  if (type < nr)
    strcpy(result, names[type]);
  else
    strcpy(result, "UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data real_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = -1;
  now.year = -1;

  return now;
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	/* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return now;
}



struct time_info_data age(struct char_data * ch)
{
  struct time_info_data player_age;

  player_age = mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return player_age;
}




/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) TELOPT_NAOFFD,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  SEND_TO_Q(on_string, d);
}



/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return TRUE;
  }

  return FALSE;
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data * ch)
{
  char buf[MAX_STRING_LENGTH];
  struct follow_type *j, *k;

/* Debug code */

  if (!ch->master) {
	sprintf(buf, "Stop_follower, char without master: %s.\n\r", GET_NAME(ch));
	log(buf);
	return;
  }

#if(0)
  assert(ch->master);
#endif

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
/* HACKED to take out self purging mobs
    if (IS_NPC(ch) && !ch->desc)
      SET_BIT(MOB_FLAGS(ch), MOB_WILL_SELF_PURGE);
*/
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
  }

  if (ch->master->followers->follower == ch) {	/* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {			/* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);

    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
  REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}



/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data * ch, struct char_data * leader)
{
  struct follow_type *k;

/*  assert(!ch->master);*/
  if (ch->master) return;

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (CAN_SEE(leader, ch))
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
}


int get_filename(char *orig_name, char *filename, int mode)
{
  char *prefix, /* *middle, */ *suffix, *ptr, name[64];

  switch (mode) {
  case CRASH_FILE:
    prefix = "plrobjs";
    suffix = "objs";
    break;
  case ETEXT_FILE:
    prefix = "plrtext";
    suffix = "text";
    break;
  case ASCII_PLAYER_FILE:
    prefix = "plrfiles";
    suffix = "char";
    break;
  case STOREROOM_FILE:
    prefix = "crash";
    suffix = "room";
    break;
  case PET_FILE:
    prefix = "pets";
    suffix = "pet";
    break;
  case ASCII_PLAYER_DATA:
    prefix = "plrfiles";
    suffix = "data";
    break;
  default:
    return 0;
    break;
  }

  if (!*orig_name)
    return 0;

  strcpy(name, orig_name);
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

/* HACKED to take out subdirectories from the different files */
/*
  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  sprintf(filename, "%s/%s/%s.%s", prefix, middle, name, suffix);
*/
  sprintf(filename, "%s/%s.%s", prefix, name, suffix);
/* end of HACK */
  return 1;
}

#define GET_OLC_ZONE(c)	((c)->player_specials->saved.olc_zone)
/*
 * arealog -- log mud messages to a file & to online imm's syslogs
 * Makes sure the error always gets logged to people with their OLC set to
 * a certain area. This used to be mudlog().
 */
void arealog(char *str, char type, sbyte level, byte file, int area) {
  char buf[MAX_STRING_LENGTH];
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
  char *tmp, tp;
  time_t ct;

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (file)
    fprintf(stderr, "%-19.19s :: %s\n", tmp, str);
  if (level < 0)
    return;

  sprintf(buf, "[ %s ]\r\n", str);

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING)) {
      tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
	    (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));

      if (((GET_LEVEL(i->character) >= level) && (tp >= type)) ||
          ((GET_OLC_ZONE(i->character) == area) && (area != -1))) {
	send_to_char(CCINFO(i->character), i->character);
	send_to_char(buf, i->character);
	send_to_char(CCNRM(i->character), i->character);
      }
    }
}
